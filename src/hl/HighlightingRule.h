#ifndef HIGHLIGHTINGRULE_H
#define HIGHLIGHTINGRULE_H

#include <QSyntaxHighlighter>

//! Container to describe a highlighting rule.
//! Based on a regular expression, a relevant match # and the format.
class HighlightingRule
{
public:
    HighlightingRule(const QString &patternStr, int n, const QTextCharFormat &matchingFormat)
    {
        originalRuleStr = patternStr;
        pattern = QRegExp(patternStr);
        nth = n;
        format = matchingFormat;
    }

    QString originalRuleStr;
    QRegExp pattern;
    int nth;
    QTextCharFormat format;
};

inline const QTextCharFormat getTextCharFormat(const QString &colorName, const QString &style = QString())
{
    QTextCharFormat charFormat;
    QColor color(colorName);
    charFormat.setForeground(color);
    if (style.contains(QStringLiteral("bold"), Qt::CaseInsensitive))
        charFormat.setFontWeight(QFont::Bold);
    if (style.contains(QStringLiteral("italic"), Qt::CaseInsensitive))
        charFormat.setFontItalic(true);
    if (style.contains(QStringLiteral("underline"), Qt::CaseInsensitive))
        charFormat.setFontUnderline(true);
    return charFormat;
}

typedef QHash<int, QTextCharFormat> HighlightingStyleSet;

#endif // HIGHLIGHTINGRULE_H
