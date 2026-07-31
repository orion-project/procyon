#ifndef GRID_VIEW_MEMO_TAB_H
#define GRID_VIEW_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QToolBar;
QT_END_NAMESPACE

class GridViewMemoTab : public MemoTab
{
public:
    explicit GridViewMemoTab(Db* db, MemoItem* memoItem);

    void beginEdit() override;

private:
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QAction *_actionEdit, *_actionSave, *_actionCancel;

    void showMemo();
    void cancelEdit();
    bool saveEdit();
    void toggleEditMode(bool on);
    void createMemo();
};

#endif // GRID_VIEW_MEMO_TAB_H