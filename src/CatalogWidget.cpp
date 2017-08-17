#include "Catalog.h"
#include "CatalogWidget.h"
#include "CatalogModel.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriDialogs.h"

#include <QMenu>
#include <QTreeView>

struct CatalogSelection
{
    QModelIndex index;
    CatalogItem* item = nullptr;
    FolderItem* folder = nullptr;
    GlassItem* glass = nullptr;

    CatalogSelection(QTreeView* view)
    {
        index = view->currentIndex();
        if (!index.isValid()) return;

        item = CatalogModel::catalogItem(index);
        if (!item) return;

        folder = item->asFolder();
        glass = item->asGlass();
    }
};


CatalogWidget::CatalogWidget(QAction* actionMakeDispPlot) : QWidget()
{
    _rootMenu = new QMenu(this);
    _rootMenu->addAction(tr("New Folder..."), this, &CatalogWidget::createFolder);
    _rootMenu->addAction(tr("New Material..."), this, &CatalogWidget::createGlass);

    _folderMenu = new QMenu(this);
    _folderMenuHeader = makeHeaderItem(_folderMenu);
    _folderMenu->addSeparator();
    _folderMenu->addAction(tr("New Subfolder..."), this, &CatalogWidget::createFolder);
    _folderMenu->addAction(tr("New Material..."), this, &CatalogWidget::createGlass);
    _folderMenu->addSeparator();
    _folderMenu->addAction(tr("Rename Folder..."), this, &CatalogWidget::renameFolder);
    _folderMenu->addAction(tr("Delete Folder"), this, &CatalogWidget::deleteFolder);

    _glassMenu = new QMenu(this);
    _glassMenuHeader = makeHeaderItem(_glassMenu);
    _glassMenu->addSeparator();
    _glassMenu->addAction(actionMakeDispPlot);
    _glassMenu->addSeparator();
    _glassMenu->addAction(tr("Delete Material"), this, &CatalogWidget::deleteGlass);
    connect(_glassMenu, &QMenu::aboutToShow, [this](){ emit this->contextMenuAboutToShow(); });

    _catalogView = new QTreeView;
    _catalogView->setHeaderHidden(true);
    _catalogView->setAlternatingRowColors(true);
    _catalogView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_catalogView, &QTreeView::customContextMenuRequested, this, &CatalogWidget::contextMenuRequested);

    Ori::Layouts::LayoutV({_catalogView})
            .setMargin(0)
            .setSpacing(0)
            .useFor(this);
}

QAction* CatalogWidget::makeHeaderItem(QMenu* menu)
{
    QAction* item = menu->addAction("");
    auto font = item->font();
    font.setBold(true);
    font.setPointSize(font.pointSize()+2);
    item->setFont(font);
    item->setEnabled(false);
    return item;
}

void CatalogWidget::setCatalog(Catalog* catalog)
{
    _catalog = catalog;
    if (_catalogModel)
    {
        delete _catalogModel;
        _catalogModel = nullptr;
    }
    if (_catalog)
        _catalogModel = new CatalogModel(_catalog);
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
        _folderMenuHeader->setIcon(_catalogModel->folderIcon());
        _folderMenu->popup(_catalogView->mapToGlobal(pos));
    }
    else if (selected.glass)
    {
        _glassMenuHeader->setText(selected.item->title());
        _glassMenuHeader->setIcon(selected.glass->formula()->icon());
        _glassMenu->popup(_catalogView->mapToGlobal(pos));
    }
}

SelectedItems CatalogWidget::selection() const
{
    CatalogSelection selected(_catalogView);
    SelectedItems result;
    result.glass = selected.glass;
    return result;
}

void CatalogWidget::createFolder()
{
    CatalogSelection selected(_catalogView);

    auto title = Ori::Dlg::inputText(tr("Enter a title for new folder"), "");
    if (title.isEmpty()) return;

    auto res = _catalog->createFolder(selected.folder, title);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    // TODO do not know about item inserted at the end and select by pointer
    auto newIndex = _catalogModel->itemAdded(selected.index);
    if (!_catalogView->isExpanded(selected.index))
        _catalogView->expand(selected.index);
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

void CatalogWidget::createGlass()
{
    CatalogSelection selected(_catalogView);
    // TODO

//    if (!GlassEditor::createGlass(_catalog, selected.folder)) return;

//    // TODO do not know about item inserted at the end and select by pointer
//    auto newIndex = _catalogModel->itemAdded(selected.index);
//    if (!_catalogView->isExpanded(selected.index))
//        _catalogView->expand(selected.index);
//    _catalogView->setCurrentIndex(newIndex);
}

void CatalogWidget::deleteGlass()
{
    CatalogSelection selected(_catalogView);
    if (!selected.glass) return;

    auto confirm = tr("Are you sure to delete material '%1'?\n\n"
                      "This action can't be undone.").arg(selected.glass->title());
    if (!Ori::Dlg::yes(confirm)) return;

    ItemRemoverGuard guard(_catalogModel, selected.index);

    auto res = _catalog->removeGlass(selected.glass);
    if (!res.isEmpty()) return Ori::Dlg::error(res);

    _catalogView->setCurrentIndex(guard.parentIndex);
}
