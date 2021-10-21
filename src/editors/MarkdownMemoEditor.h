#ifndef MARKDOWN_MEMO_EDITOR_H
#define MARKDOWN_MEMO_EDITOR_H

#include "MemoEditor.h"
#include "../AppSettings.h"

QT_BEGIN_NAMESPACE
class QStackedLayout;
class QSyntaxHighlighter;
class QTextBrowser;
QT_END_NAMESPACE

class MarkdownMemoEditor : public TextMemoEditor, public AppSettingsListener
{
    Q_OBJECT

public:
    explicit MarkdownMemoEditor(MemoItem* memoItem);
    ~MarkdownMemoEditor() override;

    void showMemo() override;
    void setFocus() override;
    QFont font() const override;
    void setFont(const QFont& f) override;
    bool isModified() const override;
    bool wordWrap() const override;
    void setWordWrap(bool on) override;
    void beginEdit() override;
    void endEdit() override;
    void saveEdit() override;
    void exportToPdf(const QString& fileName) override;

    bool isPreviewMode() const;
    void togglePreviewMode(bool on);

    void optionChanged(AppSettingsOption option) override;

private:
    QTextBrowser* _view;
    QStackedLayout* _tabs;
    QFont _memoFont;
    bool _wordWrap = false;
};

#endif // MARKDOWN_MEMO_EDITOR_H
