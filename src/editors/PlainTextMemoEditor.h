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

    void showMemo() override;

    QString highlighterName() const;
    void setHighlighterName(const QString& name);

private:
    QSyntaxHighlighter* _highlighter = nullptr;
};

#endif // PLAIN_TEXT_MEMO_EDITOR_H
