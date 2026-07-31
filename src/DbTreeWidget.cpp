#include "DbTreeWidget.h"

#include "db/Db.h"
#include "db/MemoType.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"

#include <QMenu>
#include <QTreeView>

#include <functional>

class DbTreeModel : public QAbstractItemModel
{
public:
    DbTreeModel(Db* db) : _db(db) {}

    static DbItem* dbItem(const QModelIndex &index)
    {
        return static_cast<DbItem*>(index.internalPointer());
    }

    static FolderItem* asFolder(const QModelIndex &index)
    {
        auto item = dbItem(index);
        return item ? item->asFolder() : nullptr;
    }
    
    QModelIndex findIndex(DbItem* item, const QModelIndex &parent = QModelIndex())
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

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        DbItem* item = nullptr;

        if (!parent.isValid())
        {
            // We show the root AND all its children at the top level.
            if (row == 0)
                item = _db->root();
            else if (row-1 < _db->root()->children().size())
                item = _db->root()->children().at(row-1);
        }
        else
        {
            auto parentFolder = asFolder(parent);
            if (parentFolder && row < parentFolder->children().size())
                item = parentFolder->children().at(row);
        }

        return item ? createIndex(row, column, item) : QModelIndex();
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid()) return QModelIndex();
        
        auto item = dbItem(child);
        if (!item) return QModelIndex();
    
        auto parentFolder = item->parentFolder();
        if (!parentFolder || parentFolder->isRoot())
        {
            // Root and all its children are on the top level
            return QModelIndex();
        }

        // Row index of the parent folder inside its own parent
        int row;

        auto superParentFolder = parentFolder->parentFolder();
        if (!superParentFolder)
        {
            // The parent of parent is the root
            // This should not be the case because the root tree-item has no children
            row = 0;
        }
        else
        {
            row = superParentFolder->children().indexOf(parentFolder);

            if (superParentFolder->isRoot())
            {
                // Increase for the root item
                // which is in the tree at the top level alongside with its own children
                row++;
            }
        }

        return createIndex(row, 0, parentFolder);
    }
    
    int rowCount(const QModelIndex &parent) const override
    {
        // When there is no parent, then we need top level items.
        // We show the root AND all its children at the top level.
        if (!parent.isValid())
            return _db->root()->children().size() + 1;
    
        auto folder = asFolder(parent);
        if (!folder)
            return 0;

        // Since we display the root and its children at the top level
        // there are no rows for the root folder
        if (folder->isRoot())
            return 0;

        return folder->children().size();
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
            
        auto item = dbItem(index);
        if (!item) return QVariant();
    
        switch (role)
        {
        case Qt::DisplayRole:
            return item->title();
    
        case Qt::UserRole:
            return item->id();
    
        case Qt::DecorationRole:
            if (item->isRoot())
                return _iconRoot;
            // TODO different icons for opened and closed folder
            if (item->isFolder())
                return _iconFolder;
            if (item->isMemo())
                return item->asMemo()->type()->icon();
            return _iconMemo;
        }
        return QVariant();
    }

    void itemRenamed(const QModelIndex &index)
    {
        if (!index.isValid())
        {
            qWarning() << "DbTreeModel::itemRenamed(): invalid index";
            return;
        }
        emit dataChanged(index, index);
    }

    void addItem(QTreeView *treeView, std::function<bool()> makeItem)
    {
        QModelIndex parentIndex = treeView->currentIndex();
        DbItem* parentItem = DbTreeModel::dbItem(parentIndex);
        if (!parentItem) return;
        if (parentItem->isRoot())
        {
            // If the root item is selected then we actually insert a new item
            // not inside it but at the top level (which doesn't have parent)
            parentIndex = QModelIndex();
        }
        int row = rowCount(parentIndex);
        // Shoul be called before underlying data mutation
        beginInsertRows(parentIndex, row, row);
        bool ok = makeItem();
        endInsertRows();
        if (!ok)
            return;
        // Invalid parent index means we insert at the top level
        if (parentIndex.isValid() && !treeView->isExpanded(parentIndex))
            treeView->expand(parentIndex);
        auto newIndex = index(row, 0, parentIndex);
        if (newIndex.isValid())
            treeView->setCurrentIndex(newIndex);
    }

    void removeItem(QTreeView *treeView, std::function<bool()> deleteItem)
    {
        QModelIndex removingIndex = treeView->currentIndex();
        DbItem* removingItem = dbItem(removingIndex);
        if (!removingItem) return;
        bool memoRemoved = removingItem->isMemo();

        DbItem* parentItem = removingItem->parent();
        // Only root item has no parent, it can't be removed
        if (!parentItem) return;

        QModelIndex parentIndex = parent(removingIndex);
        int row = removingIndex.row();
        beginRemoveRows(parentIndex, row, row);
        bool ok = deleteItem();
        endRemoveRows();
        if (!ok)
            return;
        parentIndex = findIndex(parentItem);
        QModelIndex currentIndex = parentIndex;
        if (memoRemoved)
        {
            int remainingRows = rowCount(parentIndex);
            if (remainingRows > 0)
            {
                if (row >= remainingRows)
                    row = remainingRows - 1;
                currentIndex = index(row, 0, parentIndex);
            }
        }
        treeView->setCurrentIndex(currentIndex);
    }

private:
    Db* _db;
    QIcon _iconRoot = QIcon(":/icon/main");
    QIcon _iconFolder = QIcon(":/icon/folder");
    QIcon _iconMemo = QIcon(":/icon/memo_plain_text");
};

//------------------------------------------------------------------------------

typedef DbTreeWidget Self;

DbTreeWidget::DbTreeWidget() : QWidget()
{
    _rootMenu = new QMenu(this);
    _rootMenu->addAction(tr("New Memo..."), this, &Self::createMemo);
    _rootMenu->addAction(tr("New Folder..."), this, &Self::createFolder);

    _folderMenu = new QMenu(this);
    _folderMenu->addAction(tr("New Memo..."), this, &Self::createMemo);
    _folderMenu->addAction(tr("New Folder..."), this, &Self::createFolder);
    _folderMenu->addAction(tr("Rename Folder..."), this, &Self::renameFolder);
    _folderMenu->addAction(tr("Delete Folder"), this, &Self::deleteFolder);

    _memoMenu = new QMenu(this);
    _memoMenu->addAction(tr("Open"), this, &Self::openMemo);
    _memoMenu->addAction(tr("Delete"), this, &Self::deleteMemo);

    _treeView = new QTreeView;
    _treeView->setObjectName("tree_view");
    _treeView->setHeaderHidden(true);
    _treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_treeView, &QTreeView::customContextMenuRequested, this, &Self::contextMenuRequested);
    connect(_treeView, &QTreeView::doubleClicked, this, [this]{ openMemo(); });

    Ori::Layouts::LayoutV({_treeView}).setMargin(0).setSpacing(0).useFor(this);
}

void DbTreeWidget::setDb(Db* db)
{
    if (_db)
        disconnect(_db, &Db::memoUpdated, this, &Self::memoUpdated);

    _db = db;
    if (_model)
    {
        delete _model;
        _model = nullptr;
    }
    if (_db)
    {
        _model = new DbTreeModel(_db);
        connect(_db, &Db::memoUpdated, this, &Self::memoUpdated);
    }
    _treeView->setModel(_model);
}

DbItem* DbTreeWidget::selectedItem() const
{
    return DbTreeModel::dbItem(_treeView->currentIndex());
}

void DbTreeWidget::contextMenuRequested(const QPoint &pos)
{
    if (!_model) return;

    auto item = selectedItem();
    if (!item) return;

    QMenu* menu = nullptr;
    if (item->isRoot())
        menu = _rootMenu;
    else if (item->isFolder())
        menu = _folderMenu;
    else if (item->isMemo())
        menu = _memoMenu;

    if (menu)
        menu->popup(_treeView->mapToGlobal(pos));
}

void DbTreeWidget::openMemo()
{
    if (!_model) return;

    auto item = selectedItem();
    if (!item || !item->isMemo()) return;

    emit memoOpenRequested(item->asMemo());
}

void DbTreeWidget::createFolder()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto title = Ori::Dlg::inputText(tr("Folder title:"), "");
    if (title.isEmpty()) return;

    _model->addItem(_treeView, [this, item, title] {
        return _db->createFolder(item->asFolder(), title).ok();
    });
}

void DbTreeWidget::renameFolder()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto title = Ori::Dlg::inputText(tr("Folder title:"), item->title());
    if (title.isEmpty()) return;

    bool ok = _db->renameFolder(item->asFolder(), title);
    if (!ok) return;

    _model->itemRenamed(_treeView->currentIndex());
    // TODO do something about items sorted after renaming
}

void DbTreeWidget::deleteFolder()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto confirm = tr("Are you sure to delete folder '%1' and all its content?").arg(item->title());
    if (!Ori::Dlg::yes(confirm)) return;

    _model->removeItem(_treeView, [this, item]{
        return _db->removeFolder(item->asFolder());
    });
}

void DbTreeWidget::createMemo()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto memoType = MemoType::selectFromDlg();
    if (!memoType) return;

    _model->addItem(_treeView, [this, item, memoType]{
        return _db->createMemo(item->asFolder(), memoType).ok();
    });
}

void DbTreeWidget::deleteMemo()
{
    auto item = selectedItem();
    if (!item || !item->isMemo()) return;

    auto confirm = tr("Are you sure to delete memo '%1'?").arg(item->title());
    if (!Ori::Dlg::yes(confirm)) return;

    _model->removeItem(_treeView, [this, item]{
        return _db->removeMemo(item->asMemo());
    });
}

void DbTreeWidget::memoUpdated(MemoItem* item)
{
    auto index = _model->findIndex(item);
    if (index.isValid())
        _model->itemRenamed(index);
}

QStringList DbTreeWidget::getExpandedIds() const
{
    QStringList ids;
    std::function<void(const QModelIndex&)> fillExpandedIds;

    fillExpandedIds = [this, &ids, &fillExpandedIds](const QModelIndex& parent){
        int rowCount = _model->rowCount(parent);
        for (int row = 0; row < rowCount; row++)
        {
            auto index = _model->index(row, 0, parent);
            if (_model->dbItem(index)->isFolder())
            {
                if (_treeView->isExpanded(index))
                    ids << QString::number(_model->data(index, Qt::UserRole).toInt());
                fillExpandedIds(index);
            }
        }
    };

    fillExpandedIds(QModelIndex());
    return ids;
}

void DbTreeWidget::setExpandedIds(const QStringList& ids)
{
    std::function<void(const QModelIndex& parent)> setExpandedIds;

    setExpandedIds = [this, &ids, &setExpandedIds](const QModelIndex& parent){
        int rowCount = _model->rowCount(parent);
        for (int row = 0; row < rowCount; row++)
        {
            auto index = _model->index(row, 0, parent);
            auto data = _model->data(index, Qt::UserRole);
            if (data.isNull()) continue;
            if (ids.contains(QString::number(data.toInt())))
                _treeView->expand(index);
            setExpandedIds(index);
        }
    };

    setExpandedIds(QModelIndex());
}
