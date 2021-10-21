#include "CssEditorPage.h"

#include "PageWidgets.h"
#include "../AppSettings.h"
#include "../widgets/CodeTextEdit.h"

#include "helpers/OriLayouts.h"

#include <QIcon>
#include <QToolBar>

CssEditorPage::CssEditorPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Markdown CSS Editor");
    setWindowIcon(QIcon(":/icon/main"));

    auto editor = new CodeTextEdit("css");
    editor->setPlainText(AppSettings::instance().markdownCss());

    auto titleEditor = PageWidgets::makeTitleEditor("Markdown CSS Editor");

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/apply"), "Apply", [editor](){
        AppSettings::instance().updateMarkdownCss(editor->toPlainText());
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), "Close", [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, editor}).setMargin(0).setSpacing(0).useFor(this);
}
