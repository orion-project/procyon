#include "StyleEditorPage.h"

#include "PageWidgets.h"

#include "helpers/OriLayouts.h"

#include <QApplication>
#include <QFrame>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QToolBar>

StyleEditorPage::StyleEditorPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Style Sheet Editor");
    setWindowIcon(QIcon(":/icon/main"));

    _editor = new QPlainTextEdit;
    _editor->setProperty("role", "memo_editor");
    _editor->setWordWrapMode(QTextOption::NoWrap);
    auto f = _editor->font();
#if defined(Q_OS_WIN)
    f.setFamily("Courier New");
    f.setPointSize(11);
#elif defined(Q_OS_MAC)
    f.setFamily("Monaco");
    f.setPointSize(13);
#else
    f.setFamily("monospace");
    f.setPointSize(11);
#endif
    _editor->setFont(f);
    _editor->setPlainText(qApp->styleSheet());

    auto titleEditor = PageWidgets::makeTitleEditor();
    titleEditor->setText(tr("Stylesheet Editor"));

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Apply"), [this](){
        qApp->setStyleSheet(_editor->toPlainText());
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/memo_close"), tr("Close"), [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, _editor}).setMargin(0).setSpacing(0).useFor(this);
}
