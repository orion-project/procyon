#include "MemoEditor.h"

#include "../Spellchecker.h"
#include "../TextEditorHelpers.h"

#include <QDebug>
#include <QDesktopServices>
#include <QMenu>
#include <QMouseEvent>
#include <QTextBlock>
#include <QToolTip>

void MemoEditor::setSpellchecker(Spellchecker* checker)
{
    if (_spellchecker == checker) return;

    if (_spellchecker)
        disconnect(_spellchecker, &Spellchecker::dictionaryChanged, this, &MemoEditor::spellcheck);

    _spellchecker = checker;

    if (_spellchecker)
        connect(_spellchecker, &Spellchecker::dictionaryChanged, this, &MemoEditor::spellcheck);
    else
        // TODO: dont't clear search result marks
        setExtraSelections(QList<QTextEdit::ExtraSelection>());
}

// Hyperlink made via syntax highlighter doesn't create some 'top level' anchor,
// so `anchorAt` returns nothing, we have to enumerate styles to find out a href.
QString MemoEditor::hyperlinkAt(const QPoint& pos) const
{
    auto cursor = cursorForPosition(viewport()->mapFromParent(pos));

    for (auto format : cursor.block().layout()->formats())
    {
        int cursorPos = cursor.positionInBlock();
        if (cursorPos >= format.start and
            cursorPos < format.start + format.length and
            format.format.isAnchor())
        {
            auto href = format.format.anchorHref();
            if (not href.isEmpty()) return href;
        }
    }
    return QString();
}

QTextCursor MemoEditor::spellingAt(const QPoint& pos) const
{
    auto cursor = cursorForPosition(viewport()->mapFromParent(pos));
    auto cursorPos = cursor.position();
    for (auto es : extraSelections())
        if (cursorPos >= es.cursor.anchor() && cursorPos < es.cursor.position())
            return es.cursor;
    return QTextCursor();
}

void MemoEditor::contextMenuEvent(QContextMenuEvent *e)
{
    if (!_spellchecker)
    {
        QTextEdit::contextMenuEvent(e);
        return;
    }

    auto pos = e->pos();
    auto cursor = spellingAt(pos);
    if (cursor.isNull())
        QTextEdit::contextMenuEvent(e);
    else
        showSpellcheckMenu(cursor, pos);
}

void MemoEditor::mousePressEvent(QMouseEvent *e)
{
    if (e->button() & Qt::LeftButton && e->modifiers().testFlag(Qt::ControlModifier))
        _clickedHref = hyperlinkAt(e->pos());

    QTextEdit::mousePressEvent(e);
}

void MemoEditor::mouseReleaseEvent(QMouseEvent *e)
{
    if (not _clickedHref.isEmpty())
    {
        QDesktopServices::openUrl(_clickedHref);
        _clickedHref.clear();
    }
    QTextEdit::mouseReleaseEvent(e);
}

bool MemoEditor::event(QEvent *event)
{
    if (event->type() != QEvent::ToolTip)
        return QTextEdit::event(event);

    auto helpEvent = dynamic_cast<QHelpEvent*>(event);
    if (not helpEvent) return false;

    auto href = hyperlinkAt(helpEvent->pos());
    if (not href.isEmpty())
    {
        auto tooltip = QStringLiteral("<p style='white-space:pre'>%1<p>%2")
                .arg(href, tr("<b>Ctrl + Click</b> to open"));
        QToolTip::showText(helpEvent->globalPos(), tooltip);
    }
    else QToolTip::hideText();

    event->accept();
    return true;
}

void MemoEditor::showSpellcheckMenu(QTextCursor &cursor, const QPoint& pos)
{
    auto word = cursor.selectedText();
    auto menu = createStandardContextMenu(pos);

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
            connect(actionWord, &QAction::triggered, [&cursor, variant]{
                cursor.insertText(variant);
            });
            actions << actionWord;
        }

    auto actionRemember = new QAction(tr("Add to dictionary"), menu);
    connect(actionRemember, &QAction::triggered, [this, &cursor, &word]{
        _spellchecker->save(word);
        _spellchecker->ignore(word);
        removeSpellErrorMark(cursor);
    });
    actions << actionRemember;

    auto actionIgnore = new QAction(tr("Ignore this world"), menu);
    connect(actionIgnore, &QAction::triggered, [this, &cursor, &word]{
        _spellchecker->ignore(word);
        removeSpellErrorMark(cursor);
    });
    actions << actionIgnore;

    auto actionSeparator = new QAction(menu);
    actionSeparator->setSeparator(true);
    actions << actionSeparator;

    menu->insertActions(menu->actions().first(), actions);
    menu->exec(mapToGlobal(pos));
    delete menu;
}

void MemoEditor::removeSpellErrorMark(const QTextCursor& cursor)
{
    QList<QTextEdit::ExtraSelection> marks;
    for (auto mark : extraSelections())
        if (mark.cursor != cursor)
            marks << mark;
    setExtraSelections(marks);
}

void MemoEditor::spellcheck()
{
    TextEditSpellcheck(this, _spellchecker).check();
}
