#ifndef CATALOGMODEL_H
#define CATALOGMODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QIcon>

class Catalog;
class CatalogItem;

class CatalogModel : public QAbstractItemModel
{
public:
    CatalogModel(Catalog* catalog);

    static CatalogItem* catalogItem(const QModelIndex &index);

    QModelIndex findIndex(CatalogItem* item, const QModelIndex &parent = QModelIndex());

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
    Catalog* _catalog;
    QIcon _iconFolder, _iconMemo;
};


class ItemRemoverGuard
{
public:
    ItemRemoverGuard(CatalogModel* model, const QModelIndex &removingIndex);
    ~ItemRemoverGuard();
    QModelIndex parentIndex;
private:
    CatalogModel* _model;
};

#endif // CATALOGMODEL_H
