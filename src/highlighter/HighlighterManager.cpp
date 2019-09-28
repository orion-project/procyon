#include "HighlighterManager.h"

const QVector<HighlighterInfo>& HighlighterManager::highlighters() const
{
    static QVector<HighlighterInfo> infos {
        { "procyon", "Procyon memo" },
        { "python", "Python code" },
    };
    return infos;
}
