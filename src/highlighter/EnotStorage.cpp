#include "EnotStorage.h"
#include "../catalog/CatalogStore.h"

using namespace Ori::Highlighter;

namespace  {
QString metaKey(const QString& name)
{
    return QStringLiteral("highlighter/%1/meta").arg(name);
}

QString specKey(const QString& name)
{
    return QStringLiteral("highlighter/%1/spec").arg(name);
}
}

QVector<Meta> EnotHighlighterStorage::loadMetas() const
{
    QVector<Meta> metas;
    auto settings = CatalogStore::settingsManager()->readSettings(QStringLiteral("highlighter/%%/meta"));
    auto it = settings.constBegin();
    while (it != settings.constEnd())
    {
        Meta meta;
        meta.name = it.key().split('/')[1];
        meta.title = it.value().toString();
        meta.source = specKey(meta.name);
        metas << meta;
        it++;
    }
    return metas;
}

QSharedPointer<Spec> EnotHighlighterStorage::loadSpec(const Meta &meta, bool withRawData) const
{
    auto sm = CatalogStore::settingsManager();
    auto text = sm->readString(specKey(meta.name), QStringLiteral("---"));
    if (text == QStringLiteral("---")) return QSharedPointer<Spec>();
    QSharedPointer<Spec> spec(new Spec);
    loadSpecRaw(spec, meta.source, &text, withRawData);
    return spec;
}

QString EnotHighlighterStorage::saveSpec(const QSharedPointer<Spec>& spec)
{
    auto sm = CatalogStore::settingsManager();
    auto res = sm->writeString(metaKey(spec->meta.name), spec->meta.title);
    if (res.isEmpty())
        res = sm->writeString(specKey(spec->meta.name), spec->storableString());
    return res;
}

QString EnotHighlighterStorage::deleteSpec(const Meta &meta)
{
    auto sm = CatalogStore::settingsManager();
    auto res = sm->remove(specKey(meta.name));
    if (res.isEmpty())
        res = sm->remove(metaKey(meta.name));
    return res;
}
