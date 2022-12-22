#include "SqlConsolePage.h"

#include "PageWidgets.h"
#include "../catalog/SqlHelper.h"
#include "../highlighter/PhlManager.h"

#include "orion/helpers/OriLayouts.h"

#include <QLabel>
#include <QSplitter>
#include <QToolBar>
#include <QPlainTextEdit>

namespace {

const int MAX_VALUE_LEN = 100; // TODO: make configurable

QString runSql(const QString& sql)
{
    QString result;
    QTextStream stream(&result);

    Ori::Sql::SelectQuery query(sql);
    if (query.isFailed())
    {
        stream << "<p>" << PageWidgets::formatError(query.error());
        return result;
    }

    int recordCount = 0;
    bool namesExtracted = false;
    stream << QStringLiteral("<table border=1 cellpadding=5 cellspacing=-1 "
                             "style='border-color:silver;border-style:solid'>");
    while (query.next())
    {
        auto r = query.record();

        if (!namesExtracted)
        {
            stream << QStringLiteral("<tr>");
            for (int i = 0; i < r.count(); i++)
                stream << QStringLiteral("<td><b>")
                       << r.fieldName(i)
                       << QStringLiteral("</b></td>");
            stream << QStringLiteral("</tr>");
            namesExtracted = true;
        }

        stream << QStringLiteral("<tr>");
        for (int i = 0; i < r.count(); i++)
        {
            stream << QStringLiteral("<td>");
            auto value = r.value(i).toString();
            if (value.length() > MAX_VALUE_LEN)
                stream << value.left(MAX_VALUE_LEN) << QStringLiteral("...");
            else stream << value;
            stream << QStringLiteral("</td>");
        }
        stream << QStringLiteral("</tr>");

        recordCount++;
    }
    stream << QStringLiteral("</table><br/><p>Rows selected: ") << recordCount;
    return result;
}

QString loadTableNames()
{
    Ori::Sql::SelectQuery query("SELECT name FROM sqlite_schema WHERE type = 'table' AND name NOT LIKE 'sqlite_%'");
    if (query.isFailed())
        return PageWidgets::formatError(query.error());

    QStringList names;
    while (query.next())
    {
        auto r = query.record();
        names << "<b>" + r.field(0).value().toString() + "</b>";
    }
    return PageWidgets::formatInfo("Available tables: " + names.join(", "));
}

} // namespace

SqlConsolePage::SqlConsolePage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("SQL Console"));
    setWindowIcon(QIcon(":/icon/main"));

    auto infoLabel = new QLabel;
    infoLabel->setProperty("role", "memo_editor");
    infoLabel->setText(loadTableNames());
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto editor = new QPlainTextEdit;
    editor->setProperty("role", "memo_editor");
    editor->setObjectName("code_editor");
    editor->setWordWrapMode(QTextOption::NoWrap);
    Phl::createHighlighter(editor, QStringLiteral("sql"));

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
    auto actionRun = toolbar->addAction(QIcon(":/toolbar/apply"), tr("Execute (F5)"), [editor, result](){
        result->setHtml(runSql(editor->toPlainText()));
    });
    actionRun->setShortcut(Qt::Key_F5);

    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), tr("Close"), [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, infoLabel, Ori::Layouts::Space(3), splitter}).setMargin(0).setSpacing(0).useFor(this);
}
