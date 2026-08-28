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
class Entry;
class TreeWidget;
class OpenTabsWidget;
class InfoWidget;
class MemoTab;
class Memo;
class TextMemoTab;

namespace Ori {
class MruFileList;
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
    Enot* _enot = nullptr;
    TreeWidget* _treeView;
    QStackedWidget* _tabsView;
    OpenTabsWidget* _openTabsView;
    Ori::MruFileList *_mruList;
    QLabel *_statusMemoCount, *_statusFileName;
    QAction *_actionMemoFont, *_actionWordWrap, *_actionMemoExportPdf, *_actionAddMemoProp;
    QString _lastOpenedDb;

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
    void addMemoProp();

    void enotOpened(Enot* enot);
    void itemCreated(Entry* entry);
    void itemRemoved(Entry* entry);

    bool closeAllMemos();
    void openMemoTab(Memo* memo);
    void exportToPdf();

    MemoTab* findMemoTab(Memo* memo) const;
    MemoTab* currentMemoTab() const;
    TextMemoTab* currentTextMemoTab() const;

    void memoMenuAboutToShow();
};

#endif // MAIN_WINDOW_H
