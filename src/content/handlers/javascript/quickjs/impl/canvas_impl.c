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
#include <wisp/layout.h>
#include "utils/libdom.h"

static void canvas_free_buffer(JSRuntime *rt, void *opaque, void *ptr) { free(ptr); }

/* Forward declarations for generated headers */
#include "JSHTMLCanvasElement.gen.h"
#include "JSCanvasRenderingContext2D.gen.h"
#include "JSCanvasGradient.gen.h"
#include "JSCanvasPattern.gen.h"

extern bool wisp_is_js_process;

JSClassID qjs_canvasrenderingcontext2d_class_id;
JSClassID qjs_canvasgradient_class_id;
JSClassID qjs_canvaspattern_class_id;

typedef struct CanvasGradientPrivate {
    double x0, y0, x1, y1;
    double r0, r1; // for radial
    bool is_radial;
    int count;
    struct {
        double offset;
        char *color;
    } stops[32];
} CanvasGradientPrivate;

typedef struct CanvasPatternPrivate {
    JSValue image;
    char *repetition;
} CanvasPatternPrivate;

extern struct wisp_table *guit;

#define QJS_CANVAS_MAGIC 0x43414E42
#define QJS_CANVAS_CONTEXT_MAGIC 0x43414E56

typedef struct CanvasState {
    colour fill_colour;
    colour stroke_colour;
    float global_alpha;
    float line_width;
    JSValue fill_style_val;
    JSValue stroke_style_val;
    char font[128];
    char textAlign[32];
    char textBaseline[32];
    char direction[32];
    char lineCap[32];
    char lineJoin[32];
    double miterLimit;
    double lineDashOffset;
    double line_dash[16];
    int line_dash_count;
    char shadowColor[64];
    double shadowBlur;
    double shadowOffsetX;
    double shadowOffsetY;
    char globalCompositeOperation[64];
    char filter[64];
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

    JSValue fill_style_val;
    JSValue stroke_style_val;

    char font[128];
    char textAlign[32];
    char textBaseline[32];
    char direction[32];
    char lineCap[32];
    char lineJoin[32];
    double miterLimit;
    double lineDashOffset;
    double line_dash[16];
    int line_dash_count;
    char shadowColor[64];
    double shadowBlur;
    double shadowOffsetX;
    double shadowOffsetY;
    char globalCompositeOperation[64];
    char filter[64];
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
            if (cpriv->canvas_node && !wisp_is_js_process) dom_node_unref(cpriv->canvas_node);
            CanvasState *s = cpriv->state_stack;
            while (s) {
                CanvasState *next = s->next;
                JS_FreeValueRT(rt, s->fill_style_val);
                JS_FreeValueRT(rt, s->stroke_style_val);
                free(s);
                s = next;
            }
            JS_FreeValueRT(rt, cpriv->fill_style_val);
            JS_FreeValueRT(rt, cpriv->stroke_style_val);
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

static void init_canvas_cpriv_defaults(CanvasContext2DPrivate *cpriv) {
    cpriv->fill_colour = 0xFF000000; cpriv->stroke_colour = 0xFF000000;
    cpriv->global_alpha = 1.0f; cpriv->line_width = 1.0f;
    cpriv->fill_style_val = JS_UNDEFINED;
    cpriv->stroke_style_val = JS_UNDEFINED;
    snprintf(cpriv->font, sizeof(cpriv->font), "10px sans-serif");
    snprintf(cpriv->textAlign, sizeof(cpriv->textAlign), "start");
    snprintf(cpriv->textBaseline, sizeof(cpriv->textBaseline), "alphabetic");
    snprintf(cpriv->direction, sizeof(cpriv->direction), "inherit");
    snprintf(cpriv->lineCap, sizeof(cpriv->lineCap), "butt");
    snprintf(cpriv->lineJoin, sizeof(cpriv->lineJoin), "miter");
    cpriv->miterLimit = 10.0;
    cpriv->lineDashOffset = 0.0;
    cpriv->line_dash_count = 0;
    snprintf(cpriv->shadowColor, sizeof(cpriv->shadowColor), "rgba(0, 0, 0, 0)");
    cpriv->shadowBlur = 0.0;
    cpriv->shadowOffsetX = 0.0;
    cpriv->shadowOffsetY = 0.0;
    snprintf(cpriv->globalCompositeOperation, sizeof(cpriv->globalCompositeOperation), "source-over");
    snprintf(cpriv->filter, sizeof(cpriv->filter), "none");
}

JSValue wisp_htmlcanvaselement_getContext_impl(JSContext *ctx, QJSNodePrivate *priv, const char * contextId, JSValue arguments)
{
    if (!priv || !priv->node) return JS_NULL;

    if (strcmp(contextId, "webgl") == 0 || strcmp(contextId, "experimental-webgl") == 0) {
        JSValue element_obj = qjs_wrap_node(ctx, (dom_node *)priv->node);
        JSValue existing = JS_GetPropertyStr(ctx, element_obj, "__webgl_context");
        if (JS_IsObject(existing)) {
            JS_FreeValue(ctx, element_obj);
            return existing;
        }
        JS_FreeValue(ctx, existing);

        JSValue global = JS_GetGlobalObject(ctx);
        JSValue ctor = JS_GetPropertyStr(ctx, global, "WebGLRenderingContext");
        JSValue ctx_obj = JS_UNDEFINED;
        if (JS_IsFunction(ctx, ctor)) {
            JSValue args[1] = { element_obj };
            ctx_obj = JS_CallConstructor(ctx, ctor, 1, args);
        } else {
            ctx_obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, ctx_obj, "canvas", JS_DupValue(ctx, element_obj));
        }
        JS_FreeValue(ctx, global);
        JS_FreeValue(ctx, ctor);

        if (!JS_IsException(ctx_obj) && !JS_IsUndefined(ctx_obj) && !JS_IsNull(ctx_obj)) {
            JS_SetPropertyStr(ctx, element_obj, "__webgl_context", JS_DupValue(ctx, ctx_obj));
        }
        JS_FreeValue(ctx, element_obj);
        return ctx_obj;
    }

    if (strcmp(contextId, "webgl2") == 0 || strcmp(contextId, "experimental-webgl2") == 0) {
        JSValue element_obj = qjs_wrap_node(ctx, (dom_node *)priv->node);
        JSValue existing = JS_GetPropertyStr(ctx, element_obj, "__webgl2_context");
        if (JS_IsObject(existing)) {
            JS_FreeValue(ctx, element_obj);
            return existing;
        }
        JS_FreeValue(ctx, existing);

        JSValue global = JS_GetGlobalObject(ctx);
        JSValue ctor = JS_GetPropertyStr(ctx, global, "WebGL2RenderingContext");
        if (!JS_IsFunction(ctx, ctor)) {
            JS_FreeValue(ctx, ctor);
            ctor = JS_GetPropertyStr(ctx, global, "WebGLRenderingContext");
        }
        JSValue ctx_obj = JS_UNDEFINED;
        if (JS_IsFunction(ctx, ctor)) {
            JSValue args[1] = { element_obj };
            ctx_obj = JS_CallConstructor(ctx, ctor, 1, args);
        } else {
            ctx_obj = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, ctx_obj, "canvas", JS_DupValue(ctx, element_obj));
        }
        JS_FreeValue(ctx, global);
        JS_FreeValue(ctx, ctor);

        if (!JS_IsException(ctx_obj) && !JS_IsUndefined(ctx_obj) && !JS_IsNull(ctx_obj)) {
            JS_SetPropertyStr(ctx, element_obj, "__webgl2_context", JS_DupValue(ctx, ctx_obj));
        }
        JS_FreeValue(ctx, element_obj);
        return ctx_obj;
    }

    if (strcmp(contextId, "2d") != 0) return JS_NULL;

    JSValue element_obj = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue existing = JS_GetPropertyStr(ctx, element_obj, "__canvas_context_2d");
    if (JS_IsObject(existing)) {
        JS_FreeValue(ctx, element_obj);
        return existing;
    }
    JS_FreeValue(ctx, existing);

    if (wisp_is_js_process || guit == NULL || guit->bitmap == NULL) {
        extern JSValue qjs_new_canvasrenderingcontext2d(JSContext *ctx, void *node, bool is_dom_node);
        CanvasContext2DPrivate *cpriv = calloc(1, sizeof(*cpriv));
        if (!cpriv) { JS_FreeValue(ctx, element_obj); return JS_ThrowOutOfMemory(ctx); }
        cpriv->magic = QJS_CANVAS_CONTEXT_MAGIC;
        cpriv->canvas_node = (struct dom_node *)priv->node;
        if (!wisp_is_js_process) dom_node_ref(cpriv->canvas_node);
        cpriv->bitmap = NULL;
        init_canvas_cpriv_defaults(cpriv);

        JSValue context_obj = qjs_new_canvasrenderingcontext2d(ctx, cpriv, false);
        JS_SetPropertyStr(ctx, element_obj, "__canvas_context_2d", JS_DupValue(ctx, context_obj));
        JS_FreeValue(ctx, element_obj);
        return context_obj;
    }

    struct bitmap *bitmap = NULL;
    dom_exception exc = DOM_NO_ERR;
    if (corestring_dom___ns_key_canvas_node_data != NULL) {
        exc = dom_node_get_user_data(priv->node, corestring_dom___ns_key_canvas_node_data, &bitmap);
    }
    if (exc != DOM_NO_ERR || bitmap == NULL) {
        int width = 300, height = 150;
        dom_string *w_attr = NULL, *h_attr = NULL;
        if (corestring_dom_width != NULL) {
            dom_element_get_attribute((dom_element *)priv->node, corestring_dom_width, &w_attr);
        }
        if (w_attr) { ns_strtoint((const char *)dom_string_data(w_attr), 10, &width); dom_string_unref(w_attr); }
        if (corestring_dom_height != NULL) {
            dom_element_get_attribute((dom_element *)priv->node, corestring_dom_height, &h_attr);
        }
        if (h_attr) { ns_strtoint((const char *)dom_string_data(h_attr), 10, &height); dom_string_unref(h_attr); }
        bitmap = (struct bitmap *)guit->bitmap->create(width, height, BITMAP_CLEAR);
        if (!bitmap) { JS_FreeValue(ctx, element_obj); return JS_ThrowInternalError(ctx, "Failed to create canvas bitmap"); }
        if (corestring_dom___ns_key_canvas_node_data != NULL) {
            dom_node_set_user_data(priv->node, corestring_dom___ns_key_canvas_node_data, bitmap, canvas_bitmap_handler, NULL);
        }
    }

    CanvasContext2DPrivate *cpriv = calloc(1, sizeof(*cpriv));
    if (!cpriv) { JS_FreeValue(ctx, element_obj); return JS_ThrowOutOfMemory(ctx); }
    cpriv->magic = QJS_CANVAS_CONTEXT_MAGIC;
    cpriv->canvas_node = (struct dom_node *)priv->node;
    if (!wisp_is_js_process) dom_node_ref(cpriv->canvas_node);
    cpriv->bitmap = bitmap;
    init_canvas_cpriv_defaults(cpriv);

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
    if (!priv || !priv->node) return JS_NewInt32(ctx, 300);
    JSValue attr = wisp_element_getAttribute_impl(ctx, priv, "width");
    int width = 300;
    if (JS_IsString(attr)) {
        const char *str = JS_ToCString(ctx, attr);
        if (str && *str) {
            width = atoi(str);
            if (width <= 0) width = 300;
        }
        if (str) JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, attr);
    return JS_NewInt32(ctx, width);
}

JSValue wisp_htmlcanvaselement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32]; snprintf(buf, sizeof(buf), "%u", value);
    return wisp_element_setAttribute_impl(ctx, priv, "width", buf);
}

JSValue wisp_htmlcanvaselement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 150);
    JSValue attr = wisp_element_getAttribute_impl(ctx, priv, "height");
    int height = 150;
    if (JS_IsString(attr)) {
        const char *str = JS_ToCString(ctx, attr);
        if (str && *str) {
            height = atoi(str);
            if (height <= 0) height = 150;
        }
        if (str) JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, attr);
    return JS_NewInt32(ctx, height);
}

JSValue wisp_htmlcanvaselement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    char buf[32]; snprintf(buf, sizeof(buf), "%u", value);
    return wisp_element_setAttribute_impl(ctx, priv, "height", buf);
}

JSValue wisp_canvasrenderingcontext2d_fillStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    if (!JS_IsUndefined(cpriv->fill_style_val)) {
        return JS_DupValue(ctx, cpriv->fill_style_val);
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", cpriv->fill_colour & 0xFF, (cpriv->fill_colour >> 8) & 0xFF, (cpriv->fill_colour >> 16) & 0xFF);
    return JS_NewString(ctx, buf);
}

JSValue wisp_canvasrenderingcontext2d_fillStyle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    if (JS_IsObject(value)) {
        JS_FreeValue(ctx, cpriv->fill_style_val);
        cpriv->fill_style_val = JS_DupValue(ctx, value);
    } else {
        const char *str = JS_ToCString(ctx, value);
        if (str) {
            cpriv->fill_colour = parse_color(str);
            JS_FreeCString(ctx, str);
            JS_FreeValue(ctx, cpriv->fill_style_val);
            cpriv->fill_style_val = JS_DupValue(ctx, value);
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_strokeStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    if (!JS_IsUndefined(cpriv->stroke_style_val)) {
        return JS_DupValue(ctx, cpriv->stroke_style_val);
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x", cpriv->stroke_colour & 0xFF, (cpriv->stroke_colour >> 8) & 0xFF, (cpriv->stroke_colour >> 16) & 0xFF);
    return JS_NewString(ctx, buf);
}

JSValue wisp_canvasrenderingcontext2d_strokeStyle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    if (JS_IsObject(value)) {
        JS_FreeValue(ctx, cpriv->stroke_style_val);
        cpriv->stroke_style_val = JS_DupValue(ctx, value);
    } else {
        const char *str = JS_ToCString(ctx, value);
        if (str) {
            cpriv->stroke_colour = parse_color(str);
            JS_FreeCString(ctx, str);
            JS_FreeValue(ctx, cpriv->stroke_style_val);
            cpriv->stroke_style_val = JS_DupValue(ctx, value);
        }
    }
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
    s->fill_style_val = JS_DupValue(ctx, cpriv->fill_style_val);
    s->stroke_style_val = JS_DupValue(ctx, cpriv->stroke_style_val);
    snprintf(s->font, sizeof(s->font), "%s", cpriv->font);
    snprintf(s->textAlign, sizeof(s->textAlign), "%s", cpriv->textAlign);
    snprintf(s->textBaseline, sizeof(s->textBaseline), "%s", cpriv->textBaseline);
    snprintf(s->direction, sizeof(s->direction), "%s", cpriv->direction);
    snprintf(s->lineCap, sizeof(s->lineCap), "%s", cpriv->lineCap);
    snprintf(s->lineJoin, sizeof(s->lineJoin), "%s", cpriv->lineJoin);
    s->miterLimit = cpriv->miterLimit;
    s->lineDashOffset = cpriv->lineDashOffset;
    s->line_dash_count = cpriv->line_dash_count;
    memcpy(s->line_dash, cpriv->line_dash, sizeof(cpriv->line_dash));
    snprintf(s->shadowColor, sizeof(s->shadowColor), "%s", cpriv->shadowColor);
    s->shadowBlur = cpriv->shadowBlur;
    s->shadowOffsetX = cpriv->shadowOffsetX;
    s->shadowOffsetY = cpriv->shadowOffsetY;
    snprintf(s->globalCompositeOperation, sizeof(s->globalCompositeOperation), "%s", cpriv->globalCompositeOperation);
    snprintf(s->filter, sizeof(s->filter), "%s", cpriv->filter);
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

    JS_FreeValue(ctx, cpriv->fill_style_val);
    cpriv->fill_style_val = s->fill_style_val; // transfers ownership

    JS_FreeValue(ctx, cpriv->stroke_style_val);
    cpriv->stroke_style_val = s->stroke_style_val; // transfers ownership

    snprintf(cpriv->font, sizeof(cpriv->font), "%s", s->font);
    snprintf(cpriv->textAlign, sizeof(cpriv->textAlign), "%s", s->textAlign);
    snprintf(cpriv->textBaseline, sizeof(cpriv->textBaseline), "%s", s->textBaseline);
    snprintf(cpriv->direction, sizeof(cpriv->direction), "%s", s->direction);
    snprintf(cpriv->lineCap, sizeof(cpriv->lineCap), "%s", s->lineCap);
    snprintf(cpriv->lineJoin, sizeof(cpriv->lineJoin), "%s", s->lineJoin);
    cpriv->miterLimit = s->miterLimit;
    cpriv->lineDashOffset = s->lineDashOffset;
    cpriv->line_dash_count = s->line_dash_count;
    memcpy(cpriv->line_dash, s->line_dash, sizeof(s->line_dash));
    snprintf(cpriv->shadowColor, sizeof(cpriv->shadowColor), "%s", s->shadowColor);
    cpriv->shadowBlur = s->shadowBlur;
    cpriv->shadowOffsetX = s->shadowOffsetX;
    cpriv->shadowOffsetY = s->shadowOffsetY;
    snprintf(cpriv->globalCompositeOperation, sizeof(cpriv->globalCompositeOperation), "%s", s->globalCompositeOperation);
    snprintf(cpriv->filter, sizeof(cpriv->filter), "%s", s->filter);

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
    CanvasContext2DPrivate *canvas_ctx = get_canvas_cpriv(priv);
    if (!canvas_ctx) return JS_ThrowTypeError(ctx, "Invalid CanvasRenderingContext2D target");
#ifdef WITH_BLEND2D
    BLMatrix2D m = { a, b, c, d, e, f };
    bl_context_apply_transform_op(&canvas_ctx->bl_ctx_obj, BL_TRANSFORM_OP_POST_TRANSFORM, &m);
#endif
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
JSValue wisp_canvasrenderingcontext2d_ellipse_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool anticlockwise)
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

    BLMatrix2D m;
    blMatrix2DSetTranslation(&m, x, y);
    blMatrix2DApplyRotation(&m, rotation);
    blMatrix2DApplyScale(&m, radiusX, radiusY);

    bl_path_add_geometry(&cpriv->current_path, BL_GEOMETRY_TYPE_ARC, &((BLArc){0, 0, 1, 1, startAngle, sweep}), &m, BL_GEOMETRY_DIRECTION_CW);
#endif
    return JS_UNDEFINED;
}

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
JSValue wisp_canvasrenderingcontext2d_clip_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue fillRule)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
        bool evenodd = false;
        if (JS_IsString(fillRule)) {
            const char *rule = JS_ToCString(ctx, fillRule);
            if (rule) {
                if (strcmp(rule, "evenodd") == 0) evenodd = true;
                JS_FreeCString(ctx, rule);
            }
        }
#ifdef WITH_BLEND2D
        if (evenodd) {
            bl_path_set_fill_rule(&cpriv->current_path, BL_FILL_RULE_EVEN_ODD);
        } else {
            bl_path_set_fill_rule(&cpriv->current_path, BL_FILL_RULE_NON_ZERO);
        }
        bl_context_clip_path_d(&cpriv->bl_ctx_obj, &cpriv->current_path);
#endif
    }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_clip_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, JSValue fillRule)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && path) {
        bool evenodd = false;
        if (JS_IsString(fillRule)) {
            const char *rule = JS_ToCString(ctx, fillRule);
            if (rule) {
                if (strcmp(rule, "evenodd") == 0) evenodd = true;
                JS_FreeCString(ctx, rule);
            }
        }
#ifdef WITH_BLEND2D
        if (evenodd) {
            bl_path_set_fill_rule((BLPathCore *)path, BL_FILL_RULE_EVEN_ODD);
        } else {
            bl_path_set_fill_rule((BLPathCore *)path, BL_FILL_RULE_NON_ZERO);
        }
        bl_context_clip_path_d(&cpriv->bl_ctx_obj, (BLPathCore *)path);
#endif
    }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_isPointInPath_0_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, JSValue fillRule) { return JS_FALSE; }
JSValue wisp_canvasrenderingcontext2d_isPointInPath_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, double x, double y, JSValue fillRule) { return JS_FALSE; }

/* Missing stubs for build completeness */
JSValue wisp_canvasrenderingcontext2d_addHitRegion_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue options) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_clearHitRegions_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_commit_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_createImageData_0_impl(JSContext *ctx, QJSNodePrivate *priv, double sw, double sh)
{
    int w = (int)sw;
    int h = (int)sh;
    if (w <= 0 || h <= 0) return JS_ThrowRangeError(ctx, "Invalid dimensions");

    size_t size = (size_t)w * h * 4;
    uint8_t *data = calloc(1, size);
    if (!data) return JS_ThrowOutOfMemory(ctx);

    JSValue buffer = JS_NewArrayBuffer(ctx, data, size, canvas_free_buffer, NULL, false);
    if (JS_IsException(buffer)) {
        free(data);
        return buffer;
    }

    JSValue args[3];
    args[0] = buffer;
    args[1] = JS_NewInt32(ctx, 0);
    args[2] = JS_NewInt32(ctx, w * h * 4);
    JSValue array = JS_NewTypedArray(ctx, 3, args, JS_TYPED_ARRAY_UINT8C);
    JS_FreeValue(ctx, args[1]);
    JS_FreeValue(ctx, args[2]);
    JS_FreeValue(ctx, buffer);
    if (JS_IsException(array)) return array;

    JSValue ret = wisp_imagedata_constructor_1_impl(ctx, array, w, h);
    JS_FreeValue(ctx, array);
    return ret;
}

JSValue wisp_canvasrenderingcontext2d_createImageData_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * imagedata)
{
    if (!imagedata) return JS_NULL;
    ImageDataPrivate *src = (ImageDataPrivate *)imagedata;
    return wisp_canvasrenderingcontext2d_createImageData_0_impl(ctx, priv, src->width, src->height);
}
JSValue wisp_canvasrenderingcontext2d_currentTransform_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_canvasrenderingcontext2d_currentTransform_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_drawFocusIfNeeded_0_impl(JSContext *ctx, QJSNodePrivate *priv, void * element) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_drawFocusIfNeeded_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, void * element) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_fillText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, double x, double y, double maxWidth)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    NSLOG(wisp, INFO, "Canvas.fillText: %s at (%f, %f)", text, x, y);
    /* Basic stub: just draw a small rectangle where the text would be */
    wisp_canvasrenderingcontext2d_fillRect_impl(ctx, priv, x, y, (double)strlen(text) * 8.0, 12.0);
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_getImageData_impl(JSContext *ctx, QJSNodePrivate *priv, double sx, double sy, double sw, double sh)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_NULL;

    int w = (int)sw;
    int h = (int)sh;
    if (w <= 0 || h <= 0) return JS_ThrowRangeError(ctx, "Invalid dimensions");

    size_t size = (size_t)w * h * 4;
    uint8_t *data = malloc(size);
    if (!data) return JS_ThrowOutOfMemory(ctx);

    if (guit && guit->bitmap && cpriv->bitmap) {
        uint8_t *src_buf = guit->bitmap->get_buffer(cpriv->bitmap);
        int src_stride = guit->bitmap->get_rowstride(cpriv->bitmap);
        int src_w = guit->bitmap->get_width(cpriv->bitmap);
        int src_h = guit->bitmap->get_height(cpriv->bitmap);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int cur_x = (int)sx + x;
                int cur_y = (int)sy + y;
                if (cur_x >= 0 && cur_x < src_w && cur_y >= 0 && cur_y < src_h) {
                    uint32_t *pixel = (uint32_t *)(src_buf + cur_y * src_stride + cur_x * 4);
                    uint32_t rgba = *pixel;
                    data[(y * w + x) * 4 + 0] = (rgba >> 16) & 0xFF;
                    data[(y * w + x) * 4 + 1] = (rgba >> 8) & 0xFF;
                    data[(y * w + x) * 4 + 2] = rgba & 0xFF;
                    data[(y * w + x) * 4 + 3] = (rgba >> 24) & 0xFF;
                } else {
                    memset(data + (y * w + x) * 4, 0, 4);
                }
            }
        }
    } else {
        memset(data, 0, size);
    }

    JSValue array_buffer = JS_NewArrayBuffer(ctx, data, size, canvas_free_buffer, NULL, false);
    if (JS_IsException(array_buffer)) {
        free(data);
        return array_buffer;
    }
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue clamped_array_ctor = JS_GetPropertyStr(ctx, global, "Uint8ClampedArray");
    JSValue array = JS_CallConstructor(ctx, clamped_array_ctor, 1, &array_buffer);
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, clamped_array_ctor);
    JS_FreeValue(ctx, array_buffer);

    if (JS_IsException(array)) return array;

    JSValue ret = wisp_imagedata_constructor_1_impl(ctx, array, w, h);
    JS_FreeValue(ctx, array);
    return ret;
}
JSValue wisp_canvasrenderingcontext2d_font_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewString(ctx, (cpriv && cpriv->font[0]) ? cpriv->font : "10px sans-serif");
}
JSValue wisp_canvasrenderingcontext2d_font_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && value) { snprintf(cpriv->font, sizeof(cpriv->font), "%s", value); }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_textAlign_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewString(ctx, (cpriv && cpriv->textAlign[0]) ? cpriv->textAlign : "start");
}
JSValue wisp_canvasrenderingcontext2d_textAlign_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && value) { snprintf(cpriv->textAlign, sizeof(cpriv->textAlign), "%s", value); }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_textBaseline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewString(ctx, (cpriv && cpriv->textBaseline[0]) ? cpriv->textBaseline : "alphabetic");
}
JSValue wisp_canvasrenderingcontext2d_textBaseline_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && value) { snprintf(cpriv->textBaseline, sizeof(cpriv->textBaseline), "%s", value); }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_direction_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewString(ctx, (cpriv && cpriv->direction[0]) ? cpriv->direction : "inherit");
}
JSValue wisp_canvasrenderingcontext2d_direction_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && value) { snprintf(cpriv->direction, sizeof(cpriv->direction), "%s", value); }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_lineCap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewString(ctx, (cpriv && cpriv->lineCap[0]) ? cpriv->lineCap : "butt");
}
JSValue wisp_canvasrenderingcontext2d_lineCap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && value) { snprintf(cpriv->lineCap, sizeof(cpriv->lineCap), "%s", value); }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_lineJoin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewString(ctx, (cpriv && cpriv->lineJoin[0]) ? cpriv->lineJoin : "miter");
}
JSValue wisp_canvasrenderingcontext2d_lineJoin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && value) { snprintf(cpriv->lineJoin, sizeof(cpriv->lineJoin), "%s", value); }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_miterLimit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewFloat64(ctx, cpriv ? cpriv->miterLimit : 10.0);
}
JSValue wisp_canvasrenderingcontext2d_miterLimit_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) { cpriv->miterLimit = value; }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_lineDashOffset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewFloat64(ctx, cpriv ? cpriv->lineDashOffset : 0.0);
}
JSValue wisp_canvasrenderingcontext2d_lineDashOffset_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) { cpriv->lineDashOffset = value; }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_getLineDash_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    JSValue arr = JS_NewArray(ctx);
    if (cpriv) {
        for (int i = 0; i < cpriv->line_dash_count; i++) {
            JS_SetPropertyUint32(ctx, arr, i, JS_NewFloat64(ctx, cpriv->line_dash[i]));
        }
    }
    return arr;
}
JSValue wisp_canvasrenderingcontext2d_setLineDash_impl(JSContext *ctx, QJSNodePrivate *priv, double segments) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
        cpriv->line_dash[0] = segments;
        cpriv->line_dash_count = (segments > 0) ? 1 : 0;
    }
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_globalCompositeOperation_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    return JS_NewString(ctx, (cpriv && cpriv->globalCompositeOperation[0]) ? cpriv->globalCompositeOperation : "source-over");
}
JSValue wisp_canvasrenderingcontext2d_globalCompositeOperation_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv && value) { snprintf(cpriv->globalCompositeOperation, sizeof(cpriv->globalCompositeOperation), "%s", value); }
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_imageSmoothingEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_TRUE; }
JSValue wisp_canvasrenderingcontext2d_imageSmoothingEnabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_imageSmoothingQuality_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "low"); }
JSValue wisp_canvasrenderingcontext2d_imageSmoothingQuality_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_isPointInStroke_0_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y) { return JS_FALSE; }
JSValue wisp_canvasrenderingcontext2d_isPointInStroke_1_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, double x, double y) { return JS_FALSE; }
JSValue wisp_canvasrenderingcontext2d_measureText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text)
{
    CanvasContext2DPrivate *canvas_ctx = get_canvas_cpriv(priv);
    JSValue obj = JS_NewObject(ctx);
    int px_width = 0;
    if (text) {
        if (canvas_ctx && guit && guit->layout && guit->layout->width) {
            struct plot_font_style style = {0};
            guit->layout->width(&style, text, strlen(text), &px_width);
        } else {
            px_width = (int)strlen(text) * 10;
        }
    }
    double width = (double)px_width;
    JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, width));
    JS_SetPropertyStr(ctx, obj, "actualBoundingBoxLeft", JS_NewFloat64(ctx, 0.0));
    JS_SetPropertyStr(ctx, obj, "actualBoundingBoxRight", JS_NewFloat64(ctx, width));
    JS_SetPropertyStr(ctx, obj, "fontBoundingBoxAscent", JS_NewFloat64(ctx, 10.0));
    JS_SetPropertyStr(ctx, obj, "fontBoundingBoxDescent", JS_NewFloat64(ctx, 2.0));
    JS_SetPropertyStr(ctx, obj, "actualBoundingBoxAscent", JS_NewFloat64(ctx, 10.0));
    JS_SetPropertyStr(ctx, obj, "actualBoundingBoxDescent", JS_NewFloat64(ctx, 2.0));
    JS_SetPropertyStr(ctx, obj, "emHeightAscent", JS_NewFloat64(ctx, 10.0));
    JS_SetPropertyStr(ctx, obj, "emHeightDescent", JS_NewFloat64(ctx, 2.0));
    return obj;
}

JSValue wisp_canvasrenderingcontext2d_putImageData_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue imagedata_val, double dx, double dy)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    QJSNodePrivate *img_priv = qjs_get_dom_priv(ctx, imagedata_val);
    if (!img_priv || !img_priv->node) return JS_UNDEFINED;

    ImageDataPrivate *idpriv = (ImageDataPrivate *)img_priv->node;
    int w = idpriv->width;
    int h = idpriv->height;

    size_t offset, byte_length, bytes_per_element;
    JSValue buffer = JS_GetTypedArrayBuffer(ctx, idpriv->data, &offset, &byte_length, &bytes_per_element);
    if (JS_IsException(buffer)) return JS_UNDEFINED;

    size_t psize;
    uint8_t *data = JS_GetArrayBuffer(ctx, &psize, buffer);
    if (!data) {
        JS_FreeValue(ctx, buffer);
        return JS_UNDEFINED;
    }
    data += offset;

    uint8_t *dst_buf = guit->bitmap->get_buffer(cpriv->bitmap);
    int dst_stride = guit->bitmap->get_rowstride(cpriv->bitmap);
    int dst_w = guit->bitmap->get_width(cpriv->bitmap);
    int dst_h = guit->bitmap->get_height(cpriv->bitmap);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int cur_x = (int)dx + x;
            int cur_y = (int)dy + y;
            if (cur_x >= 0 && cur_x < dst_w && cur_y >= 0 && cur_y < dst_h) {
                uint32_t *pixel = (uint32_t *)(dst_buf + cur_y * dst_stride + cur_x * 4);
                uint8_t r = data[(y * w + x) * 4 + 0];
                uint8_t g = data[(y * w + x) * 4 + 1];
                uint8_t b = data[(y * w + x) * 4 + 2];
                uint8_t a = data[(y * w + x) * 4 + 3];
                *pixel = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    JS_FreeValue(ctx, buffer);

    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_putImageData_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue imagedata_val, double dx, double dy, double dirtyX, double dirtyY, double dirtyWidth, double dirtyHeight)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    QJSNodePrivate *img_priv = qjs_get_dom_priv(ctx, imagedata_val);
    if (!img_priv || !img_priv->node) return JS_UNDEFINED;

    ImageDataPrivate *idpriv = (ImageDataPrivate *)img_priv->node;
    int img_w = idpriv->width;
    int img_h = idpriv->height;

    // Handle negative dimensions per HTML Canvas spec
    if (dirtyWidth < 0) {
        dirtyX += dirtyWidth;
        dirtyWidth = -dirtyWidth;
    }
    if (dirtyHeight < 0) {
        dirtyY += dirtyHeight;
        dirtyHeight = -dirtyHeight;
    }

    // Clip dirty bounds against ImageData bounds
    int start_x = (int)fmax(0, dirtyX);
    int start_y = (int)fmax(0, dirtyY);
    int end_x   = (int)fmin(img_w, dirtyX + dirtyWidth);
    int end_y   = (int)fmin(img_h, dirtyY + dirtyHeight);

    if (start_x >= end_x || start_y >= end_y) {
        return JS_UNDEFINED; // Empty clipping rect
    }

    size_t offset, byte_length, bytes_per_element;
    JSValue buffer = JS_GetTypedArrayBuffer(ctx, idpriv->data, &offset, &byte_length, &bytes_per_element);
    if (JS_IsException(buffer)) return JS_UNDEFINED;

    size_t psize;
    uint8_t *data = JS_GetArrayBuffer(ctx, &psize, buffer);
    if (!data) {
        JS_FreeValue(ctx, buffer);
        return JS_UNDEFINED;
    }
    data += offset;

    uint8_t *dst_buf = guit->bitmap->get_buffer(cpriv->bitmap);
    int dst_stride = guit->bitmap->get_rowstride(cpriv->bitmap);
    int dst_w = guit->bitmap->get_width(cpriv->bitmap);
    int dst_h = guit->bitmap->get_height(cpriv->bitmap);

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int cur_x = (int)dx + x;
            int cur_y = (int)dy + y;
            if (cur_x >= 0 && cur_x < dst_w && cur_y >= 0 && cur_y < dst_h) {
                uint32_t *pixel = (uint32_t *)(dst_buf + cur_y * dst_stride + cur_x * 4);
                uint8_t r = data[(y * img_w + x) * 4 + 0];
                uint8_t g = data[(y * img_w + x) * 4 + 1];
                uint8_t b = data[(y * img_w + x) * 4 + 2];
                uint8_t a = data[(y * img_w + x) * 4 + 3];
                *pixel = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    }

    JS_FreeValue(ctx, buffer);
    return JS_UNDEFINED;
}
JSValue wisp_canvasrenderingcontext2d_removeHitRegion_impl(JSContext *ctx, QJSNodePrivate *priv, const char * id) { return JS_UNDEFINED; }
JSValue wisp_canvasrenderingcontext2d_resetClip_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (cpriv) {
#ifdef WITH_BLEND2D
        bl_context_restore_clip(&cpriv->bl_ctx_obj);
#endif
    }
    return JS_UNDEFINED;
}
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
JSValue wisp_canvasrenderingcontext2d_strokeText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, double x, double y, double maxWidth)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;
    NSLOG(wisp, INFO, "Canvas.strokeText: %s at (%f, %f)", text, x, y);
    /* Basic stub: just draw a small rectangle where the text would be */
    wisp_canvasrenderingcontext2d_strokeRect_impl(ctx, priv, x, y, (double)strlen(text) * 8.0, 12.0);
    return JS_UNDEFINED;
}

JSValue wisp_canvasrenderingcontext2d_createLinearGradient_impl(JSContext *ctx, QJSNodePrivate *priv, double x0, double y0, double x1, double y1)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_ThrowTypeError(ctx, "Invalid CanvasRenderingContext2D target");

    CanvasGradientPrivate *grad = calloc(1, sizeof(*grad));
    if (!grad) return JS_ThrowOutOfMemory(ctx);

    grad->x0 = x0; grad->y0 = y0; grad->x1 = x1; grad->y1 = y1;
    grad->is_radial = false;
    grad->count = 0;

    return qjs_new_canvasgradient(ctx, grad, false);
}

JSValue wisp_canvasrenderingcontext2d_createRadialGradient_impl(JSContext *ctx, QJSNodePrivate *priv, double x0, double y0, double r0, double x1, double y1, double r1)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    CanvasGradientPrivate *grad = calloc(1, sizeof(*grad));
    if (!grad) return JS_ThrowOutOfMemory(ctx);

    grad->x0 = x0; grad->y0 = y0; grad->r0 = r0;
    grad->x1 = x1; grad->y1 = y1; grad->r1 = r1;
    grad->is_radial = true;
    grad->count = 0;

    return qjs_new_canvasgradient(ctx, grad, false);
}

JSValue wisp_canvasrenderingcontext2d_createPattern_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, const char * repetition)
{
    CanvasContext2DPrivate *cpriv = get_canvas_cpriv(priv);
    if (!cpriv) return JS_UNDEFINED;

    CanvasPatternPrivate *pat = calloc(1, sizeof(*pat));
    if (!pat) return JS_ThrowOutOfMemory(ctx);

    pat->image = JS_DupValue(ctx, image);
    pat->repetition = repetition ? strdup(repetition) : strdup("repeat");
    if (!pat->repetition) {
        JS_FreeValue(ctx, pat->image);
        free(pat);
        return JS_ThrowOutOfMemory(ctx);
    }

    return qjs_new_canvaspattern(ctx, pat, false);
}

JSValue wisp_canvasgradient_addColorStop_impl(JSContext *ctx, QJSNodePrivate *priv, double offset, const char * color)
{
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid CanvasGradient target");
    CanvasGradientPrivate *grad = (CanvasGradientPrivate *)priv->node;

    if (offset < 0.0 || offset > 1.0 || isnan(offset)) {
        return JS_ThrowRangeError(ctx, "INDEX_SIZE_ERR: offset must be between 0.0 and 1.0");
    }

    if (!color) {
        return JS_ThrowTypeError(ctx, "SyntaxError: color is null");
    }

    if (grad->count < 32) {
        char *col_copy = strdup(color);
        if (!col_copy) return JS_ThrowOutOfMemory(ctx);
        grad->stops[grad->count].offset = offset;
        grad->stops[grad->count].color = col_copy;
        grad->count++;
    } else {
        return JS_ThrowOutOfMemory(ctx);
    }

    return JS_UNDEFINED;
}

JSValue wisp_canvaspattern_setTransform_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue transform)
{
    return JS_UNDEFINED;
}

static void canvas_context_2d_gc_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_canvasrenderingcontext2d_class_id);
    if (priv) {
        CanvasContext2DPrivate *cpriv = (CanvasContext2DPrivate *)priv->node;
        if (cpriv && cpriv->magic == QJS_CANVAS_CONTEXT_MAGIC) {
            JS_MarkValue(rt, cpriv->fill_style_val, mark_func);
            JS_MarkValue(rt, cpriv->stroke_style_val, mark_func);
            CanvasState *s = cpriv->state_stack;
            while (s) {
                JS_MarkValue(rt, s->fill_style_val, mark_func);
                JS_MarkValue(rt, s->stroke_style_val, mark_func);
                s = s->next;
            }
        }
    }
}

static void canvas_pattern_gc_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_canvaspattern_class_id);
    if (priv) {
        CanvasPatternPrivate *pat = (CanvasPatternPrivate *)priv->node;
        if (pat) {
            JS_MarkValue(rt, pat->image, mark_func);
        }
    }
}

static void canvas_gradient_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_canvasgradient_class_id);
    if (priv) {
        CanvasGradientPrivate *grad = (CanvasGradientPrivate *)priv->node;
        if (grad) {
            for (int i = 0; i < grad->count; i++) {
                free(grad->stops[i].color);
            }
            free(grad);
        }
        free(priv);
    }
}

int qjs_init_canvasgradient(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_canvasgradient_class_id == 0) {
        JS_NewClassID(rt, &qjs_canvasgradient_class_id);
    }

    JSClassDef qjs_canvasgradient_class_manual = {
        "CanvasGradient",
        .finalizer = canvas_gradient_finalizer,
    };

    if (!JS_IsRegisteredClass(rt, qjs_canvasgradient_class_id)) {
        JS_NewClass(rt, qjs_canvasgradient_class_id, &qjs_canvasgradient_class_manual);
    }

    return qjs_init_canvasgradient_gen(ctx);
}

static void canvas_pattern_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_canvaspattern_class_id);
    if (priv) {
        CanvasPatternPrivate *pat = (CanvasPatternPrivate *)priv->node;
        if (pat) {
            JS_FreeValueRT(rt, pat->image);
            free(pat->repetition);
            free(pat);
        }
        free(priv);
    }
}

int qjs_init_canvaspattern(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_canvaspattern_class_id == 0) {
        JS_NewClassID(rt, &qjs_canvaspattern_class_id);
    }

    JSClassDef qjs_canvaspattern_class_manual = {
        "CanvasPattern",
        .finalizer = canvas_pattern_finalizer,
        .gc_mark = canvas_pattern_gc_mark,
    };

    if (!JS_IsRegisteredClass(rt, qjs_canvaspattern_class_id)) {
        JS_NewClass(rt, qjs_canvaspattern_class_id, &qjs_canvaspattern_class_manual);
    }

    return qjs_init_canvaspattern_gen(ctx);
}

int qjs_init_canvasrenderingcontext2d(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_canvasrenderingcontext2d_class_id == 0) {
        JS_NewClassID(rt, &qjs_canvasrenderingcontext2d_class_id);
    }

    JSClassDef qjs_canvas_context_2d_class_manual = {
        "CanvasRenderingContext2D",
        .finalizer = canvas_context_2d_finalizer,
        .gc_mark = canvas_context_2d_gc_mark,
    };

    if (!JS_IsRegisteredClass(rt, qjs_canvasrenderingcontext2d_class_id)) {
        JS_NewClass(rt, qjs_canvasrenderingcontext2d_class_id, &qjs_canvas_context_2d_class_manual);
    }

    return qjs_init_canvasrenderingcontext2d_gen(ctx);
}

int qjs_init_canvas(JSContext *ctx)
{
    qjs_init_htmlcanvaselement(ctx);
    qjs_init_canvasrenderingcontext2d(ctx);
    qjs_init_canvasgradient(ctx);
    qjs_init_canvaspattern(ctx);
    return 0;
}





extern dom_string *g_qjs_node_key;

JSValue qjs_new_htmlcanvaselement(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_htmlcanvaselement_class_id);
    if (JS_IsException(obj)) return obj;
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) { JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC;
    priv->node = node;
    priv->is_dom_node = is_dom_node;
    priv->ctx = ctx;
    if (!wisp_is_js_process && is_dom_node && node) {
        dom_node_ref((dom_node *)node);
        extern dom_string *g_qjs_node_key;
        if (g_qjs_node_key) {
            dom_node_set_user_data((dom_node *)node, g_qjs_node_key, (void *)JS_VALUE_GET_PTR(obj), NULL, NULL);
        }
    }
    JS_SetOpaque(obj, priv);
    return obj;
}



int qjs_init_htmlcanvaselement(JSContext *ctx)
{
    qjs_init_htmlcanvaselement_gen(ctx);
    JSValue proto = JS_GetClassProto(ctx, qjs_htmlcanvaselement_class_id);
    JSValue htmlelement_proto = JS_GetClassProto(ctx, qjs_htmlelement_class_id);
    JS_SetPrototype(ctx, proto, htmlelement_proto);
    JS_FreeValue(ctx, htmlelement_proto);
    JS_FreeValue(ctx, proto);
    return 0;
}
