#ifndef CATALOGWIDGET_H
#define CATALOGWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QTreeView;
QT_END_NAMESPACE

class Catalog;
class CatalogItem;
class CatalogModel;
class FolderItem;
class MemoItem;

struct CatalogSelection;
struct SelectedItems
{
    MemoItem* memo;
    FolderItem* folder;
};

class CatalogWidget : public QWidget
{
    Q_OBJECT

public:
    CatalogWidget(QAction *actionMakeDispPlot);

    void setCatalog(Catalog* catalog);

    SelectedItems selection() const;

    void createFolder();
    void renameFolder();
    void deleteFolder();
    void createMemo();
    void deleteMemo();
    void createTopLevelFolder();

    QStringList getExpandedIds() const;
    void setExpandedIds(const QStringList& ids);

signals:
    void contextMenuAboutToShow();

private:
    Catalog* _catalog = nullptr;
    QTreeView* _catalogView;
    CatalogModel* _catalogModel = nullptr;
    QMenu *_rootMenu, *_folderMenu, *_memoMenu;
    QAction *_openMemo;
    QAction *_folderMenuHeader, *_memoMenuHeader;

    static QAction *makeHeaderItem(QMenu* menu);

    void contextMenuRequested(const QPoint &pos);
    void doubleClicked(const QModelIndex &);

    void memoUpdated(MemoItem*);
    void createFolderInternal(const CatalogSelection& parentFolder);

    void fillExpandedIds(QStringList& ids, const QModelIndex& parentIndex) const;
    void setExpandedIds(const QStringList& ids, const QModelIndex& parentIndex);
};

#endif // CATALOGWIDGET_H
