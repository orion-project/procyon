#include "CodeTextEdit.h"

CodeTextEdit::CodeTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    setProperty("role", "memo_editor");
    setObjectName("code_editor");
    setWordWrapMode(QTextOption::NoWrap);
    /*
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
    */

}
