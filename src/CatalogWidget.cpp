#include "CatalogWidget.h"

#include "CatalogModel.h"
#include "catalog/Catalog.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriDialogs.h"
#include "widgets/OriSelectableTile.h"

#include <QApplication>
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

    void selectFolderIfNone()
    {
        if (folder) return;

        if (!index.isValid()) return;
        index = index.parent();
        if (!index.isValid()) return;

        item = CatalogModel::catalogItem(index);
        if (!item) return;

        folder = item->asFolder();
        memo = nullptr;
    }
};

// We have to recreate the menu header at each menu open as it only takes the correct size when created.
// Retaining the header action, we stick to the original calculated size, and it can be unsuitable
// for subsequent menu open at items having longer titles.
static void makeMenuHeader(QMenu* menu, const QIcon& icon, const QString& title)
{
    auto prevHeader = qobject_cast<QWidgetAction*>(menu->actions().first());
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
    menu->insertAction(menu->actions().first(), headerAction);
}

CatalogWidget::CatalogWidget() : QWidget()
{
    _rootMenu = new QMenu(this);
    _rootMenu->addAction(tr("New Folder..."), this, &CatalogWidget::createFolder);
    _rootMenu->addAction(tr("New Memo..."), this, &CatalogWidget::createMemo);

    _folderMenu = new QMenu(this);
    _folderMenu->addAction(tr("Rename..."), this, &CatalogWidget::renameFolder);
    _folderMenu->addAction(tr("Delete"), this, &CatalogWidget::deleteFolder);
    _folderMenu->addSeparator();
    _folderMenu->addAction(tr("New Memo..."), this, &CatalogWidget::createMemo);
    _folderMenu->addAction(tr("New Subfolder..."), this, &CatalogWidget::createFolder);
    _folderMenu->addAction(tr("New Top Level Folder..."), this, &CatalogWidget::createTopLevelFolder);

    auto openMemo = new QAction(tr("Open"));
    connect(openMemo, &QAction::triggered, this, &CatalogWidget::openSelectedMemo);

    _memoMenu = new QMenu(this);
    _memoMenu->addAction(openMemo);
    _memoMenu->addSeparator();
    _memoMenu->addAction(tr("Delete"), this, &CatalogWidget::deleteMemo);
    _memoMenu->addSeparator();
    _memoMenu->addAction(tr("New Memo..."), this, &CatalogWidget::createMemo);
    _memoMenu->addAction(tr("New Subfolder..."), this, &CatalogWidget::createFolder);
    _memoMenu->addAction(tr("New Top Level Folder..."), this, &CatalogWidget::createTopLevelFolder);

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

    QMenu* menu = nullptr;
    CatalogSelection selected(_catalogView);
    if (!selected.item)
    {
        menu = _rootMenu;
    }
    else if (selected.folder)
    {
        makeMenuHeader(_folderMenu, _catalogModel->folderIcon(), selected.item->title());
        menu = _folderMenu;
    }
    else if (selected.memo)
    {
        makeMenuHeader(_memoMenu, selected.memo->type()->icon(), selected.item->title());
        menu = _memoMenu;
    }
    if (menu)
        menu->popup(_catalogView->mapToGlobal(pos));
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
    CatalogSelection selection(_catalogView);
    selection.selectFolderIfNone();
    if (selection.folder)
        createFolderInternal(selection);
}

void CatalogWidget::createTopLevelFolder()
{
    createFolderInternal(CatalogSelection());
}

void CatalogWidget::createFolderInternal(const CatalogSelection& selection)
{
    auto title = Ori::Dlg::inputText(tr("Enter a title for new folder"), "");
    if (title.isEmpty()) return;

    auto res = _catalog->createFolder(selection.folder, title);
    if (!res.ok()) return Ori::Dlg::error(res.error());

    // TODO do not know about item inserted at the end and select by pointer
    auto newIndex = _catalogModel->itemAdded(selection.index);
    if (!_catalogView->isExpanded(selection.index))
        _catalogView->expand(selection.index);
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


void CatalogWidget::createMemo()
{
    CatalogSelection selection(_catalogView);
    selection.selectFolderIfNone();
    if (!selection.folder) return;

    auto memoType = selectMemoTypeDlg();
    if (!memoType) return;

    auto memoItem = new MemoItem;
    auto res = _catalog->createMemo(selection.folder, memoItem, memoType);
    if (!res.ok())
    {
        delete memoItem;
        return Ori::Dlg::error(res.error());
    }

    // TODO do not know about item inserted at the end and select by pointer
    auto newIndex = _catalogModel->itemAdded(selection.index);
    if (!_catalogView->isExpanded(selection.index))
        _catalogView->expand(selection.index);
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
        if (_catalogModel->catalogItem(index)->isFolder())
        {
            if (_catalogView->isExpanded(index))
                ids << QString::number(_catalogModel->data(index, Qt::UserRole).toInt());
            fillExpandedIds(ids, index);
        }
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
