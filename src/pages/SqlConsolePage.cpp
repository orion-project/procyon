#include "SqlConsolePage.h"

#include "PageWidgets.h"
#include "../catalog/SqlHelper.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWidgets.h"

#include <QSplitter>

namespace {

const int MAX_VALUE_LEN = 32;

QString runSql(const QString& sql)
{
    QString result;
    QTextStream stream(&result);

    Ori::Sql::SelectQuery query(sql);
    if (query.isFailed())
    {
        stream << QStringLiteral("<p style='color:red'><pre>")
               << query.error()
               << QStringLiteral("</pre>");
        return result;
    }

    bool namesExtracted = false;
    stream << QStringLiteral("<table border=1 cellpadding=5 cellspacing=-1 "
                             "style='border-color:gray;border-style:solid'>");
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
    }
    stream << QStringLiteral("</table>");
    return result;
}

} // namespace

SqlConsolePage::SqlConsolePage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("SQL Console"));
    setWindowIcon(QIcon(":/icon/main"));

    auto editor = PageWidgets::makeCodeEditor();
    auto result = PageWidgets::makeCodeEditor();
    result->setReadOnly(true);
    auto f = result->font();
    f.setPointSize(f.pointSize()-1);
    result->setFont(f);

    auto splitter = new QSplitter;
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(editor);
    splitter->addWidget(result);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 9);

    auto titleEditor = PageWidgets::makeTitleEditor(tr("SQL Console"));

    auto toolbar = new QToolBar;
    auto actionRun = toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Execute"), [editor, result](){
        result->setHtml(runSql(editor->toPlainText()));
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/memo_close"), tr("Close"), [this](){
        deleteLater();
    });

    actionRun->setShortcut(Qt::Key_F5);

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, splitter}).setMargin(0).setSpacing(0).useFor(this);
}
