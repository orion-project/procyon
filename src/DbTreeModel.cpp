#include "DbTreeModel.h"

#include "db/Db.h"

DbTreeModel::DbTreeModel(Db* db) : _db(db)
{
    _iconMemo = QIcon(":/icon/memo_plain_text");
    _iconFolder = QIcon(":/icon/folder");
}

DbItem* DbTreeModel::dbItem(const QModelIndex &index)
{
    return static_cast<DbItem*>(index.internalPointer());
}

QModelIndex DbTreeModel::findIndex(DbItem* item, const QModelIndex &parent)
{
    int rows = rowCount(parent);
    for (int row = 0; row < rows; row++)
    {
        auto currentIndex = index(row, 0, parent);
        auto currentItem = dbItem(currentIndex);
        if (currentItem == item) return currentIndex;

        auto targetIndex = findIndex(item, currentIndex);
        if (targetIndex.isValid()) return targetIndex;
    }
    return QModelIndex();
}

QModelIndex DbTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        if (row < _db->topItems().size())
            return createIndex(row, column, _db->topItems().at(row));
        return QModelIndex();
    }

    auto parentItem = dbItem(parent);
    if (!parentItem) return QModelIndex();

    auto parentFolder = parentItem->asFolder();
    if (!parentFolder) return QModelIndex();

    if (row < parentFolder->children().size())
        return createIndex(row, column, parentFolder->children().at(row));

    return QModelIndex();
}

QModelIndex DbTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();

    auto childItem = dbItem(child);
    if (!childItem) return QModelIndex();

    auto parentItem = childItem->parent();
    if (!parentItem) return QModelIndex();

    int row = parentItem->parent()
            ? parentItem->parent()->asFolder()->children().indexOf(parentItem)
            : _db->topItems().indexOf(parentItem);

    return createIndex(row, 0, parentItem);
}

int DbTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return _db->topItems().size();

    auto item = dbItem(parent);
    return item && item->isFolder() ? item->asFolder()->children().size() : 0;
}

int DbTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant DbTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto item = dbItem(index);
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

void DbTreeModel::itemRenamed(const QModelIndex &index)
{
    if (!index.isValid())
    {
        qWarning() << "DbTreeModel::itemRenamed(): invalid index";
        return;
    }
    emit dataChanged(index, index);
}

QModelIndex DbTreeModel::itemAdded(const QModelIndex &parent)
{
    int row = rowCount(parent) - 1;
    beginInsertRows(parent, row, row);
    endInsertRows();
    return index(row, 0, parent);
}

//------------------------------------------------------------------------------
//                               ItemRemoverGuard
//------------------------------------------------------------------------------

ItemRemoverGuard::ItemRemoverGuard(DbTreeModel* model, const QModelIndex &removingIndex) : _model(model)
{
    parentIndex = _model->parent(removingIndex);
    _model->beginRemoveRows(parentIndex, removingIndex.row(), removingIndex.row());
}

ItemRemoverGuard::~ItemRemoverGuard()
{
    _model->endRemoveRows();
}
