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

class Enot;
class DbItem;
class TreeWidget;
class OpenTabsWidget;
class SpellcheckControl;
class InfoWidget;
class MemoTab;
class Memo;
class TextMemoTab;

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
    Enot* _enot = nullptr;
    TreeWidget* _treeView;
    QStackedWidget* _tabsView;
    OpenTabsWidget* _openTabsView;
    Ori::MruFileList *_mruList;
    QLabel *_statusMemoCount, *_statusFileName;
    QAction *_actionMemoFont, *_actionWordWrap, *_actionMemoExportPdf;
    QString _lastOpenedDb;
    SpellcheckControl* _spellcheckControl;
    Phl::Control* _highlighterControl;
    QMenu *_spellcheckMenu = nullptr;
    QMenu *_highlighterMenu;

    void createMenu();
    void createStatusBar();
    void loadSession();
    void saveSession();

    void newEnot();
    void openEnot(const QString &fileName);
    void openEnotViaDialog();
    bool closeEnot();

    void updateCounter();

    void chooseMemoFont();
    void toggleWordWrap();

    void enotOpened(Enot* enot);
    void itemCreated(DbItem* item);
    void itemRemoved(DbItem* item);

    bool closeAllMemos();
    void openMemoTab(Memo* item);
    void exportToPdf();

    MemoTab* findMemoTab(Memo* item) const;
    MemoTab* currentMemoTab() const;
    TextMemoTab* currentTextMemoTab() const;

    void optionsMenuAboutToShow();
    void spellcheckMenuAboutToShow();
    void highlighterMenuAboutToShow();

    void setMemoSpellcheckLang(const QString& lang);
    void setMemoHighlighter(const QString& name);
};

#endif // MAIN_WINDOW_H
