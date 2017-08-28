#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QDockWidget;
class QLabel;
class QMdiArea;
class QMdiSubWindow;
QT_END_NAMESPACE

class Catalog;
class CatalogWidget;
class InfoWidget;
class MemoWindow;
class MemoItem;

namespace Ori {
class MruFileList;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

private:
    Catalog* _catalog = nullptr;
    CatalogWidget* _catalogView;
    InfoWidget* _infoView;
    QMdiArea* _mdiArea;
    Ori::MruFileList *_mruList;
    QDockWidget *_dockCatalog, *_dockInfo;
    QLabel *_statusMemoCount, *_statusFileName;
    QAction *_actionViewCatalog, *_actionViewInfo;
    QAction *_actionCatalogCreateTopLevelFolder;
    QAction *_actionOpenMemo;

    void createMenu();
    void createDocks();
    void createStatusBar();
    void saveSettings();
    void loadSettings();
    void closeCurrentFile();
    void newCatalog();
    void openCatalog(const QString &fileName);
    void openCatalogViaDialog();
    void catalogOpened(Catalog* catalog);
    void catalogClosed();
    void updateCounter();
    void updateMenuMemo();
    void updateMenuCatalog();
    void openMemo();

    QMdiSubWindow* findMemoSubWindow(MemoItem* item) const;
    MemoWindow* activePlot() const;
};

#endif // MAINWINDOW_H
