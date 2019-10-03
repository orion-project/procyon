#include "PlainTextMemoEditor.h"

#include "../catalog/Catalog.h"
#include "../highlighter/HighlighterManager.h"
#include "../widgets/MemoTextEdit.h"

#include "helpers/OriLayouts.h"

#include <QStyle>
#include <QSyntaxHighlighter>
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

QString PlainTextMemoEditor::highlighterName() const
{
    return _highlighter ? _highlighter->objectName() : QString();
}

void PlainTextMemoEditor::setHighlighterName(const QString& name)
{
    if (!_highlighter && name.isEmpty()) return;
    if (_highlighter && _highlighter->objectName() == name) return;

    _editor->setUndoRedoEnabled(false);

    if (_highlighter) delete _highlighter;

    _highlighter = HighlighterManager::instance().makeHighlighter(name, _editor->document());

    _editor->setUndoRedoEnabled(true);
}
