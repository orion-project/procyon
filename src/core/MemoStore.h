#ifndef MEMO_STORE_H
#define MEMO_STORE_H

#include <QString>
#include <QMap>
#include <QVariant>

class MemoItem;
struct MemoUpdateParam;

struct MemosResult
{
    QString error;
    QStringList warnings;

    struct Item { int folderId; MemoItem* memo; };
    QList<Item> items;
};

class MemoStore
{
public:
    QString prepare();

    QString create(MemoItem* item) const;
    QString update(MemoItem *item, const MemoUpdateParam& update) const;
    QString remove(MemoItem* item) const;
    QString load(MemoItem *memo) const;
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
