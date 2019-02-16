#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QListWidget;
class QStackedWidget;
QT_END_NAMESPACE

class Catalog;
class CatalogWidget;
class InfoWidget;
class MemoWindow;
class MemoItem;

namespace Ori {
class MruFileList;
class Settings;
}


struct MemoSettings
{
    QFont memoFont;
    QFont titleFont;
    bool wordWrap;
};


//class MemoMdiSubWindow : public QMdiSubWindow
//{
//    Q_OBJECT
//signals:
//    bool windowClosing();
//protected:
//    void closeEvent(QCloseEvent *event) override;
//};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Catalog* _catalog = nullptr;
    CatalogWidget* _catalogView;
    InfoWidget* _infoView;
    Ori::MruFileList *_mruList;
    QLabel *_statusMemoCount, *_statusFileName;
    QAction *_actionCreateTopLevelFolder, *_actionCreateFolder, *_actionRenameFolder, *_actionDeleteFolder;
    QAction *_actionOpenMemo, *_actionCreateMemo, *_actionDeleteMemo;
    MemoSettings _memoSettings;
    QStackedWidget* _memoPages;
    QListWidget* _openedMemosList;
    bool _prevWindowWasMaximized = false;

    void createMenu();
    void createStatusBar();
    void saveSettings();
    void loadSettings();
    void loadSession();
    void loadSession(Ori::Settings* settings);
    void saveSession();
    void saveSession(Ori::Settings* settings);
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
    void toggleWordWrap();
    bool memoWindowAboutToClose();
    void memoWindowAboutToActivate();
    void closeAllMemos();

    void openWindowForItem(MemoItem* item);
    QWidget* findMemoPage(MemoItem* item) const;
//    MemoWindow* memoWindowOfMdiChild(QMdiSubWindow* subWindow) const;
    MemoWindow* activeMemoWindow() const;
    QAction* addViewPanelAction(QMenu* m, const QString& title, QDockWidget* panel);
    bool canClose(MemoWindow* memoWindow);
};

#endif // MAIN_WINDOW_H
