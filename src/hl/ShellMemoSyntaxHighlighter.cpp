#include "ShellMemoSyntaxHighlighter.h"

#include <QDebug>

#define STYLE_COMMAND 0
#define STYLE_HEADER 1
#define STYLE_COMMENT 2
#define STYLE_SEPARATOR 3
#define STYLE_OUTPUT 4

HighlightingStyleSet* getShellmemoHighlightingStyles()
{
    static HighlightingStyleSet styles;
    if (styles.isEmpty())
    {
        styles.insert(STYLE_COMMAND, getTextCharFormat("darkBlue"));
        styles.insert(STYLE_HEADER, getTextCharFormat("black", "bold"));
        styles.insert(STYLE_COMMENT, getTextCharFormat("darkGreen", "italic"));
        styles.insert(STYLE_SEPARATOR, getTextCharFormat("darkGray"));
        styles.insert(STYLE_OUTPUT, getTextCharFormat("darkMagenta"));
    }
    return &styles;
}

ShellMemoSyntaxHighlighter::ShellMemoSyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    auto styles = getShellmemoHighlightingStyles();
    rules.append(HighlightingRule("^\\s*\\$\\s+.*$", 0, styles->value(STYLE_COMMAND)));
    rules.append(HighlightingRule("^\\s*\\*\\s+.*$", 0, styles->value(STYLE_HEADER)));
    rules.append(HighlightingRule("^\\s*#.*$", 0, styles->value(STYLE_COMMENT)));
    rules.append(HighlightingRule("^\\s*-{3,}.*$", 0, styles->value(STYLE_SEPARATOR)));
    rules.append(HighlightingRule("^\\s*>\\s+.*$", 0, styles->value(STYLE_OUTPUT)));
}

void ShellMemoSyntaxHighlighter::highlightBlock(const QString &text)
{
    foreach (HighlightingRule currRule, rules)
    {
        int idx = currRule.pattern.indexIn(text, 0);
        while (idx >= 0)
        {
            int pos = currRule.pattern.pos(currRule.nth);
            int length = currRule.pattern.cap(currRule.nth).length();
            setFormat(pos, length, currRule.format);
            idx = currRule.pattern.indexIn(text, pos + length);
        }
    }
}
