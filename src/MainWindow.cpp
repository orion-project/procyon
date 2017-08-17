#include "MainWindow.h"
#include "Catalog.h"
#include "CatalogWidget.h"
#include "InfoWidget.h"
#include "DispersionPlot.h"
#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWindows.h"
#include "tools/OriMruList.h"
#include "tools/OriSettings.h"
#include "tools/OriWaitCursor.h"
#include "widgets/OriMruMenu.h"
#include "widgets/OriStylesMenu.h"

#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>

MainWindow::MainWindow() : QMainWindow()
{
    setObjectName("mainWindow");
    Ori::Wnd::setWindowIcon(this, ":/icon/main");

    _mruList = new Ori::MruFileList(this);
    connect(_mruList, &Ori::MruFileList::clicked, this, &MainWindow::openCatalog);

    createMenu();

    _catalogView = new CatalogWidget(_actionMakeDispPlot);
    connect(_catalogView, &CatalogWidget::contextMenuAboutToShow, this, &MainWindow::updateMenuMaterial);

    _infoView = new InfoWidget;

    createDocks();
    createStatusBar();

    _mdiArea = new QMdiArea;
    setCentralWidget(_mdiArea);

    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();

    if (_catalog) delete _catalog;
}

void toggleWidget(QWidget* panel)
{
    if (panel->isVisible()) panel->hide(); else panel->show();
}

void MainWindow::createMenu()
{
    QMenu* menuFile = menuBar()->addMenu(tr("&File"));
    menuFile->addAction(tr("New..."), this, &MainWindow::newCatalog);
    menuFile->addAction(tr("Open..."), this, &MainWindow::openCatalogViaDialog, QKeySequence::Open);
    menuFile->addSeparator();
    auto actionExit = menuFile->addAction(tr("Exit"), this, &MainWindow::close, QKeySequence::Quit);
    new Ori::Widgets::MruMenuPart(_mruList, menuFile, actionExit, this);

    QMenu* menuView = menuBar()->addMenu(tr("&View"));
    connect(menuView, &QMenu::aboutToShow, [this](){
        this->_actionViewCatalog->setChecked(this->_dockCatalog->isVisible());
        this->_actionViewInfo->setChecked(this->_dockInfo->isVisible());
    });
    _actionViewCatalog = menuView->addAction(tr("Catalog Panel"), [this](){ toggleWidget(this->_dockCatalog); });
    _actionViewInfo = menuView->addAction(tr("Info Panel"), [this](){ toggleWidget(this->_dockInfo); });
    _actionViewCatalog->setCheckable(true);
    _actionViewInfo->setCheckable(true);
    menuView->addSeparator();
    menuView->addMenu(new Ori::Widgets::StylesMenu(this));

    QMenu* menuMaterial = menuBar()->addMenu(tr("&Material"));
    connect(menuMaterial, &QMenu::aboutToShow, this, &MainWindow::updateMenuMaterial);
    _actionMakeDispPlot = menuMaterial->addAction(QIcon(":/icon/plot"), tr("Dispersion Plot"), this, &MainWindow::makeDispersionPlot);
}

void MainWindow::createDocks()
{
    _dockCatalog = new QDockWidget(tr("Catalog"));
    _dockCatalog->setObjectName("CatalogPanel");
    _dockCatalog->setWidget(_catalogView);

    _dockInfo = new QDockWidget(tr("Info"));
    _dockInfo->setObjectName("InfoPanel");
    _dockInfo->setWidget(_infoView);

    addDockWidget(Qt::LeftDockWidgetArea, _dockCatalog);
    addDockWidget(Qt::LeftDockWidgetArea, _dockInfo);
}

void MainWindow::createStatusBar()
{
    statusBar()->addWidget(_statusGlassCount = new QLabel);
    statusBar()->addWidget(_statusFileName = new QLabel);
    statusBar()->showMessage(tr("Ready"));

    _statusGlassCount->setMargin(2);
    _statusFileName->setMargin(2);
}

void MainWindow::saveSettings()
{
    Ori::Settings s;
    s.storeWindowGeometry(this);
    s.storeDockState(this);
    s.setValue("style", qApp->style()->objectName());
    if (_catalog)
        s.setValue("database", _catalog->fileName());
}

void MainWindow::loadSettings()
{
    Ori::Settings s;
    s.restoreWindowGeometry(this);
    s.restoreDockState(this);
    _mruList->load(s.settings());
    qApp->setStyle(s.strValue("style"));
    auto lastFile = s.value("database").toString();
    if (!lastFile.isEmpty())
        QTimer::singleShot(200, [this, lastFile](){ this->openCatalog(lastFile); });
}

void MainWindow::newCatalog()
{
    QString fileName = QFileDialog::getSaveFileName(
                this, tr("Create Catalog"), QString(), Catalog::fileFilter());
    if (fileName.isEmpty()) return;

    Ori::WaitCursor c;

    catalogClosed();

    auto res = Catalog::create(fileName);
    if (res.ok())
    {
        catalogOpened(res.result());
        statusBar()->showMessage(tr("Catalog created"), 2000);
    }
    else Ori::Dlg::error(tr("Unable to create catalog.\n\n%1").arg(res.error()));
}

void MainWindow::openCatalog(const QString &fileName)
{
    if (_catalog && QFileInfo(_catalog->fileName()) == QFileInfo(fileName))
        return;

    Ori::WaitCursor c;

    catalogClosed();

    auto res = Catalog::open(fileName);
    if (res.ok())
    {
        catalogOpened(res.result());
        statusBar()->showMessage(tr("Catalog loaded"), 2000);
    }
    else Ori::Dlg::error(tr("Unable to load catalog.\n\n%1").arg(res.error()));
}

void MainWindow::openCatalogViaDialog()
{
    QString fileName = QFileDialog::getOpenFileName(
                this, tr("Open Catalog"), QString(), Catalog::fileFilter());
    if (!fileName.isEmpty())
        openCatalog(fileName);
}

void MainWindow::catalogOpened(Catalog* catalog)
{
    _catalog = catalog;
    connect(_catalog, &Catalog::glassCreated, [this](){ this->updateCounter(); });
    connect(_catalog, &Catalog::glassRemoved, [this](){ this->updateCounter(); });
    _catalogView->setCatalog(_catalog);
    auto filePath = _catalog->fileName();
    auto fileName = QFileInfo(filePath).fileName();
    setWindowTitle(fileName % " - " % qApp->applicationName());
    _mruList->append(filePath);
    _statusFileName->setText(tr("Catalog: %1").arg(QDir::toNativeSeparators(filePath)));
    updateCounter();
}

void MainWindow::catalogClosed()
{
    delete _catalog;
    _catalog = nullptr;
    _catalogView->setCatalog(nullptr);
    setWindowTitle(qApp->applicationName());
    _statusFileName->clear();
    _statusGlassCount->clear();
}

void MainWindow::updateCounter()
{
    auto res = _catalog->countGlasses();
    if (res.ok())
    {
        _statusGlassCount->setToolTip(QString());
        _statusGlassCount->setText(tr("Materials: %1").arg(res.result()));
    }
    else
    {
        _statusGlassCount->setToolTip(res.error());
        _statusGlassCount->setText(tr("Materials: ERROR"));
    }
}

void MainWindow::updateMenuMaterial()
{
    bool canMakeDispPlot = false;

    if (_catalog)
    {
        auto selected = _catalogView->selection();
        bool hasSelection = selected.glass;

        canMakeDispPlot = hasSelection;
    }

    _actionMakeDispPlot->setEnabled(canMakeDispPlot);
}

PlotWindow* MainWindow::activePlot() const
{
    auto mdiChild = _mdiArea->currentSubWindow();
    if (!mdiChild) return nullptr;
    return dynamic_cast<PlotWindow*>(mdiChild->widget());
}

void MainWindow::makeDispersionPlot()
{
    auto selected = _catalogView->selection();
    if (!selected.glass) return;
    auto plotWindow = new DispersionPlot(_catalog);
    auto mdiChild = _mdiArea->addSubWindow(plotWindow);
    mdiChild->setWindowIcon(plotWindow->windowIcon());
    mdiChild->show();
}
