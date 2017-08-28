#ifndef CATALOGMODEL_H
#define CATALOGMODEL_H

#include <QAbstractItemModel>
#include <QDebug>
#include <QIcon>

#include "Catalog.h"

class CatalogModel : public QAbstractItemModel
{
public:
    CatalogModel(Catalog* catalog) : _catalog(catalog)
    {
        _iconMemo = QIcon(":/icon/memo_default");
        _iconFolder = QIcon(":/icon/folder_closed");
    }

    static CatalogItem* catalogItem(const QModelIndex &index)
    {
        return static_cast<CatalogItem*>(index.internalPointer());
    }

    QModelIndex findIndex(CatalogItem* item, const QModelIndex &parent = QModelIndex())
    {
        int rows = rowCount(parent);
        for (int row = 0; row < rows; row++)
        {
            auto currentIndex = index(row, 0, parent);
            auto currentItem = catalogItem(currentIndex);
            if (currentItem == item) return currentIndex;

            return findIndex(item, currentIndex);
        }
        return QModelIndex();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        if (!parent.isValid())
        {
            if (row < _catalog->items().size())
                return createIndex(row, column, _catalog->items().at(row));
            return QModelIndex();
        }

        auto parentItem = catalogItem(parent);
        if (!parentItem) return QModelIndex();

        auto parentFolder = parentItem->asFolder();
        if (!parentFolder) return QModelIndex();

        if (row < parentFolder->children().size())
            return createIndex(row, column, parentFolder->children().at(row));

        return QModelIndex();
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid()) return QModelIndex();

        auto childItem = catalogItem(child);
        if (!childItem) return QModelIndex();

        auto parentItem = childItem->parent();
        if (!parentItem) return QModelIndex();

        int row = parentItem->parent()
                ? parentItem->parent()->asFolder()->children().indexOf(parentItem)
                : _catalog->items().indexOf(parentItem);

        return createIndex(row, 0, parentItem);
    }

    int rowCount(const QModelIndex &parent) const override
    {
        if (!parent.isValid())
            return _catalog->items().size();

        auto item = catalogItem(parent);
        return item && item->isFolder() ? item->asFolder()->children().size() : 0;
    }

    int columnCount(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent)
        return 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();
        if (role == Qt::DisplayRole)
        {
            auto item = catalogItem(index);
            return item ? item->title() : QVariant();
        }
        if (role == Qt::DecorationRole)
        {
            auto item = catalogItem(index);
            if (!item) return QVariant();
            // TODO different icons for opened and closed folder
            if (item->isFolder())
                return _iconFolder;
            if (item->asMemo()->type())
                return item->asMemo()->type()->icon();
            return _iconMemo;
        }
        return QVariant();
    }

    void itemRenamed(const QModelIndex &index)
    {
        emit dataChanged(index, index);
    }

    QModelIndex itemAdded(const QModelIndex &parent)
    {
        int row = rowCount(parent) - 1;
        beginInsertRows(parent, row, row);
        endInsertRows();
        return index(row, 0, parent);
    }

    friend class ItemRemoverGuard;

    const QIcon& folderIcon() const { return _iconFolder; }

private:
    Catalog* _catalog;
    QIcon _iconFolder, _iconMemo;
};


class ItemRemoverGuard
{
public:
    ItemRemoverGuard(CatalogModel* model, const QModelIndex &removingIndex) : _model(model)
    {
        parentIndex = _model->parent(removingIndex);
        _model->beginRemoveRows(parentIndex, removingIndex.row(), removingIndex.row());
    }

    ~ItemRemoverGuard()
    {
        _model->endRemoveRows();
    }

    QModelIndex parentIndex;

private:
    CatalogModel* _model;
};

#endif // CATALOGMODEL_H
