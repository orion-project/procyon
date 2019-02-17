#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QStackedWidget;
class QSplitter;
QT_END_NAMESPACE

class Catalog;
class CatalogWidget;
class OpenedPagesWidget;
class InfoWidget;
class MemoPage;
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


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QSplitter* _splitter;
    Catalog* _catalog = nullptr;
    CatalogWidget* _catalogView;
    QStackedWidget* _pagesView;
    OpenedPagesWidget* _openedPagesView;
    Ori::MruFileList *_mruList;
    QLabel *_statusMemoCount, *_statusFileName;
    QAction *_actionCreateTopLevelFolder, *_actionCreateFolder, *_actionRenameFolder, *_actionDeleteFolder;
    QAction *_actionOpenMemo, *_actionCreateMemo, *_actionDeleteMemo;
    MemoSettings _memoSettings;
    QString _lastOpenedCatalog;

    void createMenu();
    void createStatusBar();
    void saveSettings();
    void loadSettings();
    void loadSession();
    void saveSession();
    void newCatalog();
    void openCatalog(const QString &fileName);
    void openCatalogViaDialog();
    void catalogOpened(Catalog* catalog);
    bool closeCatalog();
    void updateCounter();
    void updateMenuCatalog();
    void openMemo();
    void chooseMemoFont();
    void chooseTitleFont();
    void memoCreated(MemoItem* item);
    void memoRemoved(MemoItem* item);
    void toggleWordWrap();
    bool closeAllMemos();
    void openMemoPage(MemoItem* item);
    MemoPage* findMemoPage(MemoItem* item) const;
};

#endif // MAIN_WINDOW_H
