#ifndef GRID_VIEW_MEMO_TAB_H
#define GRID_VIEW_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QMenu;
class QTableView;
class QToolBar;
QT_END_NAMESPACE

class DbItem;
class GridViewTableModel;

class GridViewMemoTab : public MemoTab
{
    Q_OBJECT

public:
    explicit GridViewMemoTab(Enot* enot, MemoItem* memoItem);

    void beginEdit() override;

signals:
    void memoOpenRequested(MemoItem* item);

private:
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QTableView *_tableView;
    GridViewTableModel *_tableModel;
    QMenu *_contextMenu;

    void showMemo();
    void cancelEdit();
    bool saveEdit();
    void toggleEditMode(bool on);
    void createMemo();
    void openSelectedMemo();
    void showContextMenu(const QPoint& pos);

    DbItem* selectedItem() const;
};

#endif // GRID_VIEW_MEMO_TAB_H