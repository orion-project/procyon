#include "HighlightingRule.h"

HighlightingRule1::HighlightingRule1(const QString& name,
                                     const QStringList &patternStrs,
                                     const QTextCharFormat& format,
                                     const Options& options)
                       : name(name),
                         format(format),
                         options(options)
{
    for (auto patternStr : patternStrs)
        patterns << QRegularExpression(patternStr);
}
