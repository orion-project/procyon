#include "MainWindow.h"
#include "Catalog.h"
#include "CatalogWidget.h"
#include "InfoWidget.h"
#include "MemoWindow.h"
#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWindows.h"
#include "tools/OriMruList.h"
#include "tools/OriSettings.h"
#include "tools/OriWaitCursor.h"
#include "widgets/OriMruMenu.h"
#include "widgets/OriStylesMenu.h"

#include <QDebug>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
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

    _catalogView = new CatalogWidget(_actionOpenMemo);
    connect(_catalogView, &CatalogWidget::contextMenuAboutToShow, this, &MainWindow::updateMenuMemo);

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

    QMenu* menuCatalog = menuBar()->addMenu(tr("&Catalog"));
    connect(menuCatalog, &QMenu::aboutToShow, this, &MainWindow::updateMenuCatalog);
    _actionCatalogCreateTopLevelFolder = menuCatalog->addAction(tr("New Top Level Folder..."),
        [this](){ this->_catalogView->createTopLevelFolder(); });

    QMenu* menuMemo = menuBar()->addMenu(tr("&Memo"));
    connect(menuMemo, &QMenu::aboutToShow, this, &MainWindow::updateMenuMemo);
    _actionOpenMemo = menuMemo->addAction(tr("Open memo"), this, &MainWindow::openMemo);

    QMenu* menuOptions = menuBar()->addMenu(tr("&Options"));
    menuOptions->addAction(tr("Choose Memo Font..."), this, &MainWindow::chooseMemoFont);
    menuOptions->addAction(tr("Choose Title Font..."), this, &MainWindow::chooseTitleFont);
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
    statusBar()->addWidget(_statusMemoCount = new QLabel);
    statusBar()->addWidget(_statusFileName = new QLabel);
    statusBar()->showMessage(tr("Ready"));

    _statusMemoCount->setMargin(2);
    _statusFileName->setMargin(2);
}

void MainWindow::saveSettings()
{
    Ori::Settings s;
    s.storeWindowGeometry(this);
    s.storeDockState(this);
    s.setValue("style", qApp->style()->objectName());
    s.setValue("memoFont", _memoFont);
    s.setValue("titleFont", _titleFont);
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

    _memoFont = qvariant_cast<QFont>(s.value("memoFont", QFont("Arial", 12)));
    _titleFont = qvariant_cast<QFont>(s.value("titleFont", QFont("Arial", 14)));

    auto lastFile = s.value("database").toString();
    if (!lastFile.isEmpty())
        QTimer::singleShot(200, [this, lastFile](){ this->openCatalog(lastFile); });
}

void MainWindow::newCatalog()
{
    QString fileName = Ori::Dlg::getSaveFileName(
                tr("Create Catalog"), Catalog::fileFilter(), Catalog::defaultFileExt());
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
    if (!QFile::exists(fileName)) return;

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
    connect(_catalog, &Catalog::memoCreated, [this](){ this->updateCounter(); });
    connect(_catalog, &Catalog::memoRemoved, [this](){ this->updateCounter(); });
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
    _statusMemoCount->clear();
}

void MainWindow::updateCounter()
{
    auto res = _catalog->countMemos();
    if (res.ok())
    {
        _statusMemoCount->setToolTip(QString());
        _statusMemoCount->setText(tr("Memos: %1").arg(res.result()));
    }
    else
    {
        _statusMemoCount->setToolTip(res.error());
        _statusMemoCount->setText(tr("Memos: ERROR"));
    }
}

void MainWindow::updateMenuCatalog()
{
    bool hasCatalog = _catalog;

    _actionCatalogCreateTopLevelFolder->setEnabled(hasCatalog);
}

void MainWindow::updateMenuMemo()
{
    bool canOpen = false;

    if (_catalog)
    {
        auto selected = _catalogView->selection();
        bool hasSelection = selected.memo;

        canOpen = hasSelection;
    }

    _actionOpenMemo->setEnabled(canOpen);
}

MemoWindow* MainWindow::activePlot() const
{
    auto mdiChild = _mdiArea->currentSubWindow();
    if (!mdiChild) return nullptr;
    return qobject_cast<MemoWindow*>(mdiChild->widget());
}

void MainWindow::openMemo()
{
    auto selected = _catalogView->selection();
    if (!selected.memo) return;

    if (!selected.memo->memo())
    {
        auto res = _catalog->loadMemo(selected.memo);
        if (!res.isEmpty()) return Ori::Dlg::error(res);
    }


    auto mdiChild = findMemoSubWindow(selected.memo);
    if (mdiChild)
        _mdiArea->setActiveSubWindow(mdiChild);
    else
    {
        auto memoWindow = new MemoWindow(_catalog, selected.memo);
        memoWindow->setTitleFont(_titleFont);
        memoWindow->setMemoFont(_memoFont);

        mdiChild = new QMdiSubWindow;
        mdiChild->setWidget(memoWindow);
        mdiChild->setAttribute(Qt::WA_DeleteOnClose);
        mdiChild->resize(_mdiArea->size() * 0.7);
        _mdiArea->addSubWindow(mdiChild);
        mdiChild->show();
    }
}

QMdiSubWindow* MainWindow::findMemoSubWindow(MemoItem* item) const
{
    for (auto mdiChild : _mdiArea->subWindowList())
    {
        auto memoWindow = qobject_cast<MemoWindow*>(mdiChild->widget());
        if (memoWindow && memoWindow->memoItem() == item) return mdiChild;
    }
    return nullptr;
}

bool chooseFont(QFont* targetFont)
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, *targetFont, qApp->activeWindow(), QString(),
        QFontDialog::ScalableFonts | QFontDialog::NonScalableFonts |
        QFontDialog::MonospacedFonts | QFontDialog::ProportionalFonts);
    if (ok) *targetFont = font;
    return ok;
}

void MainWindow::chooseMemoFont()
{
    if (chooseFont(&_memoFont))
        for (auto mdiChild : _mdiArea->subWindowList())
        {
            auto memoWindow = qobject_cast<MemoWindow*>(mdiChild->widget());
            if (memoWindow) memoWindow->setMemoFont(_memoFont);
        }
}

void MainWindow::chooseTitleFont()
{
    if (!chooseFont(&_titleFont))
        for (auto mdiChild : _mdiArea->subWindowList())
        {
            auto memoWindow = qobject_cast<MemoWindow*>(mdiChild->widget());
            if (memoWindow) memoWindow->setTitleFont(_titleFont);
        }
}
