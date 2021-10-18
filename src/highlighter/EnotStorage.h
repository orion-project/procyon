#ifndef ENOT_STORAGE_H
#define ENOT_STORAGE_H

#include "OriHighlighter.h"

class EnotHighlighterStorage : public Ori::Highlighter::SpecStorage
{
public:
    QString name() const override;
    bool readOnly() const override;
    QVector<Ori::Highlighter::Meta> loadMetas() const override;
    QSharedPointer<Ori::Highlighter::Spec> loadSpec(const QString &source, bool withRawData = false) const override;
    QString saveSpec(const QSharedPointer<Ori::Highlighter::Spec>& spec) override;
};

#endif // ENOT_STORAGE_H
