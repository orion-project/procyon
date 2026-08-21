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
#include <QResizeEvent>
#include <QScrollArea>
#include <QTextBrowser>
#include <QTextDocument>
#include <QToolBar>
#include <QTimer>

//------------------------------------------------------------------------------
//                             IssueMemoTab
//------------------------------------------------------------------------------

class IssueMemoView : public QTextBrowser
{
public:
    explicit IssueMemoView(QWidget *parent = 0) : QTextBrowser()
    {
        //_label = new QLabel("hello", this);
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

    _propsPanel = new MemoPropsPanel(enot);
    _propsPanel->setVisible(false);

    _summaryView = new IssueMemoView;
    _summaryView->setObjectName("issue_summary");
    //_summaryView->setProperty("role", "issue_text");
    _summaryView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _summaryView->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());

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

void IssueMemoTab::showMemo()
{
    _titleEditor->setText(_memo->title());
    _summaryView->setHtml(MarkdownHelper::markdownToHtml(_memo->data()));
    _summaryView->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed));

    int num = 1;
    auto comments = Store::memos()->loadSheets(_memo->id());
    for (const auto &comment : std::as_const(comments))
    {
        auto label = new QLabel(QString::number(num));

        auto header = new QFrame;
        Ori::Layouts::LayoutH({
            label,
            Ori::Layouts::Stretch(),
        }).setMargin(0).setSpacing(0).useFor(header);
        header->setProperty("role", "event_header");
        _contentLayout->addWidget(header, 0, Qt::AlignTop);

        auto sheetView = new IssueMemoView;
        sheetView->setProperty("role", "issue_text");
        sheetView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        sheetView->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
        sheetView->setHtml(MarkdownHelper::markdownToHtml(comment));
        _contentLayout->addWidget(sheetView, 0, Qt::AlignTop);
        _commentViews << sheetView;

        num++;
    }

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