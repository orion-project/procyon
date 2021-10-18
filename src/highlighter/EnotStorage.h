#ifndef ENOT_STORAGE_H
#define ENOT_STORAGE_H

#include "OriHighlighter.h"

class EnotHighlighterStorage : public Ori::Highlighter::SpecStorage
{
public:
    QString name() const override { return QStringLiteral("enot-storage"); }
    bool readOnly() const override { return false; }
    QVector<Ori::Highlighter::Meta> loadMetas() const override;
    QSharedPointer<Ori::Highlighter::Spec> loadSpec(const Ori::Highlighter::Meta &meta, bool withRawData = false) const override;
    QString saveSpec(const QSharedPointer<Ori::Highlighter::Spec>& spec) override;
    QString deleteSpec(const Ori::Highlighter::Meta& meta) override;
};

#endif // ENOT_STORAGE_H
