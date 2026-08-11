#ifndef DBTREE_WIDGET_H
#define DBTREE_WIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QMenu;
class QTreeView;
QT_END_NAMESPACE

class Enot;
class Entry;
class TreeModel;
class Folder;
class Memo;

class TreeWidget : public QWidget
{
    Q_OBJECT

public:
    TreeWidget();

    void setEnot(Enot* enot);

    QStringList getExpandedIds();
    void setExpandedIds(const QStringList& ids);

signals:
    void memoOpenRequested(Memo* item);

private:
    Enot* _enot = nullptr;
    QTreeView* _treeView;
    TreeModel* _model = nullptr;
    QMenu *_rootMenu, *_folderMenu, *_memoMenu;
    QSet<int> _expandedIds;
    bool _isFolderRemoving = false;

    Entry* selectedEntry() const;

    void createFolder();
    void renameFolder();
    void deleteFolder();
    void createMemo();
    void deleteMemo();
    void openMemo();

    void contextMenuRequested(const QPoint &pos);
    void itemCreating(Entry*, int);
    void itemCreated(Entry*);
    void itemUpdated(Entry*);
    void itemRemoving(Entry*);
    void itemRemoved(Entry*);

    void selectItem(Entry*);

    void stashExpandedIds();
    void applyExpandedIds();
};

#endif // DBTREE_WIDGET_H
