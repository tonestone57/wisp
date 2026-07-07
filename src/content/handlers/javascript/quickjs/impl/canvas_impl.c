#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/utils.h>
#include <wisp/bitmap.h>
#include <wisp/plotters.h>
#include <wisp/desktop/plot_blend2d.h>
#include <wisp/desktop/gui_table.h>
#include "utils/libdom.h"

/* Forward declarations for generated headers */
#include "JSHTMLCanvasElement.gen.h"
#include "JSCanvasRenderingContext2D.gen.h"

JSClassID qjs_canvasrenderingcontext2d_class_id;

extern struct wisp_table *guit;

#define QJS_CANVAS_MAGIC 0x43414E42
#define QJS_CANVAS_CONTEXT_MAGIC 0x43414E56

typedef struct CanvasState {
    colour fill_colour;
    colour stroke_colour;
    float global_alpha;
    float line_width;
    struct CanvasState *next;
} CanvasState;

typedef struct CanvasContext2DPrivate {
    uint32_t magic;
    struct dom_node *canvas_node;   /* Associated HTMLCanvasElement */
    struct bitmap *bitmap;
#ifdef WITH_BLEND2D
    BLContextCore bl_ctx_obj;
    struct blend2d_context b2d_ctx;
    BLPathCore current_path;
#endif
    struct redraw_context redraw_ctx;
    CanvasState *state_stack;

    /* Current state maintained for save/restore */
    colour fill_colour;
    colour stroke_colour;
    float global_alpha;
    float line_width;
} CanvasContext2DPrivate;

/* Internal helper to extract our private data from QJSNodePrivate */
static inline CanvasContext2DPrivate *get_canvas_cpriv(QJSNodePrivate *priv) {
    if (!priv || !priv->node) return NULL;
    CanvasContext2DPrivate *cpriv = (CanvasContext2DPrivate *)priv->node;
    if (cpriv->magic != QJS_CANVAS_CONTEXT_MAGIC) return NULL;
    return cpriv;
}

static void canvas_context_2d_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_canvasrenderingcontext2d_class_id);
    if (priv) {
        CanvasContext2DPrivate *cpriv = (CanvasContext2DPrivate *)priv->node;
        if (cpriv && cpriv->magic == QJS_CANVAS_CONTEXT_MAGIC) {
#ifdef WITH_BLEND2D
            bl_path_destroy(&cpriv->current_path);
            bl_context_end(&cpriv->bl_ctx_obj);
            bl_context_destroy(&cpriv->bl_ctx_obj);
#endif
            if (cpriv->canvas_node) dom_node_unref(cpriv->canvas_node);
            CanvasState *s = cpriv->state_stack;
            while (s) {
                CanvasState *next = s->next;
                free(s);
                s = next;
            }
            free(cpriv);
        }
        free(priv);
    }
}

static colour parse_color(const char *str)
{
    if (str[0] == '#') {
        uint32_t val = (uint32_t)strtol(str + 1, NULL, 16);
        if (strlen(str) == 7) {
            uint8_t r = (val >> 16) & 0xFF;
            uint8_t g = (val >> 8) & 0xFF;
            uint8_t b = val & 0xFF;
            return (0x00 << 24) | (b << 16) | (g << 8) | r;
        }
    }
    if (strcmp(str, "red") == 0) return 0x000000FF;
    if (strcmp(str, "green") == 0) return 0x0000FF00;
    if (strcmp(str, "blue") == 0) return 0x00FF0000;
    if (strcmp(str, "white") == 0) return 0x00FFFFFF;
    if (strcmp(str, "black") == 0) return 0x00000000;
    return 0x00000000;
}

static inline uint32_t colour_to_rgba32(colour c, float alpha)
{
    uint8_t r = c & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = (c >> 16) & 0xFF;
    uint8_t a = (uint8_t)(255 * alpha);
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static void canvas_bitmap_handler(dom_node_operation operation, dom_string *key, void *data, struct dom_node *src, struct dom_node *dst)
{
    struct bitmap *bitmap = data;
    if (operation == 3 /* DOM_NODE_DELETED */) {
        if (bitmap && guit->bitmap) guit->bitmap->destroy(bitmap);
    }
}

JSValue wisp_htmlcanvaselement_getContext_impl(JSContext *ctx, QJSNodePrivate *priv, const char * contextId, JSValue arguments)
{
    if (!priv || !priv->node) return JS_NULL;
    if (strcmp(contextId, "2d") != 0) return JS_NULL;

    JSValue element_obj = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue existing = JS_GetPropertyStr(ctx, element_obj, "__canvas_context_2d");
    if (JS_IsObject(existing)) {
        JS_FreeValue(ctx, element_obj);
        return existing;
    }
    JS_FreeValue(ctx, existing);

    struct bitmap *bitmap = NULL;
    dom_exception exc = dom_node_get_user_data(priv->node, corestring_dom___ns_key_canvas_node_data, &bitmap);
    if (exc != DOM_NO_ERR || bitmap == NULL) {
        int width = 300, height = 150;
        dom_string *w_attr = NULL, *h_attr = NULL;
        dom_element_get_attribute((dom_element *)priv->node, corestring_dom_width, &w_attr);
        if (w_attr) { ns_strtoint((const char *)dom_string_data(w_attr), 10, &width); dom_string_unref(w_attr); }
        dom_element_get_attribute((dom_element *)priv->node, corestring_dom_height, &h_attr);
        if (h_attr) { ns_strtoint((const char *)dom_string_data(h_attr), 10, &height); dom_string_unref(h_attr); }
        bitmap = (struct bitmap *)guit->bitmap->create(width, height, BITMAP_CLEAR);
        if (!bitmap) { JS_FreeValue(ctx, element_obj); return JS_ThrowInternalError(ctx, "Failed to create canvas bitmap"); }
        dom_node_set_user_data(priv->node, corestring_dom___ns_key_canvas_node_data, bitmap, canvas_bitmap_handler, NULL);
    }

    CanvasContext2DPrivate *cpriv = calloc(1, sizeof(*cpriv));
    if (!cpriv) { JS_FreeValue(ctx, element_obj); return JS_ThrowOutOfMemory(ctx); }
    cpriv->magic = QJS_CANVAS_CONTEXT_MAGIC;
    cpriv->canvas_node = (struct dom_node *)priv->node;
    dom_node_ref(cpriv->canvas_node);
    cpriv->bitmap = bitmap;
    cpriv->fill_colour = 0xFF000000; cpriv->stroke_colour = 0xFF000000;
    cpriv->global_alpha = 1.0f; cpriv->line_width = 1.0f;

#ifdef WITH_BLEND2D
    bl_context_init(&cpriv->bl_ctx_obj);
    cpriv->b2d_ctx.bl_ctx = &cpriv->bl_ctx_obj;
    bl_path_init(&cpriv->current_path);
    BLImageCore img;
    void *pixel_data = guit->bitmap->get_buffer(bitmap);
    int w = guit->bitmap->get_width(bitmap), h = guit->bitmap->get_height(bitmap);
    size_t stride = guit->bitmap->get_rowstride(bitmap);
    bl_image_init_as_from_data(&img, w, h, BL_FORMAT_PRGB32, pixel_data, (intptr_t)stride, BL_DATA_ACCESS_RW, NULL, NULL);
    bl_context_begin(cpriv->b2d_ctx.bl_ctx, &img, NULL);
    bl_image_destroy(&img);
    cpriv->redraw_ctx.plot = &blend2d_plotters;
    cpriv->redraw_ctx.priv = &cpriv->b2d_ctx;
#endif

    JSValue obj = JS_NewObjectClass(ctx, qjs_canvasrenderingcontext2d_class_id);
    QJSNodePrivate *qpriv = calloc(1, sizeof(*qpriv));
    if (!qpriv) { free(cpriv); JS_FreeValue(ctx, obj); JS_FreeValue(ctx, element_obj); return JS_ThrowOutOfMemory(ctx); }
    qpriv->magic = QJS_DOM_MAGIC;
    qpriv->node = cpriv;
    qpriv->ctx = ctx;
    qpriv->is_dom_node = false;
    JS_SetOpaque(obj, qpriv);

    JS_SetPropertyStr(ctx, element_obj, "__canvas_context_2d", JS_DupValue(ctx, obj));
    JS_FreeValue(ctx, element_obj);

    return obj;
}

JSValue wisp_htmlcanvaselement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    int width = 300;
    dom_string *w_attr = NULL;
    dom_element_get_attribute((dom_element *)priv->node, corestring_dom_width, &w_attr);
    if (w_attr) { ns_strtoint((const char *)dom_string_data(w_attr), 10, &width); dom_string_unref(w_attr); }
    return JS_NewInt32(ctx, width);
}

JSValue wisp_htmlcanvaselement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32]; snprintf(buf, sizeof(buf), "%u", value);
    dom_string *w_dom = NULL; dom_string_create((const uint8_t *)buf, strlen(buf), &w_dom);
    dom_element_set_attribute((dom_element *)priv->node, corestring_dom_width, w_dom);
    dom_string_unref(w_dom);
    return JS_UNDEFINED;
}

JSValue wisp_htmlcanvaselement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    int height = 150;
    dom_string *h_attr = NULL;
    dom_element_get_attribute((dom_element *)priv->node, corestring_dom_height, &h_attr);
    if (h_attr) { ns_strtoint((const char *)dom_string_data(h_attr), 10, &height); dom_string_unref(h_attr); }
    return JS_NewInt32(ctx, height);
}

JSValue wisp_htmlcanvaselement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32]; snprintf(buf, sizeof(buf), "%u", value);
    dom_string *h_dom = NULL; dom_string_create((const uint8_t *)buf, strlen(buf), &h_dom);
    dom_element_set_attribute((dom_element *)priv->node, corestring_dom_height, h_dom);
    dom_string_unref(h_dom);
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_fillStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", cpriv->fill_colour & 0xFF, (cpriv->fill_colour >> 8) & 0xFF, (cpriv->fill_colour >> 16) & 0xFF);
    return JS_NewString(ctx, buf);
}

JSValue wisp_canvasrenderingcontext2d_fillStyle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, value);
    if (str) { cpriv->fill_colour = parse_color(str); JS_FreeCString(ctx, str); }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_strokeStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", cpriv->stroke_colour & 0xFF, (cpriv->stroke_colour >> 8) & 0xFF, (cpriv->stroke_colour >> 16) & 0xFF);
    return JS_NewString(ctx, buf);
}

JSValue wisp_canvasrenderingcontext2d_strokeStyle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, value);
    if (str) { cpriv->stroke_colour = parse_color(str); JS_FreeCString(ctx, str); }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_lineWidth_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return cpriv ? JS_NewFloat64(ctx, cpriv->line_width) : JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_lineWidth_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) cpriv->line_width = (float)value;
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_globalAlpha_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return cpriv ? JS_NewFloat64(ctx, cpriv->global_alpha) : JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_globalAlpha_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) cpriv->global_alpha = (float)value;
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_save_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    CanvasState *s = malloc(sizeof(*s));
    if (!s) return JS_ThrowOutOfMemory(ctx);
    s->fill_colour = cpriv->fill_colour; s->stroke_colour = cpriv->stroke_colour;
    s->global_alpha = cpriv->global_alpha; s->line_width = cpriv->line_width;
    s->next = cpriv->state_stack; cpriv->state_stack = s;
#ifdef WITH_BLEND2D
    bl_context_save(&cpriv->bl_ctx_obj, NULL);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_restore_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv || !cpriv->state_stack) return JS_UNDEFINED;
    CanvasState *s = cpriv->state_stack; cpriv->state_stack = s->next;
    cpriv->fill_colour = s->fill_colour; cpriv->stroke_colour = s->stroke_colour;
    cpriv->global_alpha = s->global_alpha; cpriv->line_width = s->line_width;
#ifdef WITH_BLEND2D
    bl_context_restore(&cpriv->bl_ctx_obj, NULL);
#endif
    free(s); return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_fillRect_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double w, double h)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    BLRect r = { x, y, w, h };
    bl_context_set_fill_style_rgba32(&cpriv->bl_ctx_obj, colour_to_rgba32(cpriv->fill_colour, cpriv->global_alpha));
    bl_context_fill_rect_d(&cpriv->bl_ctx_obj, &r);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_strokeRect_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double w, double h)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    BLRect r = { x, y, w, h };
    bl_context_set_stroke_style_rgba32(&cpriv->bl_ctx_obj, colour_to_rgba32(cpriv->stroke_colour, cpriv->global_alpha));
    bl_context_set_stroke_width(&cpriv->bl_ctx_obj, cpriv->line_width);
    bl_context_stroke_rect_d(&cpriv->bl_ctx_obj, &r);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_clearRect_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double w, double h)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    BLRect r = { x, y, w, h };
    bl_context_save(&cpriv->bl_ctx_obj, NULL);
    bl_context_set_comp_op(&cpriv->bl_ctx_obj, BL_COMP_OP_SRC_COPY);
    bl_context_fill_rect_d_rgba32(&cpriv->bl_ctx_obj, &r, 0x00000000);
    bl_context_restore(&cpriv->bl_ctx_obj, NULL);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_beginPath_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_path_clear(&cpriv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_moveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_path_move_to(&cpriv->current_path, x, y);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_lineTo_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_path_line_to(&cpriv->current_path, x, y);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_closePath_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_path_close(&cpriv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_fill_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue fillRule)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_context_set_fill_style_rgba32(&cpriv->bl_ctx_obj, colour_to_rgba32(cpriv->fill_colour, cpriv->global_alpha));
        bl_context_fill_path_d(&cpriv->bl_ctx_obj, NULL, &cpriv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_fill_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, JSValue fillRule) { return JS_UNDEFINED; }

JSValue wisp_canvasrenderingcontext2d_stroke_0_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_context_set_stroke_style_rgba32(&cpriv->bl_ctx_obj, colour_to_rgba32(cpriv->stroke_colour, cpriv->global_alpha));
        bl_context_set_stroke_width(&cpriv->bl_ctx_obj, cpriv->line_width);
        bl_context_stroke_path_d(&cpriv->bl_ctx_obj, NULL, &cpriv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_stroke_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path) { return JS_UNDEFINED; }

JSValue wisp_canvasrenderingcontext2d_arc_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double radius, double startAngle, double endAngle, bool anticlockwise)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    double sweep = endAngle - startAngle;
    double two_pi = 2.0 * M_PI;

    if (!anticlockwise) {
        if (sweep >= two_pi) sweep = two_pi;
        else {
            sweep = fmod(sweep, two_pi);
            if (sweep < 0) sweep += two_pi;
        }
    } else {
        if (sweep <= -two_pi) sweep = -two_pi;
        else {
            sweep = fmod(sweep, two_pi);
            if (sweep > 0) sweep -= two_pi;
        }
    }

    bl_path_arc_to(&cpriv->current_path, x, y, radius, radius, startAngle, sweep, false);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_translate_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        double d[2] = { x, y }; bl_context_apply_transform_op(&cpriv->bl_ctx_obj, BL_TRANSFORM_OP_POST_TRANSLATE, d);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_scale_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        double d[2] = { x, y }; bl_context_apply_transform_op(&cpriv->bl_ctx_obj, BL_TRANSFORM_OP_POST_SCALE, d);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_rotate_impl(JSContext *ctx, QJSNodePrivate *priv, double angle)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_context_apply_transform_op(&cpriv->bl_ctx_obj, BL_TRANSFORM_OP_POST_ROTATE, &angle);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_transform_impl(JSContext *ctx, QJSNodePrivate *priv, double a, double b, double c, double d, double e, double f)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        BLMatrix2D m = { a, b, c, d, e, f }; bl_context_apply_transform_op(&cpriv->bl_ctx_obj, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_setTransform_impl(JSContext *ctx, QJSNodePrivate *priv, double a, double b, double c, double d, double e, double f)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        BLMatrix2D m = { a, b, c, d, e, f }; bl_context_apply_transform_op(&cpriv->bl_ctx_obj, BL_TRANSFORM_OP_ASSIGN, &m);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_resetTransform_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_context_apply_transform_op(&cpriv->bl_ctx_obj, BL_TRANSFORM_OP_RESET, NULL);
#endif
    }
    return JS_UNDEFINED;
}

static JSValue drawImage_internal(JSContext *ctx, QJSNodePrivate *priv, JSValue image, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    QJSNodePrivate *img_priv = qjs_get_dom_priv(ctx, image);
    if (!img_priv || !img_priv->node) return JS_ThrowTypeError(ctx, "Invalid image source");
    struct bitmap *bitmap = NULL;
    dom_exception exc = dom_node_get_user_data(img_priv->node, corestring_dom___ns_key_canvas_node_data, &bitmap);
    if (!bitmap) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    BLImageCore img;
    void *pixel_data = guit->bitmap->get_buffer(bitmap);
    int w = guit->bitmap->get_width(bitmap), h = guit->bitmap->get_height(bitmap);
    size_t stride = guit->bitmap->get_rowstride(bitmap);
    bool opaque = guit->bitmap->get_opaque(bitmap);
    bl_image_init_as_from_data(&img, w, h, opaque ? BL_FORMAT_XRGB32 : BL_FORMAT_PRGB32, pixel_data, (intptr_t)stride, BL_DATA_ACCESS_READ, NULL, NULL);

    if (sw < 0) sw = (double)w;
    if (sh < 0) sh = (double)h;
    if (dw < 0) dw = (double)w;
    if (dh < 0) dh = (double)h;

    BLRect dst_rect = { dx, dy, dw, dh }; BLRectI src_rect = { (int)sx, (int)sy, (int)sw, (int)sh };
    bl_context_blit_scaled_image_d(&cpriv->bl_ctx_obj, &dst_rect, &img, &src_rect);
    bl_image_destroy(&img);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_drawImage_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, double dx, double dy)
{ return drawImage_internal(ctx, priv, image, 0, 0, -1, -1, dx, dy, -1, -1); }

JSValue wisp_canvasrenderingcontext2d_drawImage_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, double dx, double dy, double dw, double dh)
{ return drawImage_internal(ctx, priv, image, 0, 0, -1, -1, dx, dy, dw, dh); }

JSValue wisp_canvasrenderingcontext2d_drawImage_2_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, double sx, double sy, double sw, double sh, double dx, double dy, double dw, double dh)
{ return drawImage_internal(ctx, priv, image, sx, sy, sw, sh, dx, dy, dw, dh); }

JSValue wisp_canvasrenderingcontext2d_canvas_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv || !cpriv->canvas_node) return JS_NULL;
    return qjs_wrap_node(ctx, cpriv->canvas_node);
}

JSValue wisp_canvasrenderingcontext2d_lineCap_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "butt"); }
JSValue wisp_canvasrenderingcontext2d_lineCap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_lineJoin_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "miter"); }
JSValue wisp_canvasrenderingcontext2d_lineJoin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_miterLimit_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewFloat64(ctx, 10.0); }
JSValue wisp_canvasrenderingcontext2d_miterLimit_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_quadraticCurveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double cpx, double cpy, double x, double y)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_path_quad_to(&cpriv->current_path, cpx, cpy, x, y);
#endif
    }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_bezierCurveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_path_cubic_to(&cpriv->current_path, cp1x, cp1y, cp2x, cp2y, x, y);
#endif
    }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_rect_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double w, double h)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        BLRect r = { x, y, w, h };
        bl_path_add_geometry(&cpriv->current_path, BL_GEOMETRY_TYPE_RECTD, &r, NULL, BL_GEOMETRY_DIRECTION_CW);
#endif
    }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_ellipse_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool anticlockwise) { return JS_UNDEFINED; }

JSValue wisp_canvasrenderingcontext2d_arcTo_0_impl(JSContext *ctx, QJSNodePrivate *priv, double x1, double y1, double x2, double y2, double radius)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    if (radius < 0) return JS_ThrowRangeError(ctx, "Negative radius in arcTo");

    BLPoint p0 = { 0, 0 };
    size_t size = bl_path_get_size(&cpriv->current_path);
    if (size > 0) {
        const BLPoint *pts = bl_path_get_vertex_data(&cpriv->current_path);
        p0 = pts[size - 1];
    } else {
        bl_path_move_to(&cpriv->current_path, x1, y1);
        return JS_UNDEFINED;
    }

    double v01x = x1 - p0.x, v01y = y1 - p0.y;
    double v21x = x1 - x2, v21y = y1 - y2;
    double d01 = sqrt(v01x * v01x + v01y * v01y);
    double d21 = sqrt(v21x * v21x + v21y * v21y);

    if (d01 < 1e-6 || d21 < 1e-6 || radius < 1e-6) {
        bl_path_line_to(&cpriv->current_path, x1, y1);
        return JS_UNDEFINED;
    }

    double cos_phi = (v01x * v21x + v01y * v21y) / (d01 * d21);
    if (cos_phi > 0.999999 || cos_phi < -0.999999) {
        bl_path_line_to(&cpriv->current_path, x1, y1);
        return JS_UNDEFINED;
    }

    double phi = acos(cos_phi);
    double dist = radius / tan(phi / 2.0);

    double tx1 = x1 - dist * v01x / d01;
    double ty1 = y1 - dist * v01y / d01;
    double tx2 = x1 - dist * v21x / d21;
    double ty2 = y1 - dist * v21y / d21;

    double cp_dist = sqrt(dist * dist + radius * radius);
    double v_bisect_x = (v01x / d01 + v21x / d21);
    double v_bisect_y = (v01y / d01 + v21y / d21);
    double d_bisect = sqrt(v_bisect_x * v_bisect_x + v_bisect_y * v_bisect_y);
    double cx = x1 - cp_dist * v_bisect_x / d_bisect;
    double cy = y1 - cp_dist * v_bisect_y / d_bisect;

    double start_angle = atan2(ty1 - cy, tx1 - cx);
    double end_angle = atan2(ty2 - cy, tx2 - cx);
    double sweep = end_angle - start_angle;

    if (v01x * v21y - v01y * v21x < 0) {
        if (sweep > 0) sweep -= 2 * M_PI;
    } else {
        if (sweep < 0) sweep += 2 * M_PI;
    }

    bl_path_line_to(&cpriv->current_path, tx1, ty1);
    bl_path_arc_to(&cpriv->current_path, cx, cy, radius, radius, start_angle, sweep, false);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_arcTo_1_impl(JSContext *ctx, QJSNodePrivate *priv, double x1, double y1, double x2, double y2, double radiusX, double radiusY, double rotation) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_clip_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue fillRule) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_clip_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, JSValue fillRule) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_isPointInPath_0_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, JSValue fillRule) { return JS_FALSE; }
JSValue wisp_canvasrenderingcontext2d_isPointInPath_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, double x, double y, JSValue fillRule) { return JS_FALSE; }

/* Missing stubs for build completeness */
JSValue wisp_canvasrenderingcontext2d_addHitRegion_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue options) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_clearHitRegions_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_commit_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_createImageData_0_impl(JSContext *ctx, QJSNodePrivate *priv, double sw, double sh) { return JS_NULL; }
JSValue wisp_canvasrenderingcontext2d_createImageData_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * imagedata) { return JS_NULL; }
JSValue wisp_canvasrenderingcontext2d_currentTransform_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_canvasrenderingcontext2d_currentTransform_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_drawFocusIfNeeded_0_impl(JSContext *ctx, QJSNodePrivate *priv, void * element) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_drawFocusIfNeeded_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, void * element) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_fillText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, double x, double y, double maxWidth) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_getImageData_impl(JSContext *ctx, QJSNodePrivate *priv, double sx, double sy, double sw, double sh) { return JS_NULL; }
JSValue wisp_canvasrenderingcontext2d_globalCompositeOperation_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "source-over"); }
JSValue wisp_canvasrenderingcontext2d_globalCompositeOperation_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_imageSmoothingEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_TRUE; }
JSValue wisp_canvasrenderingcontext2d_imageSmoothingEnabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_imageSmoothingQuality_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "low"); }
JSValue wisp_canvasrenderingcontext2d_imageSmoothingQuality_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_isPointInStroke_0_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y) { return JS_FALSE; }
JSValue wisp_canvasrenderingcontext2d_isPointInStroke_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, double x, double y) { return JS_FALSE; }
JSValue wisp_canvasrenderingcontext2d_measureText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text) { return JS_NULL; }
JSValue wisp_canvasrenderingcontext2d_putImageData_0_impl(JSContext *ctx, QJSNodePrivate *priv, void * imagedata, double dx, double dy) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_putImageData_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * imagedata, double dx, double dy, double dirtyX, double dirtyY, double dirtyWidth, double dirtyHeight) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_removeHitRegion_impl(JSContext *ctx, QJSNodePrivate *priv, const char * id) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_resetClip_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_scrollPathIntoView_0_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_scrollPathIntoView_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_shadowBlur_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewFloat64(ctx, 0.0); }
JSValue wisp_canvasrenderingcontext2d_shadowBlur_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_shadowColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "rgba(0,0,0,0)"); }
JSValue wisp_canvasrenderingcontext2d_shadowColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_shadowOffsetX_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewFloat64(ctx, 0.0); }
JSValue wisp_canvasrenderingcontext2d_shadowOffsetX_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_shadowOffsetY_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewFloat64(ctx, 0.0); }
JSValue wisp_canvasrenderingcontext2d_shadowOffsetY_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_strokeText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, double x, double y, double maxWidth) { return JS_UNDEFINED; }

int qjs_init_canvas(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_canvasrenderingcontext2d_class_id == 0) {
        JS_NewClassID(rt, &qjs_canvasrenderingcontext2d_class_id);
    }

    JSClassDef qjs_canvas_context_2d_class_manual = {
        "CanvasRenderingContext2D",
        .finalizer = canvas_context_2d_finalizer,
    };

    if (!JS_IsRegisteredClass(rt, qjs_canvasrenderingcontext2d_class_id)) {
        JS_NewClass(rt, qjs_canvasrenderingcontext2d_class_id, &qjs_canvas_context_2d_class_manual);
    }

    qjs_init_htmlcanvaselement_gen(ctx);
    qjs_init_canvasrenderingcontext2d_gen(ctx);
    return 0;
}
