#ifndef ISSUE_MEMO_TAB_H
#define ISSUE_MEMO_TAB_H

#include "MemoTab.h"

#include <QDateTime>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QLineEdit;
class QScrollArea;
class QToolBar;
class QVBoxLayout;
QT_END_NAMESPACE

class Folder;
class MemoEvent;
class MemoSheet;
class MemoPropsPanel;
class MemoTextEdit;
class IssueTextBrowser;
class IssueEditDlg;
class IssueCommentDlg;

class IssueMemoTab : public MemoTab
{
public:
    explicit IssueMemoTab(Enot* enot, Memo* memo);

    static void createIssue(Enot* enot, Folder* folder);

    QString tabTitle() const override;

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    MemoPropsPanel* _propsPanel;
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QScrollArea *_contentScroller;
    QVBoxLayout *_contentLayout;
    IssueTextBrowser *_summaryView;
    QSet<QDateTime> _shownEvents;
    QLabel *_labelUpdated;
    QList<QLabel*> _eventNumLabels;
    QAction *_actionSolveIssue = nullptr;
    QAction *_actionCloseIssue = nullptr;
    QAction *_actionReopenIssue = nullptr;

    struct PopupInfo
    {
        QAction *action;
        QLabel *created;
        QLabel *updated;
        QLabel *station;
    };
    PopupInfo _issueInfo;
    QPointer<IssueEditDlg> _editDlg;
    QPointer<IssueCommentDlg> _commentDlg;

    struct CommentData
    {
        QString sourceText;
        QDateTime updated;
        IssueTextBrowser* textView;
        QLabel *labelUpdated;
        PopupInfo popupInfo;
    };
    QHash<int, CommentData> _commentViews;

    struct LinksData
    {
        QWidget* blockHeader = nullptr;
        QWidget* blockBody = nullptr;
        QHash<int, QWidget*> linkRows;
    };
    LinksData _linksData;

    void showMemo();
    void showLinks();
    void hideLinks();
    void showHistory();

    void updateSummaryHeight();
    void updateCommentHeights();

    // TODO: make configurable or scriptable
    static inline const auto& propStatus = QStringLiteral("Status");
    static inline const auto& propStatusOpened = QStringLiteral("Opened");
    static inline const auto& propStatusSolved = QStringLiteral("Solved");
    static inline const auto& propStatusClosed = QStringLiteral("Closed");

    void editIssue();
    void addComment(const QString& newStatus = {});
    void commentIssue() { addComment(); }
    void solveIssue() { addComment(propStatusSolved); }
    void closeIssue() { addComment(propStatusClosed); }
    void reopenIssue() { addComment(propStatusOpened); }
    void editComment(int id);
    void copySummary();
    void addRelation();

    bool canSolveIssue() const;
    bool canCloseIssue() const;
    bool canReopenIssue() const;

    void memoLinkCreated(int id1, int id2);
    void memoLinkDeleted(int id1, int id2);

    PopupInfo makePopupInfo(int id);
};

#endif // ISSUE_MEMO_TAB_H