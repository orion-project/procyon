#include "MainWindow.h"

#include "AppSettings.h"
#include "CatalogWidget.h"
#include "OpenedPagesWidget.h"
#include "catalog/Catalog.h"
#include "catalog/CatalogStore.h"
#include "highlighter/OriHighlighter.h"
#include "highlighter/EnotStorage.h"
#include "pages/AppSettingsPage.h"
#include "pages/HelpPage.h"
#include "pages/PhlEditorPage.h"
#include "pages/CssEditorPage.h"
#include "pages/MemoPage.h"
#include "pages/SqlConsolePage.h"
#include "pages/QssEditorPage.h"

#ifdef ENABLE_SPELLCHECK
#include "spellcheck/Spellchecker.h"
#endif

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWindows.h"
#include "tools/OriMruList.h"
#include "tools/OriSettings.h"
#include "tools/OriWaitCursor.h"
#include "widgets/OriMruMenu.h"

#include <QCloseEvent>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QFrame>
#include <QIcon>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QStackedWidget>
#include <QTimer>

namespace {
template <typename TPage>
QVector<TPage*> getPages(QStackedWidget* pagesView)
{
    QVector<TPage*> pages;
    for (int i = 0; i < pagesView->count(); i++)
    {
        auto page = qobject_cast<TPage*>(pagesView->widget(i));
        if (page) pages << page;
    }
    return pages;
}

template <typename TPage>
void openNewPage(QStackedWidget* pagesView, OpenedPagesWidget* openedPagesView)
{
    auto page = new TPage;
    pagesView->addWidget(page);
    pagesView->setCurrentWidget(page);
    openedPagesView->addOpenedPage(page);
}

template <typename TPage>
void activateOrOpenNewPage(QStackedWidget* pagesView, OpenedPagesWidget* openedPagesView)
{
    for (int i = 0; i < pagesView->count(); i++)
    {
        auto widget = pagesView->widget(i);
        auto page = qobject_cast<TPage*>(widget);
        if (page)
        {
            pagesView->setCurrentWidget(page);
            openedPagesView->addOpenedPage(page);
            return;
        }
    }
    openNewPage<TPage>(pagesView, openedPagesView);
}

void activateOrOpenHighlighEditorPage(QStackedWidget* pagesView, OpenedPagesWidget* openedPagesView,
                                      QSharedPointer<Ori::Highlighter::Spec> spec)
{
    for (int i = 0; i < pagesView->count(); i++)
    {
        auto widget = pagesView->widget(i);
        auto page = qobject_cast<PhlEditorPage*>(widget);
        if (page && page->spec == spec)
        {
            pagesView->setCurrentWidget(page);
            openedPagesView->addOpenedPage(page);
            return;
        }
    }
    auto page = new PhlEditorPage(spec);
    pagesView->addWidget(page);
    pagesView->setCurrentWidget(page);
    openedPagesView->addOpenedPage(page);
}

} // namespace


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

#ifndef Q_OS_WIN
    if (AppSettings::instance().useNativeMenuBar)
        setContentsMargins(0, 3, 0, 0);
#endif
    setCentralWidget(_splitter);

#ifdef ENABLE_SPELLCHECK
    _spellcheckControl = new SpellcheckControl(this);
    connect(_spellcheckControl, &SpellcheckControl::langSelected, this, &MainWindow::setMemoSpellcheckLang);
#endif

    createMenu();

    _highlighterControl = new Ori::Highlighter::Control(_highlighterMenu, this);
    connect(_highlighterControl, &Ori::Highlighter::Control::selected, this, &MainWindow::setMemoHighlighter);
    connect(_highlighterControl, &Ori::Highlighter::Control::editorRequested, this, [this](const QSharedPointer<Ori::Highlighter::Spec>& spec){
        activateOrOpenHighlighEditorPage(_pagesView, _openedPagesView, spec);
    });

    createStatusBar();
}

MainWindow::~MainWindow()
{
    if (_catalog)
        delete _catalog;
}

void MainWindow::createMenu()
{
    QMenu* m;

    menuBar()->setNativeMenuBar(AppSettings::instance().useNativeMenuBar);

    m = menuBar()->addMenu(tr("File"));
    m->addAction(tr("New..."), this, &MainWindow::newCatalog);
    m->addAction(tr("Open..."), this, &MainWindow::openCatalogViaDialog, QKeySequence::Open);
    m->addSeparator();
    /* TODO
    m->addAction(tr("Application Settings"), this, [this]{
        activateOrOpenNewPage<AppSettingsPage>(_pagesView, _openedPagesView);
    });
    m->addSeparator();
    */
    auto actionExit = m->addAction(tr("Exit"), this, &MainWindow::close, QKeySequence::Quit);
    new Ori::Widgets::MruMenuPart(_mruList, m, actionExit, this);

    m = menuBar()->addMenu(tr("Notebook"));
    connect(m, &QMenu::aboutToShow, this, &MainWindow::updateMenuCatalog);
    _actionCreateTopLevelFolder = m->addAction(tr("New Top Level Folder..."), this, [this](){ _catalogView->createTopLevelFolder(); });
    _actionCreateFolder = m->addAction(tr("New Subfolder..."), this, [this](){ _catalogView->createFolder(); });
    _actionRenameFolder = m->addAction(tr("Rename Folder..."), this, [this](){ _catalogView->renameFolder(); });
    _actionDeleteFolder = m->addAction(tr("Delete Folder"), this, [this](){ _catalogView->deleteFolder(); });
    m->addSeparator();
    _actionOpenMemo = m->addAction(tr("Open Memo"), this, &MainWindow::openMemo);
    _actionCreateMemo = m->addAction(tr("New Memo..."), this, [this](){ _catalogView->createMemo(); });
    _actionDeleteMemo = m->addAction(tr("Delete Memo"), this, [this](){ _catalogView->deleteMemo(); });

    m = menuBar()->addMenu(tr("Memo"));
    connect(m, &QMenu::aboutToShow, this, &MainWindow::optionsMenuAboutToShow);

    _actionMemoExportPdf = m->addAction(tr("Export to PDF..."), this, &MainWindow::exportToPdf);
    m->addSeparator();

#ifdef ENABLE_SPELLCHECK
    _spellcheckMenu = _spellcheckControl->makeMenu(this);
    if (_spellcheckMenu)
    {
        connect(_spellcheckMenu, &QMenu::aboutToShow, this, &MainWindow::spellcheckMenuAboutToShow);
        m->addMenu(_spellcheckMenu);
    }
#endif

    _highlighterMenu = m->addMenu(tr("Highlighter"));
    connect(_highlighterMenu, &QMenu::aboutToShow, this, &MainWindow::highlighterMenuAboutToShow);

    _actionMemoFont = m->addAction(tr("Choose Font..."), this, &MainWindow::chooseMemoFont);

    _actionWordWrap = m->addAction(tr("Word Wrap"), this, &MainWindow::toggleWordWrap);
    _actionWordWrap->setCheckable(true);

    if (AppSettings::instance().isDevMode)
    {
        m->addSeparator();
        m->addAction(tr("Edit Application QSS"), this, [this]{
            activateOrOpenNewPage<QssEditorPage>(_pagesView, _openedPagesView);
        });
        m->addAction(tr("Edit Markdown CSS"), this, [this]{
            activateOrOpenNewPage<CssEditorPage>(_pagesView, _openedPagesView);
        });
        m->addAction(tr("Open SQL Console"), this, [this]{
            openNewPage<SqlConsolePage>(_pagesView, _openedPagesView);
        });
    }

    m->addAction(tr("Highlighter Manager..."), this, [this]{ _highlighterControl->showManager(); });

    m = menuBar()->addMenu(tr("Help"));
    /* TODO
    m->addAction(tr("Show Help"), [this]{
        activateOrOpenNewPage<HelpPage>(_pagesView, _openedPagesView);
    });
    m->addSeparator();
    */
    m->addAction(tr("Visit Homepage"), this, []{ HelpPage::visitHomePage(); });
    m->addAction(tr("Send Bug Report"), this, []{ HelpPage::sendBugReport(); });
#ifndef Q_OS_MAC
    m->addSeparator(); // "About" item will be extracted to the system menu, se we don't need the separator
#endif
    m->addAction(tr("About %1...").arg(qApp->applicationName()), this, []{ HelpPage::showAbout(); });
}

namespace  {
QWidget* makeStatusPanel(const QString& title, QLabel*& labelValue)
{
    auto labelTitle = new QLabel(title);
    labelTitle->setProperty("role", "status_title");

    labelValue = new QLabel;
    labelValue->setProperty("role", "status_value");

    auto panel = new QFrame;
    panel->setProperty("role", "status_panel");
    Ori::Layouts::LayoutH({labelTitle, labelValue}).setMargin(0).setSpacing(0).useFor(panel);
    return panel;
}
} // namespace

void MainWindow::createStatusBar()
{
    statusBar()->addWidget(makeStatusPanel(tr("Memos:"), _statusMemoCount));
    statusBar()->addWidget(makeStatusPanel(tr("Notebook:"), _statusFileName));
}

void MainWindow::saveSettings(QSettings* s)
{
    Ori::SettingsHelper::storeWindowGeometry(s, this);

    Ori::SettingsGroup group(s, "Common");

    auto sizes = _splitter->sizes();
    s->setValue("memosPanel_width", sizes.at(0));
    s->setValue("catalogPanel_width", sizes.at(2));

    if (!_lastOpenedCatalog.isEmpty())
        s->setValue("database", _lastOpenedCatalog);
}

void MainWindow::loadSettings(QSettings* s)
{
    Ori::SettingsHelper::restoreWindowGeometry(s, this);

    Ori::SettingsGroup group(s, "Common");
    _mruList->load(s);

    int w1 = s->value("memosPanel_width", 260).toInt();
    int w3 = s->value("catalogPanel_width", 260).toInt();
    int w2 = _splitter->width() - w1 - w3;
    _splitter->setSizes({w1, w2, w3});

    auto lastFile = s->value("database").toString();
    if (!lastFile.isEmpty())
        QTimer::singleShot(200, this, [this, lastFile](){ openCatalog(lastFile); });
}

void MainWindow::loadSession()
{
    auto catalogUid = _catalog->uid();
    if (catalogUid.isEmpty())
    {
        qWarning() << "Unable to get catalog uid, session will not be restored:" << _catalog->fileName();
        return;
    }

    Ori::Settings settings;

    settings.beginGroup(catalogUid);
    QStringList expandedIds = settings.value("expandedFolders").toString().split(',');
    _catalogView->setExpandedIds(expandedIds);

    QStringList openedIds = settings.value("openedMemos").toString().split(',');
    for (const auto& idStr : openedIds)
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
    auto catalogUid = _catalog->getOrMakeUid();
    if (catalogUid.isEmpty())
    {
        qWarning() << "Unable to get catalog uid, session will not be saved:" << _catalog->fileName();
        return;
    }

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
    settings.beginGroup(catalogUid);
    settings.setValue("path", _catalog->fileName());
    settings.setValue("expandedFolders", expandedIds.join(','));
    settings.setValue("openedMemos", openedIds.join(','));
    settings.setValue("activeMemo", activeId);
}

void MainWindow::newCatalog()
{
    QString fileName = Ori::Dlg::getSaveFileName(
                tr("Create Notebook"), Catalog::fileFilter(), Catalog::defaultFileExt());
    if (fileName.isEmpty()) return;

    if (!closeCatalog()) return;

    auto res = Catalog::create(fileName);
    if (res.ok())
        catalogOpened(res.result());
    else Ori::Dlg::error(tr("Unable to create notebook.\n\n%1").arg(res.error()));
}

void MainWindow::openCatalog(const QString &fileName)
{
    if (!QFile::exists(fileName)) return;

    if (_catalog && QFileInfo(_catalog->fileName()) == QFileInfo(fileName))
        return;

    if (!closeCatalog()) return;

    auto res = Catalog::open(fileName);
    if (res.ok())
        catalogOpened(res.result());
    else Ori::Dlg::error(tr("Unable to load notebook.\n\n%1").arg(res.error()));
}

void MainWindow::openCatalogViaDialog()
{
    QString fileName = QFileDialog::getOpenFileName(
                this, tr("Open Notebook"), QString(), Catalog::fileFilter());
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
    _statusFileName->setText(QDir::toNativeSeparators(filePath));
    _lastOpenedCatalog = filePath;
    _highlighterControl->loadMetas({
        //QSharedPointer<Ori::Highlighter::SpecStorage>(new Ori::Highlighter::DefaultStorage()),
        QSharedPointer<Ori::Highlighter::SpecStorage>(new Ori::Highlighter::QrcStorage()),
        QSharedPointer<Ori::Highlighter::SpecStorage>(new EnotHighlighterStorage()),
    });
    updateCounter();
    loadSession();
}

bool MainWindow::closeCatalog()
{
    if (_catalog)
    {
        saveSession();
        if (!closeAllMemos()) return false;
        _catalogView->setCatalog(nullptr);
        delete _catalog;
        _catalog = nullptr;
    }
    setWindowTitle(qApp->applicationName());
    _statusFileName->setText(tr("(n/a)"));
    _statusMemoCount->setText(tr("(none)"));
   return true;
}

bool MainWindow::closeAllMemos()
{
    QVector<QWidget*> deletingPages;
    for (int i = 0; i < _pagesView->count(); i++)
    {
        auto widget = _pagesView->widget(i);

        auto hleditPage = qobject_cast<PhlEditorPage*>(widget);
        if (hleditPage)
            // TODO: check if was modified
            deletingPages << hleditPage;

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
        _statusMemoCount->setText(QString::number(res.result()));
    }
    else
    {
        _statusMemoCount->setToolTip(res.error());
        _statusMemoCount->setText(tr("ERROR"));
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
    if (!item->isLoaded())
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
    _pagesView->addWidget(page);
    _pagesView->setCurrentWidget(page);
    _openedPagesView->addOpenedPage(page);

    // In some cases, when a page added to the pages view,
    // page's font can be reset to the parent's one.
    // For example, it happens with markdown editor.
    // So assign font _after_ the page added to the pages view.
    page->loadSettings();
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

MemoPage* MainWindow::currentMemoPage() const
{
    return dynamic_cast<MemoPage*>(_pagesView->currentWidget());
}

void MainWindow::exportToPdf()
{
    auto memoPage = currentMemoPage();
    if (!memoPage) return;

    memoPage->exportToPdf();
}

void MainWindow::chooseMemoFont()
{
    auto memoPage = currentMemoPage();
    if (!memoPage) return;

    bool ok;
    QFont font = QFontDialog::getFont(&ok, memoPage->memoFont(),
        qApp->activeWindow(), tr("Select Memo Font"),
        QFontDialog::ScalableFonts | QFontDialog::NonScalableFonts |
        QFontDialog::MonospacedFonts | QFontDialog::ProportionalFonts);
    if (ok)
        memoPage->setMemoFont(font);
}

void MainWindow::toggleWordWrap()
{
    auto memoPage = currentMemoPage();
    if (!memoPage) return;

    memoPage->setWordWrap(!memoPage->wordWrap());
}

void MainWindow::memoCreated(MemoItem* item)
{
    updateCounter();

    openMemoPage(item);

    auto page = findMemoPage(item);
    if (page) page->beginEdit();
}

void MainWindow::memoRemoved(MemoItem* item)
{
    updateCounter();

    auto page = findMemoPage(item);
    if (page) page->deleteLater();
}

void MainWindow::optionsMenuAboutToShow()
{
    auto memoPage = currentMemoPage();
    if (_spellcheckMenu)
        _spellcheckMenu->setEnabled(memoPage && !memoPage->isReadOnly());
    _highlighterMenu->setEnabled(memoPage && memoPage->memoItem()->type() == plainTextMemoType());
    _actionMemoExportPdf->setEnabled(memoPage);
    _actionMemoFont->setEnabled(memoPage);
    _actionMemoFont->setChecked(memoPage && memoPage->wordWrap());
    _actionWordWrap->setEnabled(memoPage);
    _actionWordWrap->setChecked(memoPage && memoPage->wordWrap());
}

void MainWindow::spellcheckMenuAboutToShow()
{
#ifdef ENABLE_SPELLCHECK
    auto memoPage = currentMemoPage();
    if (memoPage)
        _spellcheckControl->showCurrentLang(memoPage->spellcheckLang());
#endif
}

void MainWindow::highlighterMenuAboutToShow()
{
    QString currentHighlighter;
    auto memoPage = currentMemoPage();
    if (memoPage)
        currentHighlighter = memoPage->highlighter();
    _highlighterControl->showCurrent(currentHighlighter);
}

void MainWindow::setMemoSpellcheckLang(const QString& lang)
{
    auto memoPage = currentMemoPage();
    if (memoPage) memoPage->setSpellcheckLang(lang);
}

void MainWindow::setMemoHighlighter(const QString& name)
{
    auto memoPage = currentMemoPage();
    if (memoPage) memoPage->setHighlighter(name);
}
