#include "TreeWidget.h"

#include "core/Enot.h"
#include "core/MemoType.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"

#include <QMenu>
#include <QTreeView>
#include <QTimer>

class TreeModel : public QAbstractItemModel
{
public:
    TreeModel(Enot* enot) : _enot(enot) {}

    static DbItem* dbItem(const QModelIndex &index)
    {
        return static_cast<DbItem*>(index.internalPointer());
    }

    static FolderItem* asFolder(const QModelIndex &index)
    {
        auto item = dbItem(index);
        return item ? item->asFolder() : nullptr;
    }

    int columnCount(const QModelIndex &parent) const override
    {
        Q_UNUSED(parent)
        return 1;
    }

    int rowCount(const QModelIndex &parent) const override
    {
        // When there is no parent, then we need top level items.
        // We show the root AND all its children at the top level.
        if (!parent.isValid())
            return _enot->root()->childCount() + 1;

        auto folder = asFolder(parent);
        if (!folder)
            return 0;

        // Since we display the root and its children at the top level
        // there are no rows for the root folder
        if (folder->isRoot())
            return 0;

        return folder->childCount();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        DbItem* item = nullptr;
        FolderItem* parentFolder = nullptr;
        int index;

        if (!parent.isValid())
        {
            // We show the root AND all its children at the top level.
            if (row == 0)
                item = _enot->root();
            else
            {
                parentFolder = _enot->root();
                index = row - 1;
            }
        }
        else
        {
            parentFolder = asFolder(parent);
            index = row;
        }

        if (!item && parentFolder)
        {
            if (index < parentFolder->memos().size())
                item = parentFolder->memos().at(index);
            else
            {
                index -= parentFolder->memos().size();
                if (index < parentFolder->folders().size())
                    item = parentFolder->folders().at(index);
            }
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
            row = superParentFolder->folders().indexOf(parentFolder);

            if (superParentFolder->isRoot())
            {
                // Increase for the root item
                // which is in the tree at the top level alongside with its own children
                row++;
            }
        }

        return createIndex(row, 0, parentFolder);
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

    void itemRenamed(DbItem* item)
    {
        auto index = findIndex(item);
        if (!index.isValid())
        {
            qWarning() << "DbTreeModel::itemRenamed(): invalid index";
            return;
        }
        emit dataChanged(index, index);
    }

    void reset()
    {
        beginResetModel();
        endResetModel();
    }

private:
    Enot* _enot;
    QIcon _iconRoot = QIcon(":/icon/main");
    QIcon _iconFolder = QIcon(":/icon/folder");
    QIcon _iconMemo = QIcon(":/icon/memo_plain_text");
};

//------------------------------------------------------------------------------

typedef TreeWidget Self;

TreeWidget::TreeWidget() : QWidget()
{
    _rootMenu = new QMenu(this);
    // TODO: Can't insert memo at the top level because of FK violation
    //_rootMenu->addAction(tr("New Memo..."), this, &Self::createMemo);
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

void TreeWidget::setEnot(Enot* enot)
{
    _enot = enot;
    if (_model)
    {
        delete _model;
        _model = nullptr;
    }
    if (_enot)
    {
        _model = new TreeModel(_enot);
        connect(_enot, &Enot::itemCreating, this, &Self::itemCreating);
        connect(_enot, &Enot::itemCreated, this, &Self::itemCreated);
        connect(_enot, &Enot::itemUpdated, this, &Self::itemUpdated);
        connect(_enot, &Enot::itemRemoving, this, &Self::itemRemoving);
        connect(_enot, &Enot::itemRemoved, this, &Self::itemRemoved);
    }
    _treeView->setModel(_model);
}

DbItem* TreeWidget::selectedItem() const
{
    return TreeModel::dbItem(_treeView->currentIndex());
}

void TreeWidget::contextMenuRequested(const QPoint &pos)
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

void TreeWidget::selectItem(DbItem* item)
{
    QTimer::singleShot(0, this, [this, item]{
        auto index = _model->findIndex(item);
        if (!index.isValid()) return;

        auto parentIndex = _model->findIndex(item->parentFolder());
        if (parentIndex.isValid() && !_treeView->isExpanded(parentIndex))
            _treeView->setExpanded(parentIndex, true);

        _treeView->setCurrentIndex(index);
    });
}

void TreeWidget::openMemo()
{
    if (!_model) return;

    auto item = selectedItem();
    if (!item || !item->isMemo()) return;

    emit memoOpenRequested(item->asMemo());
}

void TreeWidget::createFolder()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto title = Ori::Dlg::inputText(tr("Folder title:"), "");
    if (title.isEmpty()) return;

    auto res = _enot->createFolder(item->asFolder(), title);
    if (!res.ok()) return;

    selectItem(res.result());
}

void TreeWidget::renameFolder()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto title = Ori::Dlg::inputText(tr("Folder title:"), item->title());
    if (title.isEmpty()) return;

    _enot->renameFolder(item->asFolder(), title);
}

void TreeWidget::deleteFolder()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto confirm = tr("Are you sure to delete folder '%1' and all its content?").arg(item->title());
    if (!Ori::Dlg::yes(confirm)) return;

    auto parentFolder = item->parentFolder();

    bool ok = _enot->removeFolder(item->asFolder());
    if (!ok) return;

    QTimer::singleShot(0, this, [this, parentFolder]{
        _treeView->setCurrentIndex(_model->findIndex(parentFolder));
    });
}

void TreeWidget::createMemo()
{
    auto item = selectedItem();
    if (!item || !item->isFolder()) return;

    auto memoType = MemoType::selectFromDlg();
    if (!memoType) return;

    auto res = _enot->createMemo(item->asFolder(), memoType);
    if (!res.ok()) return;

    selectItem(res.result());
}

void TreeWidget::deleteMemo()
{
    auto item = selectedItem();
    if (!item || !item->isMemo()) return;

    auto confirm = tr("Are you sure to delete memo '%1'?").arg(item->title());
    if (!Ori::Dlg::yes(confirm)) return;

    auto parentFolder = item->parentFolder();

    bool ok = _enot->removeMemo(item->asMemo());
    if (!ok) return;

    QTimer::singleShot(0, this, [this, parentFolder]{
        _treeView->setCurrentIndex(_model->findIndex(parentFolder));
    });
}

void TreeWidget::itemCreating(DbItem* item, int index)
{
    stashExpandedIds();
}

void TreeWidget::itemCreated(DbItem* item)
{
    _model->reset();
    QTimer::singleShot(0, this, &Self::applyExpandedIds);
}

void TreeWidget::itemUpdated(DbItem* item)
{
    _model->itemRenamed(item);
}

void TreeWidget::itemRemoving(DbItem* item)
{
    if (item->isMemo() && _isFolderRemoving)
        return;

    if (item->isFolder())
        _isFolderRemoving = true;

    stashExpandedIds();
}

void TreeWidget::itemRemoved(DbItem* item)
{
    if (item->isMemo() && _isFolderRemoving)
        return;

    if (item->isFolder())
        _isFolderRemoving = false;

    _model->reset();
    QTimer::singleShot(0, this, &Self::applyExpandedIds);
}

void TreeWidget::stashExpandedIds()
{
    std::function<void(const QModelIndex&)> fillExpandedIds;

    fillExpandedIds = [this, &fillExpandedIds](const QModelIndex& parent){
        int rowCount = _model->rowCount(parent);
        for (int row = 0; row < rowCount; row++)
        {
            auto index = _model->index(row, 0, parent);
            auto folder = _model->dbItem(index)->asFolder();
            if (folder)
            {
                if (_treeView->isExpanded(index))
                    _expandedIds << folder->id();
                fillExpandedIds(index);
            }
        }
    };

    _expandedIds.clear();
    fillExpandedIds(QModelIndex());
}

void TreeWidget::applyExpandedIds()
{
    std::function<void(const QModelIndex& parent)> expandFolders;

    expandFolders = [this, &expandFolders](const QModelIndex& parent){
        int rowCount = _model->rowCount(parent);
        for (int row = 0; row < rowCount; row++)
        {
            auto index = _model->index(row, 0, parent);
            auto folder = _model->dbItem(index)->asFolder();
            if (folder)
            {
                if (_expandedIds.contains(folder->id()))
                    _treeView->expand(index);
                expandFolders(index);
            }
        }
    };

    expandFolders(QModelIndex());
}

QStringList TreeWidget::getExpandedIds()
{
    stashExpandedIds();
    QStringList ids;
    for (int id : std::as_const(_expandedIds))
        ids << QString::number(id);
    return ids;
}

void TreeWidget::setExpandedIds(const QStringList& ids)
{
    _expandedIds.clear();
    for (const auto &id : ids)
        _expandedIds << id.toInt();
    applyExpandedIds();
}
