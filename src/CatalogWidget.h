#ifndef CATALOG_WIDGET_H
#define CATALOG_WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QTreeView;
QT_END_NAMESPACE

class Db;
class DbItem;
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
    CatalogWidget();

    void setCatalog(Db* catalog);

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
    void onOpenMemo(MemoItem* item);

private:
    Db* _catalog = nullptr;
    QTreeView* _catalogView;
    CatalogModel* _catalogModel = nullptr;
    QMenu *_rootMenu, *_folderMenu, *_memoMenu;
    QAction *_openMemo;

    void contextMenuRequested(const QPoint &pos);
    void doubleClicked(const QModelIndex &);
    void openSelectedMemo();

    void memoUpdated(MemoItem*);
    void createFolderInternal(const CatalogSelection& selection);

    void fillExpandedIds(QStringList& ids, const QModelIndex& parentIndex) const;
    void setExpandedIds(const QStringList& ids, const QModelIndex& parentIndex);
};

#endif // CATALOG_WIDGET_H
