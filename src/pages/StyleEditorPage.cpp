#include "StyleEditorPage.h"

#include "PageWidgets.h"
#include "helpers/OriLayouts.h"

#include <QApplication>
#include <QPlainTextEdit>

StyleEditorPage::StyleEditorPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("Style Sheet Editor"));
    setWindowIcon(QIcon(":/icon/main"));

    auto editor = new QPlainTextEdit;
    editor->setProperty("role", "memo_editor");
    editor->setObjectName("code_editor");
    editor->setPlainText(qApp->styleSheet());

    auto titleEditor = PageWidgets::makeTitleEditor(tr("Style Sheet Editor"));

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Apply"), [editor](){
        qApp->setStyleSheet(editor->toPlainText());
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/memo_close"), tr("Close"), [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, editor}).setMargin(0).setSpacing(0).useFor(this);
}
