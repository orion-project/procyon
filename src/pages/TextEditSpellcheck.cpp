#include "TextEditSpellcheck.h"

#include "../Spellchecker.h"
#include "../TextEditorHelpers.h"

#include <QAction>
#include <QDebug>
#include <QMenu>
#include <QTimer>

TextEditSpellcheck::TextEditSpellcheck(QTextEdit *editor, QObject *parent) : QObject(parent), _editor(editor)
{
    _editor->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_editor, &QTextEdit::customContextMenuRequested,
            this, &TextEditSpellcheck::contextMenuRequested);
    connect(_editor->document(), QOverload<int, int, int>::of(&QTextDocument::contentsChange),
            this, &TextEditSpellcheck::documentContentChanged);
}

void TextEditSpellcheck::setSpellchecker(Spellchecker* checker)
{
    if (_spellchecker == checker) return;

    if (_spellchecker)
        disconnect(_spellchecker, &Spellchecker::dictionaryChanged, this, &TextEditSpellcheck::spellcheckAll);

    _spellchecker = checker;

    if (_spellchecker)
        connect(_spellchecker, &Spellchecker::dictionaryChanged, this, &TextEditSpellcheck::spellcheckAll);
}

void TextEditSpellcheck::spellcheckAll()
{
    if (!_spellchecker) return;

    TextEditCursorBackup cursorBackup(_editor);

    _startPos = -1;
    _stopPos = -1;
    spellcheck();
    _editor->setExtraSelections(_spellErrorMarks);
}

void TextEditSpellcheck::spellcheckChanges()
{
    // TODO
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

    _spellErrorMarks.clear();

    QTextCursor cursor(_editor->document());

    if (_startPos > -1) cursor.setPosition(_startPos);

    while ((_stopPos > -1 && cursor.position() < _stopPos) || !cursor.atEnd())
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
        if (length > 1)
        {
            QString word = cursor.selectedText();

            if (!_spellchecker->check(word))
                _spellErrorMarks << QTextEdit::ExtraSelection {cursor, spellErrorFormat};
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

    if (_spellchecker)
    {
        auto cursor = spellingAt(pos);
        if (!cursor.isNull())
            addSpellcheckActions(menu, cursor);
    }

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
        removeErrorMark(cursor);
    });
    actions << actionRemember;

    auto actionIgnore = new QAction(tr("Ignore this world"), menu);
    connect(actionIgnore, &QAction::triggered, [this, cursor, word]{
        _spellchecker->ignore(word);
        removeErrorMark(cursor);
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
    if (!_spellchecker) return;
    _editor->setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

void TextEditSpellcheck::documentContentChanged(int position, int charsRemoved, int charsAdded)
{
    if (!_spellchecker || _changesLocked || _editor->isReadOnly()) return;

    qDebug() << "changed" << position << charsRemoved << charsAdded;

//    if (!_timer)
//    {
//        _timer = new QTimer(this);
//        _timer->setInterval(500);
//        connect(_timer, &QTimer::timeout, this, &TextEditSpellcheck::spellcheckChanges);
//    }
//    _timer->start();

//    if (position < _startPos) _startPos = position;

//    int
//    if (position + charsAdded > _stopPos)
//        _stopPos = position + charsAdded;

//    //TextEditCursorBackup backup(this);

////    int startOfChanges = position

//    QTextCursor cursor(document());
//    cursor.setPosition(position);
//    cursor.movePosition(QTextCursor::StartOfWord);
//    cursor.setPosition(position + charsAdded);
//    cursor.movePosition(QTextCursor::EndOfWord);
//    qDebug() << "check:" << cursor.selectedText();


    /*for (auto es : extraSelections())
    {
        if (position >= es.cursor.anchor() && position <= es.cursor.position())
        {
            qDebug() << "SPELL" << es.cursor.selectedText();
            break;
        }
    }*/
}
