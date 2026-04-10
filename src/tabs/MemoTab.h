#ifndef MEMO_TAB_H
#define MEMO_TAB_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QSyntaxHighlighter;
class QToolBar;
class QToolButton;
QT_END_NAMESPACE

class Catalog;
class MemoEditor;
class MemoItem;

class MemoTab : public QWidget
{
    Q_OBJECT

public:
    explicit MemoTab(Catalog* catalog, MemoItem* memoItem);
    ~MemoTab();

    MemoItem* memoItem() const { return _memoItem; }

    void loadSettings();

    QFont memoFont() const;
    void setMemoFont(const QFont& font);

    bool wordWrap() const;
    void setWordWrap(bool wrap);

    void setSpellcheckLang(const QString& lang);
    QString spellcheckLang() const;

    void setHighlighter(const QString& name);
    QString highlighter() const;

    void beginEdit();
    bool saveEdit();
    bool isModified() const;
    bool isReadOnly() const { return !_isEditMode; }
    bool canClose();

    void exportToPdf();

signals:
    bool onAboutToBeClosed();
    void onReadOnly(bool readOnly);
    void onModified(bool modified);

private:
    Catalog* _catalog;
    MemoItem* _memoItem;
    MemoEditor* _memoEditor;
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QAction *_actionPreview = nullptr, *_actionPreviewButton, *_separatorPreview;
    QToolButton *_previewButton;
    bool _isEditMode = false;

    void showMemo();
    void cancelEdit();
    void toggleEditMode(bool on);
    void togglePreviewMode();
};

#endif // MEMO_TAB_H
