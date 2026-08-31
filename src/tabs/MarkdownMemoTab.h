#ifndef MARKDOWN_MEMO_TAB_H
#define MARKDOWN_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QMenu;
class QStackedLayout;
QT_END_NAMESPACE

class MemoTextEdit;
class MemoTextBrowser;
class MemoPropsPanel;
namespace Ori {
class Spellcheck;
}

class MarkdownMemoTab : public MemoTab
{
    Q_OBJECT

public:
    explicit MarkdownMemoTab(Enot* enot, Memo* memo);

    bool canClose() override;
    void beginEdit() override;
    bool isReadOnly() const override;
    bool isModified() const override;

private:
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QAction *_actionModeEdit, *_actionModePreview, *_actionModeSeparator, *_actionModeToggle;
    QLineEdit *_titleEditor;
    MemoTextEdit *_textEditor = nullptr;
    MemoTextBrowser *_textView;
    QStackedLayout *_tabs;
    MemoPropsPanel* _propsPanel;
    QMenu *_spellcheckMenu;
    Ori::Spellcheck* _spellcheck = nullptr;
    QString _spellcheckLang;

    void showMemo();
    void togglePreview();
    void toggleEditMode(bool on);
    void cancelEdit();
    bool saveEdit();
    void chooseFont();
    void toggleWordWrap();
    void showSelectedSpellcheckLang();
};

#endif // MARKDOWN_MEMO_TAB_H