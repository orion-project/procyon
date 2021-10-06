#include "TextEditSpellcheck.h"

#ifdef ENABLE_SPELLCHECK

#include "Spellchecker.h"
#include "../TextEditHelpers.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QTimer>

using This = TextEditSpellcheck;

TextEditSpellcheck::TextEditSpellcheck(QTextEdit *editor, Spellchecker *spellchecker, QObject *parent)
    : QObject(parent), _editor(editor), _spellchecker(spellchecker)
{
    connect(_spellchecker, &Spellchecker::wordIgnored, this, &This::wordIgnored);

    _editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_editor, &QTextEdit::customContextMenuRequested, this, &This::contextMenuRequested);
    connect(_editor->document(), QOverload<int, int, int>::of(&QTextDocument::contentsChange), this, &This::documentChanged);
    connect(_editor, &QTextEdit::cursorPositionChanged, this, &This::cursorMoved);

    _timer = new QTimer(this);
    _timer->setInterval(500);
    connect(_timer, &QTimer::timeout, this, &This::spellcheckChanges);
}

TextEditSpellcheck::~TextEditSpellcheck()
{
    // When program closes we don't know what object is deleted first.
    // This is the only dangerous case we use QPointer for.
    if (_editor)
        _editor->setContextMenuPolicy(Qt::DefaultContextMenu);
}

void TextEditSpellcheck::spellcheckAll()
{
    _spellcheckStart = -1;
    _spellcheckStop = -1;
    spellcheck();
    _editor->setExtraSelections(_errorMarks);
}

static int selectWord(QTextCursor& cursor)
{
    // When move cursor to EndOfWord, it stops before bracket, qoute, or punctuation.
    // But quotes like &raquo; or &rdquo; become a part of the world for some reason.
    // Remove such punctuation at word boundaries.

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

void TextEditSpellcheck::spellcheck()
{
    static auto spellErrorFormat = TextFormat().spellError().get();

    _errorMarks.clear();

    QTextCursor cursor(_editor->document());

    if (_spellcheckStart > -1) cursor.setPosition(_spellcheckStart);

    while ((_spellcheckStop < 0 || cursor.position() < _spellcheckStop) && !cursor.atEnd())
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
        //
        // TODO: currently, abbreviations such as "e.i.", "e.g.", "т.д.", "т.п."
        // are splitted to series of one-letter words and therefore skipped.
        // It allows mixing of such words in different languages,
        // e.g. one can use "т.д." in a text in English, it's not ok.
        //
        // Similar issue with words like "doesn't" - it splitted into "doesn" and "t",
        // "t" is skipped and "doesn" gives spelling error. Checking is ok when
        // true apostrophe character is used (’ = U+2019). But it's not the case
        // when one type memos via keyboard - single quote is generally used as apostrophe.
        //
        // It'd be better to check such words as a whole.
        //
        if (length > 1 && TextEditHelpers::hyperlinkAt(cursor).isEmpty())
        {
            QString word = cursor.selectedText();

            if (!_spellchecker->check(word))
                _errorMarks << QTextEdit::ExtraSelection {cursor, spellErrorFormat};
        }

        cursor.movePosition(QTextCursor::NextWord, QTextCursor::MoveAnchor);
    }
}

QTextCursor TextEditSpellcheck::spellingAt(const QPoint& pos) const
{
    auto cursor = _editor->cursorForPosition(_editor->viewport()->mapFromParent(pos));
    auto cursorPos = cursor.position();
    for (auto es : _editor->extraSelections())
        if (cursorPos >= es.cursor.anchor() && cursorPos <= es.cursor.position())
            return es.cursor;
    return QTextCursor();
}

void TextEditSpellcheck::contextMenuRequested(const QPoint &pos)
{
    auto menu = _editor->createStandardContextMenu(pos);

    auto cursor = spellingAt(pos);
    if (!cursor.isNull())
        addSpellcheckActions(menu, cursor);

    menu->exec(_editor->mapToGlobal(pos));
    delete menu;
}

void TextEditSpellcheck::addSpellcheckActions(QMenu* menu, QTextCursor& cursor)
{
    auto word = cursor.selectedText();

    QList<QAction*> actions;

    auto variants = _spellchecker->suggest(word);
    if (variants.isEmpty())
    {
        auto actionNone = new QAction(tr("No variants"), menu);
        actionNone->setDisabled(true);
        actions << actionNone;
    }
    else
        for (auto variant : variants)
        {
            auto actionWord = new QAction(">  " + variant, menu);
            connect(actionWord, &QAction::triggered, [this, cursor, variant]{
                _changesLocked = true;
                const_cast<QTextCursor&>(cursor).insertText(variant);
                _changesLocked = false;
            });
            actions << actionWord;
        }

    auto actionRemember = new QAction(tr("Add to dictionary"), menu);
    connect(actionRemember, &QAction::triggered, [this, cursor, word]{
        _spellchecker->save(word);
        _spellchecker->ignore(word);
    });
    actions << actionRemember;

    auto actionIgnore = new QAction(tr("Ignore this world"), menu);
    connect(actionIgnore, &QAction::triggered, [this, cursor, word]{
        _spellchecker->ignore(word);
    });
    actions << actionIgnore;

    auto actionSeparator = new QAction(menu);
    actionSeparator->setSeparator(true);
    actions << actionSeparator;

    menu->insertActions(menu->actions().first(), actions);
}

void TextEditSpellcheck::removeErrorMark(const QTextCursor& cursor)
{
    QList<QTextEdit::ExtraSelection> marks;
    for (auto mark : _editor->extraSelections())
        if (mark.cursor != cursor)
            marks << mark;
    _editor->setExtraSelections(marks);
}

void TextEditSpellcheck::clearErrorMarks()
{
    _editor->setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

void TextEditSpellcheck::documentChanged(int position, int charsRemoved, int charsAdded)
{
    Q_UNUSED(charsRemoved)

    if (_changesLocked) return;

    if (_changesStart < 0 || position < _changesStart) _changesStart = position;

    int stopPos = position + charsAdded;
    if (stopPos > _changesStop) _changesStop = stopPos;

    _isHrefChanged = _isHrefChanged || !_hyperlinkAtCursor.isEmpty();

    _timer->start();
}

void TextEditSpellcheck::spellcheckChanges()
{
    _timer->stop();

    QTextCursor cursor(_editor->document());

    // We could insert spaces and split a word in two.
    // Then we have to check not only the current word but also the previous one.
    // That's the reason for shifting to -1 from _changesStart position.
    // But if there is/was a hyperlink in place of changes,
    // it can contain arbitrary number of words, then it better to check all the block.
    cursor.setPosition(_changesStart > 0 ? _changesStart - 1 : _changesStart);
    cursor.movePosition(_isHrefChanged ? QTextCursor::StartOfBlock : QTextCursor::StartOfWord, QTextCursor::MoveAnchor);
    _spellcheckStart = cursor.position();

    cursor.setPosition(_changesStop);
    cursor.movePosition(_isHrefChanged ? QTextCursor::EndOfBlock : QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    _spellcheckStop = cursor.position();

    // Remove marks which are in checking range, they will be recreated on spellcheck
    QList<QTextEdit::ExtraSelection> errorMarks;
    for (auto es : _editor->extraSelections())
        if (es.cursor.position() < _spellcheckStart ||
            es.cursor.anchor() >= _spellcheckStop)
            errorMarks << es;

    spellcheck();

    errorMarks.append(_errorMarks);
    _editor->setExtraSelections(errorMarks);

    _spellcheckStart = -1;
    _spellcheckStop = -1;
    _changesStart = -1;
    _changesStop = -1;
}

void TextEditSpellcheck::cursorMoved()
{
    _hyperlinkAtCursor = TextEditHelpers::hyperlinkAt(_editor->textCursor());
}

void TextEditSpellcheck::wordIgnored(const QString& word)
{
    QList<QTextEdit::ExtraSelection> errorMarks;
    for (auto es : _editor->extraSelections())
        if (es.cursor.selectedText() != word)
            errorMarks << es;
    _editor->setExtraSelections(errorMarks);
}

#endif // ENABLE_SPELLCHECK
