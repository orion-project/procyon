#include "CmdConsolePage.h"

#include "PageWidgets.h"
#include "../catalog/Catalog.h"
#include "../catalog/SqlHelper.h"

#include "orion/helpers/OriLayouts.h"

#include <QLabel>
#include <QSplitter>
#include <QToolBar>
#include <QPlainTextEdit>

namespace CmdConsoleImpl {

class Cmd {
public:
    virtual ~Cmd() {}
    virtual bool html() const { return false; }
    virtual QString run() = 0;
};

struct CmdConsole {
    QMap<QString, QSharedPointer<Cmd>> cmds;
    Catalog* catalog;

    QString formatNames() const
    {
        QStringList names;
        auto it = cmds.constBegin();
        while (it != cmds.constEnd()) {
            names << "<b>" + it.key() + "</b>";
            it++;
        }
        return "Known commands: " + names.join(", ");
    }
};

class HelpCmd : public Cmd
{
public:
    HelpCmd(const CmdConsole* impl): _impl(impl) {}
    bool html() const override { return true; }
    QString run() override { return "<p>" + _impl->formatNames(); }
private:
    const CmdConsole* _impl;
};

class PrintDbCmd : public Cmd
{
public:
    PrintDbCmd(const CmdConsole* impl): _impl(impl) {}
    QString run() override
    {
        if (!_impl->catalog)
            return "Database is not opened";
        QString r;
        QTextStream res(&r);
        for (auto f : _impl->catalog->topItems())
            printItem(res, 0, f);
        return r;
    }
private:
    const CmdConsole* _impl;

    void printItem(QTextStream& res, int level, CatalogItem* it)
    {
        res << QString(level*4, ' ');
        if (it->isFolder())
        {
            res << "[#" << it->id() << ' ' << it->title() << "]\n";
            for (auto f : it->asFolder()->children())
                printItem(res, level+1, f);
        }
        else
            res << "· #" << it->id() << ' ' << it->title() << '\n';
    }
};

}

using namespace CmdConsoleImpl;

CmdConsolePage::CmdConsolePage(Catalog* catalog) : QWidget()
{
    setWindowTitle(tr("Command Console"));
    setWindowIcon(QIcon(":/icon/main"));

    _impl = QSharedPointer<CmdConsole>::create();
    _impl->catalog = catalog;
    _impl->cmds["help"] = QSharedPointer<HelpCmd>::create(_impl.get());
    _impl->cmds["print_db"] = QSharedPointer<PrintDbCmd>::create(_impl.get());

    auto infoLabel = new QLabel;
    infoLabel->setProperty("role", "memo_editor");
    infoLabel->setText(PageWidgets::formatInfo(_impl->formatNames()));
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto editor = new QPlainTextEdit;
    editor->setProperty("role", "memo_editor");
    editor->setObjectName("code_editor");
    editor->setWordWrapMode(QTextOption::NoWrap);

    auto result = new QTextEdit;
    result->setWordWrapMode(QTextOption::NoWrap);
    result->setProperty("role", "memo_editor");
    result->setObjectName("sql_console_result");
    result->setReadOnly(true);

    auto splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(editor);
    splitter->addWidget(result);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 9);

    auto titleEditor = PageWidgets::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    auto actionRun = toolbar->addAction(QIcon(":/toolbar/apply"), tr("Execute (F5)"), [this, editor, result](){
        QString cmdName = editor->toPlainText().trimmed();
        if (cmdName.isEmpty()) return;
        if (!_impl->cmds.contains(cmdName))
        {
            result->setHtml(PageWidgets::formatError("Unknown command: <b>%%1</b>").arg(cmdName));
            return;
        }
        QString res = _impl->cmds[cmdName]->run();
        if (_impl->cmds[cmdName]->html())
            result->setHtml(res);
        else result->setPlainText(res);
    });
    actionRun->setShortcut(Qt::Key_F5);

    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), tr("Close"), [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, infoLabel, Ori::Layouts::Space(3), splitter}).setMargin(0).setSpacing(0).useFor(this);
}

void CmdConsolePage::setCatalog(Catalog* catalog)
{
    _impl->catalog = catalog;
}
