#include "CatalogWidget.h"
#include "CatalogModel.h"
#include "catalog/Catalog.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriDialogs.h"

#include <QFrame>
#include <QLabel>
#include <QMenu>
#include <QTreeView>
#include <QWidgetAction>

struct CatalogSelection
{
    QModelIndex index;
    CatalogItem* item = nullptr;
    FolderItem* folder = nullptr;
    MemoItem* memo = nullptr;

    CatalogSelection() {}

    CatalogSelection(QTreeView* view)
    {
        index = view->currentIndex();
        if (!index.isValid()) return;

        item = CatalogModel::catalogItem(index);
        if (!item) return;

        folder = item->asFolder();
        memo = item->asMemo();
    }
};

namespace {
QAction* makeMenuHeader(QWidget* parent, QLabel*& iconLabel, QLabel*& titleLabel)
{
    iconLabel = new QLabel;
    iconLabel->setProperty("role", "context_menu_header_icon");

    titleLabel = new QLabel;
    titleLabel->setProperty("role", "context_menu_header_text");

    auto panel = new QFrame;
    panel->setProperty("role", "context_menu_header_panel");
    Ori::Layouts::LayoutH({iconLabel, titleLabel,
        Ori::Layouts::Stretch()}).setSpacing(0).setMargin(0).useFor(panel);

    auto action = new QWidgetAction(parent);
    action->setDefaultWidget(panel);
    return action;
}
} // namespace

CatalogWidget::CatalogWidget() : QWidget()
{
    _rootMenu = new QMenu(this);
    _rootMenu->addAction(tr("New Folder..."), this, &CatalogWidget::createFolder);
    _rootMenu->addAction(tr("New Memo"), this, &CatalogWidget::createMemo);

    _folderMenu = new QMenu(this);
    _folderMenu->addAction(makeMenuHeader(this, _folderMenuIcon, _folderMenuHeader));
    _folderMenu->addAction(tr("New Subfolder..."), this, &CatalogWidget::createFolder);
    _folderMenu->addAction(tr("New Memo"), this, &CatalogWidget::createMemo);
    _folderMenu->addSeparator();
    _folderMenu->addAction(tr("Rename..."), this, &CatalogWidget::renameFolder);
    _folderMenu->addAction(tr("Delete"), this, &CatalogWidget::deleteFolder);

    auto openMemo = new QAction(tr("Open"));
    connect(openMemo, &QAction::triggered, this, &CatalogWidget::openSelectedMemo);

    _memoMenu = new QMenu(this);
    _memoMenu->addAction(makeMenuHeader(this, _memoMenuIcon, _memoMenuHeader));
    _memoMenu->addAction(openMemo);
    _memoMenu->addSeparator();
    _memoMenu->addAction(tr("Delete"), this, &CatalogWidget::deleteMemo);

    _catalogView = new QTreeView;
    _catalogView->setObjectName("notebook_view");
    _catalogView->setHeaderHidden(true);
    _catalogView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_catalogView, &QTreeView::customContextMenuRequested, this, &CatalogWidget::contextMenuRequested);
    connect(_catalogView, &QTreeView::doubleClicked, this, &CatalogWidget::doubleClicked);

    Ori::Layouts::LayoutV({_catalogView})
            .setMargin(0)
            .setSpacing(0)
            .useFor(this);
}

void CatalogWidget::setCatalog(Catalog* catalog)
{
    if (_catalog)
        disconnect(_catalog, &Catalog::memoUpdated, this, &CatalogWidget::memoUpdated);

    _catalog = catalog;
    if (_catalogModel)
    {
        delete _catalogModel;
        _catalogModel = nullptr;
    }
    if (_catalog)
    {
        _catalogModel = new CatalogModel(_catalog);
        connect(_catalog, &Catalog::memoUpdated, this, &CatalogWidget::memoUpdated);
    }
    _catalogView->setModel(_catalogModel);
}

void CatalogWidget::contextMenuRequested(const QPoint &pos)
{
    if (!_catalogModel) return;

    CatalogSelection selected(_catalogView);
    if (!selected.item)
    {
        _rootMenu->popup(_catalogView->mapToGlobal(pos));
    }
    else if (selected.folder)
    {
        _folderMenuHeader->setText(selected.item->title());
        _folderMenuIcon->setPixmap(_catalogModel->folderIcon().pixmap(16, 16));
        _folderMenu->popup(_catalogView->mapToGlobal(pos));
    }
    else if (selected.memo)
    {
        _memoMenuHeader->setText(selected.item->title());
        _memoMenuIcon->setPixmap(selected.memo->type()->icon().pixmap(16, 16));
        _memoMenu->popup(_catalogView->mapToGlobal(pos));
    }
}

void CatalogWidget::openSelectedMemo()
{
    if (!_catalogModel) return;

    CatalogSelection selected(_catalogView);
    if (selected.memo) onOpenMemo(selected.memo);
}

void CatalogWidget::doubleClicked(const QModelIndex&)
{
    openSelectedMemo();
}

SelectedItems CatalogWidget::selection() const
{
    CatalogSelection selected(_catalogView);
    SelectedItems result;
    result.memo = selected.memo;
    result.folder = selected.folder;
    return result;
}

void CatalogWidget::createFolder()
{
    createFolderInternal(CatalogSelection(_catalogView));
}

void CatalogWidget::createTopLevelFolder()
{
    createFolderInternal(CatalogSelection());
}

void CatalogWidget::createFolderInternal(const CatalogSelection& parentFolder)
{

    auto title = Ori::Dlg::inputText(tr("Enter a title for new folder"), "");
    if (title.isEmpty()) return;

    auto res = _catalog->createFolder(parentFolder.folder, title);
    if (!res.ok()) return Ori::Dlg::error(res.error());

    // TODO do not know about item inserted at the end and select by pointer
    auto newIndex = _catalogModel->itemAdded(parentFolder.index);
    if (!_catalogView->isExpanded(parentFolder.index))
        _catalogView->expand(parentFolder.index);
    _catalogView->setCurrentIndex(newIndex);
}

void CatalogWidget::renameFolder()
{
    CatalogSelection selected(_catalogView);
    if (!selected.folder) return;

    auto title = Ori::Dlg::inputText(tr("Enter new title for folder"), selected.folder->title());
    if (title.isEmpty()) return;

    auto res = _catalog->renameFolder(selected.folder, title);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    _catalogModel->itemRenamed(selected.index);
    // TODO do something about items sorted after renaming
}

void CatalogWidget::deleteFolder()
{
    CatalogSelection selected(_catalogView);
    if (!selected.folder) return;

    auto confirm = tr("Are you sure to delete folder '%1' and all its content?\n\n"
                      "This action can't be undone.").arg(selected.folder->title());
    if (!Ori::Dlg::yes(confirm)) return;

    ItemRemoverGuard guard(_catalogModel, selected.index);

    auto res = _catalog->removeFolder(selected.folder);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    _catalogView->setCurrentIndex(guard.parentIndex);
}

void CatalogWidget::createMemo()
{
    CatalogSelection parentFolder(_catalogView);

    auto memoType = plainTextMemoType();

    auto memo = memoType->makeMemo();
    auto res = _catalog->createMemo(parentFolder.folder, memo);
    if (!res.ok()) return Ori::Dlg::error(res.error());

    // TODO do not know about item inserted at the end and select by pointer
    auto newIndex = _catalogModel->itemAdded(parentFolder.index);
    if (!_catalogView->isExpanded(parentFolder.index))
        _catalogView->expand(parentFolder.index);
    _catalogView->setCurrentIndex(newIndex);
}

void CatalogWidget::deleteMemo()
{
    CatalogSelection selected(_catalogView);
    if (!selected.memo) return;

    auto confirm = tr("Are you sure to delete memo '%1'?\n\n"
                      "This action can't be undone.").arg(selected.memo->title());
    if (!Ori::Dlg::yes(confirm)) return;

    ItemRemoverGuard guard(_catalogModel, selected.index);

    auto res = _catalog->removeMemo(selected.memo);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    _catalogView->setCurrentIndex(guard.parentIndex);
}

void CatalogWidget::memoUpdated(MemoItem* item)
{
    auto index = _catalogModel->findIndex(item);
    if (index.isValid())
        _catalogModel->itemRenamed(index);
}

QStringList CatalogWidget::getExpandedIds() const
{
    QStringList ids;
    fillExpandedIds(ids, QModelIndex());
    return ids;
}

void CatalogWidget::setExpandedIds(const QStringList& ids)
{
    setExpandedIds(ids, QModelIndex());
}

void CatalogWidget::fillExpandedIds(QStringList& ids, const QModelIndex& parentIndex) const
{
    int rowCount = _catalogModel->rowCount(parentIndex);
    for (int row = 0; row < rowCount; row++)
    {
        auto index = _catalogModel->index(row, 0, parentIndex);
        if (_catalogView->isExpanded(index))
        {
            auto data = _catalogModel->data(index, Qt::UserRole);
            if (data.isNull()) continue;
            ids << QString::number(data.toInt());
        }
        fillExpandedIds(ids, index);
    }
}

void CatalogWidget::setExpandedIds(const QStringList& ids, const QModelIndex& parentIndex)
{
    int rowCount = _catalogModel->rowCount(parentIndex);
    for (int row = 0; row < rowCount; row++)
    {
        auto index = _catalogModel->index(row, 0, parentIndex);
        auto data = _catalogModel->data(index, Qt::UserRole);
        if (data.isNull()) continue;
        if (ids.contains(QString::number(data.toInt())))
            _catalogView->expand(index);
        setExpandedIds(ids, index);
    }
}
