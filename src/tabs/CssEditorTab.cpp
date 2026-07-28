#include "CssEditorTab.h"

#include "TabHelpers.h"
#include "../AppSettings.h"
#include "../highlighter/PhlManager.h"

#include "helpers/OriLayouts.h"
#include "widgets/OriCodeEditor.h"

#include <QIcon>
#include <QToolBar>

CssEditorTab::CssEditorTab(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Markdown CSS Editor");
    setWindowIcon(QIcon(":/icon/main"));

    auto editor = new Ori::Widgets::CodeEditor;
    editor->setProperty("role", "memo_editor");
    editor->setObjectName("code_editor");
    editor->setPlainText(AppSettings::instance().markdownCss());
    Phl::createHighlighter(editor, "css");

    auto titleEditor = TabHelpers::makeTitleEditor("Markdown CSS Editor");

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/apply"), "Apply", [editor](){
        AppSettings::instance().updateMarkdownCss(editor->toPlainText());
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), "Close", [this](){
        deleteLater();
    });

    auto toolPanel = TabHelpers::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, editor}).setMargin(0).setSpacing(0).useFor(this);
}
