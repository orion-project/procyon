#include "HighlighterManager.h"

#include "PythonSyntaxHighlighter.h"
#include "ProcyonSyntaxHighlighter.h"

namespace  {
class HighlighterMakerBase
{
public:
    QString name, title;

    HighlighterMakerBase(const QString& name, const QString& title): name(name), title(title) {}
    virtual ~HighlighterMakerBase() {}

    virtual QSyntaxHighlighter* make(QTextDocument* doc) = 0;
};

template <class THighlighter> class HighlighterMaker : public HighlighterMakerBase
{
public:
    HighlighterMaker(const QString& name, const QString& title): HighlighterMakerBase(name, title) {}

    QSyntaxHighlighter* make(QTextDocument* doc) override
    {
        QSyntaxHighlighter* hl = new THighlighter(doc);
        hl->setObjectName(name);
        return hl;
    }
};

QVector<HighlighterMakerBase*>& highlighterMakers()
{
    static QVector<HighlighterMakerBase*> makers {
        new HighlighterMaker<ProcyonSyntaxHighlighter>("procyon", "Procyon memo"),
        new HighlighterMaker<PythonSyntaxHighlighter>("python", "Python code"),
    };
    return makers;
}
} // namespace

QVector<HighlighterInfo> HighlighterManager::highlighters() const
{
    QVector<HighlighterInfo> infos;
    for (auto maker : highlighterMakers())
        infos.append({ maker->name, maker->title });
    return infos;
}

QSyntaxHighlighter* HighlighterManager::makeHighlighter(const QString& name, QTextDocument *doc)
{
    for (auto maker : highlighterMakers())
        if (maker->name == name)
            return maker->make(doc);
    return nullptr;
}
