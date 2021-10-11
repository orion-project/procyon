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

class Catalog;
class CatalogWidget;
class OpenedPagesWidget;
class SpellcheckControl;
class InfoWidget;
class MemoPage;
class MemoItem;

namespace Ori {
class MruFileList;

namespace Highlighter {
    class Control;
} // namespace Highlighter
} // namespace Ori


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
    Catalog* _catalog = nullptr;
    CatalogWidget* _catalogView;
    QStackedWidget* _pagesView;
    OpenedPagesWidget* _openedPagesView;
    Ori::MruFileList *_mruList;
    QLabel *_statusMemoCount, *_statusFileName;
    QAction *_actionCreateTopLevelFolder, *_actionCreateFolder, *_actionRenameFolder, *_actionDeleteFolder;
    QAction *_actionMemoFont, *_actionWordWrap, *_actionMemoExportPdf;
    QAction *_actionOpenMemo, *_actionCreateMemo, *_actionDeleteMemo;
    QString _lastOpenedCatalog;
    SpellcheckControl* _spellcheckControl;
    Ori::Highlighter::Control* _highlighterControl;
    QMenu *_spellcheckMenu = nullptr;
    QMenu *_highlighterMenu;

    void createMenu();
    void createStatusBar();
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
    void toggleWordWrap();
    void memoCreated(MemoItem* item);
    void memoRemoved(MemoItem* item);
    bool closeAllMemos();
    void openMemoPage(MemoItem* item);
    void exportToPdf();
    MemoPage* findMemoPage(MemoItem* item) const;
    MemoPage* currentMemoPage() const;
    void optionsMenuAboutToShow();
    void spellcheckMenuAboutToShow();
    void highlighterMenuAboutToShow();
    void setMemoSpellcheckLang(const QString& lang);
    void setMemoHighlighter(const QString& name);
};

#endif // MAIN_WINDOW_H
