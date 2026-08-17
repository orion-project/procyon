#ifndef TEXT_MEMO_TAB_H
#define TEXT_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QSyntaxHighlighter;
class QToolBar;
class QToolButton;
QT_END_NAMESPACE

class MemoEditor;

class TextMemoTab : public MemoTab
{
public:
    explicit TextMemoTab(Enot* enot, Memo* memo);

    QFont memoFont() const;
    void setMemoFont(const QFont& font);

    bool wordWrap() const;
    void setWordWrap(bool wrap);

    void setSpellcheckLang(const QString& lang);
    QString spellcheckLang() const;

    void setHighlighter(const QString& name);
    QString highlighter() const;

    void exportToPdf();

    void loadSettings() override;
    bool canClose() override;
    void beginEdit() override;
    bool isReadOnly() const override { return !_isEditMode; }
    bool isModified() const override;

private:
    MemoEditor* _memoEditor;
    QLineEdit* _titleEditor;
    QToolBar* _toolbar;
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QAction *_actionPreview = nullptr, *_actionPreviewButton, *_separatorPreview;
    QToolButton *_previewButton;
    bool _isEditMode = false;

    void showMemo();
    void cancelEdit();
    bool saveEdit();

    void toggleEditMode(bool on);
    void togglePreviewMode();
};

#endif // TEXT_MEMO_TAB_H