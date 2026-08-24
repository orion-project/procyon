#ifndef ISSUE_MEMO_TAB_H
#define ISSUE_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QScrollArea;
class QToolBar;
class QVBoxLayout;
QT_END_NAMESPACE

class MemoSheet;
class MemoPropsPanel;
class IssueMemoView;

class IssueMemoTab : public MemoTab
{
public:
    explicit IssueMemoTab(Enot* enot, Memo* memo);
    ~IssueMemoTab() override;

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
    QList<IssueMemoView*> _commentViews;
    QList<MemoSheet*> _comments;
    QLabel *_labelUpdated;

    struct PopupInfo
    {
        QAction *action;
        QLabel *created;
        QLabel *updated;
        QLabel *station;
    };
    PopupInfo _issueInfo;

    void showMemo();
    void cancelEdit();
    bool saveEdit();
    void toggleEditMode(bool on);

    void updateViewHeights();

    PopupInfo makePopupInfo();
};

#endif // ISSUE_MEMO_TAB_H