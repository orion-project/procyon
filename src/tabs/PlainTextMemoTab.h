#ifndef PLAIN_TEXT_MEMO_TAB_H
#define PLAIN_TEXT_MEMO_TAB_H

#include "MemoTab.h"

QT_BEGIN_NAMESPACE
class QAction;
class QLineEdit;
class QStackedLayout;
QT_END_NAMESPACE

class MemoTextEdit;
class MemoPropsPanel;

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

    void showMemo();
    void toggleEditMode(bool on);
    void cancelEdit();
    bool saveEdit();
    void chooseFont();
    void toggleWordWrap();
};

#endif // PLAIN_TEXT_MEMO_TAB_H