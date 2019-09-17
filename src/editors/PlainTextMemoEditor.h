#ifndef PLAIN_TEXT_MEMO_EDITOR_H
#define PLAIN_TEXT_MEMO_EDITOR_H

#include "MemoEditor.h"

QT_BEGIN_NAMESPACE
class QSyntaxHighlighter;
QT_END_NAMESPACE

class PlainTextMemoEditor : public TextMemoEditor
{
    Q_OBJECT

public:
    explicit PlainTextMemoEditor(MemoItem* memoItem, QWidget *parent = nullptr);

    void setReadOnly(bool on) override;
    void showMemo(MemoItem*) override;

private:
    QSyntaxHighlighter* _highlighter = nullptr;

    void applyHighlighter();
};

#endif // PLAIN_TEXT_MEMO_EDITOR_H
