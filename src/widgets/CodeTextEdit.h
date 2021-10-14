#ifndef CODE_TEXT_EDIT_H
#define CODE_TEXT_EDIT_H

#include <QPlainTextEdit>

class CodeTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeTextEdit(QWidget *parent = nullptr);
};

#endif // CODE_TEXT_EDIT_H
