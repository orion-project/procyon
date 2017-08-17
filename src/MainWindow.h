#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QAction;
class QDockWidget;
class QLabel;
class QMdiArea;
QT_END_NAMESPACE

class Catalog;
class CatalogWidget;
class InfoWidget;
class PlotWindow;

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
    QLabel *_statusGlassCount, *_statusFileName;
    QAction *_actionViewCatalog, *_actionViewInfo;
    QAction *_actionMakeDispPlot;

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
    void updateMenuMaterial();
    void makeDispersionPlot();

    PlotWindow* activePlot() const;
};

#endif // MAINWINDOW_H
