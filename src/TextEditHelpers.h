#ifndef TEXT_EDIT_HELPERS_H
#define TEXT_EDIT_HELPERS_H

#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

struct TextFormat
{
    TextFormat() {}
    TextFormat(const QString& colorName): _colorName(colorName) {}

    TextFormat& family(const QString& family) { _fontFamily = family; return *this; }
    TextFormat& bold() { _bold = true; return *this; }
    TextFormat& italic() { _italic = true; return *this; }
    TextFormat& underline() { _underline = true; return *this; }
    TextFormat& strikeOut() { _strikeOut = true; return *this; }
    TextFormat& anchor() { _anchor = true; return *this; }
    TextFormat& spellError() { _spellError = true; return *this; }
    TextFormat& background(const QString& colorName) { _backColorName = colorName; return *this; }

    QTextCharFormat get() const;

private:
    QString _fontFamily;
    QString _colorName;
    QString _backColorName;
    bool _bold = false;
    bool _italic = false;
    bool _underline = false;
    bool _anchor = false;
    bool _spellError = false;
    bool _strikeOut = false;
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


namespace TextEditHelpers
{
QString hyperlinkAt(const QTextCursor& cursor);
void exportToPdf(QTextDocument* doc, const QString& fileName);
}

#endif // TEXT_EDIT_HELPERS_H
