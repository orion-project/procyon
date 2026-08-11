#ifndef MEMO_STORE_H
#define MEMO_STORE_H

#include <QString>
#include <QMap>
#include <QVariant>

class Memo;
struct MemoUpdateParam;

struct MemosResult
{
    QString error;
    QStringList warnings;

    struct Item { int folderId; Memo* memo; };
    QList<Item> items;
};

class MemoStore
{
public:
    QString prepare();

    QString create(Memo* item) const;
    QString update(Memo *item, const MemoUpdateParam& update) const;
    QString remove(Memo* item) const;
    QString load(Memo *memo) const;
    MemosResult selectAll() const;
    QString countAll(int* count) const;
    QMap<QString, QVariant> selectOptions(int memoId) const;
    QString updateOption(int memoId, const QString& name, const QVariant& value) const;
};

namespace Store
{
MemoStore* memos();
}

#endif // MEMO_STORE_H
