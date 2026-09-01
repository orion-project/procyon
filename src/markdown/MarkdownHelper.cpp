#include "MarkdownHelper.h"

#include "core/FileStore.h"

#include "ori_html.h"

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
