#ifndef DISPERSIONPLOT_H
#define DISPERSIONPLOT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
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

private:
    Catalog* _catalog;
    MemoItem* _memoItem;
    QTextEdit* _memoEditor;
    QLineEdit* _titleEditor;
    QPushButton *_buttonEdit, *_buttonSave, *_buttonCancel;

    void memoRemoved(MemoItem* item);
    void showMemo();
    void beginEditing();
    void cancelEditing();
    void saveEditing();
    void toggleEditMode(bool on);
};

#endif // DISPERSIONPLOT_H
