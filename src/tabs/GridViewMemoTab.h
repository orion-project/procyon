#ifndef GRID_VIEW_MEMO_TAB_H
#define GRID_VIEW_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QMenu;
class QTableView;
class QToolBar;
class QSortFilterProxyModel;
QT_END_NAMESPACE

class Entry;
class GridViewTableModel;

class GridViewMemoTab : public MemoTab
{
    Q_OBJECT

public:
    explicit GridViewMemoTab(Enot* enot, Memo* memo);

    void beginEdit() override;

signals:
    void memoOpenRequested(Memo* item);

private:
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QTableView *_tableView;
    GridViewTableModel *_tableModel;
    QSortFilterProxyModel *_proxyModel;
    QMenu *_contextMenu, *_toolMenu;

    void showMemo();
    void cancelEdit();
    bool saveEdit();
    void toggleEditMode(bool on);
    void createMemo();
    void openSelectedMemo();
    void showContextMenu(const QPoint& pos);
    void chooseColumns();

    Entry* selectedEntry() const;

    void applyColumns(const QStringList& propNames);
    void saveSortMode();
};

#endif // GRID_VIEW_MEMO_TAB_H