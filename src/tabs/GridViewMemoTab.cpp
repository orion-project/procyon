#include "GridViewMemoTab.h"

#include "TabHelpers.h"
#include "core/Db.h"
#include "core/MemoManager.h"
#include "core/MemoType.h"

#include <QAbstractTableModel>
#include <QHeaderView>
#include <QTableView>
#include <QToolBar>
#include <QLineEdit>
#include <QMenu>

namespace {

enum { COL_ID, COL_TITLE, COL_UPDATED, COL_COUNT };

}

//------------------------------------------------------------------------------
//                            GridViewTableModel
//------------------------------------------------------------------------------

class GridViewTableModel : public QAbstractTableModel
{
public:
    GridViewTableModel(MemoItem *item) : QAbstractTableModel()
    {
        _self = item;
        _folder = item->parentFolder();
    }

    int rowCount(const QModelIndex&) const override
    {
        return _folder->memos().size();
    }

    int columnCount(const QModelIndex&) const override { return COL_COUNT; }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role == Qt::DisplayRole)
        {
            switch (orientation)
            {
            case Qt::Vertical:
                return section + 1;
            case Qt::Horizontal:
                switch (section)
                {
                case COL_ID: return tr("ID");
                case COL_TITLE: return tr("Title");
                case COL_UPDATED: return tr("Updated");
                }
            }
        }
        return QVariant();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) return QVariant();
        int col = index.column();
        int row = index.row();
        const auto& memo = _folder->memos().at(row);

        if (role == Qt::DecorationRole)
        {
            if (col == COL_ID)
            {
                return memo->type()->icon();
            }
        }
        else if (role == Qt::ToolTipRole)
        {
            if (col == COL_ID)
            {
                return memo->type()->title();
            }
        }
        else if (role == Qt::DisplayRole)
        {
            if (col == COL_ID)
                return memo->id();

            if (col == COL_TITLE)
                return memo->title();

            if (col == COL_UPDATED)
            {
                return memo->updated();
            }
        }

        return QVariant();
    }

    void itemCreating(DbItem* item, int index)
    {
        if (item->isMemo() && item->parentFolder() == _folder)
        {
            _isRowCountChanging = true;
            beginInsertRows(QModelIndex(), index, index);
        }
    }

    void itemCreated(DbItem* item)
    {
        if (_isRowCountChanging)
        {
            _isRowCountChanging = false;
            endInsertRows();
        }
    }

    void itemRemoving(DbItem* item)
    {
        if (item->isMemo() && item->parentFolder() == _folder)
        {
            _isRowCountChanging = true;
            int index = _folder->memos().indexOf(item);
            beginRemoveRows(QModelIndex(), index, index);
        }
    }

    void itemRemoved(DbItem* item)
    {
        if (_isRowCountChanging)
        {
            _isRowCountChanging = false;
            endRemoveRows();
        }
    }

private:
    MemoItem *_self;
    FolderItem *_folder;
    bool _isRowCountChanging = false;
};

//------------------------------------------------------------------------------
//                             GridViewMemoTab
//------------------------------------------------------------------------------

typedef GridViewMemoTab Self;

GridViewMemoTab::GridViewMemoTab(Db* db, MemoItem* memoItem) : MemoTab(db, memoItem)
{
    _titleEditor = TabHelpers::makeTitleEditor();

    _toolbar = TabHelpers::makeHeaderToolBar();

    _actionEdit = _toolbar->addAction(QIcon(":/toolbar/edit"), tr("Edit"), this, &Self::beginEdit);
    _actionSave = _toolbar->addAction(QIcon(":/toolbar/apply"), tr("Save"), this, &Self::saveEdit);
    _actionCancel = _toolbar->addAction(QIcon(":/toolbar/cancel"), tr("Cancel"), this, &Self::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    _toolbar->addSeparator();
    _toolbar->addAction(tr("New Memo"), this, &Self::createMemo);
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){
        if (canClose()) deleteLater();
    });

    _contextMenu = new QMenu;
    auto actionOpen = _contextMenu->addAction(tr("Open"), Qt::Key_Return, this, &Self::openSelectedMemo);

    auto toolPanel = TabHelpers::makeHeaderPanel({_titleEditor, _toolbar});

    _tableModel = new GridViewTableModel(memoItem);
    connect(_db, &Db::itemCreating, _tableModel, &GridViewTableModel::itemCreating);
    connect(_db, &Db::itemCreated, _tableModel, &GridViewTableModel::itemCreated);
    connect(_db, &Db::itemRemoved, _tableModel, &GridViewTableModel::itemRemoved);
    connect(_db, &Db::itemRemoving, _tableModel, &GridViewTableModel::itemRemoving);
    connect(_db, &Db::itemRemoved, _tableModel, &GridViewTableModel::itemRemoved);

    _tableView = new QTableView;
    _tableView->setModel(_tableModel);
    _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableView->verticalHeader()->setVisible(false);
    _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    _tableView->addAction(actionOpen);
    connect(_tableView, &QTableView::doubleClicked, this, &Self::openSelectedMemo);
    connect(_tableView, &QTableView::customContextMenuRequested, this, &Self::showContextMenu);

    auto h = _tableView->horizontalHeader();
    h->setMinimumSectionSize(32);
    h->setSectionResizeMode(COL_ID, QHeaderView::ResizeToContents);
    h->setSectionResizeMode(COL_TITLE, QHeaderView::Stretch);
    h->setSectionResizeMode(COL_UPDATED, QHeaderView::ResizeToContents);
    h->setHighlightSections(false);

    Ori::Layouts::LayoutV({toolPanel, _tableView}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
}

void GridViewMemoTab::showMemo()
{
    _titleEditor->setText(_memoItem->title());

    setWindowTitle(_memoItem->title());
}

void GridViewMemoTab::beginEdit()
{
    toggleEditMode(true);

    _titleEditor->setFocus();
    _titleEditor->selectAll();
}

void GridViewMemoTab::cancelEdit()
{
    toggleEditMode(false);
    showMemo();
}

bool GridViewMemoTab::saveEdit()
{
    MemoUpdateParam update;
    update.title = _titleEditor->text().trimmed();

    auto ok = _db->updateMemo(_memoItem, update);
    if (!ok) return false;

    setWindowTitle(_memoItem->title());
    toggleEditMode(false);
    return true;
}

void GridViewMemoTab::toggleEditMode(bool on)
{
    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    TabHelpers::setTitleEditorReadOnly(_titleEditor, !on);
}

void GridViewMemoTab::createMemo()
{
    auto memoType = MemoType::selectFromDlg();
    if (!memoType) return;

    _db->createMemo(_memoItem->parentFolder(), memoType);
}

DbItem* GridViewMemoTab::selectedItem() const
{
    QModelIndexList selection = _tableView->selectionModel()->selectedRows();
    if (selection.empty()) return nullptr;
    int row = selection.at(0).row();
    return _memoItem->parentFolder()->memos().at(row);
}

void GridViewMemoTab::showContextMenu(const QPoint& pos)
{
    auto dbItem = selectedItem();
    if (dbItem->isMemo())
        _contextMenu->popup(_tableView->mapToGlobal(pos));
}

void GridViewMemoTab::openSelectedMemo()
{
    auto dbItem = selectedItem();
    if (dbItem->isMemo())
        emit memoOpenRequested(dbItem->asMemo());
}
