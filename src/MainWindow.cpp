#include "MainWindow.h"

#include "AppSettings.h"
#include "core/Enot.h"
#include "core/MemoType.h"
#include "highlighter/PhlManager.h"
#include "tabs/HelpTab.h"
#include "tabs/PhlEditorTab.h"
#include "tabs/CssEditorTab.h"
#include "tabs/IssueMemoTab.h"
#include "tabs/PlainTextMemoTab.h"
#include "tabs/TextMemoTab.h"
#include "tabs/GridViewMemoTab.h"
#include "tabs/SqlConsoleTab.h"
#include "tabs/QssEditorTab.h"
#include "tabs/CmdConsoleTab.h"
#include "widgets/OpenTabsWidget.h"
#include "widgets/TreeWidget.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWindows.h"
#include "tools/OriMruList.h"
#include "tools/OriSettings.h"
#include "widgets/OriMruMenu.h"
#include "widgets/OriLabels.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
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
    connect(_mruList, &Ori::MruFileList::clicked, this, &MainWindow::openEnot);

    _tabsView = new QStackedWidget;

    _openTabsView = new OpenTabsWidget;
    connect(_openTabsView, &OpenTabsWidget::onActivateTab, _tabsView, &QStackedWidget::setCurrentWidget);

    _treeView = new TreeWidget;
    connect(_treeView, &TreeWidget::memoOpenRequested, this, &MainWindow::openMemoTab);

    _splitter = new QSplitter;
    _splitter->addWidget(_openTabsView);
    _splitter->addWidget(_tabsView);
    _splitter->addWidget(_treeView);
    _splitter->setStretchFactor(0, 0);
    _splitter->setStretchFactor(1, 1);
    _splitter->setStretchFactor(2, 0);

#ifndef Q_OS_WIN
    if (AppSettings::instance().useNativeMenuBar)
        setContentsMargins(0, 3, 0, 0);
#endif
    setCentralWidget(_splitter);

    createMenu();
    createStatusBar();
}

MainWindow::~MainWindow()
{
    if (_enot)
        delete _enot;
}

void MainWindow::createMenu()
{
    QMenu* m;

    menuBar()->setNativeMenuBar(AppSettings::instance().useNativeMenuBar);

    m = menuBar()->addMenu(tr("File"));
    m->addAction(tr("New..."), this, &MainWindow::newEnot);
    m->addAction(tr("Open..."), QKeySequence::Open, this, &MainWindow::openEnotViaDialog);
    m->addSeparator();
    /* TODO
    m->addAction(tr("Application Settings"), this, [this]{
        activateOrOpenNewTab<AppSettingsTab>(_tabsView, _openTabsView);
    });
    m->addSeparator();
    */
    auto actionExit = m->addAction(tr("Exit"), QKeySequence::Quit, this, &MainWindow::close);
    new Ori::Widgets::MruMenuPart(_mruList, m, actionExit, this);

    m = menuBar()->addMenu(tr("Tools"));

    if (AppSettings::instance().isDevMode)
    {
        m->addSeparator();
        m->addAction(tr("Application QSS"), this, [this]{
            activateOrOpenNewTab<QssEditorTab>(_tabsView, _openTabsView);
        });
        m->addAction(tr("Markdown CSS"), this, [this]{
            activateOrOpenNewTab<CssEditorTab>(_tabsView, _openTabsView);
        });
        m->addAction(tr("SQL Console"), this, [this]{
            openNewTab<SqlConsoleTab>(_tabsView, _openTabsView);
        });
        m->addAction(tr("Command Console"), this, [this]{
            openNewTab<CmdConsoleTab>(_tabsView, _openTabsView, _enot);
        });
    }

    m->addAction(tr("Highlighter Manager..."), this, [this]{
        Phl::showManagerDlg([this](Phl::SpecPtr spec){
            activateOrOpenHighlighEditorTab(_tabsView, _openTabsView, spec);
        });
    });

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

    auto versionLabel = new Ori::Widgets::Label(qApp->applicationVersion());
    connect(versionLabel, &Ori::Widgets::Label::doubleClicked, this, []{ HelpTab::showAbout(); });
    statusBar()->addPermanentWidget(versionLabel);
}

void MainWindow::saveSettings(QSettings* s)
{
    Ori::SettingsHelper::storeWindowGeometry(s, this);

    Ori::SettingsGroup group(s, "Common");

    auto sizes = _splitter->sizes();
    s->setValue("memosPanel_width", sizes.at(0));
    s->setValue("foldersPanel_width", sizes.at(2));

    if (!_lastOpenedDb.isEmpty())
        s->setValue("database", _lastOpenedDb);
}

void MainWindow::loadSettings(QSettings* s)
{
    Ori::SettingsHelper::restoreWindowGeometry(s, this);

    Ori::SettingsGroup group(s, "Common");
    _mruList->load(s);

    int w1 = s->value("memosPanel_width", 260).toInt();
    int w3 = s->value("foldersPanel_width", 260).toInt();
    int w2 = _splitter->width() - w1 - w3;
    _splitter->setSizes({w1, w2, w3});

    auto lastFile = s->value("database").toString();
    if (!lastFile.isEmpty())
        QTimer::singleShot(200, this, [this, lastFile](){ openEnot(lastFile); });
}

void MainWindow::loadSession()
{
    auto dbUid = _enot->uid();
    if (dbUid.isEmpty())
    {
        qWarning() << "Unable to get database uid, session will not be restored:" << _enot->fileName();
        return;
    }

    Ori::Settings settings;

    settings.beginGroup(dbUid);
    QStringList expandedIds = settings.value("expandedFolders").toString().split(',');
    _treeView->setExpandedIds(expandedIds);

    QStringList openedIds = settings.value("openedMemos").toString().split(',');
    for (const auto& idStr : std::as_const(openedIds))
    {
        auto memo = _enot->findMemoById(idStr.toInt());
        if (!memo) continue;
        openMemoTab(memo);
    }

    int activeId = settings.value("activeMemo", -1).toInt();
    auto activeMemoItem = _enot->findMemoById(activeId);
    if (activeMemoItem) openMemoTab(activeMemoItem);
}

void MainWindow::saveSession()
{
    auto dbUid = _enot->getOrMakeUid();
    if (dbUid.isEmpty())
    {
        qWarning() << "Unable to get database uid, session will not be saved:" << _enot->fileName();
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
        int memoId = memoWindow->memo()->id();
        openedIds << QString::number(memoId);
        if (widget == activeWidget)
            activeId = memoId;
    }
    QStringList expandedIds = _treeView->getExpandedIds();
    settings.beginGroup(dbUid);
    settings.setValue("path", _enot->fileName());
    settings.setValue("expandedFolders", expandedIds.join(','));
    settings.setValue("openedMemos", openedIds.join(','));
    settings.setValue("activeMemo", activeId);
}

void MainWindow::newEnot()
{
    QString fileName = Ori::Dlg::getSaveFileName(
                tr("Create Notebook"), Enot::fileFilter(), Enot::defaultFileExt());
    if (fileName.isEmpty()) return;

    if (!closeEnot()) return;

    auto res = Enot::create(fileName);
    if (res.ok())
        enotOpened(res.result());
    else Ori::Dlg::error(tr("Unable to create notebook.\n\n%1").arg(res.error()));
}

void MainWindow::openEnot(const QString &fileName)
{
    if (!QFile::exists(fileName)) return;

    if (_enot && QFileInfo(_enot->fileName()) == QFileInfo(fileName))
        return;

    if (!closeEnot()) return;

    auto res = Enot::open(fileName);
    if (res.ok())
        enotOpened(res.result());
    else Ori::Dlg::error(tr("Unable to load notebook %1.\n\n%2").arg(fileName, res.error()));
}

void MainWindow::openEnotViaDialog()
{
    QString fileName = QFileDialog::getOpenFileName(
                this, tr("Open Notebook"), QString(), Enot::fileFilter());
    if (!fileName.isEmpty())
        openEnot(fileName);
}

void MainWindow::enotOpened(Enot* enot)
{
    _enot = enot;
    connect(_enot, &Enot::entryCreated, this, &MainWindow::itemCreated);
    connect(_enot, &Enot::entryDeleted, this, &MainWindow::itemRemoved);
    connect(_enot, &Enot::errorOccurred, this, [](const QString& error){
        Ori::Dlg::Defer::error(error);
    });
    _treeView->setEnot(_enot);
    auto filePath = _enot->fileName();
    auto fileName = QFileInfo(filePath).fileName();
    setWindowTitle(fileName % " - " % qApp->applicationName());
    _mruList->append(filePath);
    _statusFileName->setText(QDir::toNativeSeparators(filePath));
    _lastOpenedDb = filePath;
    Phl::reset();
    updateCounter();
    loadSession();

    auto cmdConsole = findTab<CmdConsoleTab>(_tabsView);
    if (cmdConsole) cmdConsole->setEnot(_enot);
}

bool MainWindow::closeEnot()
{
    if (_enot)
    {
        saveSession();
        if (!closeAllMemos()) return false;
        _treeView->setEnot(nullptr);
        delete _enot;
        _enot = nullptr;
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
    for (auto tab : std::as_const(deletingPages))
        tab->deleteLater();
    return true;
}

void MainWindow::updateCounter()
{
    auto res = _enot->countMemos();
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!closeEnot())
    {
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::openMemoTab(Memo* memo)
{
    if (!memo->isLoaded())
    {
        auto res = _enot->loadMemo(memo);
        if (!res.isEmpty()) return Ori::Dlg::error(res);
    }

    auto existedPage = findMemoTab(memo);
    if (existedPage)
    {
        _tabsView->setCurrentWidget(existedPage);
        _openTabsView->addOpenedTab(existedPage);
        return;
    }

    MemoTab* tab = nullptr;

    if (memo->type() == MemoType::plainText())
        tab = new PlainTextMemoTab(_enot, memo);
    else if (memo->type() == MemoType::markdown())
        tab = new TextMemoTab(_enot, memo);
    else if (memo->type() == MemoType::gridView())
    {
        tab = new GridViewMemoTab(_enot, memo);
        connect((GridViewMemoTab*)tab, &GridViewMemoTab::memoOpenRequested, this, &MainWindow::openMemoTab);
    }
    else if (memo->type() == MemoType::issue())
        tab = new IssueMemoTab(_enot, memo);

    if (!tab)
    {
        qWarning() << "Unknown how to open the memo of type" << memo->type()->name();
        return;
    }

    _tabsView->addWidget(tab);
    _tabsView->setCurrentWidget(tab);
    _openTabsView->addOpenedTab(tab);

    // In some cases, when a tab added to the tabs view,
    // tab's font can be reset to the parent's one.
    // For example, it happens with markdown editor.
    // So assign font _after_ the tab added to the tabs view.
    tab->loadSettings();
}

MemoTab* MainWindow::findMemoTab(Memo* memo) const
{
    for (int i = 0; i < _tabsView->count(); i++)
    {
        auto widget = _tabsView->widget(i);
        auto tab = qobject_cast<MemoTab*>(widget);
        if (!tab) continue;
        if (tab->memo() == memo)
            return tab;
    }
    return nullptr;
}

MemoTab* MainWindow::currentMemoTab() const
{
    return dynamic_cast<MemoTab*>(_tabsView->currentWidget());
}

void MainWindow::itemCreated(Entry* entry)
{
    auto memo = entry->asMemo();
    if (!memo) return;

    updateCounter();

    openMemoTab(memo);

    auto tab = findMemoTab(memo);
    if (tab) tab->beginEdit();
}

void MainWindow::itemRemoved(Entry* entry)
{
    auto memo = entry->asMemo();
    if (!memo) return;

    updateCounter();

    auto tab = findMemoTab(memo);
    if (tab) tab->deleteLater();
}
