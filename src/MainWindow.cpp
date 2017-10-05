#include "MainWindow.h"
#include "Catalog.h"
#include "CatalogStore.h"
#include "CatalogWidget.h"
#include "InfoWidget.h"
#include "MemoWindow.h"
#include "WindowsWidget.h"
#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWindows.h"
#include "tools/OriMruList.h"
#include "tools/OriSettings.h"
#include "tools/OriWaitCursor.h"
#include "widgets/OriMdiToolBar.h"
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

    _mdiArea = new QMdiArea;
    setCentralWidget(_mdiArea);

    createMenu();

    _catalogView = new CatalogWidget(_actionOpenMemo);
    connect(_catalogView, &CatalogWidget::contextMenuAboutToShow, this, &MainWindow::updateMenuCatalog);

    _infoView = new InfoWidget;

    _windowsView = new WindowsWidget(_mdiArea);

    createDocks();
    createStatusBar();
    createToolBars();

    loadSettings();
    loadSession();
}

MainWindow::~MainWindow()
{
    saveSettings();
    saveSession();

    if (_catalog) delete _catalog;
}

QAction* MainWindow::addViewPanelAction(QMenu* m, const QString& title, QDockWidget* panel)
{
    auto action = m->addAction(title, [panel](){
        if (panel->isVisible()) panel->hide(); else panel->show();
    });
    action->setCheckable(true);
    return action;
}

void MainWindow::createMenu()
{
    QMenu* m;

    menuBar()->setNativeMenuBar(false);

    m = menuBar()->addMenu(tr("&File"));
    m->addAction(tr("New..."), this, &MainWindow::newCatalog);
    m->addAction(tr("Open..."), this, &MainWindow::openCatalogViaDialog, QKeySequence::Open);
    m->addSeparator();
    auto actionExit = m->addAction(tr("Exit"), this, &MainWindow::close, QKeySequence::Quit);
    new Ori::Widgets::MruMenuPart(_mruList, m, actionExit, this);

    m = menuBar()->addMenu(tr("&View"));
    connect(m, &QMenu::aboutToShow, [this](){
        this->_actionViewCatalog->setChecked(this->_dockCatalog->isVisible());
        this->_actionViewInfo->setChecked(this->_dockInfo->isVisible());
        this->_actionViewWindows->setChecked(this->_dockWindows->isVisible());
    });
    _actionViewCatalog = addViewPanelAction(m, tr("Catalog Panel"), _dockCatalog);
    _actionViewInfo = addViewPanelAction(m, tr("Info Panel"), _dockInfo);
    _actionViewWindows = addViewPanelAction(m, tr("Memos Panel"), _dockWindows);
    m->addSeparator();
    m->addMenu(new Ori::Widgets::StylesMenu(this));

    m = menuBar()->addMenu(tr("&Catalog"));
    connect(m, &QMenu::aboutToShow, this, &MainWindow::updateMenuCatalog);
    _actionCreateTopLevelFolder = m->addAction(tr("New Top Level Folder..."),
        [this](){ this->_catalogView->createTopLevelFolder(); });
    _actionCreateFolder = m->addAction(tr("New Folder..."),
        [this](){ this->_catalogView->createFolder(); });
    _actionRenameFolder = m->addAction(tr("Rename Folder..."),
        [this](){ this->_catalogView->renameFolder(); });
    _actionDeleteFolder = m->addAction(tr("Delete Folder"),
        [this](){ this->_catalogView->deleteFolder(); });
    m->addSeparator();
    _actionOpenMemo = m->addAction(tr("Open memo"), this, &MainWindow::openMemo);
    _actionCreateMemo = m->addAction(tr("New memo"),
        [this](){ this->_catalogView->createMemo(); });
    _actionDeleteMemo = m->addAction(tr("Delete memo"),
        [this](){ this->_catalogView->deleteMemo(); });

    m = menuBar()->addMenu(tr("&Options"));
    m->addAction(tr("Choose Memo Font..."), this, &MainWindow::chooseMemoFont);
    m->addAction(tr("Choose Title Font..."), this, &MainWindow::chooseTitleFont);
}

void MainWindow::createToolBars()
{
    addToolBar(Qt::RightToolBarArea, new Ori::Widgets::MdiToolBar(tr("Windows"), _mdiArea));
}

void MainWindow::createDocks()
{
    _dockCatalog = new QDockWidget(tr("Catalog"));
    _dockCatalog->setObjectName("CatalogPanel");
    _dockCatalog->setWidget(_catalogView);

    _dockInfo = new QDockWidget(tr("Info"));
    _dockInfo->setObjectName("InfoPanel");
    _dockInfo->setWidget(_infoView);

    _dockWindows = new QDockWidget(tr("Memos"));
    _dockWindows->setObjectName("WindowsPanel");
    _dockWindows->setWidget(_windowsView);

    addDockWidget(Qt::LeftDockWidgetArea, _dockCatalog);
    addDockWidget(Qt::LeftDockWidgetArea, _dockInfo);
    addDockWidget(Qt::RightDockWidgetArea, _dockWindows);
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

void MainWindow::loadSession()
{
    QVector<int> ids = CatalogStore::settingsManager()->readIntArray("openedMemos");
    for (int id: ids)
    {
        CatalogItem* item = _catalog->findById(id);
        if (item && item->isMemo())
            openWindowForItem(item->asMemo());
    }
}

void MainWindow::saveSession()
{
    QVector<int> ids;
    for (auto subWindow: _mdiArea->subWindowList())
    {
        auto memoWindow = memoWindowOfMdiChild(subWindow);
        if (!memoWindow) continue;
        ids << memoWindow->memoItem()->id();
    }
    CatalogStore::settingsManager()->writeIntArray("openedMemos", ids);
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
    connect(_catalog, &Catalog::memoCreated, this, &MainWindow::memoCreated);
    connect(_catalog, &Catalog::memoRemoved, this, &MainWindow::memoRemoved);
    _catalogView->setCatalog(_catalog);
    auto filePath = _catalog->fileName();
    auto fileName = QFileInfo(filePath).fileName();
    setWindowTitle(fileName % " - " % qApp->applicationName());
    _mruList->append(filePath);
    _statusFileName->setText(tr("Catalog: %1").arg(QDir::toNativeSeparators(filePath)));
    updateCounter();
    loadSession();
}

void MainWindow::catalogClosed()
{
    saveSession();
    delete _catalog;
    _catalog = nullptr;
    _catalogView->setCatalog(nullptr);
    setWindowTitle(qApp->applicationName());
    _statusFileName->clear();
    _statusMemoCount->clear();
    _mdiArea->closeAllSubWindows();
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
    bool hasFolder = false;
    bool hasMemo = false;
    if (hasCatalog)
    {
        auto selected = _catalogView->selection();
        hasFolder = selected.folder;
        hasMemo = selected.memo;
    }
    _actionCreateTopLevelFolder->setEnabled(hasCatalog);
    _actionCreateFolder->setEnabled(hasFolder);
    _actionRenameFolder->setEnabled(hasFolder);
    _actionDeleteFolder->setEnabled(hasFolder);
    _actionOpenMemo->setEnabled(hasMemo);
    _actionDeleteMemo->setEnabled(hasMemo);
    _actionCreateMemo->setEnabled(hasFolder);
}

MemoWindow* MainWindow::activeMemoWindow() const
{
    auto mdiChild = _mdiArea->currentSubWindow();
    if (!mdiChild) return nullptr;
    return memoWindowOfMdiChild(mdiChild);
}

void MainWindow::openMemo()
{
    auto selected = _catalogView->selection();
    if (selected.memo)
        openWindowForItem(selected.memo);
}

void MainWindow::openWindowForItem(MemoItem* item)
{
    if (!item->memo())
    {
        auto res = _catalog->loadMemo(item);
        if (!res.isEmpty()) return Ori::Dlg::error(res);
    }

    auto mdiChild = findMemoMdiChild(item);
    if (mdiChild)
        _mdiArea->setActiveSubWindow(mdiChild);
    else
    {
        auto memoWindow = new MemoWindow(_catalog, item);
        memoWindow->setTitleFont(_titleFont);
        memoWindow->setMemoFont(_memoFont);

        bool isMaximized = _mdiArea->activeSubWindow() &&
            (_mdiArea->activeSubWindow()->windowState() & Qt::WindowMaximized);

        mdiChild = new QMdiSubWindow;
        mdiChild->setWidget(memoWindow);
        mdiChild->setAttribute(Qt::WA_DeleteOnClose);
        mdiChild->resize(_mdiArea->size() * 0.7);
        _mdiArea->addSubWindow(mdiChild);
        mdiChild->show();

        if (isMaximized)
            mdiChild->setWindowState(Qt::WindowMaximized);
    }
}

QMdiSubWindow* MainWindow::findMemoMdiChild(MemoItem* item) const
{
    for (auto mdiChild : _mdiArea->subWindowList())
    {
        auto memoWindow = memoWindowOfMdiChild(mdiChild);
        if (memoWindow && memoWindow->memoItem() == item) return mdiChild;
    }
    return nullptr;
}

MemoWindow* MainWindow::memoWindowOfMdiChild(QMdiSubWindow* subWindow) const
{
    return subWindow ? qobject_cast<MemoWindow*>(subWindow->widget()) : nullptr;
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
            auto memoWindow = memoWindowOfMdiChild(mdiChild);
            if (memoWindow) memoWindow->setMemoFont(_memoFont);
        }
}

void MainWindow::chooseTitleFont()
{
    if (chooseFont(&_titleFont))
        for (auto mdiChild : _mdiArea->subWindowList())
        {
            auto memoWindow = memoWindowOfMdiChild(mdiChild);
            if (memoWindow) memoWindow->setTitleFont(_titleFont);
        }
}

void MainWindow::memoCreated(MemoItem* item)
{
    updateCounter();

    openWindowForItem(item);

    auto memoWindow = memoWindowOfMdiChild(findMemoMdiChild(item));
    if (memoWindow) memoWindow->beginEditing();
}

void MainWindow::memoRemoved(MemoItem* item)
{
    updateCounter();

    auto mdiChild = findMemoMdiChild(item);
    if (mdiChild) delete mdiChild;
}
