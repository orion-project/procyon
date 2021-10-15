#include "CatalogModel.h"

#include "catalog/Catalog.h"

CatalogModel::CatalogModel(Catalog* catalog) : _catalog(catalog)
{
    _iconMemo = QIcon(":/icon/memo_plain_text");
    _iconFolder = QIcon(":/icon/folder");
}

CatalogItem* CatalogModel::catalogItem(const QModelIndex &index)
{
    return static_cast<CatalogItem*>(index.internalPointer());
}

QModelIndex CatalogModel::findIndex(CatalogItem* item, const QModelIndex &parent)
{
    int rows = rowCount(parent);
    for (int row = 0; row < rows; row++)
    {
        auto currentIndex = index(row, 0, parent);
        auto currentItem = catalogItem(currentIndex);
        if (currentItem == item) return currentIndex;

        auto targetIndex = findIndex(item, currentIndex);
        if (targetIndex.isValid()) return targetIndex;
    }
    return QModelIndex();
}

QModelIndex CatalogModel::index(int row, int column, const QModelIndex &parent) const
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

QModelIndex CatalogModel::parent(const QModelIndex &child) const
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

int CatalogModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return _catalog->items().size();

    auto item = catalogItem(parent);
    return item && item->isFolder() ? item->asFolder()->children().size() : 0;
}

int CatalogModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant CatalogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto item = catalogItem(index);
    if (!item) return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
        return item->title();

    case Qt::UserRole:
        return item->id();

    case Qt::DecorationRole:
        // TODO different icons for opened and closed folder
        if (item->isFolder())
            return _iconFolder;
        if (item->isMemo())
            return item->asMemo()->type()->icon();
        return _iconMemo;
    }
    return QVariant();
}

void CatalogModel::itemRenamed(const QModelIndex &index)
{
    if (!index.isValid())
    {
        qWarning() << "CatalogModel::itemRenamed(): invalid index";
        return;
    }
    emit dataChanged(index, index);
}

QModelIndex CatalogModel::itemAdded(const QModelIndex &parent)
{
    int row = rowCount(parent) - 1;
    beginInsertRows(parent, row, row);
    endInsertRows();
    return index(row, 0, parent);
}

//------------------------------------------------------------------------------
//                               ItemRemoverGuard
//------------------------------------------------------------------------------

ItemRemoverGuard::ItemRemoverGuard(CatalogModel* model, const QModelIndex &removingIndex) : _model(model)
{
    parentIndex = _model->parent(removingIndex);
    _model->beginRemoveRows(parentIndex, removingIndex.row(), removingIndex.row());
}

ItemRemoverGuard::~ItemRemoverGuard()
{
    _model->endRemoveRows();
}
