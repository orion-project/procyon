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
class WindowsWidget;

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
    WindowsWidget* _windowsView;
    QMdiArea* _mdiArea;
    Ori::MruFileList *_mruList;
    QDockWidget *_dockCatalog, *_dockInfo, *_dockWindows;
    QLabel *_statusMemoCount, *_statusFileName;
    QAction *_actionViewCatalog, *_actionViewInfo, *_actionViewWindows;
    QAction *_actionCreateTopLevelFolder, *_actionCreateFolder, *_actionRenameFolder, *_actionDeleteFolder;
    QAction *_actionOpenMemo, *_actionCreateMemo, *_actionDeleteMemo;
    QFont _memoFont, _titleFont;

    void createMenu();
    void createToolBars();
    void createDocks();
    void createStatusBar();
    void saveSettings();
    void loadSettings();
    void loadSession();
    void saveSession();
    void closeCurrentFile();
    void newCatalog();
    void openCatalog(const QString &fileName);
    void openCatalogViaDialog();
    void catalogOpened(Catalog* catalog);
    void catalogClosed();
    void updateCounter();
    void updateMenuCatalog();
    void openMemo();
    void chooseMemoFont();
    void chooseTitleFont();
    void memoCreated(MemoItem* item);
    void memoRemoved(MemoItem* item);

    void openWindowForItem(MemoItem* item);
    QMdiSubWindow* findMemoMdiChild(MemoItem* item) const;
    MemoWindow* memoWindowOfMdiChild(QMdiSubWindow* subWindow) const;
    MemoWindow* activeMemoWindow() const;
    QAction* addViewPanelAction(QMenu* m, const QString& title, QDockWidget* panel);
};

#endif // MAINWINDOW_H
