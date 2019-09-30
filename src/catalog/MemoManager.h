#ifndef MEMO_MANAGER_H
#define MEMO_MANAGER_H

#include <QString>
#include <QMap>
#include <QVariant>

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
    QMap<QString, QVariant> selectOptions(int memoId) const;
    QString updateOption(int memoId, const QString& name, const QVariant& value) const;
};

#endif // MEMO_MANAGER_H
