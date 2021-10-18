#include "EnotStorage.h"
#include "../catalog/CatalogStore.h"

using namespace Ori::Highlighter;

QString EnotHighlighterStorage::name() const
{
    return QStringLiteral("enot-storage");
}

bool EnotHighlighterStorage::readOnly() const
{
    return false;
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
        meta.source = QStringLiteral("highlighter/%1/spec").arg(meta.name);
        metas << meta;
        it++;
    }
    return metas;
}

QSharedPointer<Spec> EnotHighlighterStorage::loadSpec(const QString &source, bool withRawData) const
{
    auto text = CatalogStore::settingsManager()->readString(source, QStringLiteral("doen not exist"));
    if (text == QStringLiteral("doen not exist"))
        return QSharedPointer<Spec>();
    QSharedPointer<Spec> spec(new Spec);
    loadSpecRaw(spec, source, &text, withRawData);
    return spec;
}

QString EnotHighlighterStorage::saveSpec(const QSharedPointer<Spec>& spec)
{
    auto res = CatalogStore::settingsManager()->writeString(QStringLiteral("highlighter/%1/meta").arg(spec->meta.name), spec->meta.title);
    if (res.isEmpty())
        res = CatalogStore::settingsManager()->writeString(QStringLiteral("highlighter/%1/spec").arg(spec->meta.name), spec->storableString());
    return res;
}
