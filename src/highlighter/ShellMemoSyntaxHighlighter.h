#ifndef SHELLMEMOSYNTAXHIGHLIGHTER_H
#define SHELLMEMOSYNTAXHIGHLIGHTER_H

#include "HighlightingRule.h"

class ShellMemoSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    ShellMemoSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text);

private:
    HighlightingStyleSet* styles;
    QList<HighlightingRule> rules;
};

#endif // SHELLMEMOSYNTAXHIGHLIGHTER_H
