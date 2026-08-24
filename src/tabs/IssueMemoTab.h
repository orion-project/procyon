#ifndef ISSUE_MEMO_TAB_H
#define ISSUE_MEMO_TAB_H

#include "MemoTab.h"

#include <QDateTime>

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QScrollArea;
class QToolBar;
class QVBoxLayout;
QT_END_NAMESPACE

class MemoEvent;
class MemoSheet;
class MemoPropsPanel;
class IssueMemoView;

class IssueMemoTab : public MemoTab
{
public:
    explicit IssueMemoTab(Enot* enot, Memo* memo);

    void beginEdit() override;

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    MemoPropsPanel* _propsPanel;
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QScrollArea *_contentScroller;
    QVBoxLayout *_contentLayout;
    IssueMemoView *_summaryView;
    QSet<QDateTime> _shownEvents;
    QLabel *_labelUpdated;

    struct PopupInfo
    {
        QAction *action;
        QLabel *created;
        QLabel *updated;
        QLabel *station;
    };
    PopupInfo _issueInfo;

    struct CommentData
    {
        QString sourceText;
        QDateTime updated;
        IssueMemoView* textView;
        QLabel *labelUpdated;
        PopupInfo popupInfo;
    };
    QHash<int, CommentData> _commentViews;

    void showMemo();
    void showHistory();
    void cancelEdit();
    bool saveEdit();
    void toggleEditMode(bool on);

    void updateViewHeights();

    void editComment(int id);

    PopupInfo makePopupInfo();
};

#endif // ISSUE_MEMO_TAB_H