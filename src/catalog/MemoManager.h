#ifndef MEMO_MANAGER_H
#define MEMO_MANAGER_H

#include <QString>
#include <QMap>

class MemoItem;
struct MemoUpdateParam;

struct MemosResult
{
    QString error;
    QStringList warnings;

    // folderId -> [memoItems]
    QMap<int, QList<MemoItem*>> items;

    // memoId -> memoItem
    QMap<int, MemoItem*> allMemos;
};

class MemoManager
{
public:
    QString prepare();

    QString create(MemoItem* item) const;
    QString update(MemoItem *item, const MemoUpdateParam& update) const;
    QString remove(MemoItem* item) const;
    QString load(MemoItem *memo) const;
    MemosResult selectAll() const;
    QString countAll(int* count) const;
};

#endif // MEMO_MANAGER_H
