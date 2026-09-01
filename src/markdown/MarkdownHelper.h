#ifndef MARKDOWN_HELPER_H
#define MARKDOWN_HELPER_H

#include <QString>

class MarkdownHelper
{
public:

static QString markdownToHtml(const QString& markdown);

static inline const auto httpScheme = QStringLiteral("http");
static inline const auto fileScheme = QStringLiteral("file:");
static inline const auto enotScheme = QStringLiteral("enot:");
};

#endif // MARKDOWN_HELPER_H
