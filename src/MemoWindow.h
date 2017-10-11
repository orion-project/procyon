#ifndef DISPERSIONPLOT_H
#define DISPERSIONPLOT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QTextEdit;
class QSyntaxHighlighter;
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

    void setMemoFont(const QFont& font);
    void setTitleFont(const QFont& font);
    void setWordWrap(bool wrap);

    void beginEditing();

private:
    Catalog* _catalog;
    MemoItem* _memoItem;
    QTextEdit* _memoEditor;
    QLineEdit* _titleEditor;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QSyntaxHighlighter* _highlighter = nullptr;

    void showMemo();
    void cancelEditing();
    void saveEditing();
    void toggleEditMode(bool on);
    void applyTextStyles();
    void processHyperlinks();
    void applyHighlighter();
};

#endif // DISPERSIONPLOT_H
