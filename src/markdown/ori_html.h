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

hoedown_renderer* hoedown_html_renderer_new_ori() HOEDOWN_MALLOC;
void hoedown_html_renderer_free_ori(hoedown_renderer *renderer);

#undef HOEDOWN_MALLOC

#ifdef __cplusplus
}
#endif

#endif // HOEDOWN_ORI_HTML_H
