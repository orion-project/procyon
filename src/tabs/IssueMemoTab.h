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

    void showMemo();
    void showHistory();
    // void cancelEdit();
    // bool saveEdit();
    // void toggleEditMode(bool on);

    void updateSummaryHeight();
    void updateCommentHeights();

    void editIssue();
    void addComment();
    void editComment(int id);
    void copySummary();

    PopupInfo makePopupInfo(int id);
};

#endif // ISSUE_MEMO_TAB_H