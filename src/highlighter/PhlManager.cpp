#include "PhlManager.h"

#include "EnotStorage.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWidgets.h"
#include "widgets/OriPopupMessage.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTextDocument>
#include <QTextEdit>

using namespace Ori::Highlighter;

namespace Phl {

SpecPtr createSpec(const Meta& meta, bool withRawData)
{
    auto spec = meta.storage->loadSpec(meta, withRawData);
    if (!spec) return QSharedPointer<Spec>();
    spec->meta.source = meta.source;
    spec->meta.storage = meta.storage;
    return spec;
}

//------------------------------------------------------------------------------
//                                 SpecCache
//------------------------------------------------------------------------------

struct SpecCache
{
    QMap<QString, Meta> allMetas;
    QMap<QString, QSharedPointer<Spec>> loadedSpecs;
    QSharedPointer<SpecStorage> customStorage;

    void reset()
    {
        allMetas.clear();
        loadedSpecs.clear();
        customStorage.reset();
        _loaded = false;
    }

    const auto& getAllMetas()
    {
        if (!_loaded) load();

        return allMetas;
    }

    SpecPtr getSpec(QString name)
    {
        if (!_loaded) load();

        if (!allMetas.contains(name))
        {
            qWarning() << "Highlighters::SpecCache: unknown name" << name;
            return QSharedPointer<Spec>();
        }
        if (!loadedSpecs.contains(name))
        {
            const auto& meta = allMetas[name];
            if (!meta.storage)
            {
                qWarning() << "Highlighters::SpecCache: storage not set" << name;
                return QSharedPointer<Spec>();
            }
            auto spec = createSpec(meta, false);
            if (!spec) return QSharedPointer<Spec>();
            loadedSpecs[name] = spec;
        }
        return loadedSpecs[name];
    }

private:
    bool _loaded = false;

    void load()
    {
        QVector<QSharedPointer<SpecStorage>> storages = {
            //QSharedPointer<SpecStorage>(new FileStorage(getHighlightersDir())),
            QSharedPointer<SpecStorage>(new QrcStorage({
                QStringLiteral(":/syntax/css"),
                QStringLiteral(":/syntax/ohl"),
                QStringLiteral(":/syntax/procyon"),
                QStringLiteral(":/syntax/python"),
                QStringLiteral(":/syntax/qss"),
                QStringLiteral(":/syntax/sql"),
            })),
            QSharedPointer<SpecStorage>(new EnotHighlighterStorage()),
        };

        allMetas.clear();
        loadedSpecs.clear();
        for (const auto& storage : storages)
        {
            // The first writable storage becomes a default storage
            // for new highlighters, this is enough for now
            if (!storage->readOnly() && !customStorage)
                customStorage = storage;

            for (auto& meta : storage->loadMetas())
            {
                if (allMetas.contains(meta.name))
                {
                    const auto& existedMeta = allMetas[meta.name];
                    qWarning() << "Highlighter is already registered" << existedMeta.name << existedMeta.source
                               << (existedMeta.storage ? existedMeta.storage->name() : QString("null-storage"));
                    continue;
                }
                meta.storage = storage;
                allMetas[meta.name] = meta;
                qDebug() << "Highlighter registered" << meta.name << meta.source << meta.storage->name();
            }
        }

        _loaded = true;
    }
};

static SpecCache& specCache()
{
    static SpecCache cache;
    return cache;
}

SpecPtr getSpec(const QString& name)
{
    return specCache().getSpec(name);
}

QPair<bool, bool> checkDuplicates(const Meta& meta)
{
    bool name = false;
    bool title = false;
    const auto& metas = specCache().allMetas;
    auto it = metas.constBegin();
    while (it != metas.constEnd())
    {
        const auto& m = it.value();
        if (m.source != meta.source)
        {
            if (!name && m.name == meta.name)
                name = true;
            if (!title && m.title == meta.title)
                title = true;
            if (name && title)
                break;
        }
        it++;
    }
    return {name, title};
}

// static QString getHighlightersDir()
// {
//     QDir dir(qApp->applicationDirPath() + "/syntax");
//     #ifdef Q_OS_MAC
//         if (!dir.exists())
//         {
//             // Look near the application bundle, it is for development mode
//             dir = QDir(qApp->applicationDirPath() % "/../../../syntax");
//         }
//     #endif
//     return dir.absolutePath();
// }

//------------------------------------------------------------------------------
//                                 ManagerDlg
//------------------------------------------------------------------------------

class ManagerDlg : public QWidget
{
public:
    ManagerDlg(SpecEditRequest onEdit) : QWidget(qApp->activeWindow()), _onEdit(onEdit)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

        _specList = new QListWidget;
        _specList->setObjectName("tabs_list");
        auto it = specCache().allMetas.constBegin();
        while (it != specCache().allMetas.constEnd())
        {
            const auto& meta = it.value();
            QString title = meta.displayTitle();
            if (!meta.storage)
                title += tr(" (invalid, no storage)");
            else if (meta.storage->readOnly())
                title += tr(" (built-in, read only)");
            auto item = new QListWidgetItem(title, _specList);
            item->setData(Qt::UserRole, meta.name);
            it++;
        }

        Ori::Layouts::LayoutH({
            _specList,
            Ori::Layouts::LayoutV({
                Ori::Gui::button(tr("Edit"), this, &ManagerDlg::editHighlighter),
                Ori::Gui::button(tr("New Empty"), this, &ManagerDlg::createHighlighterEmpty),
                Ori::Gui::button(tr("New As Copy"), this, &ManagerDlg::createHighlighterCopy),
                Ori::Layouts::Stretch(),
                Ori::Layouts::Space(100),
                Ori::Gui::button(tr("Delete"), this, &ManagerDlg::deleteHighlighter),
                Ori::Layouts::Space(100),
                Ori::Layouts::Stretch(),
                Ori::Gui::button(tr("Close"), this, &ManagerDlg::close),
            })
        }).setMargins(7, 10, 10, 10).useFor(this);
    }

private:
    QListWidget *_specList;
    SpecEditRequest _onEdit;

    QString selectedSpecName() const
    {
        auto item = _specList->currentItem();
        return item ? item->data(Qt::UserRole).toString() : QString();
    }

    void editHighlighter()
    {
        auto& cache = specCache();
        auto name = selectedSpecName();
        if (name.isEmpty())
            return Ori::Dlg::info(tr("No highlighter is selected"));

        const auto& meta = cache.allMetas[name];
        if (!meta.storage)
            return Ori::Dlg::warning(tr("Hihghlighter storage is not set"));

        if (!meta.storage->readOnly())
        {
            // reload spec with code and sample text
            auto fullSpec = createSpec(meta, true);
            if (!fullSpec)
                return Ori::Dlg::error("Failed to load highlighter");
            _onEdit(fullSpec);
            close();
            return;
        }

        if (Ori::Dlg::yes(tr("Highlighter \"%1\" is built-in and can not be edited. "
                             "Do you want to create a new highlighter on its base instead?"
                             ).arg(meta.displayTitle())))
        {
            newHighlighterWithBase(meta);
            close();
        }
    }

    void createHighlighterEmpty()
    {
        auto& cache = specCache();
        QSharedPointer<Spec> spec(new Spec);
        spec->meta.storage = cache.customStorage;
        _onEdit(spec);
        close();
    }

    void createHighlighterCopy()
    {
        auto& cache = specCache();
        auto name = selectedSpecName();
        if (name.isEmpty())
            return Ori::Dlg::info(tr("No highlighter is selected"));

        const auto& meta = cache.allMetas[name];
        if (!meta.storage)
            return Ori::Dlg::warning(tr("Hihghlighter storage is not set"));

        newHighlighterWithBase(meta);
        close();
    }

    void deleteHighlighter()
    {
        auto& cache = specCache();
        auto name = selectedSpecName();
        if (name.isEmpty())
            return Ori::Dlg::info(tr("No highlighter is selected"));

        const auto& meta = cache.allMetas[name];
        if (!meta.storage)
            return Ori::Dlg::warning(tr("Hihghlighter storage is not set"));

        if (meta.storage->readOnly())
            return Ori::Dlg::info(tr("Highlighter \"%1\" is built-in and can not be deleted").arg(meta.displayTitle()));

        if (!Ori::Dlg::yes(tr("Delete highlighter \"%1\"?").arg(meta.displayTitle())))
            return;

        auto res = meta.storage->deleteSpec(meta);
        if (!res.isEmpty())
            Ori::Dlg::error(tr("There is an error during highlighter deletion\n\n%1").arg(res));

        Ori::Gui::PopupMessage::affirm(tr("Highlighter successfully deleted\n\n"
            "Application is required to be restarted to reflect changes"));

        delete _specList->currentItem();
        _specList->setCurrentItem(_specList->item(0));
    }

    void newHighlighterWithBase(const Ori::Highlighter::Meta& meta)
    {
        auto spec = createSpec(meta, true);
        if (!spec)
        {
            Ori::Dlg::error("Failed to load base highlighter");
            spec.reset(new Spec());
        }
        spec->meta.name = "";
        spec->meta.source = "";
        spec->meta.title = "";
        spec->meta.storage = specCache().customStorage;
        _onEdit(spec);
    }
};

// We store dlg pointer only to be able to close it when another db loaded
QPointer<ManagerDlg> __managerDlg = {};

void showManagerDlg(SpecEditRequest onEdit)
{
    if (!__managerDlg)
        __managerDlg = new ManagerDlg(onEdit);
    __managerDlg->show();
    __managerDlg->activateWindow();
}

void fillMenu(QMenu *menu, std::function<void(const QString&)> onSelect)
{
    menu->clear();
    const auto& metas = specCache().getAllMetas();
    for (auto it = metas.cbegin(); it != metas.cend(); it++)
    {
        const auto& meta = it.value();
        QString highlighterName = meta.name;
        auto action = menu->addAction(meta.displayTitle(),
            [onSelect, highlighterName]{ onSelect(highlighterName); });
        action->setCheckable(true);
        action->setData(meta.name);
    }
}

QSyntaxHighlighter* createHighlighter(QPlainTextEdit *editor, const QString& name);
QSyntaxHighlighter* createHighlighter(QTextEdit *editor, const QString& name);

void reset()
{
    if (__managerDlg)
        __managerDlg->deleteLater();

    specCache().reset();
}

} // namespace Phl
