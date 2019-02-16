#include "MainWindow.h"
#include "CatalogWidget.h"
#include "InfoWidget.h"
#include "MemoWindow.h"
#include "WindowsWidget.h"
#include "catalog/Catalog.h"
#include "catalog/CatalogStore.h"
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
#include <QMessageBox>
#include <QMenuBar>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>


void MemoMdiSubWindow::closeEvent(QCloseEvent *event)
{
    bool canClose = emit windowClosing();
    if (canClose)
        event->accept();
    else
        event->ignore();
}


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
}

MainWindow::~MainWindow()
{
    saveSettings();

    if (_catalog)
        delete _catalog;
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
    auto actionViewCatalog = addViewPanelAction(m, tr("Catalog Panel"), _dockCatalog);
    auto actionViewInfo = addViewPanelAction(m, tr("Info Panel"), _dockInfo);
    auto actionViewWindows = addViewPanelAction(m, tr("Memos Panel"), _dockWindows);
    m->addSeparator();
    m->addMenu(new Ori::Widgets::StylesMenu(this));
    connect(m, &QMenu::aboutToShow, [this, actionViewCatalog, actionViewInfo, actionViewWindows](){
        actionViewCatalog->setChecked(_dockCatalog->isVisible());
        actionViewInfo->setChecked(_dockInfo->isVisible());
        actionViewWindows->setChecked(_dockWindows->isVisible());
    });

    m = menuBar()->addMenu(tr("&Catalog"));
    connect(m, &QMenu::aboutToShow, this, &MainWindow::updateMenuCatalog);
    _actionCreateTopLevelFolder = m->addAction(tr("New Top Level Folder..."), [this](){ _catalogView->createTopLevelFolder(); });
    _actionCreateFolder = m->addAction(tr("New Folder..."), [this](){ _catalogView->createFolder(); });
    _actionRenameFolder = m->addAction(tr("Rename Folder..."), [this](){ _catalogView->renameFolder(); });
    _actionDeleteFolder = m->addAction(tr("Delete Folder"), [this](){ _catalogView->deleteFolder(); });
    m->addSeparator();
    _actionOpenMemo = m->addAction(tr("Open memo"), this, &MainWindow::openMemo);
    _actionCreateMemo = m->addAction(tr("New memo"), [this](){ _catalogView->createMemo(); });
    _actionDeleteMemo = m->addAction(tr("Delete memo"), [this](){ _catalogView->deleteMemo(); });

    m = menuBar()->addMenu(tr("&Options"));
    m->addAction(tr("Choose Memo Font..."), this, &MainWindow::chooseMemoFont);
    m->addAction(tr("Choose Title Font..."), this, &MainWindow::chooseTitleFont);
    auto actionWordWrap = m->addAction(tr("Word Wrap"), this, &MainWindow::toggleWordWrap);
    actionWordWrap->setCheckable(true);
    connect(m, &QMenu::aboutToShow, [this, actionWordWrap](){
        actionWordWrap->setChecked(_memoSettings.wordWrap);
    });
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
    s.setValue("memoFont", _memoSettings.memoFont);
    s.setValue("titleFont", _memoSettings.titleFont);
    s.setValue("wordWrap", _memoSettings.wordWrap);
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

    _memoSettings.memoFont = qvariant_cast<QFont>(s.value("memoFont", QFont("Arial", 12)));
    _memoSettings.titleFont = qvariant_cast<QFont>(s.value("titleFont", QFont("Arial", 14)));
    _memoSettings.wordWrap = s.value("wordWrap", false).toBool();

    auto lastFile = s.value("database").toString();
    if (!lastFile.isEmpty())
        QTimer::singleShot(200, [this, lastFile](){ openCatalog(lastFile); });
}

void MainWindow::loadSession()
{
    bool isMaximized = CatalogStore::settingsManager()->readBool("isMaximized", false);
    QVector<int> ids = CatalogStore::settingsManager()->readIntArray("openedMemos");
    int activeId = CatalogStore::settingsManager()->readInt("activeMemo", -1);
    QMdiSubWindow *activeWindow = nullptr;
    for (int id: ids)
    {
        MemoItem* memo = _catalog->findMemoById(id);
        if (memo)
        {
            openWindowForItem(memo);
            if (memo->id() == activeId)
                activeWindow = findMemoMdiChild(memo);
        }
    }
    if (!activeWindow)
    {
        auto subWindows = _mdiArea->subWindowList();
        if (!subWindows.isEmpty())
            activeWindow = subWindows.last();
    }
    if (activeWindow)
    {
        _mdiArea->setActiveSubWindow(activeWindow);
        if (isMaximized)
            if (!(activeWindow->windowState() & Qt::WindowMaximized))
                activeWindow->setWindowState(Qt::WindowMaximized);
    }
}

void MainWindow::saveSession()
{
    QVector<int> ids;
    bool isMaximized = false;
    int activeId = -1;
    for (auto subWindow: _mdiArea->subWindowList())
    {
        auto memoWindow = memoWindowOfMdiChild(subWindow);
        if (!memoWindow) continue;
        ids << memoWindow->memoItem()->id();
        if (!isMaximized)
            isMaximized = subWindow->windowState() & Qt::WindowMaximized;
        if (subWindow == _mdiArea->activeSubWindow())
            activeId = memoWindow->memoItem()->id();
    }
    CatalogStore::settingsManager()->writeIntArray("openedMemos", ids, SettingsManager::RespectValuesOrder);
    CatalogStore::settingsManager()->writeBool("isMaximized", isMaximized);
    if (activeId >= 0)
        CatalogStore::settingsManager()->writeInt("activeMemo", activeId);
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
    if (_catalog)
    {
        saveSession();
        delete _catalog;
        _catalog = nullptr;
    }
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    for (auto mdiChild : _mdiArea->subWindowList())
    {
        auto memoWindow = memoWindowOfMdiChild(mdiChild);
        if (!memoWindow) continue;
        if (!canClose(memoWindow))
        {
            event->ignore();
            return;
        }
    }
    event->accept();
    saveSession();
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

    bool isMaximized = _mdiArea->activeSubWindow() &&
            (_mdiArea->activeSubWindow()->windowState() & Qt::WindowMaximized);

    auto existedChild = findMemoMdiChild(item);
    if (existedChild)
    {
        _mdiArea->setActiveSubWindow(existedChild);
        if (isMaximized)
            existedChild->setWindowState(Qt::WindowMaximized);
        return;
    }

    auto memoWindow = new MemoWindow(_catalog, item);
    memoWindow->setTitleFont(_memoSettings.titleFont);
    memoWindow->setMemoFont(_memoSettings.memoFont);
    memoWindow->setWordWrap(_memoSettings.wordWrap);

    auto mdiChild = new MemoMdiSubWindow;
    mdiChild->setWidget(memoWindow);
    mdiChild->setAttribute(Qt::WA_DeleteOnClose);
    mdiChild->resize(_mdiArea->size() * 0.7);
    connect(mdiChild, &MemoMdiSubWindow::windowClosing, this, &MainWindow::memoWindowAboutToClose);
    connect(mdiChild, &MemoMdiSubWindow::aboutToActivate, this, &MainWindow::memoWindowAboutToActivate);
    _mdiArea->addSubWindow(mdiChild);
    mdiChild->show();
    if (isMaximized)
        mdiChild->setWindowState(Qt::WindowMaximized);
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

bool MainWindow::memoWindowAboutToClose()
{
    auto subWindow = qobject_cast<QMdiSubWindow*>(sender());
    if (!subWindow) return true;

    _prevWindowWasMaximized = subWindow->windowState() & Qt::WindowMaximized;

    auto memoWindow = memoWindowOfMdiChild(subWindow);
    return memoWindow && canClose(memoWindow);
}

bool MainWindow::canClose(MemoWindow* memoWindow)
{
    if (!memoWindow->isModified()) return true;

    int res = Ori::Dlg::yesNoCancel(tr("<b>%1</b><br><br>"
                                       "This memo has been changed. "
                                       "Save changes before closing?")
                                    .arg(memoWindow->windowTitle()));
    if (res == QMessageBox::Cancel) return false;
    if (res == QMessageBox::No) return true;
    if (!memoWindow->saveEditing()) return false;

    return true;
}

void MainWindow::memoWindowAboutToActivate()
{
    auto window = qobject_cast<QMdiSubWindow*>(sender());
    if (window && _prevWindowWasMaximized)
        if (!(window->windowState() & Qt::WindowMaximized))
            window->setWindowState(Qt::WindowMaximized);
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
    if (chooseFont(&_memoSettings.memoFont))
        for (auto mdiChild : _mdiArea->subWindowList())
        {
            auto memoWindow = memoWindowOfMdiChild(mdiChild);
            if (memoWindow) memoWindow->setMemoFont(_memoSettings.memoFont);
        }
}

void MainWindow::chooseTitleFont()
{
    if (chooseFont(&_memoSettings.titleFont))
        for (auto mdiChild : _mdiArea->subWindowList())
        {
            auto memoWindow = memoWindowOfMdiChild(mdiChild);
            if (memoWindow) memoWindow->setTitleFont(_memoSettings.titleFont);
        }
}

void MainWindow::toggleWordWrap()
{
    _memoSettings.wordWrap = !_memoSettings.wordWrap;
    for (auto mdiChild : _mdiArea->subWindowList())
    {
        auto memoWindow = memoWindowOfMdiChild(mdiChild);
        if (memoWindow) memoWindow->setWordWrap(_memoSettings.wordWrap);
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
