#include "QssEditorPage.h"

#include "PageWidgets.h"
#include "../highlighter/OriHighlighter.h"
#include "../widgets/CodeTextEdit.h"
#include "helpers/OriLayouts.h"

#include <QApplication>
#include <QIcon>
#include <QPlainTextEdit>
#include <QToolBar>

QssEditorPage::QssEditorPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Application QSS Editor");
    setWindowIcon(QIcon(":/icon/main"));

    auto editor = new CodeTextEdit;
    editor->setPlainText(qApp->styleSheet());
    Ori::Highlighter::createHighlighter(editor, QStringLiteral("qss"));

    auto titleEditor = PageWidgets::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/apply"), "Apply", [editor](){
        qApp->setStyleSheet(editor->toPlainText());
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), "Close", [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, editor}).setMargin(0).setSpacing(0).useFor(this);
}
