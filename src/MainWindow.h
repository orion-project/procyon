#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QStackedWidget;
class QSplitter;
class QSettings;
QT_END_NAMESPACE

class Db;
class DbTreeWidget;
class OpenTabsWidget;
class SpellcheckControl;
class InfoWidget;
class MemoTab;
class MemoItem;

namespace Ori {
class MruFileList;
} // namespace Ori

namespace Phl {
class Control;
} // namespace Highlighter


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow() override;

    void loadSettings(QSettings* s);
    void saveSettings(QSettings* s);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QSplitter* _splitter;
    Db* _db = nullptr;
    DbTreeWidget* _treeView;
    QStackedWidget* _tabsView;
    OpenTabsWidget* _openTabsView;
    Ori::MruFileList *_mruList;
    QLabel *_statusMemoCount, *_statusFileName;
    //QAction *_actionCreateTopLevelFolder, *_actionCreateFolder, *_actionRenameFolder, *_actionDeleteFolder;
    QAction *_actionMemoFont, *_actionWordWrap, *_actionMemoExportPdf;
    //QAction *_actionOpenMemo, *_actionCreateMemo, *_actionDeleteMemo;
    QString _lastOpenedDb;
    SpellcheckControl* _spellcheckControl;
    Phl::Control* _highlighterControl;
    QMenu *_spellcheckMenu = nullptr;
    QMenu *_highlighterMenu;

    void createMenu();
    void createStatusBar();
    void loadSession();
    void saveSession();
    void newDb();
    void openDb(const QString &fileName);
    void openDbViaDialog();
    void dbOpened(Db* db);
    bool closeDb();
    void updateCounter();
    //void updateMenuDb();
    //void openMemo();
    void chooseMemoFont();
    void toggleWordWrap();
    void memoCreated(MemoItem* item);
    void memoRemoved(MemoItem* item);
    bool closeAllMemos();
    void openMemoTab(MemoItem* item);
    void exportToPdf();
    MemoTab* findMemoTab(MemoItem* item) const;
    MemoTab* currentMemoTab() const;
    void optionsMenuAboutToShow();
    void spellcheckMenuAboutToShow();
    void highlighterMenuAboutToShow();
    void setMemoSpellcheckLang(const QString& lang);
    void setMemoHighlighter(const QString& name);
};

#endif // MAIN_WINDOW_H
