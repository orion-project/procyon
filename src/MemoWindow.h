#ifndef DISPERSIONPLOT_H
#define DISPERSIONPLOT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QTextEdit;
class QPushButton;
QT_END_NAMESPACE

class Catalog;
class MemoItem;

class MemoWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MemoWindow(Catalog* catalog, MemoItem* memoItem);
    ~MemoWindow();

    MemoItem* memoItem() const { return _memoItem; }

private:
    Catalog* _catalog;
    MemoItem* _memoItem;
    QTextEdit* _memoEditor;
    QLineEdit* _titleEditor;
    QAction *_actionEdit, *_actionSave, *_actionCancel;

    void memoRemoved(MemoItem* item);
    void showMemo();
    void beginEditing();
    void cancelEditing();
    void saveEditing();
    void toggleEditMode(bool on);
};

#endif // DISPERSIONPLOT_H
