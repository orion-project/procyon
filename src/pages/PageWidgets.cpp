#include "PageWidgets.h"

#include <QFrame>
#include <QLineEdit>
#include <QTextEdit>

namespace PageWidgets
{

QLineEdit* makeTitleEditor(const QString &title)
{
    auto titleEditor = new QLineEdit;
    titleEditor->setReadOnly(true);
    titleEditor->setObjectName("memo_title_editor");
    titleEditor->setProperty("role", "memo_title");
    titleEditor->setText(title);
    return titleEditor;
}

QFrame* makeHeaderPanel(Ori::Layouts::LayoutItems items)
{
    auto toolPanel = new QFrame;
    toolPanel->setObjectName("memo_header_panel");
    Ori::Layouts::LayoutH(items).setMargin(0).useFor(toolPanel);
    return toolPanel;
}

QTextEdit* makeCodeEditor()
{
    auto editor = new QTextEdit;
    editor->setAcceptRichText(false);
    editor->setProperty("role", "memo_editor");
    editor->setWordWrapMode(QTextOption::NoWrap);
    auto f = editor->font();
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
    editor->setFont(f);
    return editor;
}

} // namespace PageWidgets
