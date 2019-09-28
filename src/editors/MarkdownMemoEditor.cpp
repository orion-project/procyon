#include "MarkdownMemoEditor.h"

#include "MarkdownHelper.h"
#include "MemoTextBrowser.h"
#include "MemoTextEdit.h"
#include "../AppSettings.h"
#include "../catalog/Catalog.h"

#include "helpers/OriLayouts.h"

#include <QStackedLayout>

MarkdownMemoEditor::MarkdownMemoEditor(MemoItem* memoItem, QWidget *parent) : TextMemoEditor(memoItem, parent)
{
    _view = new MemoTextBrowser;
    _view->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
    _view->document()->setDocumentMargin(10);

    _tabs = new QStackedLayout;

    Ori::Layouts::LayoutV({_tabs}).setMargin(0).useFor(this);

    // This should be after setting of editor's layout.
    // Otherwise, the _view splashes for a short time as a popup window.
    _tabs->addWidget(_view);
    _tabs->setCurrentWidget(_view);

    AppSettings::instance().registerListener(this);
}

MarkdownMemoEditor::~MarkdownMemoEditor()
{
    AppSettings::instance().unregisterListener(this);
}

void MarkdownMemoEditor::showMemo()
{
    _view->setHtml(MarkdownHelper::markdownToHtml(_memoItem->data()));
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
            _view->setHtml(MarkdownHelper::markdownToHtml(_editor->toPlainText()));
        _tabs->setCurrentWidget(_view);
    }
    else
    {
        if (_editor)
            _tabs->setCurrentWidget(_editor);
    }
}

void MarkdownMemoEditor::optionChanged(AppSettingsOption option)
{
    if (option != AppSettingsOption::MARKDOWN_CSS) return;
    _view->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
    _view->setHtml(MarkdownHelper::markdownToHtml(_editor ? _editor->toPlainText() : _memoItem->data()));
}
