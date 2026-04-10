#include "MainWindow.h"

#include "AppSettings.h"
#include "CatalogWidget.h"
#include "OpenTabsWidget.h"
#include "db/Db.h"
#include "highlighter/PhlManager.h"
#include "tabs/HelpTab.h"
#include "tabs/PhlEditorTab.h"
#include "tabs/CssEditorTab.h"
#include "tabs/MemoTab.h"
#include "tabs/SqlConsoleTab.h"
#include "tabs/QssEditorTab.h"
#include "tabs/CmdConsoleTab.h"

#ifdef ENABLE_SPELLCHECK
#include "spellcheck/Spellchecker.h"
#endif

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWindows.h"
#include "tools/OriMruList.h"
#include "tools/OriSettings.h"
#include "widgets/OriMruMenu.h"

#include <QApplication>
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
template <typename TTab>
QVector<TTab*> getPages(QStackedWidget* tabsView)
{
    QVector<TTab*> tabs;
    for (int i = 0; i < tabsView->count(); i++)
    {
        auto tab = qobject_cast<TTab*>(tabsView->widget(i));
        if (tab) tabs << tab;
    }
    return tabs;
}

template <typename TTab, typename... Args>
void openNewTab(QStackedWidget* tabsView, OpenTabsWidget* openTabsView, Args && ...arguments)
{
    auto tab = new TTab(std::forward<Args>(arguments)...);
    tabsView->addWidget(tab);
    tabsView->setCurrentWidget(tab);
    openTabsView->addOpenedTab(tab);
}

template <typename TTab>
TTab* findTab(QStackedWidget* tabsView)
{
    for (int i = 0; i < tabsView->count(); i++)
    {
        auto widget = tabsView->widget(i);
        auto tab = qobject_cast<TTab*>(widget);
        if (tab) return tab;
    }
    return nullptr;
}

template <typename TTab>
void activateOrOpenNewTab(QStackedWidget* tabsView, OpenTabsWidget* openTabsView)
{
    auto tab = findTab<TTab>(tabsView);
    if (tab)
    {
        tabsView->setCurrentWidget(tab);
        openTabsView->addOpenedTab(tab);
        return;
    }
    openNewTab<TTab>(tabsView, openTabsView);
}

void activateOrOpenHighlighEditorTab(QStackedWidget* tabsView, OpenTabsWidget* openTabsView,
                                      QSharedPointer<Ori::Highlighter::Spec> spec)
{
    for (int i = 0; i < tabsView->count(); i++)
    {
        auto widget = tabsView->widget(i);
        auto tab = qobject_cast<PhlEditorTab*>(widget);
        if (tab && tab->spec == spec)
        {
            tabsView->setCurrentWidget(tab);
            openTabsView->addOpenedTab(tab);
            return;
        }
    }
    auto tab = new PhlEditorTab(spec);
    tabsView->addWidget(tab);
    tabsView->setCurrentWidget(tab);
    openTabsView->addOpenedTab(tab);
}

} // namespace


MainWindow::MainWindow() : QMainWindow()
{
    setObjectName("mainWindow");
    Ori::Wnd::setWindowIcon(this, ":/icon/main");

    _mruList = new Ori::MruFileList(this);
    connect(_mruList, &Ori::MruFileList::clicked, this, &MainWindow::openCatalog);

    _tabsView = new QStackedWidget;

    _openTabsView = new OpenTabsWidget;
    connect(_openTabsView, &OpenTabsWidget::onActivateTab, _tabsView, &QStackedWidget::setCurrentWidget);

    _catalogView = new CatalogWidget;
    connect(_catalogView, &CatalogWidget::onOpenMemo, this, &MainWindow::openMemoTab);

    _splitter = new QSplitter;
    _splitter->addWidget(_openTabsView);
    _splitter->addWidget(_tabsView);
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

    _highlighterControl = new Phl::Control(_highlighterMenu, this);
    connect(_highlighterControl, &Phl::Control::selected, this, &MainWindow::setMemoHighlighter);
    connect(_highlighterControl, &Phl::Control::editorRequested, this, [this](const QSharedPointer<Ori::Highlighter::Spec>& spec){
        activateOrOpenHighlighEditorTab(_tabsView, _openTabsView, spec);
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
        activateOrOpenNewTab<AppSettingsTab>(_tabsView, _openTabsView);
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
            activateOrOpenNewTab<QssEditorTab>(_tabsView, _openTabsView);
        });
        m->addAction(tr("Edit Markdown CSS"), this, [this]{
            activateOrOpenNewTab<CssEditorTab>(_tabsView, _openTabsView);
        });
        m->addAction(tr("Open SQL Console"), this, [this]{
            openNewTab<SqlConsoleTab>(_tabsView, _openTabsView);
        });
        m->addAction(tr("Open Command Console"), this, [this]{
            openNewTab<CmdConsoleTab>(_tabsView, _openTabsView, _catalog);
        });
    }

    m->addAction(tr("Highlighter Manager..."), this, [this]{ _highlighterControl->showManager(); });

    m = menuBar()->addMenu(tr("Help"));
    /* TODO
    m->addAction(tr("Show Help"), [this]{
        activateOrOpenNewTab<HelpTab>(_tabsView, _openTabsView);
    });
    m->addSeparator();
    */
    m->addAction(tr("Visit Homepage"), this, []{ HelpTab::visitHomePage(); });
    m->addAction(tr("Send Bug Report"), this, []{ HelpTab::sendBugReport(); });
#ifndef Q_OS_MAC
    m->addSeparator(); // "About" item will be extracted to the system menu, se we don't need the separator
#endif
    m->addAction(tr("About %1...").arg(qApp->applicationName()), this, []{ HelpTab::showAbout(); });
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
        openMemoTab(memoItem);
    }

    int activeId = settings.value("activeMemo", -1).toInt();
    auto activeMemoItem = _catalog->findMemoById(activeId);
    if (activeMemoItem) openMemoTab(activeMemoItem);
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
    auto activeWidget = _tabsView->currentWidget();
    for (int i = 0; i < _tabsView->count(); i++)
    {
        auto widget = _tabsView->widget(i);
        auto memoWindow = qobject_cast<MemoTab*>(widget);
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
                tr("Create Notebook"), Db::fileFilter(), Db::defaultFileExt());
    if (fileName.isEmpty()) return;

    if (!closeCatalog()) return;

    auto res = Db::create(fileName);
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

    auto res = Db::open(fileName);
    if (res.ok())
        catalogOpened(res.result());
    else Ori::Dlg::error(tr("Unable to load notebook %1.\n\n%2").arg(fileName, res.error()));
}

void MainWindow::openCatalogViaDialog()
{
    QString fileName = QFileDialog::getOpenFileName(
                this, tr("Open Notebook"), QString(), Db::fileFilter());
    if (!fileName.isEmpty())
        openCatalog(fileName);
}

void MainWindow::catalogOpened(Db* catalog)
{
    _catalog = catalog;
    connect(_catalog, &Db::memoCreated, this, &MainWindow::memoCreated);
    connect(_catalog, &Db::memoRemoved, this, &MainWindow::memoRemoved);
    _catalogView->setCatalog(_catalog);
    auto filePath = _catalog->fileName();
    auto fileName = QFileInfo(filePath).fileName();
    setWindowTitle(fileName % " - " % qApp->applicationName());
    _mruList->append(filePath);
    _statusFileName->setText(QDir::toNativeSeparators(filePath));
    _lastOpenedCatalog = filePath;
    _highlighterControl->loadMetas();
    updateCounter();
    loadSession();

    auto cmdConsole = findTab<CmdConsoleTab>(_tabsView);
    if (cmdConsole) cmdConsole->setCatalog(_catalog);
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
    for (int i = 0; i < _tabsView->count(); i++)
    {
        auto widget = _tabsView->widget(i);

        auto hleditPage = qobject_cast<PhlEditorTab*>(widget);
        if (hleditPage)
            // TODO: check if was modified
            deletingPages << hleditPage;

        auto tab = qobject_cast<MemoTab*>(widget);
        if (!tab) continue;
        if (!tab->canClose())
            return false;
        deletingPages << tab;
    }
    for (auto tab : deletingPages)
        tab->deleteLater();
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
    if (selected.memo) openMemoTab(selected.memo);
}

void MainWindow::openMemoTab(MemoItem* item)
{
    if (!item->isLoaded())
    {
        auto res = _catalog->loadMemo(item);
        if (!res.isEmpty()) return Ori::Dlg::error(res);
    }

    auto existedPage = findMemoTab(item);
    if (existedPage)
    {
        _tabsView->setCurrentWidget(existedPage);
        _openTabsView->addOpenedTab(existedPage);
        return;
    }

    auto tab = new MemoTab(_catalog, item);
    _tabsView->addWidget(tab);
    _tabsView->setCurrentWidget(tab);
    _openTabsView->addOpenedTab(tab);

    // In some cases, when a tab added to the tabs view,
    // tab's font can be reset to the parent's one.
    // For example, it happens with markdown editor.
    // So assign font _after_ the tab added to the tabs view.
    tab->loadSettings();
}

MemoTab* MainWindow::findMemoTab(MemoItem* item) const
{
    for (int i = 0; i < _tabsView->count(); i++)
    {
        auto widget = _tabsView->widget(i);
        auto tab = qobject_cast<MemoTab*>(widget);
        if (!tab) continue;
        if (tab->memoItem() == item)
            return tab;
    }
    return nullptr;
}

MemoTab* MainWindow::currentMemoTab() const
{
    return dynamic_cast<MemoTab*>(_tabsView->currentWidget());
}

void MainWindow::exportToPdf()
{
    auto memoPage = currentMemoTab();
    if (!memoPage) return;

    memoPage->exportToPdf();
}

void MainWindow::chooseMemoFont()
{
    auto memoPage = currentMemoTab();
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
    auto memoPage = currentMemoTab();
    if (!memoPage) return;

    memoPage->setWordWrap(!memoPage->wordWrap());
}

void MainWindow::memoCreated(MemoItem* item)
{
    updateCounter();

    openMemoTab(item);

    auto tab = findMemoTab(item);
    if (tab) tab->beginEdit();
}

void MainWindow::memoRemoved(MemoItem* item)
{
    updateCounter();

    auto tab = findMemoTab(item);
    if (tab) tab->deleteLater();
}

void MainWindow::optionsMenuAboutToShow()
{
    auto memoPage = currentMemoTab();
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
    auto memoPage = currentMemoTab();
    if (memoPage)
        _spellcheckControl->showCurrentLang(memoPage->spellcheckLang());
#endif
}

void MainWindow::highlighterMenuAboutToShow()
{
    QString currentHighlighter;
    auto memoPage = currentMemoTab();
    if (memoPage)
        currentHighlighter = memoPage->highlighter();
    _highlighterControl->showCurrent(currentHighlighter);
}

void MainWindow::setMemoSpellcheckLang(const QString& lang)
{
    auto memoPage = currentMemoTab();
    if (memoPage) memoPage->setSpellcheckLang(lang);
}

void MainWindow::setMemoHighlighter(const QString& name)
{
    auto memoPage = currentMemoTab();
    if (memoPage) memoPage->setHighlighter(name);
}
