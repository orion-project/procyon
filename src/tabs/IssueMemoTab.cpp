#include "IssueMemoTab.h"

#include "AppSettings.h"
#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "markdown/MarkdownHelper.h"
#include "widgets/IssueTextEdit.h"
#include "widgets/MemoPropsPanel.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriWidgets.h"
#include "helpers/OriWindows.h"

#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
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


static QString dateToStr(const QDateTime& date)
{
    return QLocale::system().toString(date, QLocale::ShortFormat);
}

//------------------------------------------------------------------------------
//                             IssueMemoView
//------------------------------------------------------------------------------

class IssueMemoView : public QTextBrowser
{
public:
    explicit IssueMemoView(QWidget *parent = 0) : QTextBrowser()
    {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
    }

private:
    void linkClicked(const QUrl&)
    {

    }

    void linkHovered(const QUrl&)
    {

    }
};

//------------------------------------------------------------------------------
//                             IssueMemoTab
//------------------------------------------------------------------------------

class IssueTextDlg : public QWidget
{
public:
    IssueTextDlg(QWidget *parent, const QString& title, const QString& text = {}) : QWidget(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setWindowFlags(Qt::Tool);
        setWindowTitle(title);
    
        _editor = new IssueTextEdit;
        _editor->setObjectName("code_editor");
        _editor->setProperty("role", "issue_text_in_dlg");
        _editor->setPlainText(text);
        _editor->document()->setModified(false);

        _preview = new QTextBrowser;
        _preview->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
        _preview->setProperty("role", "issue_text_in_dlg");

        _tabs = new QTabWidget;
        _tabs->addTab(_editor, tr("Edit"));
        _tabs->addTab(_preview, tr("Preview"));
        connect(_tabs, &QTabWidget::currentChanged, this, &IssueTextDlg::updatePreview);

        // Actions to make hotkeys available
        addAction(Ori::Gui::action("", this, &IssueTextDlg::cancelDlg, 0, QKeySequence(Qt::Key_Escape, Qt::Key_Escape)));
        addAction(Ori::Gui::action("", this, &IssueTextDlg::applyDlg, 0, QKeySequence("Ctrl+Return")));

        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttons, &QDialogButtonBox::rejected, this, &IssueTextDlg::cancelDlg);
        connect(buttons, &QDialogButtonBox::accepted, this, &IssueTextDlg::applyDlg);
        
        Ori::Layouts::LayoutV({_tabs, buttons}).useFor(this);
        
        restoreGeometry();
        
        _editor->setFocus();
    }
    
    ~IssueTextDlg()
    {
        __storedGeometry = saveGeometry();
    }
    
    std::function<bool(const QString& text)> onApply;
    
protected:
    void closeEvent(class QCloseEvent* e) override
    {
        if (_saved || canClose())
            e->accept();
        else
            e->ignore();
    }

private:
    void updatePreview()
    {
        if (_tabs->currentIndex() == 1)
            _preview->setHtml(MarkdownHelper::markdownToHtml(_editor->toPlainText()));
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
        close();
    }
    
    void applyDlg()
    {
        bool canClose = true;
        if (_editor->document()->isModified() && onApply)
            canClose = onApply(_editor->toPlainText());
        
        if (canClose)
        {
            _saved = true;
            close();
        }
    }
    
    void restoreGeometry()
    {
        if (!__storedGeometry.isEmpty())
            QWidget::restoreGeometry(__storedGeometry);
        else
        {
            resize(600, 300);
            Ori::Wnd::moveToScreenCenter(this, qApp->activeWindow());
        }
    }
    
    IssueTextEdit *_editor;
    QTextBrowser *_preview;
    QTabWidget *_tabs;
    bool _saved = false;

    static QByteArray __storedGeometry;
};

QByteArray IssueTextDlg::__storedGeometry = {};

//------------------------------------------------------------------------------
//                             IssueMemoTab
//------------------------------------------------------------------------------

typedef IssueMemoTab Self;

IssueMemoTab::IssueMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    auto idLabel = new QLabel('#' + QString::number(memo->id()));
    idLabel->setObjectName("issue_id");
    
    _titleEditor = TabHelpers::makeTitleEditor();
    
    _toolbar = TabHelpers::makeHeaderToolBar();

    _actionEdit = _toolbar->addAction(QIcon(":/toolbar/edit"), tr("Edit"), this, &Self::beginEdit);
    _actionSave = _toolbar->addAction(QIcon(":/toolbar/apply"), tr("Save"), this, &Self::saveEdit);
    _actionCancel = _toolbar->addAction(QIcon(":/toolbar/cancel"), tr("Cancel"), this, &Self::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){ deleteLater(); });
    
    auto toolPanel = TabHelpers::makeHeaderPanel({idLabel, _titleEditor, _toolbar});

    _labelUpdated = new QLabel;
    _labelUpdated->setObjectName("issue_updated");

    _issueInfo = makePopupInfo();

    auto toolMenu = new QMenu(this);
    toolMenu->addAction(_issueInfo.action);
    toolMenu->addSeparator();
    toolMenu->addAction(tr("Add Comment..."), this, &Self::addComment);

    _propsPanel = new MemoPropsPanel(enot, {_labelUpdated, TabHelpers::makeMenuButton(toolMenu)});
    _propsPanel->hideWhenEmpty = false;

    _summaryView = new IssueMemoView;
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

    const auto& props = memo->props();
    for (auto it = props.cbegin(); it != props.cend(); it++)
        _propsPanel->addProp(it.key(), it.value());

    showMemo();
    showHistory();
    toggleEditMode(false);
}

IssueMemoTab::PopupInfo IssueMemoTab::makePopupInfo()
{
    PopupInfo info;

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
    _summaryView->setHtml(MarkdownHelper::markdownToHtml(_memo->data()));
    _labelUpdated->setText(dateToStr(_memo->updated()));
    _issueInfo.created->setText(dateToStr(_memo->created()));
    _issueInfo.updated->setText(dateToStr(_memo->updated()));
    _issueInfo.station->setText(_memo->station());
    setWindowTitle(_memo->title());
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

    int eventNum = 1;

    auto makeNumLabel = [&eventNum]{
        auto label = new QLabel(QString::number(eventNum++));
        label->setObjectName("issue_event_num");
        return label;
    };

    auto makeDateLabel = [](const QDateTime& date){
        auto label = new QLabel(dateToStr(date));
        label->setObjectName("issue_event_date");
        return label;
    };

    auto makePropChangeWidget = [this, &propsChange, &makeNumLabel, &makeDateLabel](){
        if (!propsChange) return;

        auto propNames = propsChange->propValues.keys();
        propNames.sort();
        QStringList report;
        for (const auto &propName : std::as_const(propNames))
        {
            const auto& change = propsChange->propValues.value(propName);
            QString oldValue = change.first.isEmpty() ? tr("(none)") : change.first;
            QString newValue = change.second.isEmpty() ? tr("(none)") : change.second;
            report << u"%1:&nbsp;<b>%2&nbsp;→&nbsp;%3</b>"_s.arg(propName, oldValue, newValue);
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
                const auto& commentView = _commentViews.value(comment.id());
                if (comment.updated() > commentView.updated)
                {
                    commentView.textView->setHtml(MarkdownHelper::markdownToHtml(comment.data()));
                    commentView.labelUpdated->setText(dateToStr(comment.updated()));
                    commentView.popupInfo.updated->setText(dateToStr(comment.updated()));
                    commentView.popupInfo.station->setText(comment.station());
                }
                continue;
            }

            CommentData commentView;

            commentView.sourceText = comment.data();
            commentView.updated = comment.updated();

            commentView.textView = new IssueMemoView;
            commentView.textView->setProperty("role", "issue_comment");
            commentView.textView->setHtml(MarkdownHelper::markdownToHtml(comment.data()));

            commentView.labelUpdated = makeDateLabel(comment.updated());

            commentView.popupInfo = makePopupInfo();
            commentView.popupInfo.created->setText(dateToStr(comment.created()));
            commentView.popupInfo.updated->setText(dateToStr(comment.updated()));
            commentView.popupInfo.station->setText(comment.station());

            auto menu = new QMenu(commentView.textView);
            menu->addAction(commentView.popupInfo.action);
            menu->addSeparator();
            menu->addAction(tr("Edit Comment..."), this, [this, comment]{ editComment(comment.id()); });

            auto header = new QFrame;
            Ori::Layouts::LayoutH({
                    makeNumLabel(),
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

    _contentLayout->addStretch();
    _contentScroller->setUpdatesEnabled(true);

    QTimer::singleShot(0, this, &Self::updateViewHeights);
}

void IssueMemoTab::beginEdit()
{
    toggleEditMode(true);

    _titleEditor->setFocus();
    _titleEditor->selectAll();
}

void IssueMemoTab::cancelEdit()
{
    toggleEditMode(false);
    _titleEditor->setText(_memo->title());
}

bool IssueMemoTab::saveEdit()
{
    _propsPanel->apply();

    MemoUpdateParam update;
    QString newTitle = _titleEditor->text().trimmed();
    if (newTitle != _memo->title())
        update.title = newTitle;
    if (_propsPanel->hasValues())
        update.props = _propsPanel->values();

    auto ok = _enot->updateMemo(_memo, update);
    if (!ok) return false;

    setWindowTitle(_memo->title());
    toggleEditMode(false);
    return true;
}

void IssueMemoTab::toggleEditMode(bool on)
{
    _propsPanel->setReadOnly(on);

    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    TabHelpers::setTitleEditorReadOnly(_titleEditor, !on);
}

void IssueMemoTab::updateViewHeights()
{
    const int maxBordersWidth = 40;
    if (!_memo->data().isEmpty())
    {
        _summaryView->setVisible(true);
        _summaryView->setFixedHeight(_summaryView->document()->size().height() + maxBordersWidth);
    }
    else
        _summaryView->setVisible(false);
    for (const auto& commentView : std::as_const(_commentViews))
        commentView.textView->setFixedHeight(commentView.textView->document()->size().height() + maxBordersWidth);
}

void IssueMemoTab::resizeEvent(QResizeEvent *e)
{
    MemoTab::resizeEvent(e);
    updateViewHeights();
}

void IssueMemoTab::addComment()
{
    if (!_commentDlg)
    {
        _commentDlg = new IssueTextDlg(this, tr("Add Comment For Issue #%1").arg(_memo->id()));
        _commentDlg->onApply = [this](const QString& text){
            QString res = Store::memos()->addSheet(_memo->id(), text);
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

void IssueMemoTab::editComment(int id)
{
    if (!_commentDlg)
    {
        const auto& commentView = _commentViews.value(id);
        _commentDlg = new IssueTextDlg(this, tr("Edit Comment For Issue #%1").arg(_memo->id()), commentView.sourceText);
        _commentDlg->onApply = [this, id](const QString& text){
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
