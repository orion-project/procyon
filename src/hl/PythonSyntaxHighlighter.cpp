/*
https://talktomachines.blogspot.ru/2014/01/python-syntax-highlighting-for-qtextedit.html

$Id: PythonSyntaxHighlighter.cpp 167 2013-11-03 17:01:22Z oliver $
This is a C++ port of the following PyQt example
http://diotavelli.net/PyQtWiki/Python%20syntax%20highlighting
C++ port by Frankie Simon (www.kickdrive.de, www.fuh-edv.de)

The following free software license applies for this file ("X11 license"):

Permission is hereby granted, free of charge, to any person obtaining a copy of this software
and associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "PythonSyntaxHighlighter.h"

#define STYLE_KEYWORD 0
#define STYLE_OPERATOR 1
#define STYLE_BRACE 2
#define STYLE_DEFCLASS 3
#define STYLE_STRING 4
#define STYLE_STRING2 5
#define STYLE_COMMENT 6
#define STYLE_SELF 7
#define STYLE_NUMBER 8

HighlightingStyleSet* getPythonHighlightingStyles()
{
    static HighlightingStyleSet styles;
    if (styles.isEmpty())
    {
        styles.insert(STYLE_KEYWORD, getTextCharFormat("blue"));
        styles.insert(STYLE_OPERATOR, getTextCharFormat("red"));
        styles.insert(STYLE_BRACE, getTextCharFormat("darkGray"));
        styles.insert(STYLE_DEFCLASS, getTextCharFormat("black", "bold"));
        styles.insert(STYLE_STRING, getTextCharFormat("magenta"));
        styles.insert(STYLE_STRING2, getTextCharFormat("darkMagenta"));
        styles.insert(STYLE_COMMENT, getTextCharFormat("darkGreen", "italic"));
        styles.insert(STYLE_SELF, getTextCharFormat("black", "italic"));
        styles.insert(STYLE_NUMBER, getTextCharFormat("brown"));
    }
    return &styles;
}


PythonSyntaxHighlighter::PythonSyntaxHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    // TODO there are lot of static data but it is initialized for each highlighter instance

    keywords = QStringList() << "and" << "assert" << "break" << "class" << "continue" << "def" <<
        "del" << "elif" << "else" << "except" << "exec" << "finally" <<
        "for" << "from" << "global" << "if" << "import" << "in" <<
        "is" << "lambda" << "not" << "or" << "pass" << "print" <<
        "raise" << "return" << "try" << "while" << "yield" <<
        "None" << "True" << "False";

    operators = QStringList() << "=" <<
        // Comparison
        "==" << "!=" << "<" << "<=" << ">" << ">=" <<
        // Arithmetic
        "\\+" << "-" << "\\*" << "/" << "//" << "%" << "\\*\\*" <<
        // In-place
        "\\+=" << "-=" << "\\*=" << "/=" << "%=" <<
        // Bitwise
        "\\^" << "\\|" << "&" << "~" << ">>" << "<<";

    braces = QStringList() << "{" << "}" << "\\(" << "\\)" << "\\[" << "\\]";

    styles = getPythonHighlightingStyles();

    triSingleQuote.setPattern("'''");
    triDoubleQuote.setPattern("\"\"\"");

    foreach (QString currKeyword, keywords)
        rules.append(HighlightingRule(QString("\\b%1\\b").arg(currKeyword), STYLE_KEYWORD));
    foreach (QString currOperator, operators)
        rules.append(HighlightingRule(QString("%1").arg(currOperator), STYLE_OPERATOR));
    foreach (QString currBrace, braces)
        rules.append(HighlightingRule(QString("%1").arg(currBrace), STYLE_BRACE));

    // 'self'
    rules.append(HighlightingRule("\\bself\\b", STYLE_SELF));

    // Double-quoted string, possibly containing escape sequences
    // FF: originally in python : r'"[^"\\]*(\\.[^"\\]*)*"'
    rules.append(HighlightingRule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", STYLE_STRING));
    // Single-quoted string, possibly containing escape sequences
    // FF: originally in python : r"'[^'\\]*(\\.[^'\\]*)*'"
    rules.append(HighlightingRule("'[^'\\\\]*(\\\\.[^'\\\\]*)*'", STYLE_STRING));

    // 'def' followed by an identifier
    // FF: originally: r'\bdef\b\s*(\w+)'
    rules.append(HighlightingRule("\\bdef\\b\\s*(\\w+)", STYLE_DEFCLASS, 1));
    // 'class' followed by an identifier
    // FF: originally: r'\bclass\b\s*(\w+)'
    rules.append(HighlightingRule("\\bclass\\b\\s*(\\w+)", STYLE_DEFCLASS, 1));

    // From '#' until a newline
    // FF: originally: r'#[^\\n]*'
    rules.append(HighlightingRule("#[^\\n]*", STYLE_COMMENT));

    // Numeric literals
    rules.append(HighlightingRule("\\b[+-]?[0-9]+[lL]?\\b", STYLE_NUMBER)); // r'\b[+-]?[0-9]+[lL]?\b'
    rules.append(HighlightingRule("\\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\\b", STYLE_NUMBER)); // r'\b[+-]?0[xX][0-9A-Fa-f]+[lL]?\b'
    rules.append(HighlightingRule("\\b[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b", STYLE_NUMBER)); // r'\b[+-]?[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\b'
}

void PythonSyntaxHighlighter::highlightBlock(const QString &text)
{
    foreach (HighlightingRule currRule, rules)
    {
        int idx = currRule.pattern.indexIn(text, 0);
        while (idx >= 0)
        {
            // Get index of Nth match
            idx = currRule.pattern.pos(currRule.nth);
            int length = currRule.pattern.cap(currRule.nth).length();
            setFormat(idx, length, styles->value(currRule.styleId));
            idx = currRule.pattern.indexIn(text, idx + length);
        }
    }

    setCurrentBlockState(0);

    // Do multi-line strings
    bool isInMultilne = matchMultiline(text, triSingleQuote, 1, styles->value(STYLE_STRING2));
    if (!isInMultilne)
        isInMultilne = matchMultiline(text, triDoubleQuote, 2, styles->value(STYLE_STRING2));
}

bool PythonSyntaxHighlighter::matchMultiline(
        const QString &text, const QRegExp &delimiter, const int inState, const QTextCharFormat &style)
{
    int start = -1;
    int add = -1;
    int end = -1;
    int length = 0;

    // If inside triple-single quotes, start at 0
    if (previousBlockState() == inState)
    {
        start = 0;
        add = 0;
    }
    // Otherwise, look for the delimiter on this line
    else
    {
        start = delimiter.indexIn(text);
        // Move past this match
        add = delimiter.matchedLength();
    }

    // As long as there's a delimiter match on this line...
    while (start >= 0)
    {
        // Look for the ending delimiter
        end = delimiter.indexIn(text, start + add);
        // Ending delimiter on this line?
        if (end >= add)
        {
            length = end - start + add + delimiter.matchedLength();
            setCurrentBlockState(0);
        }
        // No; multi-line string
        else
        {
            setCurrentBlockState(inState);
            length = text.length() - start + add;
        }
        // Apply formatting and look for next
        setFormat(start, length, style);
        start = delimiter.indexIn(text, start + length);
    }
    // Return True if still inside a multi-line string, False otherwise
    return currentBlockState() == inState;
}
