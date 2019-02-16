#ifndef MEMOWINDOW_H
#define MEMOWINDOW_H

#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QSyntaxHighlighter;
QT_END_NAMESPACE

class Catalog;
class MemoItem;

class MemoEditor : public QTextEdit
{
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
private:
    QString _clickedAnchor;
    bool shouldProcess(QMouseEvent *e);
};


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
    bool saveEditing();
    bool isModified() const;

private:
    Catalog* _catalog;
    MemoItem* _memoItem;
    QTextEdit* _memoEditor;
    QLineEdit* _titleEditor;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QSyntaxHighlighter* _highlighter = nullptr;

    void showMemo();
    void cancelEditing();
    void toggleEditMode(bool on);
    void applyTextStyles();
    void processHyperlinks();
    void applyHighlighter();
};

#endif // MEMOWINDOW_H
