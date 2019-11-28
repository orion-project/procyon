#include "MemoEditor.h"

#include "../catalog/Catalog.h"
#include "../spellcheck/TextEditSpellcheck.h"
#include "../spellcheck/Spellchecker.h"
#include "../widgets/MemoTextEdit.h"

#include <QPrinter>

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
    exportToPdf(_editor->document(), fileName);
}

void TextMemoEditor::exportToPdf(QTextDocument* doc, const QString& fileName)
{
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPaperSize(QPrinter::A4);
    printer.setOutputFileName(fileName);

    doc->print(&printer);
}
