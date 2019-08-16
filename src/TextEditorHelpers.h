#ifndef TEXT_EDITOR_HELPERS_H
#define TEXT_EDITOR_HELPERS_H

#include <QTextEdit>

class Spellchecker;

struct TextFormat
{
    TextFormat() {}
    TextFormat(const QString& colorName): _colorName(colorName) {}

    TextFormat& bold() { _bold = true; return *this; }
    TextFormat& italic() { _italic = true; return *this; }
    TextFormat& underline() { _underline = true; return *this; }
    TextFormat& anchor() { _anchor = true; return *this; }
    TextFormat& spellError() { _spellError = true; return *this; }

    QTextCharFormat get() const;

private:
    QString _colorName;
    bool _bold = false;
    bool _italic = false;
    bool _underline = false;
    bool _anchor = false;
    bool _spellError = false;
};

struct TextEditCursorBackup
{
    TextEditCursorBackup(QTextEdit* editor)
    {
        _editor = editor;
        _cursor = editor->textCursor();
    }

    ~TextEditCursorBackup()
    {
        _editor->setTextCursor(_cursor);
    }

private:
    QTextEdit* _editor;
    QTextCursor _cursor;
};

struct TextEditSpellcheck
{
    TextEditSpellcheck(QTextEdit* editor, Spellchecker* spellchecker);

    void check();

private:
    QTextEdit* _editor;
    Spellchecker* _spellchecker;
};

#endif // TEXT_EDITOR_HELPERS_H
