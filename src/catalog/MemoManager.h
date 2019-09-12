#ifndef MEMO_MANAGER_H
#define MEMO_MANAGER_H

#include <QString>
#include <QMap>

class Memo;
class MemoItem;

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
    QString update(Memo* memo, const QString& info) const;
    QString remove(MemoItem* item) const;
    QString load(Memo* memo) const;
    MemosResult selectAll() const;
    QString countAll(int* count) const;
};

#endif // MEMO_MANAGER_H
