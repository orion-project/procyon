#ifndef PLAIN_TEXT_MEMO_TAB_H
#define PLAIN_TEXT_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QMenu;
class QStackedLayout;
class QSyntaxHighlighter;
QT_END_NAMESPACE

class MemoTextEdit;
class MemoPropsPanel;
namespace Ori {
class Spellcheck;
}

class PlainTextMemoTab : public MemoTab
{
    Q_OBJECT

public:
    explicit PlainTextMemoTab(Enot* enot, Memo* memo);

    void loadSettings() override;
    bool canClose() override;
    void beginEdit() override;
    bool isReadOnly() const override;
    bool isModified() const override;

private:
    QAction *_actionEdit, *_actionSave, *_actionCancel;
    QLineEdit *_titleEditor;
    MemoTextEdit *_textEditor;
    MemoPropsPanel* _propsPanel;
    QMenu *_highlighterMenu, *_spellcheckMenu;
    QSyntaxHighlighter* _highlighter = nullptr;
    Ori::Spellcheck* _spellcheck;
    QString _spellcheckLang;

    void showMemo();
    void toggleEditMode(bool on);
    void cancelEdit();
    bool saveEdit();
    void chooseFont();
    void toggleWordWrap();
    void setHighlighterName(const QString& name);
    void showSelectedHighlighter();
    void showSelectedSpellcheckLang();
};

#endif // PLAIN_TEXT_MEMO_TAB_H