#include "IssueMemoTab.h"

#include "AppSettings.h"
#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "core/MemoType.h"
#include "widgets/MemoTextEdit.h"
#include "widgets/MemoTextBrowser.h"
#include "widgets/MemoPropsPanel.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriWidgets.h"
#include "helpers/OriWindows.h"
#include "tools/OriPersistentState.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextDocument>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QWidgetAction>

using namespace Qt::StringLiterals;

#define FIXED_TEXT_HEIGHT_EXTRA  40


static QString dateToStr(const QDateTime& date)
{
    return QLocale::system().toString(date, QLocale::ShortFormat);
}

//------------------------------------------------------------------------------
//                             IssueTextBrowser
//------------------------------------------------------------------------------

class IssueTextBrowser : public MemoTextBrowser
{
public:
    explicit IssueTextBrowser(QWidget *parent = 0) : MemoTextBrowser()
    {
    }

    void setText(const QString& text) = delete;

    void setIssueText(const QString& text)
    {
        MemoTextBrowser::setText(text);
    }
};

//------------------------------------------------------------------------------
//                                IssueEditDlg
//------------------------------------------------------------------------------

class IssueEditDlg : public QWidget
{
public:
    IssueEditDlg(QWidget *parent, Enot *enot, Memo *memo) : QWidget(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowFlags(Qt::Tool);
        setWindowTitle(memo ? tr("Edit Issue #%1").arg(memo->id()) : tr("Create New Issue"));

        auto addPropButton = new QToolButton;
        addPropButton->setToolTip(tr("Add Property..."));
        addPropButton->setIcon(QIcon(":/toolbar/plus"));

        _propsPanel = new MemoPropsPanel(enot, {addPropButton});
        _propsPanel->hideWhenEmpty = false;

        connect(addPropButton, &QToolButton::clicked, _propsPanel, &MemoPropsPanel::addPropViaDlg);

        _title = new QPlainTextEdit;
        _title->setProperty("role", "issue_title_in_dlg");
        _title->setWordWrapMode(QTextOption::WrapMode::WordWrap);
        _title->setAcceptDrops(false);
        _title->setTabChangesFocus(true);

        _summary = new MemoTextEdit;
        _summary->setObjectName("code_editor");
        _summary->setProperty("role", "issue_text_in_tab");
        _summary->setWordWrap(true);
        _summary->document()->setModified(false);

        _preview = new IssueTextBrowser;
        _preview->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
        _preview->setProperty("role", "issue_text_in_tab");

        _tabs = new QTabWidget;
        _tabs->addTab(_summary, tr("Edit"));
        _tabs->addTab(_preview, tr("Preview"));
        connect(_tabs, &QTabWidget::currentChanged, this, &IssueEditDlg::updatePreview);

        // Actions to make hotkeys available
        addAction(Ori::Gui::action("", this, &IssueEditDlg::cancelDlg, 0, QKeySequence(Qt::Key_Escape, Qt::Key_Escape)));
        addAction(Ori::Gui::action("", this, &IssueEditDlg::applyDlg, 0, QKeySequence("Ctrl+Return")));

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::rejected, this, &IssueEditDlg::cancelDlg);
        connect(buttons, &QDialogButtonBox::accepted, this, &IssueEditDlg::applyDlg);

        Ori::Layouts::LayoutV({_propsPanel, tr("Title"), _title, tr("Summary"), _tabs, buttons}).useFor(this);

        Ori::PersistentState::restoreWindowGeometry("IssueEditDlg", this, {800, 400});

        if (memo)
        {
            _title->setPlainText(memo->title());
            _summary->setPlainText(memo->data());
            _propsPanel->setValues(memo->props());
        }
        else
        {
            _isNewMemo = true;
            auto existingProps = enot->propNames();
            auto jsonState = Ori::PersistentState::load("issue_memo");
            auto jsonProps = jsonState["recent_props"].toArray();
            QHash<QString, QString> values;
            for (auto it = jsonProps.cbegin(); it != jsonProps.cend(); it++)
            {
                QString propName = it->toString();
                if (existingProps.contains(propName))
                    values.insert(propName, QString());
            }
            _propsPanel->setValues(values);
        }

        QTimer::singleShot(0, this, [this]{ _propsPanel->setReadOnly(false); });

        if (!memo || memo->title().isEmpty())
            _title->setFocus();
        else
            _summary->setFocus();
    }

    ~IssueEditDlg()
    {
        Ori::PersistentState::storeWindowGeometry("IssueEditDlg", this);
    }

    QString titleText() const
    {
        QString text = _title->toPlainText();
        text.replace('\n', ' ');
        return text.trimmed();
    }

    QString summaryText() const
    {
        return _summary->toPlainText();
    }

    std::optional<QHash<QString, QString>> memoProps() const
    {
        if (_propsPanel->hasValues() && _propsPanel->isModified())
            return _propsPanel->values();
        return {};
    }

    std::function<bool()> onApply;

protected:
    void closeEvent(class QCloseEvent* e) override
    {
        if (_canClose || canClose())
            e->accept();
        else
            e->ignore();
    }

private:
    void updatePreview()
    {
        if (_tabs->currentIndex() == 1)
            _preview->setIssueText(_summary->toPlainText());
    }

    bool canClose() const
    {
        if (isModified())
            return Ori::Dlg::yes(tr("There are unsaved changes. Cancel anyway?"));
        return true;
    }

    void cancelDlg()
    {
        if (!canClose()) return;
        _summary->cleanFiles();
        _canClose = true;
        close();
    }

    void applyDlg()
    {
        if (_isNewMemo)
        {
            auto jsonState = Ori::PersistentState::load("issue_memo");
            jsonState["recent_props"] = QJsonArray::fromStringList(_propsPanel->propNames());
            Ori::PersistentState::save("issue_memo", jsonState);
        }

        bool canClose = true;
        if (isModified() && onApply)
        {
            _propsPanel->apply();
            canClose = onApply();
        }
        if (canClose)
        {
            _canClose = true;
            close();
        }
    }

    bool isModified() const
    {
        return _title->document()->isModified() || _summary->document()->isModified() || _propsPanel->isModified();
    }

    MemoPropsPanel *_propsPanel;
    QPlainTextEdit *_title;
    MemoTextEdit *_summary;
    IssueTextBrowser *_preview;
    QTabWidget *_tabs;
    bool _canClose = false;
    bool _isNewMemo = false;
};

//------------------------------------------------------------------------------
//                               IssueCommentDlg
//------------------------------------------------------------------------------

class IssueCommentDlg : public QWidget
{
public:
    IssueCommentDlg(QWidget *parent, const QString& title, const QString& text = {}) : QWidget(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowFlags(Qt::Tool);
        setWindowTitle(title);
    
        _editor = new MemoTextEdit;
        _editor->setObjectName("code_editor");
        _editor->setProperty("role", "issue_text_in_tab");
        _editor->setWordWrap(true);
        _editor->setPlainText(text);
        _editor->document()->setModified(false);

        _preview = new IssueTextBrowser;
        _preview->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
        _preview->setProperty("role", "issue_text_in_tab");

        _tabs = new QTabWidget;
        _tabs->addTab(_editor, tr("Edit"));
        _tabs->addTab(_preview, tr("Preview"));
        connect(_tabs, &QTabWidget::currentChanged, this, &IssueCommentDlg::updatePreview);

        // Actions to make hotkeys available
        addAction(Ori::Gui::action("", this, &IssueCommentDlg::cancelDlg, 0, QKeySequence(Qt::Key_Escape, Qt::Key_Escape)));
        addAction(Ori::Gui::action("", this, &IssueCommentDlg::applyDlg, 0, QKeySequence("Ctrl+Return")));

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::rejected, this, &IssueCommentDlg::cancelDlg);
        connect(buttons, &QDialogButtonBox::accepted, this, &IssueCommentDlg::applyDlg);
        
        Ori::Layouts::LayoutV({_tabs, buttons}).useFor(this);
        
        Ori::PersistentState::restoreWindowGeometry("IssueCommentDlg", this, {800, 400});
        
        _editor->setFocus();
    }
    
    ~IssueCommentDlg()
    {
        Ori::PersistentState::storeWindowGeometry("IssueCommentDlg", this);
    }

    std::function<bool(const QString& text)> onApply;
    
protected:
    void closeEvent(class QCloseEvent* e) override
    {
        if (_canClose || canClose())
            e->accept();
        else
            e->ignore();
    }

private:
    void updatePreview()
    {
        if (_tabs->currentIndex() == 1)
            _preview->setIssueText(_editor->toPlainText());
    }
    
    bool canClose() const
    {
        if (_editor->document()->isModified())
            return Ori::Dlg::yes(tr("Text has been changed. Cancel anyway?"));
        return true;
    }
    
    void cancelDlg()
    {
        if (!canClose()) return;
        _editor->cleanFiles();
        _canClose = true;
        close();
    }
    
    void applyDlg()
    {
        bool canClose = true;
        if (onApply)
            canClose = onApply(_editor->toPlainText());
        
        if (canClose)
        {
            _canClose = true;
            close();
        }
    }
    
    MemoTextEdit *_editor;
    IssueTextBrowser *_preview;
    QTabWidget *_tabs;
    bool _canClose = false;
};

//------------------------------------------------------------------------------
//                             IssueMemoTab
//------------------------------------------------------------------------------

typedef IssueMemoTab Self;

IssueMemoTab::IssueMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
#define A_ Ori::Gui::action

    auto idLabel = new QLabel('#' + QString::number(memo->id()));
    idLabel->setObjectName("issue_id");
    
    _titleEditor = TabHelpers::makeTitleEditor();
    
    // TODO: make them optional
    _actionSolveIssue = A_(tr("Solve Issue"), this, &Self::solveIssue, ":/toolbar/apply");
    _actionCloseIssue = A_(tr("Close Issue"), this, &Self::closeIssue, ":/toolbar/stop");
    _actionReopenIssue = A_(tr("Reopen Issue"), this, &Self::reopenIssue, ":/toolbar/restore");

    _toolbar = TabHelpers::makeHeaderToolBar();
    _toolbar->addSeparator();
    _toolbar->addAction(_actionSolveIssue);
    _toolbar->addAction(_actionCloseIssue);
    _toolbar->addAction(_actionReopenIssue);
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){ deleteLater(); });
    
    auto toolPanel = TabHelpers::makeHeaderPanel({idLabel, _titleEditor, _toolbar});

    _labelUpdated = new QLabel;
    _labelUpdated->setObjectName("issue_updated");

    _issueInfo = makePopupInfo(_memo->id());

    auto toolMenu = new QMenu(this);
    toolMenu->addAction(_issueInfo.action);
    toolMenu->addSeparator();
    toolMenu->addAction(QIcon(":/toolbar/edit"), tr("Edit Issue..."), this, &Self::editIssue);
    toolMenu->addAction(QIcon(":/toolbar/comment"), tr("Add Comment..."), this, &Self::commentIssue);
    toolMenu->addAction(QIcon(":/toolbar/copy"), tr("Copy Summary"), this, &Self::copySummary);
    toolMenu->addSeparator();
    toolMenu->addAction(_actionSolveIssue);
    toolMenu->addAction(_actionCloseIssue);
    toolMenu->addAction(_actionReopenIssue);

    _propsPanel = new MemoPropsPanel(enot, {_labelUpdated, TabHelpers::makeMenuButton(toolMenu)});
    _propsPanel->hideWhenEmpty = false;

    _summaryView = new IssueTextBrowser;
    _summaryView->setObjectName("issue_summary");

    auto contentWidget = new QWidget;
    contentWidget->setObjectName("issue_content_widget");

    _contentLayout = new QVBoxLayout(contentWidget);
    _contentLayout->setSpacing(0);
    _contentLayout->setContentsMargins(0, 0, 0, 0);
    _contentLayout->addWidget(_summaryView, 0, Qt::AlignTop);

    _contentScroller = new QScrollArea;
    _contentScroller->setObjectName("issue_content_scroller");
    _contentScroller->setProperty("role", "memo_editor");
    _contentScroller->setWidgetResizable(true);
    _contentScroller->setWidget(contentWidget);

    Ori::Layouts::LayoutV({toolPanel, _propsPanel, _contentScroller}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    showHistory();
    //toggleEditMode(false);

#undef A_
}

IssueMemoTab::PopupInfo IssueMemoTab::makePopupInfo(int id)
{
    PopupInfo info;

    // auto idName = new QLabel(tr("Id:"));
    // idName->setProperty("role", "isssue_popup_info_name");
    // auto idValue = new QLabel(QString::number(id));
    // idValue->setProperty("role", "issue_popup_info_value");

    auto created = new QLabel(tr("Created:"));
    created->setProperty("role", "isssue_popup_info_name");
    info.created = new QLabel;
    info.created->setProperty("role", "issue_popup_info_value");

    auto updated = new QLabel(tr("Updated:"));
    updated->setProperty("role", "isssue_popup_info_name");
    info.updated = new QLabel;
    info.updated->setProperty("role", "issue_popup_info_value");

    auto station = new QLabel(tr("Station:"));
    station->setProperty("role", "isssue_popup_info_name");
    info.station = new QLabel;
    info.station->setProperty("role", "issue_popup_info_value");

    auto widget = new QWidget(this);

    Ori::Layouts::Grid({
        //{ idName, idValue },
        { created, info.created },
        { updated, info.updated },
        { station, info.station }
    }).useFor(widget);

    auto action = new QWidgetAction(this);
    action->setDefaultWidget(widget);

    info.action = action;
    return info;
}

void IssueMemoTab::showMemo()
{
    _titleEditor->setText(_memo->title());
    _summaryView->setIssueText(_memo->data());
    _labelUpdated->setText(dateToStr(_memo->updated()));
    _issueInfo.created->setText(dateToStr(_memo->created()));
    _issueInfo.updated->setText(dateToStr(_memo->updated()));
    _issueInfo.station->setText(_memo->station());
    _propsPanel->setValues(_memo->props());
    if (_actionSolveIssue)
    {
        bool on = canSolveIssue();
        _actionSolveIssue->setVisible(on);
        _actionSolveIssue->setEnabled(on);
    }
    if (_actionCloseIssue)
    {
        bool on = canCloseIssue();
        _actionCloseIssue->setVisible(on);
        _actionCloseIssue->setEnabled(on);
    }
    if (_actionReopenIssue)
    {
        bool on = canReopenIssue();
        _actionReopenIssue->setVisible(on);
        _actionReopenIssue->setEnabled(on);
    }
    setWindowTitle(_memo->title());
    QTimer::singleShot(0, this, &Self::updateSummaryHeight);
}

void IssueMemoTab::showHistory()
{
    _contentScroller->setUpdatesEnabled(false);
    
    // Remove the last stretch item
    if (_contentLayout->count() > 0)
    {
        auto item = _contentLayout->itemAt(_contentLayout->count()-1);
        if (dynamic_cast<QSpacerItem*>(item))
            _contentLayout->removeItem(item);
    }

    auto events = Store::memos()->loadEvents(_memo->id());
    auto comments = Store::memos()->loadSheets(_memo->id());

    // Events and comments are store in different tables
    // but should be displayed at the same list orderred by their date
    struct HistoryItem
    {
        QDateTime moment;
        std::variant<MemoEvent, MemoSheet> item;
    };
    QList<HistoryItem> history;
    for (const auto& event : std::as_const(events))
        history << HistoryItem{ .moment = event.moment(), .item = event };
    for (const auto& comment : std::as_const(comments))
        history << HistoryItem{ .moment = comment.created(), .item = comment };
    std::sort(history.begin(), history.end(), [](const HistoryItem& a, const HistoryItem& b){
        return a.moment < b.moment;
    });

    // Property changes are stored in different rows
    // but should be displayed gropped by date
    struct PropChangeItem
    {
        QDateTime moment;
        QHash<QString, QPair<QString, QString>> propValues;
    };
    std::optional<PropChangeItem> propsChange;
    QHash<QString, QString> propValues;

    auto makeNumLabel = [this](int id = 0){
        auto label = new QLabel();
        label->setObjectName("issue_event_num");
        if (id > 0)
            label->setToolTip(tr("Comment #%1").arg(id));
        _eventNumLabels << label;
        return label;
    };

    auto makeDateLabel = [](const QDateTime& date){
        auto label = new QLabel(dateToStr(date));
        label->setObjectName("issue_event_date");
        return label;
    };

    auto makePropChangeWidget = [this, &propsChange, &makeNumLabel, &makeDateLabel](){
        if (!propsChange)
            return;

        auto propNames = propsChange->propValues.keys();
        propNames.sort();
        QStringList report;
        int initialValueCount = 0;
        for (const auto &propName : std::as_const(propNames))
        {
            const auto& change = propsChange->propValues.value(propName);
            if (change.first.isEmpty() && !change.second.isEmpty())
            {
                // This this the first history record
                // No need to show that all properties changed from "(none)" to some value
                initialValueCount++;
            }
            else
            {
                QString oldValue = change.first.isEmpty() ? tr("(none)") : change.first;
                QString newValue = change.second.isEmpty() ? tr("(none)") : change.second;
                report << u"%1:&nbsp;<b>%2&nbsp;→&nbsp;%3</b>"_s.arg(propName, oldValue, newValue);
            }
        }

        // This this the first history record
        // No need to show that all properties changed from "(none)" to some value
        if (initialValueCount == propNames.size())
        {
            propsChange.reset();
            return;
        }

        auto propLabel = new QLabel(report.join(u". "_s));
        propLabel->setWordWrap(true);
        propLabel->setObjectName("issue_props_changes");
        propLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

        auto header = new QFrame;
        Ori::Layouts::LayoutH({
                makeNumLabel(),
                propLabel,
                makeDateLabel(propsChange->moment),
                TabHelpers::makeMenuButton(nullptr),
            }).setMargin(0).setSpacing(0).useFor(header);
        header->setProperty("role", "issue_event_header");
        header->setProperty("role1", "issue_prop_changes");
        _contentLayout->addWidget(header, 0, Qt::AlignTop);
        _shownEvents.insert(propsChange->moment);

        propsChange.reset();
    };

    for (const auto& item : std::as_const(history))
    {
        if (std::holds_alternative<MemoEvent>(item.item))
        {
            const auto& event = std::get<MemoEvent>(item.item);
            if (!event.what().startsWith("prop:"_L1))
                continue;

            QString propName = event.what().split(':').last();
            QString oldValue = propValues.value(propName);
            QString newValue = event.value();
            propValues[propName] = newValue;

            if (_shownEvents.contains(event.moment()))
                continue;

            if (!propsChange || event.moment() > propsChange->moment)
            {
                if (propsChange)
                    makePropChangeWidget();

                propsChange = PropChangeItem();
                propsChange->moment = event.moment();
            }

            propsChange->propValues.insert(propName, qMakePair(oldValue, newValue));
        }
        else if (std::holds_alternative<MemoSheet>(item.item))
        {
            makePropChangeWidget();

            const auto& comment = std::get<MemoSheet>(item.item);

            if (_commentViews.contains(comment.id()))
            {
                auto& commentView = _commentViews[comment.id()];
                if (comment.updated() > commentView.updated)
                {
                    commentView.sourceText = comment.data();
                    commentView.textView->setIssueText(comment.data());
                    commentView.labelUpdated->setText(dateToStr(comment.updated()));
                    commentView.popupInfo.updated->setText(dateToStr(comment.updated()));
                    commentView.popupInfo.station->setText(comment.station());
                }
                continue;
            }

            CommentData commentView;

            commentView.sourceText = comment.data();
            commentView.updated = comment.updated();

            commentView.textView = new IssueTextBrowser;
            commentView.textView->setProperty("role", "issue_comment");
            commentView.textView->setIssueText(comment.data());

            commentView.labelUpdated = makeDateLabel(comment.updated());

            commentView.popupInfo = makePopupInfo(comment.id());
            commentView.popupInfo.created->setText(dateToStr(comment.created()));
            commentView.popupInfo.updated->setText(dateToStr(comment.updated()));
            commentView.popupInfo.station->setText(comment.station());

            auto menu = new QMenu(commentView.textView);
            menu->addAction(commentView.popupInfo.action);
            menu->addSeparator();
            menu->addAction(QIcon(":/toolbar/edit"), tr("Edit Comment..."), this, [this, comment]{
                editComment(comment.id());
            });
            menu->addAction(QIcon(":/toolbar/copy"), tr("Copy Comment"), this, [this, comment]{
                qApp->clipboard()->setText(comment.data());
            });

            auto header = new QFrame;
            Ori::Layouts::LayoutH({
                    makeNumLabel(comment.id()),
                    Ori::Layouts::Stretch(),
                    commentView.labelUpdated,
                    TabHelpers::makeMenuButton(menu),
                }).setMargin(0).setSpacing(0).useFor(header);
            header->setProperty("role", "issue_event_header");
            _contentLayout->addWidget(header, 0, Qt::AlignTop);

            _contentLayout->addWidget(commentView.textView, 0, Qt::AlignTop);
            _commentViews.insert(comment.id(), commentView);
        }
    }
    makePropChangeWidget();

    for (int i = 0; i < _eventNumLabels.size(); i++)
        _eventNumLabels.at(i)->setText(QString::number(i+1));

    _contentLayout->addStretch();
    _contentScroller->setUpdatesEnabled(true);

    QTimer::singleShot(0, this, &Self::updateCommentHeights);
}

void IssueMemoTab::updateSummaryHeight()
{
    if (!_memo->data().isEmpty())
    {
        _summaryView->setFixedHeight(_summaryView->document()->size().height() + FIXED_TEXT_HEIGHT_EXTRA);
        _summaryView->setEnabled(true);
    }
    else
    {
        // Make margin above the first comment block
        // to have even margins around all comments blocks when there is no summary block
        _summaryView->setFixedHeight(4);
        _summaryView->setEnabled(false);
    }
}

void IssueMemoTab::updateCommentHeights()
{
    for (const auto& commentView : std::as_const(_commentViews))
        commentView.textView->setFixedHeight(commentView.textView->document()->size().height() + FIXED_TEXT_HEIGHT_EXTRA);
}

void IssueMemoTab::resizeEvent(QResizeEvent *e)
{
    MemoTab::resizeEvent(e);
    updateSummaryHeight();
    updateCommentHeights();
}

void IssueMemoTab::createIssue(Enot* enot, Folder* folder)
{
    auto dlg = new IssueEditDlg(qApp->activeWindow(), enot, nullptr);
    dlg->onApply = [dlg, enot, folder](){
        MemoUpdateParam initialData;
        initialData.title = dlg->titleText();
        initialData.data = dlg->summaryText();
        initialData.props = dlg->memoProps();

        auto res = enot->createMemo(folder, MemoType::issue(), initialData);
        if (!res.ok())
            return false;

        return true;
    };
    dlg->show();
    dlg->activateWindow();
}

void IssueMemoTab::editIssue()
{
    if (!_editDlg)
    {
        _editDlg = new IssueEditDlg(this, _enot, _memo);
        _editDlg->onApply = [this](){
            MemoUpdateParam update;
            QString newTitle = _editDlg->titleText();
            if (newTitle != _memo->title())
                update.title = newTitle;
            QString newSummary = _editDlg->summaryText();
            if (newSummary != _memo->data())
                update.data = newSummary;
            update.props = _editDlg->memoProps();

            auto ok = _enot->updateMemo(_memo, update);
            if (!ok) return false;

            QTimer::singleShot(0, this, &Self::showMemo);
            if (update.props)
                QTimer::singleShot(0, this, &Self::showHistory);
            return true;
        };
    }
    _editDlg->show();
    _editDlg->activateWindow();
}

void IssueMemoTab::addComment(const QString &newStatus)
{
    if (!_commentDlg)
    {
        QString title;
        if (newStatus == propStatusSolved)
            title = tr("Solve Issue #%1");
        else if (newStatus == propStatusClosed)
            title = tr("Close Issue #%1");
        else if (newStatus == propStatusOpened)
            title = tr("Reopen Issue #%1");
        else
            title = tr("Add Comment For Issue #%1");
        _commentDlg = new IssueCommentDlg(this, title.arg(_memo->id()));
        _commentDlg->onApply = [this, newStatus](const QString& text){
            bool propsChanged = false;
            bool historyChanged = false;
            auto now = QDateTime::currentDateTime();
            if (!text.isEmpty())
            {
                QString res = Store::memos()->addSheet(_memo->id(), text, now);
                if (!res.isEmpty()) {
                    Ori::Dlg::Defer::error(res);
                    return false;
                }
                historyChanged = true;
            }
            if (!newStatus.isEmpty())
            {
                QHash<QString, QString> props = _memo->props();
                props[propStatus] = newStatus;
                _enot->updateMemoProps(_memo, props, now);
                historyChanged = true;
                propsChanged = true;
            }
            if (historyChanged)
                QTimer::singleShot(0, this, &Self::showHistory);
            if (propsChanged)
                QTimer::singleShot(0, this, &Self::showMemo);
            return true;
        };
    }
    _commentDlg->show();
    _commentDlg->activateWindow();
}

void IssueMemoTab::editComment(int id)
{
    if (!_commentDlg)
    {
        const auto& commentView = _commentViews.value(id);
        _commentDlg = new IssueCommentDlg(this, tr("Edit Comment For Issue #%1").arg(_memo->id()), commentView.sourceText);
        _commentDlg->onApply = [this, id](const QString& text){
            if (text == _memo->data())
                return true;
            QString res = Store::memos()->updateSheet(id, text);
            if (!res.isEmpty()) {
                Ori::Dlg::Defer::error(res);
                return false;
            }
            QTimer::singleShot(0, this, &Self::showHistory);
            return true;
        };
    }
    _commentDlg->show();
    _commentDlg->activateWindow();
}

void IssueMemoTab::copySummary()
{
    qApp->clipboard()->setText(_memo->data());
}

bool IssueMemoTab::canSolveIssue() const
{
    return _memo->props().value(propStatus) == propStatusOpened;
}

bool IssueMemoTab::canCloseIssue() const
{
    return _memo->props().value(propStatus) == propStatusSolved;
}

bool IssueMemoTab::canReopenIssue() const
{
    return _memo->props().value(propStatus) == propStatusClosed;
}
