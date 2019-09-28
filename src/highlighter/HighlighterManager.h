#ifndef HIGHLIGHTER_MANAGER_H
#define HIGHLIGHTER_MANAGER_H

#include "core/OriTemplates.h"

struct HighlighterInfo
{
    QString name;
    QString title;
};

class HighlighterManager : public Singleton<HighlighterManager>
{
public:
    const QVector<HighlighterInfo>& highlighters() const;

private:
    HighlighterManager() {}

    friend class Singleton<HighlighterManager>;
};

#endif // HIGHLIGHTER_MANAGER_H
