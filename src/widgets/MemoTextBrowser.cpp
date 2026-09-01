#include "MemoTextBrowser.h"

#include "AppSettings.h"
#include "markdown/MarkdownHelper.h"

#include "helpers/OriDialogs.h"
#include "widgets/OriPopupMessage.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHelpEvent>
#include <QToolTip>

MemoTextBrowser::MemoTextBrowser(QWidget *parent) : QTextBrowser(parent)
{
    setOpenLinks(false);
    setProperty("role", "memo_editor");
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
    connect(this, &QTextBrowser::anchorClicked, this, &MemoTextBrowser::linkClicked);
}

bool MemoTextBrowser::event(QEvent *event)
{
    if (event->type() != QEvent::ToolTip)
        return QTextBrowser::event(event);

    auto helpEvent = dynamic_cast<QHelpEvent*>(event);
    if (!helpEvent) return false;

    auto href = anchorAt(helpEvent->pos());
    if (!href.isEmpty())
    {
        QString tooltip;
        if (href.startsWith(MarkdownHelper::fileScheme))
        {
            href = href.right(href.length() - 5);
            tooltip = tr("Download: ");
        }
        else if (href.startsWith(MarkdownHelper::enotScheme))
        {
            href.clear();
            tooltip = tr("Open memo");
        }
        else
        {
            tooltip = tr("Open in browser: ");
        }
        if (!href.isEmpty())
            tooltip += QStringLiteral("<p style='white-space:pre'>%1").arg(href);
        QToolTip::showText(helpEvent->globalPos(), tooltip);
    }
    else QToolTip::hideText();

    event->accept();
    return true;
}

static void downloadFile(const QString& filePath)
{
    if (!QFile::exists(filePath))
        return Ori::Dlg::warning(qApp->tr("File not found:\n%1").arg(filePath));

    auto targetFile = QFileDialog::getSaveFileName(qApp->activeWindow(), qApp->tr("Export Attached File"));
    if (targetFile.isEmpty()) return;

    if (QFile::exists(targetFile) && !QFile::remove(targetFile))
        return Ori::Dlg::error(qApp->tr("Failed to owerwrite file in the target directory"));

    if (!QFile::copy(filePath, targetFile))
        return Ori::Dlg::error(qApp->tr("Failed to copy file to the target directory"));

    Ori::Gui::PopupMessage::affirm(qApp->tr("File saved"));
}

void MemoTextBrowser::linkClicked(const QUrl& url)
{
    QString href = url.toString();
    if (href.startsWith(MarkdownHelper::httpScheme))
        QDesktopServices::openUrl(url);
    else if (href.startsWith(MarkdownHelper::fileScheme))
        downloadFile(url.path());
    else if (href.startsWith(MarkdownHelper::enotScheme))
        emit memoOpenRequested(url.path().toInt());
}

void MemoTextBrowser::setText(const QString& text)
{
    setHtml(MarkdownHelper::markdownToHtml(text));
}
