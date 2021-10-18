#include "MemoEditor.h"

#include "../catalog/Catalog.h"
#include "../highlighter/OriHighlighter.h"
#include "../spellcheck/TextEditSpellcheck.h"
#include "../spellcheck/Spellchecker.h"
#include "../widgets/MemoTextEdit.h"
#include "../TextEditHelpers.h"
#include "orion/helpers/OriLayouts.h"

#include <QStyle>
#include <QTimer>

//------------------------------------------------------------------------------
//                                 MemoEditor
//------------------------------------------------------------------------------

MemoEditor::MemoEditor(MemoItem *memoItem) : QWidget(), _memoItem(memoItem)
{
}

//------------------------------------------------------------------------------
//                                TextMemoEditor
//------------------------------------------------------------------------------

TextMemoEditor::TextMemoEditor(MemoItem* memoItem) : TextMemoEditor(memoItem, true)
{
}

TextMemoEditor::TextMemoEditor(MemoItem* memoItem, bool createEditor) : MemoEditor(memoItem)
{
    if (!createEditor) return;

    setEditor(new MemoTextEdit);
    _editor->setReadOnly(true);

    Ori::Layouts::LayoutV({_editor}).setMargin(0).useFor(this);

    QTimer::singleShot(0, this, [this](){
        auto sb = 1.5 * style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        _editor->document()->setTextWidth(_editor->width() - sb);
    });
}

void TextMemoEditor::setEditor(MemoTextEdit *editor)
{
    _editor = editor;
    connect(_editor, &MemoTextEdit::undoAvailable, this, &MemoEditor::onModified);
}

void TextMemoEditor::setFocus()
{
    _editor->setFocus();
}

QFont TextMemoEditor::font() const
{
    return _editor->font();
}

void TextMemoEditor::setFont(const QFont& f)
{
    _editor->setFont(f);

    // TODO: Some styles get reset when font changes at least on macOS
    // e.g. bold header in shell-memo becomes normal
}

bool TextMemoEditor::isModified() const
{
    return _editor->document()->isModified();
}

void TextMemoEditor::setModified(bool on)
{
    _editor->document()->setModified(on);
}

bool TextMemoEditor::wordWrap() const
{
    return _editor->wordWrap();
}

void TextMemoEditor::setWordWrap(bool on)
{
    _editor->setWordWrap(on);
}

QString TextMemoEditor::data() const
{
    return _editor->toPlainText();
}

void TextMemoEditor::toggleSpellcheck(bool on)
{
#ifdef ENABLE_SPELLCHECK
    if (on)
    {
        if (!_spellcheckLang.isEmpty())
        {
            auto spellchecker = Spellchecker::get(_spellcheckLang);
            if (!spellchecker) return; // Unable to open dictionary
            _spellcheck = new TextEditSpellcheck(_editor, spellchecker, this);
            _spellcheck->spellcheckAll();
        }
    }
    else
    {
        if (_spellcheck)
        {
            _spellcheck->clearErrorMarks();
            delete _spellcheck;
            _spellcheck = nullptr;
        }
    }
#else
    Q_UNUSED(on)
#endif
}

void TextMemoEditor::setSpellcheckLang(const QString &lang)
{
    toggleSpellcheck(false);
    _spellcheckLang = lang;
    if (_editor && !_editor->isReadOnly())
        toggleSpellcheck(true);
}

void TextMemoEditor::beginEdit()
{
    setReadOnly(false);
    toggleSpellcheck(true);
    _editor->setFocus();
}

void TextMemoEditor::endEdit()
{
    setReadOnly(true);
    toggleSpellcheck(false);
    _editor->document()->setModified(false);
}

void TextMemoEditor::setReadOnly(bool on)
{
    _editor->setReadOnly(on);
    Qt::TextInteractionFlags flags = Qt::LinksAccessibleByMouse |
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard;
    if (!on) flags |= Qt::TextEditable;
    _editor->setTextInteractionFlags(flags);
}

void TextMemoEditor::exportToPdf(const QString& fileName)
{
    TextEditHelpers::exportToPdf(_editor->document(), fileName);
}

void TextMemoEditor::showMemo()
{
    _editor->setPlainText(_memoItem->data());
    _editor->document()->setModified(false);
}

QString TextMemoEditor::highlighterName() const
{
    return _highlighter ? _highlighter->objectName() : QString();
}

void TextMemoEditor::setHighlighterName(const QString& name)
{
    if (!_highlighter && name.isEmpty()) return;
    if (_highlighter && _highlighter->objectName() == name) return;

    _editor->setUndoRedoEnabled(false);

    if (_highlighter) delete _highlighter;
    if (!name.isEmpty())
    {
        auto spec = Ori::Highlighter::getSpec(name);
        if (spec)
            _highlighter = new Ori::Highlighter::Highlighter(_editor->document(), spec);
    }

    _editor->setUndoRedoEnabled(true);
}
