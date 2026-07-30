#ifndef DBTREE_WIDGET_H
#define DBTREE_WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QMenu;
class QTreeView;
QT_END_NAMESPACE

class Db;
class DbItem;
class DbTreeModel;
class FolderItem;
class MemoItem;

class DbTreeWidget : public QWidget
{
    Q_OBJECT

public:
    DbTreeWidget();

    void setDb(Db* db);

    QStringList getExpandedIds() const;
    void setExpandedIds(const QStringList& ids);

signals:
    void onOpenMemo(MemoItem* item);

private:
    Db* _db = nullptr;
    QTreeView* _treeView;
    DbTreeModel* _model = nullptr;
    QMenu *_rootMenu, *_folderMenu, *_memoMenu;

    DbItem* selectedItem() const;

    void createFolder();
    void renameFolder();
    void deleteFolder();
    void createMemo();
    void deleteMemo();
    void openMemo();

    void contextMenuRequested(const QPoint &pos);
    void doubleClicked(const QModelIndex &);
    void memoUpdated(MemoItem*);

    void fillExpandedIds(QStringList& ids, const QModelIndex& parentIndex) const;
    void setExpandedIds(const QStringList& ids, const QModelIndex& parentIndex);
};

#endif // DBTREE_WIDGET_H
