#ifndef HOEDOWN_ORI_HTML_H
#define HOEDOWN_ORI_HTML_H

#include "document.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#define HOEDOWN_MALLOC
#else
#define HOEDOWN_MALLOC __attribute__ ((malloc))
#endif

typedef struct {
    void *context;
    int (*correct_file_path)(void *context,
        const uint8_t *in_data, size_t in_size,
        uint8_t **out_data, size_t *out_size);
} render_augments_ori;

void set_render_augments_ori(render_augments_ori value);

hoedown_renderer* hoedown_html_renderer_new_ori() HOEDOWN_MALLOC;
void hoedown_html_renderer_free_ori(hoedown_renderer *renderer);

#undef HOEDOWN_MALLOC

#ifdef __cplusplus
}
#endif

#endif // HOEDOWN_ORI_HTML_H
