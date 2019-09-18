#ifndef MARKDOWN_MEMO_EDITOR_H
#define MARKDOWN_MEMO_EDITOR_H

#include "MemoEditor.h"

QT_BEGIN_NAMESPACE
class QStackedLayout;
class QSyntaxHighlighter;
QT_END_NAMESPACE

class MarkdownMemoEditor : public TextMemoEditor
{
    Q_OBJECT

public:
    explicit MarkdownMemoEditor(MemoItem* memoItem, QWidget *parent = nullptr);

    void showMemo() override;
    void setFocus() override;
    void setFont(const QFont& f) override;
    bool isModified() const override;
    void setWordWrap(bool on) override;
    void beginEdit() override;
    void endEdit() override;

private:
    QTextEdit* _view;
    QStackedLayout* _tabs;
};

#endif // MARKDOWN_MEMO_EDITOR_H
