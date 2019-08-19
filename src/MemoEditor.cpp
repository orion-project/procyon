#include "MemoEditor.h"

#include "Spellchecker.h"
#include "TextEditHelpers.h"

#include <QDebug>
#include <QDesktopServices>
#include <QMenu>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTimer>
#include <QToolTip>

MemoEditor::MemoEditor(QWidget* parent) : QTextEdit(parent)
{
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


