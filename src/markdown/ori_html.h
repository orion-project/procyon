#ifndef HOEDOWN_ORI_HTML_H
#define HOEDOWN_ORI_HTML_H

#include "document.h"

#ifdef __cplusplus
extern "C" {
#endif

hoedown_renderer* hoedown_html_renderer_new_ori() __attribute__ ((malloc));
void hoedown_html_renderer_free_ori(hoedown_renderer *renderer);

#ifdef __cplusplus
}
#endif

#endif // HOEDOWN_ORI_HTML_H
