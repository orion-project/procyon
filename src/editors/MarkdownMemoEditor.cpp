#include "MarkdownMemoEditor.h"

#include "MemoTextEdit.h"
#include "../catalog/Catalog.h"

#include "helpers/OriLayouts.h"

#include <QStackedLayout>
#include <QTextEdit>

MarkdownMemoEditor::MarkdownMemoEditor(MemoItem* memoItem, QWidget *parent) : TextMemoEditor(memoItem, parent)
{
    _view = new QTextEdit;
    _view->setReadOnly(true);
    _view->setWordWrapMode(QTextOption::NoWrap);
    _view->setProperty("role", "memo_editor");

    QFile file(":/style/markdown");
    file.open(QIODevice::ReadOnly);
    _view->document()->setDefaultStyleSheet(file.readAll());
    _view->document()->setDocumentMargin(10);

    _tabs = new QStackedLayout;

    Ori::Layouts::LayoutV({_tabs}).setMargin(0).useFor(this);

    // This should be after setting of editor's layout.
    // Otherwise, the _view splashes for a short time as a popup window.
    _tabs->addWidget(_view);
    _tabs->setCurrentWidget(_view);
}

void MarkdownMemoEditor::showMemo()
{
    // TODO: convert markdown to html
    _view->setHtml("<html><body>Markdown should be here</body></html>");
}

void MarkdownMemoEditor::setFocus()
{
    if (_tabs->currentWidget() == _view)
        _view->setFocus();
    else if (_editor)
        _editor->setFocus();
}

void MarkdownMemoEditor::setFont(const QFont& f)
{
    _view->setFont(f);
    if (_editor)
        TextMemoEditor::setFont(f);
}

bool MarkdownMemoEditor::isModified() const
{
    return _editor && TextMemoEditor::isModified();
}

void MarkdownMemoEditor::setWordWrap(bool on)
{
    _view->setWordWrapMode(on ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    if (_editor)
        TextMemoEditor::setWordWrap(on);
}

void MarkdownMemoEditor::beginEdit()
{
    if (!_editor)
    {
        setEditor(new MemoTextEdit);
        _editor->setFont(_view->font());
        _editor->setWordWrapMode(_view->wordWrapMode());
        // TODO: set highlighter
        _editor->setPlainText(_memoItem->data());
        _tabs->addWidget(_editor);
    }
    _tabs->setCurrentWidget(_editor);
    TextMemoEditor::beginEdit();
}

void MarkdownMemoEditor::endEdit()
{
    _tabs->setCurrentWidget(_view);
    TextMemoEditor::endEdit();
    _view->setFocus();
}
