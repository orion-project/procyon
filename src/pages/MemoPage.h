#ifndef MEMO_PAGE_H
#define MEMO_PAGE_H

#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QSyntaxHighlighter;
QT_END_NAMESPACE

class Catalog;
class MemoItem;
class SpellChecker;

class MemoEditor : public QTextEdit
{
protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
private:
    QString _clickedAnchor;
    bool shouldProcess(QMouseEvent *e);
};


class MemoPage : public QWidget
{
    Q_OBJECT

public:
    explicit MemoPage(Catalog* catalog, MemoItem* memoItem);
    ~MemoPage();

    MemoItem* memoItem() const { return _memoItem; }

    void setMemoFont(const QFont& font);
    void setWordWrap(bool wrap);

    void beginEditing();
    bool saveEditing();
    bool isModified() const;
    bool canClose();

signals:
    bool onAboutToBeClosed();

private:
    Catalog* _catalog;
    MemoItem* _memoItem;
    QTextEdit* _memoEditor;
    QLineEdit* _titleEditor;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QSyntaxHighlighter* _highlighter = nullptr;
    SpellChecker* _spellChecker = nullptr;

    void showMemo();
    void cancelEditing();
    void toggleEditMode(bool on);
    void applyTextStyles();
    void processHyperlinks();
    void applyHighlighter();
};

#endif // MEMO_PAGE_H
