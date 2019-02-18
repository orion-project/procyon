#include "MainWindow.h"
#include "CatalogWidget.h"
#include "MemoPage.h"
#include "StyleEditorPage.h"
#include "OpenedPagesWidget.h"
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
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStackedWidget>
#include <QTimer>

MainWindow::MainWindow() : QMainWindow()
{
    setObjectName("mainWindow");
    Ori::Wnd::setWindowIcon(this, ":/icon/main");

    _mruList = new Ori::MruFileList(this);
    connect(_mruList, &Ori::MruFileList::clicked, this, &MainWindow::openCatalog);

    _pagesView = new QStackedWidget;

    _openedPagesView = new OpenedPagesWidget;
    connect(_openedPagesView, &OpenedPagesWidget::onActivatePage, _pagesView, &QStackedWidget::setCurrentWidget);

    _catalogView = new CatalogWidget;
    connect(_catalogView, &CatalogWidget::onOpenMemo, this, &MainWindow::openMemoPage);

    _splitter = new QSplitter;
    _splitter->addWidget(_openedPagesView);
    _splitter->addWidget(_pagesView);
    _splitter->addWidget(_catalogView);
    _splitter->setStretchFactor(0, 0);
    _splitter->setStretchFactor(1, 1);
    _splitter->setStretchFactor(2, 0);
    setCentralWidget(_splitter);

    createMenu();
    createStatusBar();

    loadSettings();
}

MainWindow::~MainWindow()
{
    saveSettings();

    if (_catalog)
        delete _catalog;
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
    m->addSeparator();
    m->addAction(tr("Edit Style Sheet"), this, &MainWindow::editStyleSheet);
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

    auto sizes = _splitter->sizes();
    s.setValue("memosPanel_width", sizes.at(0));
    s.setValue("catalogPanel_width", sizes.at(2));

    if (!_lastOpenedCatalog.isEmpty())
        s.setValue("database", _lastOpenedCatalog);
}

void MainWindow::loadSettings()
{
    Ori::Settings s;
    s.restoreWindowGeometry(this);
    _mruList->load(s.settings());

    _memoSettings.memoFont = qvariant_cast<QFont>(s.value("memoFont", QFont("Arial", 12)));
    _memoSettings.titleFont = qvariant_cast<QFont>(s.value("titleFont", QFont("Arial", 14)));
    _memoSettings.wordWrap = s.value("wordWrap", false).toBool();

    int w1 = s.value("memosPanel_width", 200).toInt();
    int w3 = s.value("catalogPanel_width", 200).toInt();
    int w2 = _splitter->width() - w1 - w3;
    _splitter->setSizes({w1, w2, w3});

    auto lastFile = s.value("database").toString();
    if (!lastFile.isEmpty())
        QTimer::singleShot(200, [this, lastFile](){ openCatalog(lastFile); });
}

void MainWindow::loadSession()
{
    Ori::Settings settings;

    settings.beginGroup(QFileInfo(_catalog->fileName()).baseName());
    QStringList expandedIds = settings.value("expandedFolders").toString().split(',');
    _catalogView->setExpandedIds(expandedIds);

    QStringList openedIds = settings.value("openedMemos").toString().split(',');
    for (auto idStr : openedIds)
    {
        auto memoItem = _catalog->findMemoById(idStr.toInt());
        if (!memoItem) continue;
        openMemoPage(memoItem);
    }

    int activeId = settings.value("activeMemo", -1).toInt();
    auto activeMemoItem = _catalog->findMemoById(activeId);
    if (activeMemoItem) openMemoPage(activeMemoItem);
}

void MainWindow::saveSession()
{
    Ori::Settings settings;

    QStringList openedIds;
    int activeId = -1;
    auto activeWidget = _pagesView->currentWidget();
    for (int i = 0; i < _pagesView->count(); i++)
    {
        auto widget = _pagesView->widget(i);
        auto memoWindow = qobject_cast<MemoPage*>(widget);
        if (!memoWindow) continue;
        int memoId = memoWindow->memoItem()->id();
        openedIds << QString::number(memoId);
        if (widget == activeWidget)
            activeId = memoId;
    }
    QStringList expandedIds = _catalogView->getExpandedIds();
    settings.beginGroup(QFileInfo(_catalog->fileName()).baseName());
    settings.setValue("expandedFolders", expandedIds.join(','));
    settings.setValue("openedMemos", openedIds.join(','));
    settings.setValue("activeMemo", activeId);
}

void MainWindow::newCatalog()
{
    if (!closeCatalog()) return;

    QString fileName = Ori::Dlg::getSaveFileName(
                tr("Create Catalog"), Catalog::fileFilter(), Catalog::defaultFileExt());
    if (fileName.isEmpty()) return;

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

    if (!closeCatalog()) return;

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
    _lastOpenedCatalog = filePath;
    updateCounter();
    loadSession();
}

bool MainWindow::closeCatalog()
{
    if (_catalog)
    {
        saveSession();
        if (!closeAllMemos()) return false;
        delete _catalog;
        _catalog = nullptr;
    }
    _catalogView->setCatalog(nullptr);
    setWindowTitle(qApp->applicationName());
    _statusFileName->clear();
    _statusMemoCount->clear();
   return true;
}

bool MainWindow::closeAllMemos()
{
    QVector<QWidget*> deletingPages;
    for (int i = 0; i < _pagesView->count(); i++)
    {
        auto widget = _pagesView->widget(i);
        auto page = qobject_cast<MemoPage*>(widget);
        if (!page) continue;
        if (!page->canClose())
            return false;
        deletingPages << page;
    }
    for (auto page : deletingPages)
        page->deleteLater();
    return true;
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
    if (!closeCatalog())
    {
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::openMemo()
{
    auto selected = _catalogView->selection();
    if (selected.memo) openMemoPage(selected.memo);
}

void MainWindow::openMemoPage(MemoItem* item)
{
    if (!item->memo())
    {
        auto res = _catalog->loadMemo(item);
        if (!res.isEmpty()) return Ori::Dlg::error(res);
    }

    auto existedPage = findMemoPage(item);
    if (existedPage)
    {
        _pagesView->setCurrentWidget(existedPage);
        _openedPagesView->addOpenedPage(existedPage);
        return;
    }

    auto page = new MemoPage(_catalog, item);
    page->setTitleFont(_memoSettings.titleFont);
    page->setMemoFont(_memoSettings.memoFont);
    page->setWordWrap(_memoSettings.wordWrap);
    _pagesView->addWidget(page);
    _pagesView->setCurrentWidget(page);
    _openedPagesView->addOpenedPage(page);
}

MemoPage* MainWindow::findMemoPage(MemoItem* item) const
{
    for (int i = 0; i < _pagesView->count(); i++)
    {
        auto widget = _pagesView->widget(i);
        auto page = qobject_cast<MemoPage*>(widget);
        if (!page) continue;
        if (page->memoItem() == item)
            return page;
    }
    return nullptr;
}

namespace {
bool chooseFont(QFont* targetFont)
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, *targetFont, qApp->activeWindow(), QString(),
        QFontDialog::ScalableFonts | QFontDialog::NonScalableFonts |
        QFontDialog::MonospacedFonts | QFontDialog::ProportionalFonts);
    if (ok) *targetFont = font;
    return ok;
}

template <typename TPage>
QVector<TPage*> getPages(QStackedWidget* pagesView)
{
    QVector<TPage*> pages;
    for (int i = 0; i < pagesView->count(); i++)
    {
        auto page = qobject_cast<MemoPage*>(pagesView->widget(i));
        if (page) pages << page;
    }
    return pages;
}
} // namespace

void MainWindow::chooseMemoFont()
{
    if (chooseFont(&_memoSettings.memoFont))
        for (auto page : getPages<MemoPage>(_pagesView))
            page->setMemoFont(_memoSettings.memoFont);
}

void MainWindow::chooseTitleFont()
{
    if (chooseFont(&_memoSettings.titleFont))
        for (auto page : getPages<MemoPage>(_pagesView))
            page->setTitleFont(_memoSettings.titleFont);
}

void MainWindow::toggleWordWrap()
{
    _memoSettings.wordWrap = !_memoSettings.wordWrap;
    for (auto page : getPages<MemoPage>(_pagesView))
        page->setWordWrap(_memoSettings.wordWrap);
}

void MainWindow::memoCreated(MemoItem* item)
{
    updateCounter();

    openMemoPage(item);

    auto page = findMemoPage(item);
    if (page) page->beginEditing();
}

void MainWindow::memoRemoved(MemoItem* item)
{
    updateCounter();

    auto page = findMemoPage(item);
    if (page) page->deleteLater();
}

void MainWindow::editStyleSheet()
{
    for (int i = 0; i < _pagesView->count(); i++)
    {
        auto widget = _pagesView->widget(i);
        auto page = qobject_cast<StyleEditorPage*>(widget);
        if (page)
        {
            _pagesView->setCurrentWidget(page);
            _openedPagesView->addOpenedPage(page);
            return;
        }
    }
    auto page = new StyleEditorPage;
    _pagesView->addWidget(page);
    _pagesView->setCurrentWidget(page);
    _openedPagesView->addOpenedPage(page);
}
