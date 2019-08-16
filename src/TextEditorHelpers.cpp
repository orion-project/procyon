#include "TextEditorHelpers.h"

#include "Spellchecker.h"

//------------------------------------------------------------------------------
//                                  TextFormat
//------------------------------------------------------------------------------

QTextCharFormat TextFormat::get() const
{
    QTextCharFormat f;
    if (!_colorName.isEmpty())
        f.setForeground(QColor(_colorName));
    if (_bold)
        f.setFontWeight(QFont::Bold);
    f.setFontItalic(_italic);
    f.setFontUnderline(_underline);
    f.setAnchor(_anchor);
    if (_spellError)
    {
        f.setUnderlineColor("red");
        f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    }
    return f;
}

//------------------------------------------------------------------------------
//                              TextEditSpellcheck
//------------------------------------------------------------------------------

TextEditSpellcheck::TextEditSpellcheck(QTextEdit* editor): _editor(editor)
{
}

// When move cursor to EndOfWord, it stops before bracket, qoute, or punctuation.
// But quotes like &raquo; or &rdquo; become a part of the world for some reason.
// Remove such punctuation at word boundaries.
int TextEditSpellcheck::selectWord(QTextCursor& cursor)
{
    QString word = cursor.selectedText();
    int length = word.length();
    int start = 0;
    int stop = length - 1;

    while (start < length)
    {
        auto ch = word.at(start);
        if (ch.isLetter() || ch.isDigit()) break;
        start++;
    }

    while (stop > start)
    {
        auto ch = word.at(stop);
        if (ch.isLetter() || ch.isDigit()) break;
        stop--;
    }

    length = stop - start + 1;
    int anchor = cursor.anchor();
    cursor.setPosition(anchor + start, QTextCursor::MoveAnchor);
    cursor.setPosition(anchor + start + length, QTextCursor::KeepAnchor);
    return length;
}

void TextEditSpellcheck::check(const QString &lang)
{
    auto spellchecker = Spellchecker::get(lang);
    if (!spellchecker) return;

    static auto spellErrorFormat = TextFormat().spellError().get();
    QList<QTextEdit::ExtraSelection> spellErrorMarks;
    TextEditCursorBackup cursorBackup(_editor);
    QTextCursor cursor(_editor->document());

    while (!cursor.atEnd())
    {
        cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        int length = selectWord(cursor);

        // When we've skipped punctuation and there is no word,
        // the cursor may be at the beginning of the next word already.
        // For example, have text "(word1) word2",
        // the bracket "(" is treated by QTextEdit as a separate word.
        // After skipping it, we become at the "w" of "word1" - at the next word after the bracket!
        // Then moving to `NextWord` shifts the cursor to "word2", and "word1" gets missed.
        // To avoid, try to find the end of a (possible current) word after skipping punctuation.
        if (length == 0)
        {
            cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
            length = selectWord(cursor);
        }

        // Skip one-letter words
        // TODO: currently, abbreviations such as "e.i.", "e.g.", "т.д.", "т.п."
        // are splitted to series of one-letter words and therefore skipped.
        // It allows mixing of such words in different languages,
        // e.g. one can use "т.д." in a text in English, it's not ok.
        // It'd be better to check such words as a whole.
        if (length > 1)
        {
            QString word = cursor.selectedText();

            if (!spellchecker->check(word))
                spellErrorMarks << QTextEdit::ExtraSelection {cursor, spellErrorFormat};
        }

        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor);
    }

    if (!spellErrorMarks.isEmpty())
        _editor->setExtraSelections(spellErrorMarks);
}
