#include "PhlManager.h"

#include "EnotStorage.h"
#include "../widgets/PopupMessage.h"

#include "orion/helpers/OriDialogs.h"

#include <QActionGroup>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextDocument>
#include <QBoxLayout>

using namespace Ori::Highlighter;

namespace Phl {

QSharedPointer<Spec> createSpec(const Meta& meta, bool withRawData)
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

    QSharedPointer<Spec> getSpec(QString name)
    {
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
};

static SpecCache& specCache()
{
    static SpecCache cache;
    return cache;
}

QSharedPointer<Spec> getSpec(const QString& name)
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

//------------------------------------------------------------------------------
//                                 Highlighter
//------------------------------------------------------------------------------

QSyntaxHighlighter* createHighlighter(QPlainTextEdit* editor, const QString& name)
{
    auto hl = getSpec(name);
    return hl ? new Highlighter(editor->document(), hl) : nullptr;
}

//------------------------------------------------------------------------------
//                                 Control
//------------------------------------------------------------------------------

static QString getHighlightersDir()
{
    QDir dir(qApp->applicationDirPath() + "/syntax");
    #ifdef Q_OS_MAC
        if (!dir.exists())
        {
            // Look near the application bundle, it is for development mode
            dir = QDir(qApp->applicationDirPath() % "/../../../syntax");
        }
    #endif
    return dir.absolutePath();
}

Control::Control(QMenu *menu, QObject *parent) : QObject(parent), _menu(menu)
{
}

void Control::loadMetas()
{
    if (_managerDlg)
        _managerDlg->close();

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

    auto& cache = specCache();
    cache.allMetas.clear();
    cache.loadedSpecs.clear();
    for (const auto& storage : storages)
    {
        // The first writable storage becomes a default storage
        // for new highlighters, this is enough for now
        if (!storage->readOnly() && !cache.customStorage)
            cache.customStorage = storage;

        for (auto& meta : storage->loadMetas())
        {
            if (cache.allMetas.contains(meta.name))
            {
                const auto& existedMeta = cache.allMetas[meta.name];
                qWarning() << "Highlighter is already registered" << existedMeta.name << existedMeta.source
                           << (existedMeta.storage ? existedMeta.storage->name() : QString("null-storage"));
                continue;
            }
            meta.storage = storage;
            cache.allMetas[meta.name] = meta;
            qDebug() << "Highlighter registered" << meta.name << meta.source << meta.storage->name();
        }
    }

    makeMenu();
}

void Control::makeMenu()
{
    if (_actionGroup) delete _actionGroup;

    _actionGroup = new QActionGroup(this);
    connect(_actionGroup, &QActionGroup::triggered, this, &Control::actionGroupTriggered);

    auto actionNone = new QAction(tr("None"), this);
    actionNone->setCheckable(true);
    _actionGroup->addAction(actionNone);

    const auto& allMetas = specCache().allMetas;
    auto it = allMetas.constBegin();
    while (it != allMetas.constEnd())
    {
        const auto& meta = it.value();
        auto actionDict = new QAction(meta.displayTitle(), _actionGroup);
        actionDict->setCheckable(true);
        actionDict->setData(meta.name);
        it++;
    }

    _menu->clear();
    _menu->addActions(_actionGroup->actions());
}

void Control::showCurrent(const QString& name)
{
    if (!_actionGroup) return;
    for (const auto& action : _actionGroup->actions())
        if (action->data().toString() == name)
        {
            action->setChecked(true);
            break;
        }
}

void Control::setEnabled(bool on)
{
    _actionGroup->setEnabled(on);
}

void Control::actionGroupTriggered(QAction* action)
{
    emit selected(action->data().toString());
}

void Control::showManager()
{
    // We store dlg pointer only to be able to close it when another db loaded
    _managerDlg = new ManagerDlg(this);
    connect(_managerDlg, &QObject::destroyed, this, [this]{ _managerDlg = nullptr; });
    _managerDlg->show();
    _managerDlg->activateWindow();
}

//------------------------------------------------------------------------------
//                                 Control
//------------------------------------------------------------------------------

ManagerDlg::ManagerDlg(Control *parent) : QWidget(), _parent(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    _specList = new QListWidget;
    _specList->setObjectName("pages_list");
    auto it = specCache().allMetas.constBegin();
    while (it != specCache().allMetas.constEnd())
    {
        const auto& meta = it.value();
        QString title = meta.displayTitle();
        if (!meta.storage)
            title += " (invalid, no storage)";
        else if (meta.storage->readOnly())
            title += " (built-in, read only)";
        auto item = new QListWidgetItem(title, _specList);
        item->setData(Qt::UserRole, meta.name);
        it++;
    }

    auto buttonEdit = new QPushButton(tr("Edit"));
    connect(buttonEdit, &QPushButton::clicked, this, &ManagerDlg::editHighlighter);

    auto buttonNewEmpty = new QPushButton(tr("New Empty"));
    connect(buttonNewEmpty, &QPushButton::clicked, this, &ManagerDlg::createHighlighterEmpty);

    auto buttonNewCopy = new QPushButton(tr("New As Copy"));
    connect(buttonNewCopy, &QPushButton::clicked, this, &ManagerDlg::createHighlighterCopy);

    auto buttonDelete = new QPushButton(tr("Delete"));
    connect(buttonDelete, &QPushButton::clicked, this, &ManagerDlg::deleteHighlighter);

    auto buttonClose = new QPushButton(tr("Close"));
    connect(buttonClose, &QPushButton::clicked, this, &ManagerDlg::close);

    auto layoutButtons = new QVBoxLayout;
    layoutButtons->addWidget(buttonEdit);
    layoutButtons->addWidget(buttonNewEmpty);
    layoutButtons->addWidget(buttonNewCopy);
    layoutButtons->addSpacing(50);
    layoutButtons->addWidget(buttonDelete);
    layoutButtons->addSpacing(50);
    layoutButtons->addStretch();
    layoutButtons->addWidget(buttonClose);

    auto layout = new QHBoxLayout(this);
    layout->addWidget(_specList);
    layout->addLayout(layoutButtons);
    layout->setContentsMargins(7, 10, 10, 10);
}

QString ManagerDlg::selectedSpecName() const
{
    auto item = _specList->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void ManagerDlg::editHighlighter()
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
        emit _parent->editorRequested(fullSpec);
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

void ManagerDlg::createHighlighterEmpty()
{
    auto& cache = specCache();
    QSharedPointer<Spec> spec(new Spec);
    spec->meta.storage = cache.customStorage;
    emit _parent->editorRequested(spec);
    close();
}

void ManagerDlg::createHighlighterCopy()
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

void ManagerDlg::newHighlighterWithBase(const Meta &meta)
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
    emit _parent->editorRequested(spec);
}

void ManagerDlg::deleteHighlighter()
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

    PopupMessage::affirm(tr("Highlighter successfully deleted\n\n"
        "Application is required to be restarted to reflect changes"));

    delete _specList->currentItem();
    _specList->setCurrentItem(_specList->item(0));
}

} // namespace Phl
