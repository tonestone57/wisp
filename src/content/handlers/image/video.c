#include <wisp/content/content.h>
#include <wisp/plotters.h>
#include <wisp/content/llcache.h>
#include <wisp/utils/utils.h>
#include <wisp/content/content_protected.h>
#include <wisp/utils/log.h>
#include "content/content_factory.h"
#include "content/handlers/image/video.h"

typedef struct nsvideo_content {
    struct content base;
    struct bitmap *current_bitmap;
} nsvideo_content;

static void nsvideo_destroy(struct content *c) {
    nsvideo_content *video = (nsvideo_content *)c;
    content__init(&video->base, NULL, NULL, NULL, NULL, NULL, false); // Placeholder
    free(video);
}

static bool nsvideo_redraw(struct content *c, struct content_redraw_data *data, const struct rect *clip, const struct redraw_context *ctx) {
    return true;
}

static nserror nsvideo_create(const struct content_handler *handler, lwc_string *imime_type, const struct http_parameter *params,
    struct llcache_handle *llcache, const char *fallback_charset, bool quirks, struct content **c) {
    nsvideo_content *video = calloc(1, sizeof(*video));
    if (!video) return NSERROR_NOMEM;
    *c = &video->base;
    return NSERROR_OK;
}

static const content_handler nsvideo_content_handler = {
    .create = nsvideo_create,
    .destroy = nsvideo_destroy,
    .redraw = nsvideo_redraw,
};

nserror nsvideo_init(void) {
    lwc_string *type;
    lwc_intern_string("video/mp4", 9, &type);
    return content_factory_register_handler(type, &nsvideo_content_handler);
}
