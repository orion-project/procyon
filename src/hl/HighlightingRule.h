#ifndef HIGHLIGHTINGRULE_H
#define HIGHLIGHTINGRULE_H

#include <QSyntaxHighlighter>

//! Container to describe a highlighting rule.
//! Based on a regular expression, a relevant match # and the format.
class HighlightingRule
{
public:
    HighlightingRule(const QString &patternStr, int styleId, int nth = 0)
    {
        this->patternStr = patternStr;
        this->pattern = QRegExp(patternStr);
        this->styleId = styleId;
        this->nth = nth;
    }

    int id;
    QString patternStr;
    QRegExp pattern;
    int styleId;
    int nth;
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
