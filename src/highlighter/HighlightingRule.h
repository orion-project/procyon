#ifndef HIGHLIGHTING_RULE_H
#define HIGHLIGHTING_RULE_H

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class HighlightingRule
{
public:
    HighlightingRule(const QString &patternStr, int styleId, int nth = 0)
    {
        this->patternStr = patternStr;
        this->pattern = QRegularExpression(patternStr);
        this->styleId = styleId;
        this->nth = nth;
    }

    int id;
    QString patternStr;
    QRegularExpression pattern;
    int styleId;
    int nth;
};

struct HighlightingRule1
{
public:
    struct Options
    {
        Options() {}
        Options& hyperlink() { isHyperlink = true; return *this; }
        Options& group(int g) { matchGroup = g; return *this; }
        Options& sizeDelta(int d) { fontSizeDelta = d; return *this; }

        int matchGroup = 0;
        bool isHyperlink = false;
        int fontSizeDelta = 0;
    };


    HighlightingRule1(const QString& name,
                      const QStringList &patternStr,
                      const QTextCharFormat& format,
                      const Options& options = Options());

    QString name;
    QVector<QRegularExpression> patterns;
    QTextCharFormat format;
    Options options;
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

#endif // HIGHLIGHTING_RULE_H
