#ifndef ISSUE_MEMO_TAB_H
#define ISSUE_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QToolBar;
QT_END_NAMESPACE

class MemoPropsPanel;

class IssueMemoTab : public MemoTab
{
public:
    explicit IssueMemoTab(Enot* enot, Memo* memo);

    void beginEdit() override;

private:
    MemoPropsPanel* _propsPanel;
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QAction *_actionEdit, *_actionSave, *_actionCancel;

    void showMemo();
    void cancelEdit();
    bool saveEdit();
    void toggleEditMode(bool on);
};

#endif // ISSUE_MEMO_TAB_H