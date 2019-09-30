#ifndef PROCYON_SYNTAX_HIGHLIGHTER_H
#define PROCYON_SYNTAX_HIGHLIGHTER_H

#include "HighlightingRule.h"

class ProcyonSyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    ProcyonSyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text);

private:
    QList<HighlightingRule1>* rules;
    QTextDocument* _parentDocument;
};

#endif // PROCYON_SYNTAX_HIGHLIGHTER_H
