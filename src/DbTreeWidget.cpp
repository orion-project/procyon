#include "DbTreeWidget.h"

#include "db/Db.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriDialogs.h"
#include "widgets/OriSelectableTile.h"

#include <QApplication>
#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QTreeView>
#include <QWidgetAction>
#include <QFileInfo>

class DbTreeModel : public QAbstractItemModel
{
public:
    DbTreeModel(Db* db) : _db(db) {}

    static DbItem* dbItem(const QModelIndex &index)
    {
        return static_cast<DbItem*>(index.internalPointer());
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

    QModelIndex parent(const QModelIndex &child) const override
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
    
    int rowCount(const QModelIndex &parent) const override
    {
        if (!parent.isValid())
            return _db->topItems().size();
    
        auto item = dbItem(parent);
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

    void itemRenamed(const QModelIndex &index)
    {
        if (!index.isValid())
        {
            qWarning() << "DbTreeModel::itemRenamed(): invalid index";
            return;
        }
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
    const QIcon& memoIcon() const { return _iconMemo; }
    
private:
    Db* _db;
    QIcon _iconFolder = QIcon(":/icon/folder");
    QIcon _iconMemo = QIcon(":/icon/memo_plain_text");
};

//------------------------------------------------------------------------------

class ItemRemoverGuard
{
public:
    ItemRemoverGuard(DbTreeModel* model, const QModelIndex &removingIndex) : _model(model)
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
    DbTreeModel* _model;
};

//------------------------------------------------------------------------------

struct DbTreeSelection
{
    QModelIndex index;
    DbItem* item = nullptr;
    FolderItem* folder = nullptr;
    MemoItem* memo = nullptr;

    DbTreeSelection() {}

    DbTreeSelection(QTreeView* view)
    {
        index = view->currentIndex();
        if (!index.isValid()) return;

        item = DbTreeModel::dbItem(index);
        if (!item) return;

        folder = item->asFolder();
        memo = item->asMemo();
    }

    void selectFolderIfNone()
    {
        if (folder) return;

        if (!index.isValid()) return;
        index = index.parent();
        if (!index.isValid()) return;

        item = DbTreeModel::dbItem(index);
        if (!item) return;

        folder = item->asFolder();
        memo = nullptr;
    }
};

//------------------------------------------------------------------------------

// We have to recreate the menu header at each menu open as it only takes the correct size when created.
// Retaining the header action, we stick to the original calculated size, and it can be unsuitable
// for subsequent menu open at items having longer titles.
static void makeMenuHeader(QMenu* menu, const QIcon& icon, const QString& title)
{
    auto actions = menu->actions();
    auto prevHeader = qobject_cast<QWidgetAction*>(actions.first());
    if (prevHeader) delete prevHeader;

    auto iconLabel = new QLabel;
    iconLabel->setPixmap(icon.pixmap(16, 16));
    iconLabel->setProperty("role", "context_menu_header_icon");

    auto titleLabel = new QLabel(title);
    titleLabel->setProperty("role", "context_menu_header_text");

    auto panel = new QFrame;
    panel->setProperty("role", "context_menu_header_panel");
    Ori::Layouts::LayoutH({iconLabel, titleLabel,
        Ori::Layouts::Stretch()}).setSpacing(0).setMargin(0).useFor(panel);

    auto headerAction = new QWidgetAction(menu);
    headerAction->setDefaultWidget(panel);
    menu->insertAction(actions.first(), headerAction);
}

//------------------------------------------------------------------------------

DbTreeWidget::DbTreeWidget() : QWidget()
{
    _rootMenu = new QMenu(this);
    _rootMenu->addAction(tr("New Folder..."), this, &DbTreeWidget::createFolder);
    _rootMenu->addAction(tr("New Memo..."), this, &DbTreeWidget::createMemo);

    _folderMenu = new QMenu(this);
    _folderMenu->addAction(tr("Rename..."), this, &DbTreeWidget::renameFolder);
    _folderMenu->addAction(tr("Delete"), this, &DbTreeWidget::deleteFolder);
    _folderMenu->addSeparator();
    _folderMenu->addAction(tr("New Memo..."), this, &DbTreeWidget::createMemo);
    _folderMenu->addAction(tr("New Subfolder..."), this, &DbTreeWidget::createFolder);
    _folderMenu->addAction(tr("New Top Level Folder..."), this, &DbTreeWidget::createTopLevelFolder);

    auto openMemo = new QAction(tr("Open"));
    connect(openMemo, &QAction::triggered, this, &DbTreeWidget::openSelectedMemo);

    _memoMenu = new QMenu(this);
    _memoMenu->addAction(openMemo);
    _memoMenu->addSeparator();
    _memoMenu->addAction(tr("Delete"), this, &DbTreeWidget::deleteMemo);
    _memoMenu->addSeparator();
    _memoMenu->addAction(tr("New Memo..."), this, &DbTreeWidget::createMemo);
    _memoMenu->addAction(tr("New Subfolder..."), this, &DbTreeWidget::createFolder);
    _memoMenu->addAction(tr("New Top Level Folder..."), this, &DbTreeWidget::createTopLevelFolder);

    _treeView = new QTreeView;
    _treeView->setObjectName("tree_view");
    _treeView->setHeaderHidden(true);
    _treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_treeView, &QTreeView::customContextMenuRequested, this, &DbTreeWidget::contextMenuRequested);
    connect(_treeView, &QTreeView::doubleClicked, this, &DbTreeWidget::doubleClicked);

    Ori::Layouts::LayoutV({_treeView})
            .setMargin(0)
            .setSpacing(0)
            .useFor(this);
}

void DbTreeWidget::setDb(Db* db)
{
    if (_db)
        disconnect(_db, &Db::memoUpdated, this, &DbTreeWidget::memoUpdated);

    _db = db;
    if (_model)
    {
        delete _model;
        _model = nullptr;
    }
    if (_db)
    {
        _model = new DbTreeModel(_db);
        connect(_db, &Db::memoUpdated, this, &DbTreeWidget::memoUpdated);
        //_rootTitle->setText(QFileInfo(_db->fileName()).baseName());
    }
    _treeView->setModel(_model);
}

void DbTreeWidget::contextMenuRequested(const QPoint &pos)
{
    if (!_model) return;

    QMenu* menu = nullptr;
    DbTreeSelection selected(_treeView);
    if (!selected.item)
    {
        menu = _rootMenu;
    }
    else if (selected.folder)
    {
        makeMenuHeader(_folderMenu, _model->folderIcon(), selected.item->title());
        menu = _folderMenu;
    }
    else if (selected.memo)
    {
        makeMenuHeader(_memoMenu, selected.memo->type()->icon(), selected.item->title());
        menu = _memoMenu;
    }
    if (menu)
        menu->popup(_treeView->mapToGlobal(pos));
}

void DbTreeWidget::openSelectedMemo()
{
    if (!_model) return;

    DbTreeSelection selected(_treeView);
    if (selected.memo)
        emit onOpenMemo(selected.memo);
}

void DbTreeWidget::doubleClicked(const QModelIndex&)
{
    openSelectedMemo();
}

SelectedItems DbTreeWidget::selection() const
{
    DbTreeSelection selected(_treeView);
    SelectedItems result;
    result.memo = selected.memo;
    result.folder = selected.folder;
    return result;
}

void DbTreeWidget::createFolder()
{
    if (_db->topItems().isEmpty())
        return createTopLevelFolder();

    DbTreeSelection selection(_treeView);
    selection.selectFolderIfNone();
    if (selection.folder)
        createFolderInternal(selection);
}

void DbTreeWidget::createTopLevelFolder()
{
    createFolderInternal(DbTreeSelection());
}

void DbTreeWidget::createFolderInternal(const DbTreeSelection& selection)
{
    auto title = Ori::Dlg::inputText(tr("Enter a title for new folder"), "");
    if (title.isEmpty()) return;

    auto res = _db->createFolder(selection.folder, title);
    if (!res.ok()) return Ori::Dlg::error(res.error());

    // TODO do not know about item inserted at the end and select by pointer
    auto newIndex = _model->itemAdded(selection.index);
    if (!_treeView->isExpanded(selection.index))
        _treeView->expand(selection.index);
    _treeView->setCurrentIndex(newIndex);
}

void DbTreeWidget::renameFolder()
{
    DbTreeSelection selected(_treeView);
    if (!selected.folder) return;

    auto title = Ori::Dlg::inputText(tr("Enter new title for folder"), selected.folder->title());
    if (title.isEmpty()) return;

    auto res = _db->renameFolder(selected.folder, title);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    _model->itemRenamed(selected.index);
    // TODO do something about items sorted after renaming
}

void DbTreeWidget::deleteFolder()
{
    DbTreeSelection selected(_treeView);
    if (!selected.folder) return;

    auto confirm = tr("Are you sure to delete folder '%1' and all its content?\n\n"
                      "This action can't be undone.").arg(selected.folder->title());
    if (!Ori::Dlg::yes(confirm)) return;

    ItemRemoverGuard guard(_model, selected.index);

    auto res = _db->removeFolder(selected.folder);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    _treeView->setCurrentIndex(guard.parentIndex);
}

static MemoType* selectMemoTypeDlg()
{
    Ori::Widgets::SelectableTileRadioGroup tripTypeGroup;

    auto tripTypeLayout = new QHBoxLayout();
    tripTypeLayout->setContentsMargins(0, 0, 0, 0);
    tripTypeLayout->setSpacing(12);
    for (auto memoType : QVector<MemoType*>({plainTextMemoType(), markdownMemoType()}))
    {
        auto tile = new Ori::Widgets::SelectableTile;
        tile->setPixmap(memoType->icon().pixmap(48, 48));
        tile->setTitle(memoType->title());
        tile->setData(QVariant::fromValue(reinterpret_cast<void*>(memoType)));
        tile->setTitleStyleSheet("font-size:15px;margin:0 15px 0 15px;");
        tile->selectionFollowsFocus = true;
        tripTypeLayout->addWidget(tile);
        tripTypeGroup.addTile(tile);
    }

    QWidget content;
    Ori::Layouts::LayoutV({tripTypeLayout}).setMargin(0).setSpacing(12).useFor(&content);

    auto dlg = Ori::Dlg::Dialog(&content, false)
            .withTitle(qApp->tr("Choose Memo Type"))
            .withContentToButtonsSpacingFactor(3)
            .withOkSignal(&tripTypeGroup, SIGNAL(doubleClicked(QVariant)));
    if (dlg.exec())
        return reinterpret_cast<MemoType*>(tripTypeGroup.selectedData().value<void*>());
    return nullptr;
}


void DbTreeWidget::createMemo()
{
    if (_db->topItems().isEmpty())
    {
        Ori::Dlg::info(tr("Db is empty, you have to create at least one top level folder first"));
        createTopLevelFolder();
    }

    DbTreeSelection selection(_treeView);
    selection.selectFolderIfNone();
    if (!selection.folder) return;

    auto memoType = selectMemoTypeDlg();
    if (!memoType) return;

    auto memoItem = new MemoItem;
    auto res = _db->createMemo(selection.folder, memoItem, memoType);
    if (!res.ok())
    {
        delete memoItem;
        return Ori::Dlg::error(res.error());
    }

    // TODO do not know about item inserted at the end and select by pointer
    auto newIndex = _model->itemAdded(selection.index);
    if (!_treeView->isExpanded(selection.index))
        _treeView->expand(selection.index);
    _treeView->setCurrentIndex(newIndex);
}

void DbTreeWidget::deleteMemo()
{
    DbTreeSelection selected(_treeView);
    if (!selected.memo) return;

    auto confirm = tr("Are you sure to delete memo '%1'?\n\n"
                      "This action can't be undone.").arg(selected.memo->title());
    if (!Ori::Dlg::yes(confirm)) return;

    ItemRemoverGuard guard(_model, selected.index);

    auto res = _db->removeMemo(selected.memo);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    _treeView->setCurrentIndex(guard.parentIndex);
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
    fillExpandedIds(ids, QModelIndex());
    return ids;
}

void DbTreeWidget::setExpandedIds(const QStringList& ids)
{
    setExpandedIds(ids, QModelIndex());
}

void DbTreeWidget::fillExpandedIds(QStringList& ids, const QModelIndex& parentIndex) const
{
    int rowCount = _model->rowCount(parentIndex);
    for (int row = 0; row < rowCount; row++)
    {
        auto index = _model->index(row, 0, parentIndex);
        if (_model->dbItem(index)->isFolder())
        {
            if (_treeView->isExpanded(index))
                ids << QString::number(_model->data(index, Qt::UserRole).toInt());
            fillExpandedIds(ids, index);
        }
    }
}

void DbTreeWidget::setExpandedIds(const QStringList& ids, const QModelIndex& parentIndex)
{
    int rowCount = _model->rowCount(parentIndex);
    for (int row = 0; row < rowCount; row++)
    {
        auto index = _model->index(row, 0, parentIndex);
        auto data = _model->data(index, Qt::UserRole);
        if (data.isNull()) continue;
        if (ids.contains(QString::number(data.toInt())))
            _treeView->expand(index);
        setExpandedIds(ids, index);
    }
}
