#include "IssueMemoTab.h"

#include "AppSettings.h"
#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "markdown/MarkdownHelper.h"
#include "widgets/MemoPropsPanel.h"

#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTextDocument>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QWidgetAction>

using namespace Qt::StringLiterals;

//------------------------------------------------------------------------------
//                             IssueMemoTab
//------------------------------------------------------------------------------

class IssueMemoView : public QTextBrowser
{
public:
    explicit IssueMemoView(QWidget *parent = 0) : QTextBrowser()
    {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
        //setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));
    }

protected:
    void resizeEvent(QResizeEvent *e) override
    {
        QTextBrowser::resizeEvent(e);
        //_label->move(e->size().height() - _label->width(), 0);
    }

private:
    void linkClicked(const QUrl&)
    {

    }

    void linkHovered(const QUrl&)
    {

    }

    QLabel *_label;
};

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
    //toolMenu->addSeparator();
    //toolMenu->addAction(tr("Add Property..."), this, [this]{ _propsPanel->addPropViaDlg(); });

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
    toggleEditMode(false);
}

IssueMemoTab::~IssueMemoTab()
{
    qDeleteAll(_comments);
    qDeleteAll(_events);
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
        { created , info.created },
        { updated , info.updated },
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
    _labelUpdated->setText(QLocale::system().toString(_memo->updated(), QLocale::ShortFormat));
    _issueInfo.created->setText(QLocale::system().toString(_memo->created(), QLocale::ShortFormat));
    _issueInfo.updated->setText(QLocale::system().toString(_memo->updated(), QLocale::ShortFormat));
    _issueInfo.station->setText(_memo->station());

    _events = Store::memos()->loadEvents(_memo->id());
    _comments = Store::memos()->loadSheets(_memo->id());

    // Events and comments are store in different tables
    // but should be displayed at the same list orderred by their date
    struct HistoryItem
    {
        QDateTime moment;
        std::variant<MemoEvent*, MemoSheet*> item;
    };
    QList<HistoryItem> history;
    for (auto event : std::as_const(_events))
        history << HistoryItem{ .moment = event->moment(), .item = event };
    for (auto comment : std::as_const(_comments))
        history << HistoryItem{ .moment = comment->created(), .item = comment };
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
        auto label = new QLabel(QLocale::system().toString(date, QLocale::ShortFormat));
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
            QString oldValue = change.first.isEmpty() ? u"(none)"_s : change.first;
            QString newValue = change.second.isEmpty() ? u"(none)"_s : change.second;
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

        propsChange.reset();
    };

    for (const auto& item : std::as_const(history))
    {
        if (std::holds_alternative<MemoEvent*>(item.item))
        {
            auto event = std::get<MemoEvent*>(item.item);
            if (!event->what().startsWith("prop:"_L1))
                continue;

            QString propName = event->what().split(':').last();
            QString oldValue = propValues.value(propName);
            QString newValue = event->value();
            propValues[propName] = newValue;

            if (!propsChange || event->moment() > propsChange->moment)
            {
                if (propsChange)
                    makePropChangeWidget();

                propsChange = PropChangeItem();
                propsChange->moment = event->moment();
            }

            propsChange->propValues.insert(propName, qMakePair(oldValue, newValue));
        }
        else if (std::holds_alternative<MemoSheet*>(item.item))
        {
            makePropChangeWidget();

            auto comment = std::get<MemoSheet*>(item.item);

            auto sheetView = new IssueMemoView;
            sheetView->setProperty("role", "issue_comment");
            sheetView->setHtml(MarkdownHelper::markdownToHtml(comment->data()));

            auto info = makePopupInfo();
            info.created->setText(QLocale::system().toString(comment->created(), QLocale::ShortFormat));
            info.updated->setText(QLocale::system().toString(comment->updated(), QLocale::ShortFormat));
            info.station->setText(comment->station());

            auto menu = new QMenu(sheetView);
            menu->addAction(info.action);

            auto header = new QFrame;
            Ori::Layouts::LayoutH({
                    makeNumLabel(),
                    Ori::Layouts::Stretch(),
                    makeDateLabel(comment->updated()),
                    TabHelpers::makeMenuButton(menu),
                }).setMargin(0).setSpacing(0).useFor(header);
            header->setProperty("role", "issue_event_header");
            _contentLayout->addWidget(header, 0, Qt::AlignTop);

            _contentLayout->addWidget(sheetView, 0, Qt::AlignTop);
            _commentViews << sheetView;
        }
    }
    makePropChangeWidget();

    _contentLayout->addStretch();

    QTimer::singleShot(0, this, &Self::updateViewHeights);
    
    setWindowTitle(_memo->title());
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
    _summaryView->setFixedHeight(_summaryView->document()->size().height() + maxBordersWidth);
    for (auto commentView : std::as_const(_commentViews))
        commentView->setFixedHeight(commentView->document()->size().height() + maxBordersWidth);
}

void IssueMemoTab::resizeEvent(QResizeEvent *e)
{
    MemoTab::resizeEvent(e);
    updateViewHeights();
}