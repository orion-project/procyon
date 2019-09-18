#include "MemoEditor.h"

#include "../catalog/Catalog.h"
#include "../TextEditSpellcheck.h"
#include "../Spellchecker.h"

#include <QTextEdit>

//------------------------------------------------------------------------------
//                                 MemoEditor
//------------------------------------------------------------------------------

MemoEditor::MemoEditor(MemoItem *memoItem, QWidget *parent) : QWidget(parent), _memoItem(memoItem)
{
}

//------------------------------------------------------------------------------
//                                TextMemoEditor
//------------------------------------------------------------------------------

TextMemoEditor::TextMemoEditor(MemoItem* memoItem, QWidget *parent) : MemoEditor(memoItem, parent)
{
}

void TextMemoEditor::setEditor(QTextEdit* editor)
{
    _editor = editor;
    _editor->setAcceptRichText(false);
    _editor->setWordWrapMode(QTextOption::NoWrap);
    _editor->setProperty("role", "memo_editor");

    connect(_editor, &QTextEdit::undoAvailable, this, &MemoEditor::onModified);
}

void TextMemoEditor::setFocus()
{
    _editor->setFocus();
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

void TextMemoEditor::setWordWrap(bool on)
{
    _editor->setWordWrapMode(on ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

QString TextMemoEditor::data() const
{
    return _editor->toPlainText();
}

void TextMemoEditor::toggleSpellcheck(bool on)
{
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
}

void TextMemoEditor::setSpellcheckLang(const QString &lang)
{
    toggleSpellcheck(false);
    _spellcheckLang = lang;
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
