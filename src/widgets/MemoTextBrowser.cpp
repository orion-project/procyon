#include "MemoTextBrowser.h"

#include "AppSettings.h"
#include "core/FileStore.h"
#include "markdown/ori_html.h"

#include "helpers/OriDialogs.h"
#include "widgets/OriPopupMessage.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHelpEvent>
#include <QToolTip>

//------------------------------------------------------------------------------
//                             MarkdownHelper
//------------------------------------------------------------------------------

class MarkdownHelper
{
public:
    static QString markdownToHtml(const QString& markdown);

    static inline const auto httpScheme = QStringLiteral("http");
    static inline const auto fileScheme = QStringLiteral("file:");
    static inline const auto enotScheme = QStringLiteral("enot:");
};

static int correct_file_path(void *context,
    const uint8_t *in_data, size_t in_size, uint8_t **out_data, size_t *out_size)
{
    QString fileName = QString::fromUtf8((const char*)in_data, in_size);
    if (fileName.startsWith(MarkdownHelper::fileScheme))
        return 0;
    if (fileName.startsWith(MarkdownHelper::enotScheme))
        return 0;
    QString filePath;
    if (fileName.contains('/') || fileName.contains('\\'))
        filePath = QFileInfo(fileName).absoluteFilePath();
    else
        filePath = Store::files()->attachedFile(fileName).absoluteFilePath();
    auto fp = (MarkdownHelper::fileScheme + filePath).toUtf8();
    auto storage = reinterpret_cast<QList<QByteArray>*>(context);
    storage->append(fp);
    *out_data = (uint8_t*)fp.constData();
    *out_size = fp.size();
    return 1;
}

QString MarkdownHelper::markdownToHtml(const QString& markdown)
{
    auto markdownBytes = markdown.toUtf8();

    QList<QByteArray> renderStorage;
    set_render_augments_ori(render_augments_ori {
        .context = &renderStorage,
        .correct_file_path = correct_file_path,
    });

    hoedown_extensions extensions = hoedown_extensions(HOEDOWN_EXT_BLOCK | HOEDOWN_EXT_SPAN);
    hoedown_renderer* renderer = hoedown_html_renderer_new_ori();
    hoedown_document* document = hoedown_document_new(renderer, extensions, 16);
    hoedown_buffer* in_buf = hoedown_buffer_new(1024);
    hoedown_buffer_set(in_buf, (const uint8_t*)markdownBytes.data(), (size_t)markdownBytes.size());
    hoedown_buffer* out_buf = hoedown_buffer_new(64);
    hoedown_document_render(document, out_buf, in_buf->data, in_buf->size);
    hoedown_buffer_free(in_buf);
    hoedown_document_free(document);
    hoedown_html_renderer_free_ori(renderer);

    QByteArray htmlBytes((char*)out_buf->data, out_buf->size);
    QString html = QStringLiteral("<html></body>\n") + QString::fromUtf8(htmlBytes) + QStringLiteral("\n</body></html>");

    hoedown_buffer_free(out_buf);

    return html;
}

//------------------------------------------------------------------------------
//                             MemoTextBrowser
//------------------------------------------------------------------------------

MemoTextBrowser::MemoTextBrowser(QWidget *parent) : QTextBrowser(parent)
{
    setOpenLinks(false);
    setProperty("role", "memo_editor");
    setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
    // Setting text margins via QTextBrowser's QSS shifts scroll bars
    // It also doesn't understand neither HTML or BODY paddings/margins in CSS
    // So don it in code
    document()->setDocumentMargin(10);
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
