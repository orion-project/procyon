#include "ShellMemoSyntaxHighlighter.h"

#include "../TextEditorHelpers.h"

#include <QDebug>

static QList<HighlightingRule1>* getRules()
{
#define R_ HighlightingRule1
#define F_ TextFormat
    static QList<HighlightingRule1> rules
    {
        R_("Command",   "^\\s*\\$\\s+.*$", F_("darkBlue").get()),
        R_("Header",    "^\\s*\\*\\s+.*$", F_("black").bold().get()),
        R_("Subheader", "^\\s*\\-\\s+.*$", F_("midnightBlue").bold().get()),
        R_("Section",   "^\\s*\\+\\s+.*$", F_("darkOrchid").bold().get()),
        R_("Exclame",   "^\\s*\\!\\s+.*$", F_("red").get()),
        R_("Question",  "^\\s*\\?\\s+.*$", F_("magenta").get()),
        R_("Output",    "^\\s*\\>\\s+.*$", F_("darkMagenta").get()),
        R_("Comment",   "^\\s*#.*$",       F_("darkGreen").italic().get()),
        R_("Option",    "^\\s*-{2}.*$",    F_("darkSlateBlue").get()),

        R_("Hyperlink",
           "\\bhttp(s?)://[^\\s]+\\b",
           F_("blue").underline().anchor().get(),
           HighlightingRule1::Options().hyperlink()),

        R_("Separator", "^\\s*-{3,}.*$",   F_("darkGray").get()),
        R_("Quiet",     "^\\s*\\..*$",     F_("gainsboro").get()),
    };
    return &rules;
#undef R_
#undef F_
}

ShellMemoSyntaxHighlighter::ShellMemoSyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    rules = getRules();
}

void ShellMemoSyntaxHighlighter::highlightBlock(const QString &text)
{
    for (const HighlightingRule1& rule : *rules)
    {
        int idx = rule.pattern.indexIn(text);
        while (idx >= 0)
        {
            int pos = rule.pattern.pos(rule.options.nth);
            int length = rule.pattern.cap(rule.options.nth).length();

            // Font style is applied correctly but highlighter can't make anchors and apply tooltips.
            // We do it manually overriding event handlers in MemoEditor.
            // There is the bug but seems nobody cares: https://bugreports.qt.io/browse/QTBUG-21553
            if (rule.options.isHyperlink)
            {
                QStringRef href(&text, pos, length);
                QTextCharFormat format(rule.format);
                format.setAnchorHref(href.toString());
                setFormat(pos, length, format);
            }
            else
                setFormat(pos, length, rule.format);
            idx = rule.pattern.indexIn(text, pos + length);
        }
    }
}
