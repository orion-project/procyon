#ifndef SHELL_MEMO_SYNTAX_HIGHLIGHTER_H
#define SHELL_MEMO_SYNTAX_HIGHLIGHTER_H

#include "HighlightingRule.h"

class ShellMemoSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    ShellMemoSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text);

private:
    QList<HighlightingRule1>* rules;
    QTextDocument* _parentDocument;
};

#endif // SHELL_MEMO_SYNTAX_HIGHLIGHTER_H
