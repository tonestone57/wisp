#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
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
    struct dom_node *node;   /* Associated HTMLCanvasElement */
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

static void canvas_context_2d_finalizer(JSRuntime *rt, JSValue val)
{
    CanvasContext2DPrivate *priv = JS_GetOpaque(val, qjs_canvasrenderingcontext2d_class_id);
    if (priv) {
#ifdef WITH_BLEND2D
        bl_path_destroy(&priv->current_path);
        bl_context_end(&priv->bl_ctx_obj);
        bl_context_destroy(&priv->bl_ctx_obj);
#endif
        if (priv->node) dom_node_unref(priv->node);
        CanvasState *s = priv->state_stack;
        while (s) {
            CanvasState *next = s->next;
            free(s);
            s = next;
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

JSValue wisp_htmlcanvaselement_getContext_impl(JSContext *ctx, QJSNodePrivate *priv, const char * contextId)
{
    if (!priv || !priv->node) return JS_NULL;
    if (strcmp(contextId, "2d") != 0) return JS_NULL;

    /* Ensure identity stability: get existing context from JS element wrapper if present */
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
    cpriv->node = (struct dom_node *)priv->node;
    dom_node_ref(cpriv->node);
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
    JS_SetOpaque(obj, cpriv);

    /* Store it on the canvas element for reuse and identity stability */
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

JSValue wisp_canvasrenderingcontext2d_fillStyle_get_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (!priv) return JS_UNDEFINED;
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", priv->fill_colour & 0xFF, (priv->fill_colour >> 8) & 0xFF, (priv->fill_colour >> 16) & 0xFF);
    return JS_NewString(ctx, buf);
}

JSValue wisp_canvasrenderingcontext2d_fillStyle_set_impl(JSContext *ctx, CanvasContext2DPrivate *priv, JSValue value)
{
    if (!priv) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, value);
    if (str) { priv->fill_colour = parse_color(str); JS_FreeCString(ctx, str); }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_strokeStyle_get_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (!priv) return JS_UNDEFINED;
    char buf[16];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", priv->stroke_colour & 0xFF, (priv->stroke_colour >> 8) & 0xFF, (priv->stroke_colour >> 16) & 0xFF);
    return JS_NewString(ctx, buf);
}

JSValue wisp_canvasrenderingcontext2d_strokeStyle_set_impl(JSContext *ctx, CanvasContext2DPrivate *priv, JSValue value)
{
    if (!priv) return JS_UNDEFINED;
    const char *str = JS_ToCString(ctx, value);
    if (str) { priv->stroke_colour = parse_color(str); JS_FreeCString(ctx, str); }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_lineWidth_get_impl(JSContext *ctx, CanvasContext2DPrivate *priv) { return priv ? JS_NewFloat64(ctx, priv->line_width) : JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_lineWidth_set_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double value) { if (priv) priv->line_width = (float)value; return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_globalAlpha_get_impl(JSContext *ctx, CanvasContext2DPrivate *priv) { return priv ? JS_NewFloat64(ctx, priv->global_alpha) : JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_globalAlpha_set_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double value) { if (priv) priv->global_alpha = (float)value; return JS_UNDEFINED; }

JSValue wisp_canvasrenderingcontext2d_save_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (!priv) return JS_UNDEFINED;
    CanvasState *s = malloc(sizeof(*s));
    if (!s) return JS_ThrowOutOfMemory(ctx);
    s->fill_colour = priv->fill_colour; s->stroke_colour = priv->stroke_colour;
    s->global_alpha = priv->global_alpha; s->line_width = priv->line_width;
    s->next = priv->state_stack; priv->state_stack = s;
#ifdef WITH_BLEND2D
    bl_context_save(&priv->bl_ctx_obj, NULL);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_restore_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (!priv || !priv->state_stack) return JS_UNDEFINED;
    CanvasState *s = priv->state_stack; priv->state_stack = s->next;
    priv->fill_colour = s->fill_colour; priv->stroke_colour = s->stroke_colour;
    priv->global_alpha = s->global_alpha; priv->line_width = s->line_width;
#ifdef WITH_BLEND2D
    bl_context_restore(&priv->bl_ctx_obj, NULL);
#endif
    free(s); return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_fillRect_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y, double w, double h)
{
    if (!priv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    BLRect r = { x, y, w, h };
    bl_context_set_fill_style_rgba32(&priv->bl_ctx_obj, colour_to_rgba32(priv->fill_colour, priv->global_alpha));
    bl_context_fill_rect_d(&priv->bl_ctx_obj, &r);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_strokeRect_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y, double w, double h)
{
    if (!priv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    BLRect r = { x, y, w, h };
    bl_context_set_stroke_style_rgba32(&priv->bl_ctx_obj, colour_to_rgba32(priv->stroke_colour, priv->global_alpha));
    bl_context_set_stroke_width(&priv->bl_ctx_obj, priv->line_width);
    bl_context_stroke_rect_d(&priv->bl_ctx_obj, &r);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_clearRect_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y, double w, double h)
{
    if (!priv) return JS_UNDEFINED;
#ifdef WITH_BLEND2D
    BLRect r = { x, y, w, h };
    bl_context_save(&priv->bl_ctx_obj, NULL);
    bl_context_set_comp_op(&priv->bl_ctx_obj, BL_COMP_OP_SRC_COPY);
    bl_context_fill_rect_d_rgba32(&priv->bl_ctx_obj, &r, 0x00000000);
    bl_context_restore(&priv->bl_ctx_obj, NULL);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_beginPath_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_path_clear(&priv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_moveTo_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_path_move_to(&priv->current_path, x, y);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_lineTo_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_path_line_to(&priv->current_path, x, y);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_closePath_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_path_close(&priv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_fill_impl(JSContext *ctx, CanvasContext2DPrivate *priv, JSValue fillRule)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_context_set_fill_style_rgba32(&priv->bl_ctx_obj, colour_to_rgba32(priv->fill_colour, priv->global_alpha));
        bl_context_fill_path_d(&priv->bl_ctx_obj, NULL, &priv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_stroke_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_context_set_stroke_style_rgba32(&priv->bl_ctx_obj, colour_to_rgba32(priv->stroke_colour, priv->global_alpha));
        bl_context_set_stroke_width(&priv->bl_ctx_obj, priv->line_width);
        bl_context_stroke_path_d(&priv->bl_ctx_obj, NULL, &priv->current_path);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_arc_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y, double radius, double startAngle, double endAngle, bool anticlockwise)
{
    if (priv) {
#ifdef WITH_BLEND2D
        BLArc arc = { x, y, radius, radius, startAngle, endAngle - startAngle };
        bl_path_add_geometry(&priv->current_path, BL_GEOMETRY_TYPE_ARC, &arc, NULL, anticlockwise ? BL_GEOMETRY_DIRECTION_CCW : BL_GEOMETRY_DIRECTION_CW);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_translate_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y)
{
    if (priv) {
#ifdef WITH_BLEND2D
        double d[2] = { x, y }; bl_context_apply_transform_op(&priv->bl_ctx_obj, BL_TRANSFORM_OP_POST_TRANSLATE, d);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_scale_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x, double y)
{
    if (priv) {
#ifdef WITH_BLEND2D
        double d[2] = { x, y }; bl_context_apply_transform_op(&priv->bl_ctx_obj, BL_TRANSFORM_OP_POST_SCALE, d);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_rotate_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double angle)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_context_apply_transform_op(&priv->bl_ctx_obj, BL_TRANSFORM_OP_POST_ROTATE, &angle);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_transform_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double a, double b, double c, double d, double e, double f)
{
    if (priv) {
#ifdef WITH_BLEND2D
        BLMatrix2D m = { a, b, c, d, e, f }; bl_context_apply_transform_op(&priv->bl_ctx_obj, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_setTransform_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double a, double b, double c, double d, double e, double f)
{
    if (priv) {
#ifdef WITH_BLEND2D
        BLMatrix2D m = { a, b, c, d, e, f }; bl_context_apply_transform_op(&priv->bl_ctx_obj, BL_TRANSFORM_OP_ASSIGN, &m);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_resetTransform_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (priv) {
#ifdef WITH_BLEND2D
        bl_context_apply_transform_op(&priv->bl_ctx_obj, BL_TRANSFORM_OP_RESET, NULL);
#endif
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_drawImage_impl(JSContext *ctx, CanvasContext2DPrivate *priv, JSValue image, double dx, double dy, double dw, double dh, double sx, double sy, double sw, double sh)
{
    if (!priv) return JS_UNDEFINED;
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
    BLRect dst_rect = { dx, dy, dw, dh }; BLRectI src_rect = { (int)sx, (int)sy, (int)sw, (int)sh };
    bl_context_blit_scaled_image_d(&priv->bl_ctx_obj, &dst_rect, &img, &src_rect);
    bl_image_destroy(&img);
#endif
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_canvas_get_impl(JSContext *ctx, CanvasContext2DPrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    return qjs_wrap_node(ctx, priv->node);
}

JSValue wisp_canvasrenderingcontext2d_arcTo_impl(JSContext *ctx, CanvasContext2DPrivate *priv, double x1, double y1, double x2, double y2, double radius) { return JS_UNDEFINED; }

int qjs_init_canvas(JSContext *ctx)
{
    qjs_init_htmlcanvaselement_gen(ctx);
    qjs_init_canvasrenderingcontext2d_gen(ctx);
    return 0;
}
