#ifndef DBTREE_WIDGET_H
#define DBTREE_WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QMenu;
class QTreeView;
QT_END_NAMESPACE

class Enot;
class DbItem;
class TreeModel;
class FolderItem;
class MemoItem;

class TreeWidget : public QWidget
{
    Q_OBJECT

public:
    TreeWidget();

    void setEnot(Enot* enot);

    QStringList getExpandedIds();
    void setExpandedIds(const QStringList& ids);

signals:
    void memoOpenRequested(MemoItem* item);

private:
    Enot* _enot = nullptr;
    QTreeView* _treeView;
    TreeModel* _model = nullptr;
    QMenu *_rootMenu, *_folderMenu, *_memoMenu;
    QSet<int> _expandedIds;
    bool _isFolderRemoving = false;

    DbItem* selectedItem() const;

    void createFolder();
    void renameFolder();
    void deleteFolder();
    void createMemo();
    void deleteMemo();
    void openMemo();

    void contextMenuRequested(const QPoint &pos);
    void itemCreating(DbItem*, int);
    void itemCreated(DbItem*);
    void itemUpdated(DbItem*);
    void itemRemoving(DbItem*);
    void itemRemoved(DbItem*);

    void selectItem(DbItem*);

    void stashExpandedIds();
    void applyExpandedIds();
};

#endif // DBTREE_WIDGET_H
