#ifndef DBTREE_MODEL_H
#define DBTREE_MODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QIcon>

class Db;
class DbItem;

class DbTreeModel : public QAbstractItemModel
{
public:
    DbTreeModel(Db* db);

    static DbItem* dbItem(const QModelIndex &index);

    QModelIndex findIndex(DbItem* item, const QModelIndex &parent = QModelIndex());

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

    void itemRenamed(const QModelIndex &index);
    QModelIndex itemAdded(const QModelIndex &parent);

    friend class ItemRemoverGuard;

    const QIcon& folderIcon() const { return _iconFolder; }
    const QIcon& memoIcon() const { return _iconMemo; }
private:
    Db* _db;
    QIcon _iconFolder, _iconMemo;
};


class ItemRemoverGuard
{
public:
    ItemRemoverGuard(DbTreeModel* model, const QModelIndex &removingIndex);
    ~ItemRemoverGuard();
    QModelIndex parentIndex;
private:
    DbTreeModel* _model;
};

#endif // DBTREE_MODEL_H
