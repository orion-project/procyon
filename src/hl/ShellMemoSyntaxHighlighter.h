#ifndef SHELLMEMOSYNTAXHIGHLIGHTER_H
#define SHELLMEMOSYNTAXHIGHLIGHTER_H

#include "HighlightingRule.h"

class ShellMemoSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    ShellMemoSyntaxHighlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    QList<HighlightingRule> rules;
};

#endif // SHELLMEMOSYNTAXHIGHLIGHTER_H
