#include "StyleEditorPage.h"

#include "helpers/OriLayouts.h"

#include <QApplication>
#include <QPlainTextEdit>
#include <QPushButton>

StyleEditorPage::StyleEditorPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Style Sheet Editor");

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

    auto buttonApply = new QPushButton("Apply");
    buttonApply->setToolTip("Apply style sheet");
    connect(buttonApply, &QPushButton::clicked, [this](){
        qApp->setStyleSheet(_editor->toPlainText());
    });

    Ori::Layouts::LayoutV({
        Ori::Layouts::LayoutH({
            Ori::Layouts::Stretch(),
            buttonApply
        }).setMargin(6),
        _editor
    }).setMargin(0).setSpacing(0).useFor(this);
}
