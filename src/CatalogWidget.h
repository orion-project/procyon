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

struct SelectedItems
{
    MemoItem* memo;
};

class CatalogWidget : public QWidget
{
    Q_OBJECT

public:
    CatalogWidget(QAction *actionMakeDispPlot);

    void setCatalog(Catalog* catalog);

    SelectedItems selection() const;

signals:
    void contextMenuAboutToShow();

private:
    Catalog* _catalog;
    QTreeView* _catalogView;
    CatalogModel* _catalogModel = nullptr;
    QMenu *_rootMenu, *_folderMenu, *_memoMenu;
    QAction *_folderMenuHeader, *_memoMenuHeader;

    static QAction *makeHeaderItem(QMenu* menu);

    void contextMenuRequested(const QPoint &pos);

    void createFolder();
    void renameFolder();
    void deleteFolder();
    void createMemo();
    void deleteMemo();
};

#endif // CATALOGWIDGET_H
