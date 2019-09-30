#ifndef HIGHLIGHTER_MANAGER_H
#define HIGHLIGHTER_MANAGER_H

#include "core/OriTemplates.h"

QT_BEGIN_NAMESPACE
class QSyntaxHighlighter;
class QTextDocument;
QT_END_NAMESPACE

struct HighlighterInfo
{
    QString name;
    QString title;
};

class HighlighterManager : public Singleton<HighlighterManager>
{
public:
    QVector<HighlighterInfo> highlighters() const;

    QSyntaxHighlighter* makeHighlighter(const QString& name, QTextDocument* doc);

private:
    HighlighterManager() {}

    friend class Singleton<HighlighterManager>;
};

#endif // HIGHLIGHTER_MANAGER_H
