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
class GlassItem;

struct SelectedItems
{
    GlassItem* glass;
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
    QMenu *_rootMenu, *_folderMenu, *_glassMenu;
    QAction *_folderMenuHeader, *_glassMenuHeader;

    static QAction *makeHeaderItem(QMenu* menu);

    void contextMenuRequested(const QPoint &pos);

    void createFolder();
    void renameFolder();
    void deleteFolder();
    void createGlass();
    void deleteGlass();
};

#endif // CATALOGWIDGET_H
