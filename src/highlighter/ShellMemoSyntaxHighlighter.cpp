#include "ShellMemoSyntaxHighlighter.h"

#include "../TextEditHelpers.h"

#include <QDebug>

using R_ = HighlightingRule1;
using F_ = TextFormat;

static QList<HighlightingRule1>* getRules()
{
    static QList<HighlightingRule1> rules
    {
        R_("Command",     "^\\s*\\${1}\\s+.*$", F_("darkBlue").get()),
        R_("Subcommand",  "^\\s*\\${2}\\s+.*$", F_("mediumBlue").get()),
        R_("Quote",       "^\\s*\\|\\s+.*$", F_("teal").get()),
        R_("Header",      "^\\s*\\*\\s+.*$", F_("black").bold().get()),
        R_("Subheader",   "^\\s*\\-\\s+.*$", F_("midnightBlue").bold().get()),
        R_("Section",     "^\\s*\\+\\s+.*$", F_("darkOrchid").bold().get()),
        R_("Exclame",     "^\\s*\\!\\s+.*$", F_("red").get()),
        R_("Question",    "^\\s*\\?\\s+.*$", F_("magenta").get()),
        R_("Output",      "^\\s*\\>\\s+.*$", F_("darkMagenta").get()),
        R_("Option",      "^\\s*-{2}.*$",    F_("darkSlateBlue").get()),
        R_("Comment",     "\\s*#.*$",       F_("darkGreen").italic().get()),

        R_("Inline code",
           "[\\s:;.,!?()]+(`[^`]+`)[\\s:;.,!?()]+",
           F_("maroon").background("seashell").get(),
           HighlightingRule1::Options().group(1)),

        R_("Inline bold",
           "[\\s:;.,!?()]+(\\*[^\\*]+\\*)[\\s:;.,!?()]+",
           F_().bold().get(),
           HighlightingRule1::Options().group(1)),

        R_("Inline italic",
           "[\\s:;.,!?()]+(_[^_]+_)[\\s:;.,!?()]+",
           F_().italic().get(),
           HighlightingRule1::Options().group(1)),

        R_("Inline strikeout",
           "[\\s:;.,!?()]+(~[^~]+~)[\\s:;.,!?()]+",
           F_().strikeOut().get(),
           HighlightingRule1::Options().group(1)),

        R_("Hyperlink",
           "\\bhttp(s?)://[^\\s]+\\b",
           F_("blue").underline().anchor().get(),
           HighlightingRule1::Options().hyperlink()),

        R_("Separator", "^\\s*-{3,}.*$",   F_("darkGray").get()),
        R_("Quiet",     "^\\s*\\..*$",     F_("gainsboro").get()),
    };
    return &rules;
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
            int pos = rule.pattern.pos(rule.options.matchGroup);
            int length = rule.pattern.cap(rule.options.matchGroup).length();

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
