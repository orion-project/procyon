#include "MemoTextBrowser.h"

#include <QHelpEvent>
#include <QToolTip>

MemoTextBrowser::MemoTextBrowser(QWidget *parent) : QTextBrowser(parent)
{
    setOpenExternalLinks(true);
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    setProperty("role", "memo_editor");
}

bool MemoTextBrowser::event(QEvent *event)
{
    if (event->type() != QEvent::ToolTip)
        return QTextBrowser::event(event);

    auto helpEvent = dynamic_cast<QHelpEvent*>(event);
    if (not helpEvent) return false;

    auto href = anchorAt(helpEvent->pos());
    if (not href.isEmpty())
    {
        auto tooltip = QStringLiteral("<p style='white-space:pre'>%1").arg(href);
        QToolTip::showText(helpEvent->globalPos(), tooltip);
    }
    else QToolTip::hideText();

    event->accept();
    return true;
}
