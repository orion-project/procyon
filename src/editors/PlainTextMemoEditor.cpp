#include "PlainTextMemoEditor.h"

#include "MemoTextEdit.h"
#include "../catalog/Catalog.h"
#include "../highlighter/PythonSyntaxHighlighter.h"
#include "../highlighter/ShellMemoSyntaxHighlighter.h"

#include "helpers/OriLayouts.h"

#include <QStyle>
#include <QTimer>

PlainTextMemoEditor::PlainTextMemoEditor(MemoItem *memoItem, QWidget *parent) : TextMemoEditor(memoItem, parent)
{
    setEditor(new MemoTextEdit);
    _editor->setReadOnly(true);

    Ori::Layouts::LayoutV({_editor}).setMargin(0).useFor(this);

    QTimer::singleShot(0, [this](){
        auto sb = 1.5 * style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        _editor->document()->setTextWidth(_editor->width() - sb);
    });
}

void PlainTextMemoEditor::showMemo()
{
    _editor->setPlainText(_memoItem->data());
    _editor->document()->setModified(false);
}

void PlainTextMemoEditor::applyHighlighter()
{
    _editor->setUndoRedoEnabled(false);

    // TODO preserve highlighter if its type is not changed
    if (_highlighter)
    {
        delete _highlighter;
        _highlighter = nullptr;
    }

    auto text = _memoItem->data();

    // TODO highlighter should be selected by user and saved into catalog
    if (text.startsWith("#!/usr/bin/env python"))
        _highlighter = new PythonSyntaxHighlighter(_editor->document());
    else if (text.startsWith("#shell-memo"))
        _highlighter = new ShellMemoSyntaxHighlighter(_editor->document());

    _editor->setUndoRedoEnabled(true);
}
