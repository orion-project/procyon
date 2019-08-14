#include "MemoEditor.h"

#include <QDesktopServices>
#include <QMouseEvent>
#include <QTextBlock>
#include <QToolTip>

bool MemoEditor::shouldProcess(QMouseEvent *e)
{
    return e->modifiers() & Qt::ControlModifier && e->button() & Qt::LeftButton;
}

// Hyperlink made via syntax highlighter doesn't create some 'top level' anchor,
// so `anchorAt` returns nothing, we have to enumerate styles to find out a href.
QString MemoEditor::hrefAtWidgetPos(const QPoint& pos) const
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
    if (shouldProcess(e))
        _clickedHref = hrefAtWidgetPos(e->pos());

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

    auto href = hrefAtWidgetPos(helpEvent->pos());
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
