#ifndef DBTREE_WIDGET_H
#define DBTREE_WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QTreeView;
QT_END_NAMESPACE

class Db;
class DbItem;
class DbTreeModel;
class FolderItem;
class MemoItem;

struct DbTreeSelection;
struct SelectedItems
{
    MemoItem* memo;
    FolderItem* folder;
};

class DbTreeWidget : public QWidget
{
    Q_OBJECT

public:
    DbTreeWidget();

    void setDb(Db* db);

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
    Db* _db = nullptr;
    QTreeView* _treeView;
    DbTreeModel* _model = nullptr;
    QMenu *_rootMenu, *_folderMenu, *_memoMenu;
    QAction *_openMemo;

    void contextMenuRequested(const QPoint &pos);
    void doubleClicked(const QModelIndex &);
    void openSelectedMemo();

    void memoUpdated(MemoItem*);
    void createFolderInternal(const DbTreeSelection& selection);

    void fillExpandedIds(QStringList& ids, const QModelIndex& parentIndex) const;
    void setExpandedIds(const QStringList& ids, const QModelIndex& parentIndex);
};

#endif // DBTREE_WIDGET_H
