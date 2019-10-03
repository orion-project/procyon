#include "MarkdownHelper.h"

#include "ori_html.h"

namespace MarkdownHelper {

QString markdownToHtml(const QString& markdown)
{
    auto markdownBytes = markdown.toUtf8();

    hoedown_extensions extensions = hoedown_extensions(HOEDOWN_EXT_BLOCK | HOEDOWN_EXT_SPAN);
    hoedown_renderer* renderer = hoedown_html_renderer_new_ori();
    hoedown_document* document = hoedown_document_new(renderer, extensions, 16);
    hoedown_buffer* in_buf = hoedown_buffer_new(1024);
    hoedown_buffer_set(in_buf, reinterpret_cast<const uint8_t*>(markdownBytes.data()), static_cast<size_t>(markdownBytes.size()));
    hoedown_buffer* out_buf = hoedown_buffer_new(64);
    hoedown_document_render(document, out_buf, in_buf->data, in_buf->size);
    hoedown_buffer_free(in_buf);
    hoedown_document_free(document);
    hoedown_html_renderer_free_ori(renderer);

    QByteArray htmlBytes(reinterpret_cast<char*>(out_buf->data), static_cast<int>(out_buf->size));
    QString html = QStringLiteral("<html></body>\n") + QString::fromUtf8(htmlBytes) + QStringLiteral("\n</body></html>");

    hoedown_buffer_free(out_buf);

    return html;
}

} // namespace MarkdownHelper
