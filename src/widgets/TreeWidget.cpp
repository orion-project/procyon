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

    static Entry* asEntry(const QModelIndex &index)
    {
        return static_cast<Entry*>(index.internalPointer());
    }

    static Folder* asFolder(const QModelIndex &index)
    {
        auto entry = asEntry(index);
        return entry ? entry->asFolder() : nullptr;
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
        Entry* entry = nullptr;
        Folder* parentFolder = nullptr;
        int index;

        if (!parent.isValid())
        {
            // We show the root AND all its children at the top level.
            if (row == 0)
                entry = _enot->root();
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

        if (!entry && parentFolder)
        {
            if (index < parentFolder->memos().size())
                entry = parentFolder->memos().at(index);
            else
            {
                index -= parentFolder->memos().size();
                if (index < parentFolder->folders().size())
                    entry = parentFolder->folders().at(index);
            }
        }

        return entry ? createIndex(row, column, entry) : QModelIndex();
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid()) return QModelIndex();
        
        auto entry = asEntry(child);
        if (!entry) return QModelIndex();
    
        auto parentFolder = entry->parent();
        if (!parentFolder || parentFolder->isRoot())
        {
            // Root and all its children are on the top level
            return QModelIndex();
        }

        // Row index of the parent folder inside its own parent
        int row;

        auto superParentFolder = parentFolder->parent();
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
            
        auto entry = asEntry(index);
        if (!entry) return QVariant();
    
        switch (role)
        {
        case Qt::DisplayRole:
            return entry->title();
    
        case Qt::UserRole:
            return entry->id();
    
        case Qt::DecorationRole:
            if (entry->isRoot())
                return _iconRoot;
            // TODO different icons for opened and closed folder
            if (entry->isFolder())
                return _iconFolder;
            if (entry->isMemo())
                return entry->asMemo()->type()->icon();
            return _iconMemo;
        }
        return QVariant();
    }

    QModelIndex findIndex(Entry* entry, const QModelIndex &parent = QModelIndex())
    {
        int rows = rowCount(parent);
        for (int row = 0; row < rows; row++)
        {
            auto currentIndex = index(row, 0, parent);
            auto currentEntry = asEntry(currentIndex);
            if (currentEntry == entry) return currentIndex;

            auto targetIndex = findIndex(entry, currentIndex);
            if (targetIndex.isValid()) return targetIndex;
        }
        return QModelIndex();
    }

    void itemRenamed(Entry* item)
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
        connect(_enot, &Enot::entryCreating, this, &Self::entryCreating);
        connect(_enot, &Enot::entryCreated, this, &Self::entryCreated);
        connect(_enot, &Enot::entryUpdated, this, &Self::entryUpdated);
        connect(_enot, &Enot::entryDeleting, this, &Self::entryDeleting);
        connect(_enot, &Enot::entryDeleted, this, &Self::entryDeleted);
    }
    _treeView->setModel(_model);
}

Entry* TreeWidget::selectedEntry() const
{
    return TreeModel::asEntry(_treeView->currentIndex());
}

void TreeWidget::contextMenuRequested(const QPoint &pos)
{
    if (!_model) return;

    auto entry = selectedEntry();
    if (!entry) return;

    QMenu* menu = nullptr;
    if (entry->isRoot())
        menu = _rootMenu;
    else if (entry->isFolder())
        menu = _folderMenu;
    else if (entry->isMemo())
        menu = _memoMenu;

    if (menu)
        menu->popup(_treeView->mapToGlobal(pos));
}

void TreeWidget::selectEntry(Entry* entry)
{
    QTimer::singleShot(0, this, [this, entry]{
        auto index = _model->findIndex(entry);
        if (!index.isValid()) return;

        auto parentIndex = _model->findIndex(entry->parent());
        if (parentIndex.isValid() && !_treeView->isExpanded(parentIndex))
            _treeView->setExpanded(parentIndex, true);

        _treeView->setCurrentIndex(index);
    });
}

void TreeWidget::openMemo()
{
    if (!_model) return;

    auto entry = selectedEntry();
    if (!entry || !entry->isMemo()) return;

    emit memoOpenRequested(entry->asMemo());
}

void TreeWidget::createFolder()
{
    auto entry = selectedEntry();
    if (!entry || !entry->isFolder()) return;

    auto title = Ori::Dlg::inputText(tr("Folder title:"), "");
    if (title.isEmpty()) return;

    auto res = _enot->createFolder(entry->asFolder(), title);
    if (!res.ok()) return;

    selectEntry(res.result());
}

void TreeWidget::renameFolder()
{
    auto entry = selectedEntry();
    if (!entry || !entry->isFolder()) return;

    auto title = Ori::Dlg::inputText(tr("Folder title:"), entry->title());
    if (title.isEmpty()) return;

    _enot->renameFolder(entry->asFolder(), title);
}

void TreeWidget::deleteFolder()
{
    auto entry = selectedEntry();
    if (!entry || !entry->isFolder()) return;

    auto confirm = tr("Are you sure to delete folder '%1' and all its content?").arg(entry->title());
    if (!Ori::Dlg::yes(confirm)) return;

    auto parentFolder = entry->parent();

    bool ok = _enot->deleteFolder(entry->asFolder());
    if (!ok) return;

    QTimer::singleShot(0, this, [this, parentFolder]{
        _treeView->setCurrentIndex(_model->findIndex(parentFolder));
    });
}

void TreeWidget::createMemo()
{
    auto entry = selectedEntry();
    if (!entry || !entry->isFolder()) return;

    auto memoType = MemoType::selectFromDlg();
    if (!memoType) return;

    auto res = _enot->createMemo(entry->asFolder(), memoType);
    if (!res.ok()) return;

    selectEntry(res.result());
}

void TreeWidget::deleteMemo()
{
    auto entry = selectedEntry();
    if (!entry || !entry->isMemo()) return;

    auto confirm = tr("Are you sure to delete memo '%1'?").arg(entry->title());
    if (!Ori::Dlg::yes(confirm)) return;

    auto parentFolder = entry->parent();

    bool ok = _enot->deleteMemo(entry->asMemo());
    if (!ok) return;

    QTimer::singleShot(0, this, [this, parentFolder]{
        _treeView->setCurrentIndex(_model->findIndex(parentFolder));
    });
}

void TreeWidget::entryCreating(Entry* entry, int index)
{
    stashExpandedIds();
}

void TreeWidget::entryCreated(Entry* entry)
{
    _model->reset();
    QTimer::singleShot(0, this, &Self::applyExpandedIds);
}

void TreeWidget::entryUpdated(Entry* entry)
{
    _model->itemRenamed(entry);
}

void TreeWidget::entryDeleting(Entry* entry)
{
    if (entry->isMemo() && _isFolderDeleting)
        return;

    if (entry->isFolder())
        _isFolderDeleting = true;

    stashExpandedIds();
}

void TreeWidget::entryDeleted(Entry* entry)
{
    if (entry->isMemo() && _isFolderDeleting)
        return;

    if (entry->isFolder())
        _isFolderDeleting = false;

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
            auto folder = _model->asEntry(index)->asFolder();
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
            auto folder = _model->asEntry(index)->asFolder();
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
