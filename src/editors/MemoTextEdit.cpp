#include "MemoTextEdit.h"

#include "../TextEditHelpers.h"

#include <QDebug>
#include <QDesktopServices>
#include <QMenu>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTimer>
#include <QToolTip>

MemoTextEdit::MemoTextEdit(QWidget* parent) : QTextEdit(parent)
{
}

// Hyperlink made via syntax highlighter doesn't create some 'top level' anchor,
// so `anchorAt` returns nothing, we have to enumerate styles to find out a href.
QString MemoTextEdit::hyperlinkAt(const QPoint& pos) const
{
    return TextEditHelpers::hyperlinkAt(cursorForPosition(viewport()->mapFromParent(pos)));
}

void MemoTextEdit::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->modifiers().testFlag(Qt::ControlModifier))
        _clickedHref = hyperlinkAt(e->pos());

    // There is no selection -> move cursor to the point of click
    auto cursor = textCursor();
    if (e->button() == Qt::RightButton && cursor.anchor() == cursor.position())
        setTextCursor(cursorForPosition(viewport()->mapFromParent(e->pos())));

    QTextEdit::mousePressEvent(e);
}

void MemoTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    if (not _clickedHref.isEmpty())
    {
        QDesktopServices::openUrl(_clickedHref);
        _clickedHref.clear();
    }
    QTextEdit::mouseReleaseEvent(e);
}

bool MemoTextEdit::event(QEvent *event)
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
