#include "MarkdownMemoEditor.h"

#include "MemoTextEdit.h"
#include "ori_html.h"
#include "../catalog/Catalog.h"

#include "helpers/OriLayouts.h"

#include <QStackedLayout>
#include <QTextEdit>

static QString markdownToHtml(const QString& markdown)
{
    auto markdownBytes = markdown.toUtf8();

    hoedown_extensions extensions = hoedown_extensions(HOEDOWN_EXT_BLOCK | HOEDOWN_EXT_SPAN);
    hoedown_renderer* renderer = hoedown_html_renderer_new_ori();
    hoedown_document* document = hoedown_document_new(renderer, extensions, 16);
    hoedown_buffer* in_buf = hoedown_buffer_new(1024);
    hoedown_buffer_set(in_buf, reinterpret_cast<const uint8_t*>(markdownBytes.data()), static_cast<size_t>(markdownBytes.size()));
    hoedown_buffer* out_buf = hoedown_buffer_new(64);
    hoedown_document_render(document, out_buf, in_buf->data, in_buf->size);
    hoedown_buffer_free(in_buf);
    hoedown_document_free(document);
    hoedown_html_renderer_free_ori(renderer);

    QByteArray htmlBytes(reinterpret_cast<char*>(out_buf->data), static_cast<int>(out_buf->size));
    QString html = QStringLiteral("<html></body>\n") + QString::fromUtf8(htmlBytes) + QStringLiteral("\n</body></html>");

    hoedown_buffer_free(out_buf);

    return html;
}

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
    _view->setHtml(markdownToHtml(_memoItem->data()));
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

void MarkdownMemoEditor::saveEdit()
{
    // If we are in preview mode, _view already contains the latest html
    if (_tabs->currentWidget() == _editor)
        showMemo();

    endEdit();
}

bool MarkdownMemoEditor::isPreviewMode() const
{
    return _tabs->currentWidget() == _view;
}

void MarkdownMemoEditor::togglePreviewMode(bool on)
{
    if (on)
    {
        if (_editor)
            _view->setHtml(markdownToHtml(_editor->toPlainText()));
        _tabs->setCurrentWidget(_view);
    }
    else
    {
        if (_editor)
            _tabs->setCurrentWidget(_editor);
    }
}
