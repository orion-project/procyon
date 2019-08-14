#ifndef HIGHLIGHTING_RULE_H
#define HIGHLIGHTING_RULE_H

#include <QSyntaxHighlighter>

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

struct HighlightingRule1
{
public:
    struct Options
    {
        Options() {}
        Options& hyperlink() { isHyperlink = true; return *this; }

        int nth = 0;
        bool isHyperlink = false;
    };


    HighlightingRule1(const QString& name,
                      const QString &patternStr,
                      const QTextCharFormat& format,
                      const Options& options = Options())
        : name(name),
          pattern(QRegExp(patternStr)),
          format(format),
          options(options) {}

    QString name;
    QRegExp pattern;
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
