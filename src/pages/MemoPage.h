#ifndef MEMO_PAGE_H
#define MEMO_PAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QSyntaxHighlighter;
QT_END_NAMESPACE

class Catalog;
class MemoEditor;
class MemoItem;
class TextEditSpellcheck;

class MemoPage : public QWidget
{
    Q_OBJECT

public:
    explicit MemoPage(Catalog* catalog, MemoItem* memoItem);
    ~MemoPage();

    MemoItem* memoItem() const { return _memoItem; }

    void setMemoFont(const QFont& font);
    void setWordWrap(bool wrap);

    void setSpellcheckLang(const QString& lang);
    QString spellcheckLang() const { return _spellcheckLang; }

    void beginEditing();
    bool saveEditing();
    bool isModified() const;
    bool isReadOnly() const;
    bool canClose();

signals:
    bool onAboutToBeClosed();
    void onReadOnly(bool readOnly);
    void onModified(bool modified);

private:
    Catalog* _catalog;
    MemoItem* _memoItem;
    MemoEditor* _memoEditor;
    QLineEdit* _titleEditor;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QSyntaxHighlighter* _highlighter = nullptr;
    TextEditSpellcheck* _spellcheck = nullptr;
    QString _spellcheckLang;

    void showMemo();
    void cancelEditing();
    void toggleEditMode(bool on);
    void toggleSpellcheck(bool on);
    void applyHighlighter();
};

#endif // MEMO_PAGE_H
