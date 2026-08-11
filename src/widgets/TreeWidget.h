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
    bool _isFolderDeleting = false;

    Entry* selectedEntry() const;
    void selectEntry(Entry*);

    void createFolder();
    void renameFolder();
    void deleteFolder();
    void createMemo();
    void deleteMemo();
    void openMemo();

    void contextMenuRequested(const QPoint &pos);
    void entryCreating(Entry*, int);
    void entryCreated(Entry*);
    void entryUpdated(Entry*);
    void entryDeleting(Entry*);
    void entryDeleted(Entry*);

    void stashExpandedIds();
    void applyExpandedIds();
};

#endif // DBTREE_WIDGET_H
