#include "ShellMemoSyntaxHighlighter.h"

#include <QDebug>

#define STYLE_COMMAND 0
#define STYLE_HEADER 1
#define STYLE_COMMENT 2
#define STYLE_SEPARATOR 3
#define STYLE_OUTPUT 4
#define STYLE_EXCLAME 5
#define STYLE_QUESTION 6
#define STYLE_OPTION 7
#define STYLE_SUBHEADER 8
#define STYLE_SECTION 9
#define STYLE_HYPERLINK 10
#define STYLE_QUIET 11

HighlightingStyleSet* getShellmemoHighlightingStyles()
{
    static HighlightingStyleSet styles;
    if (styles.isEmpty())
    {
        // Take color names from here:
        // https://www.w3.org/TR/SVG/types.html#ColorKeywords
        styles.insert(STYLE_COMMAND, getTextCharFormat("darkBlue"));
        styles.insert(STYLE_HEADER, getTextCharFormat("black", "bold"));
        styles.insert(STYLE_SUBHEADER, getTextCharFormat("midnightBlue", "bold"));
        styles.insert(STYLE_SECTION, getTextCharFormat("darkOrchid", "bold"));
        styles.insert(STYLE_COMMENT, getTextCharFormat("darkGreen", "italic"));
        styles.insert(STYLE_SEPARATOR, getTextCharFormat("darkGray"));
        styles.insert(STYLE_OUTPUT, getTextCharFormat("darkMagenta"));
        styles.insert(STYLE_EXCLAME, getTextCharFormat("red"));
        styles.insert(STYLE_QUESTION, getTextCharFormat("magenta"));
        styles.insert(STYLE_OPTION, getTextCharFormat("darkSlateBlue"));
        styles.insert(STYLE_QUIET, getTextCharFormat("gainsboro"));

        /*QTextCharFormat hrefFormat = getTextCharFormat("blue");
        hrefFormat.setAnchor(true);
        hrefFormat.setFontUnderline(true);
        styles.insert(STYLE_HYPERLINK, hrefFormat);*/
    }
    return &styles;
}

ShellMemoSyntaxHighlighter::ShellMemoSyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    styles = getShellmemoHighlightingStyles();
    rules.append(HighlightingRule("^\\s*\\$\\s+.*$", STYLE_COMMAND));
    rules.append(HighlightingRule("^\\s*\\*\\s+.*$", STYLE_HEADER));
    rules.append(HighlightingRule("^\\s*\\-\\s+.*$", STYLE_SUBHEADER));
    rules.append(HighlightingRule("^\\s*\\+\\s+.*$", STYLE_SECTION));
    rules.append(HighlightingRule("^\\s*\\!\\s+.*$", STYLE_EXCLAME));
    rules.append(HighlightingRule("^\\s*\\?\\s+.*$", STYLE_QUESTION));
    rules.append(HighlightingRule("^\\s*\\>\\s+.*$", STYLE_OUTPUT));
    rules.append(HighlightingRule("^\\s*#.*$", STYLE_COMMENT));
    rules.append(HighlightingRule("^\\s*-{2}.*$", STYLE_OPTION));
    rules.append(HighlightingRule("^\\s*-{3,}.*$", STYLE_SEPARATOR));
    rules.append(HighlightingRule("^\\s*\\..*$", STYLE_QUIET));

    // Hyperlink rule should be added after others to be applied correctly
    // as most of rules here modifiy format of entire line and hyperlink as well.
    //rules.append(HighlightingRule("\\bhttp(s?)://[^\\s]+\\b", STYLE_HYPERLINK));
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

            // TODO font style is applied correctly but for some reasons
            // highlighter can't make anchors and apply tooltip too
            /*if (currRule.styleId == STYLE_HYPERLINK)
            {
                QStringRef href(&text, pos, length);
                auto format = styles->value(currRule.styleId);
                format.setAnchorHref(href.toString());
                format.setToolTip(href + tr("\nCtrl + Click to open"));
                setFormat(pos, length, format);
            }
            else*/
                setFormat(pos, length, styles->value(currRule.styleId));
            idx = currRule.pattern.indexIn(text, pos + length);
        }
    }
}
