#include "MarkdownCssEditorPage.h"

#include "PageWidgets.h"
#include "../AppSettings.h"

#include "helpers/OriLayouts.h"

#include <QPlainTextEdit>

MarkdownCssEditorPage::MarkdownCssEditorPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Markdown CSS Editor");
    setWindowIcon(QIcon(":/icon/main"));

    auto editor = new QPlainTextEdit;
    editor->setProperty("role", "memo_editor");
    editor->setObjectName("code_editor");
    editor->setPlainText(AppSettings::instance().markdownCss());

    auto titleEditor = PageWidgets::makeTitleEditor("Markdown CSS Editor");

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/memo_save"), "Apply", [editor](){
        AppSettings::instance().updateMarkdownCss(editor->toPlainText());
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/memo_close"), "Close", [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, editor}).setMargin(0).setSpacing(0).useFor(this);
}
