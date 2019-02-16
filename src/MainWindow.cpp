#include "MainWindow.h"
#include "CatalogWidget.h"
#include "MemoWindow.h"
//#include "WindowsWidget.h"
#include "catalog/Catalog.h"
#include "catalog/CatalogStore.h"
#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWindows.h"
#include "tools/OriMruList.h"
#include "tools/OriSettings.h"
#include "tools/OriWaitCursor.h"
#include "widgets/OriMruMenu.h"

#include <QDebug>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMdiArea>
#include <QMessageBox>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>


//void MemoMdiSubWindow::closeEvent(QCloseEvent *event)
//{
//    bool canClose = emit windowClosing();
//    if (canClose)
//        event->accept();
//    else
//        event->ignore();
//}


MainWindow::MainWindow() : QMainWindow()
{
    setObjectName("mainWindow");
    Ori::Wnd::setWindowIcon(this, ":/icon/main");

    _mruList = new Ori::MruFileList(this);
    connect(_mruList, &Ori::MruFileList::clicked, this, &MainWindow::openCatalog);

    _openedMemosList = new QListWidget;

    _memoPages = new QStackedWidget;

    createMenu();

    _catalogView = new CatalogWidget(_actionOpenMemo);
    connect(_catalogView, &CatalogWidget::contextMenuAboutToShow, this, &MainWindow::updateMenuCatalog);

    //_windowsView = new WindowsWidget(_mdiArea);

    auto splitter = new QSplitter;
    splitter->addWidget(_openedMemosList);
    splitter->addWidget(_memoPages);
    splitter->addWidget(_catalogView);
    setCentralWidget(splitter);

    createStatusBar();

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
//    auto actionViewCatalog = addViewPanelAction(m, tr("Catalog Panel"), _dockCatalog);
//    auto actionViewInfo = addViewPanelAction(m, tr("Info Panel"), _dockInfo);
//    auto actionViewWindows = addViewPanelAction(m, tr("Memos Panel"), _dockWindows);
//    m->addSeparator();
//    connect(m, &QMenu::aboutToShow, [this, actionViewCatalog, actionViewInfo, actionViewWindows](){
//        actionViewCatalog->setChecked(_dockCatalog->isVisible());
//        actionViewInfo->setChecked(_dockInfo->isVisible());
//        actionViewWindows->setChecked(_dockWindows->isVisible());
//    });

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
    s.setValue("memoFont", _memoSettings.memoFont);
    s.setValue("titleFont", _memoSettings.titleFont);
    s.setValue("wordWrap", _memoSettings.wordWrap);
    if (_catalog)
    {
        s.setValue("database", _catalog->fileName());
        saveSession(&s);
    }
}

void MainWindow::loadSettings()
{
    Ori::Settings s;
    s.restoreWindowGeometry(this);
    _mruList->load(s.settings());

    _memoSettings.memoFont = qvariant_cast<QFont>(s.value("memoFont", QFont("Arial", 12)));
    _memoSettings.titleFont = qvariant_cast<QFont>(s.value("titleFont", QFont("Arial", 14)));
    _memoSettings.wordWrap = s.value("wordWrap", false).toBool();

    auto lastFile = s.value("database").toString();
    if (!lastFile.isEmpty())
        QTimer::singleShot(200, [this, lastFile](){ openCatalog(lastFile); });
}

void MainWindow::loadSession()
{
    Ori::Settings settings;
    loadSession(&settings);
}

void MainWindow::loadSession(Ori::Settings* settings)
{
    settings->beginGroup(QFileInfo(_catalog->fileName()).baseName());
    QStringList expandedIds = settings->value("expandedFolders").toString().split(',');
    _catalogView->setExpandedIds(expandedIds);

    QStringList openedIds = settings->value("openedMemos").toString().split(',');
    for (auto idStr : openedIds)
    {
        auto memoItem = _catalog->findMemoById(idStr.toInt());
        if (!memoItem) continue;
        openWindowForItem(memoItem);
    }

    int activeId = settings->value("activeMemo", -1).toInt();
    auto activeMemoItem = _catalog->findMemoById(activeId);
    if (activeMemoItem) openWindowForItem(activeMemoItem);
}

void MainWindow::saveSession()
{
    Ori::Settings settings;
    saveSession(&settings);
}

void MainWindow::saveSession(Ori::Settings* settings)
{
    QStringList openedIds;
    int activeId = -1;
    auto activeWidget = _memoPages->currentWidget();
    for (int i = 0; i < _memoPages->count(); i++)
    {
        auto widget = _memoPages->widget(i);
        auto memoWindow = qobject_cast<MemoWindow*>(widget);
        if (!memoWindow) continue;
        int memoId = memoWindow->memoItem()->id();
        openedIds << QString::number(memoId);
        if (widget == activeWidget)
            activeId = memoId;
    }
    QStringList expandedIds = _catalogView->getExpandedIds();
    settings->beginGroup(QFileInfo(_catalog->fileName()).baseName());
    settings->setValue("expandedFolders", expandedIds.join(','));
    settings->setValue("openedMemos", openedIds.join(','));
    settings->setValue("activeMemo", activeId);
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
    closeAllMemos();
}

void MainWindow::closeAllMemos()
{
    // TODO
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
//    for (auto mdiChild : _mdiArea->subWindowList())
//    {
//        auto memoWindow = memoWindowOfMdiChild(mdiChild);
//        if (!memoWindow) continue;
//        if (!canClose(memoWindow))
//        {
//            event->ignore();
//            return;
//        }
//    }
    event->accept();
    saveSession();
}

MemoWindow* MainWindow::activeMemoWindow() const
{
//    auto mdiChild = _mdiArea->currentSubWindow();
//    if (!mdiChild) return nullptr;
//    return memoWindowOfMdiChild(mdiChild);
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

    auto existedPage = findMemoPage(item);
    if (existedPage)
    {
        _memoPages->setCurrentWidget(existedPage);
        return;
    }

    auto memoWindow = new MemoWindow(_catalog, item);
    memoWindow->setTitleFont(_memoSettings.titleFont);
    memoWindow->setMemoFont(_memoSettings.memoFont);
    memoWindow->setWordWrap(_memoSettings.wordWrap);
    _memoPages->addWidget(memoWindow);
}

QWidget* MainWindow::findMemoPage(MemoItem* item) const
{
    for (int i = 0; i < _memoPages->count(); i++)
    {
        auto widget = _memoPages->widget(i);
        auto memoWindow = qobject_cast<MemoWindow*>(widget);
        if (!memoWindow) continue;
        if (memoWindow->memoItem() == item)
            return widget;
    }
    return nullptr;
}

//MemoWindow* MainWindow::memoWindowOfMdiChild(QMdiSubWindow* subWindow) const
//{
//    return subWindow ? qobject_cast<MemoWindow*>(subWindow->widget()) : nullptr;
//}

bool MainWindow::memoWindowAboutToClose()
{
//    auto subWindow = qobject_cast<QMdiSubWindow*>(sender());
//    if (!subWindow) return true;

//    _prevWindowWasMaximized = subWindow->windowState() & Qt::WindowMaximized;

//    auto memoWindow = memoWindowOfMdiChild(subWindow);
//    return memoWindow && canClose(memoWindow);
    return true;
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
//    auto window = qobject_cast<QMdiSubWindow*>(sender());
//    if (window && _prevWindowWasMaximized)
//        if (!(window->windowState() & Qt::WindowMaximized))
//            window->setWindowState(Qt::WindowMaximized);
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
//    if (chooseFont(&_memoSettings.memoFont))
//        for (auto mdiChild : _mdiArea->subWindowList())
//        {
//            auto memoWindow = memoWindowOfMdiChild(mdiChild);
//            if (memoWindow) memoWindow->setMemoFont(_memoSettings.memoFont);
//        }
}

void MainWindow::chooseTitleFont()
{
//    if (chooseFont(&_memoSettings.titleFont))
//        for (auto mdiChild : _mdiArea->subWindowList())
//        {
//            auto memoWindow = memoWindowOfMdiChild(mdiChild);
//            if (memoWindow) memoWindow->setTitleFont(_memoSettings.titleFont);
//        }
}

void MainWindow::toggleWordWrap()
{
//    _memoSettings.wordWrap = !_memoSettings.wordWrap;
//    for (auto mdiChild : _mdiArea->subWindowList())
//    {
//        auto memoWindow = memoWindowOfMdiChild(mdiChild);
//        if (memoWindow) memoWindow->setWordWrap(_memoSettings.wordWrap);
//    }
}

void MainWindow::memoCreated(MemoItem* item)
{
    updateCounter();

    openWindowForItem(item);

//    auto memoWindow = memoWindowOfMdiChild(findMemoMdiChild(item));
//    if (memoWindow) memoWindow->beginEditing();
}

void MainWindow::memoRemoved(MemoItem* item)
{
    updateCounter();

//    auto mdiChild = findMemoMdiChild(item);
//    if (mdiChild) delete mdiChild;
}
