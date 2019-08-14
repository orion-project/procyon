#include "TextEditorHelpers.h"

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
