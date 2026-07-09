#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "JSImageData.gen.h"

JSClassID qjs_imagedata_class_id;

static void imagedata_free_buffer(JSRuntime *rt, void *opaque, void *ptr)
{
    free(ptr);
}

static void imagedata_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_imagedata_class_id);
    if (priv) {
        ImageDataPrivate *idpriv = (ImageDataPrivate *)priv->node;
        if (idpriv) {
            JS_FreeValueRT(rt, idpriv->data);
            free(idpriv);
        }
        free(priv);
    }
}

static void imagedata_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_imagedata_class_id);
    if (priv) {
        ImageDataPrivate *idpriv = (ImageDataPrivate *)priv->node;
        if (idpriv) {
            JS_MarkValue(rt, idpriv->data, mark_func);
        }
    }
}

static JSClassDef wisp_imagedata_class = {
    "ImageData",
    .finalizer = imagedata_finalizer,
    .gc_mark = imagedata_mark,
};

JSValue wisp_imagedata_width_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    ImageDataPrivate *idpriv = (ImageDataPrivate *)priv->node;
    return JS_NewUint32(ctx, idpriv->width);
}

JSValue wisp_imagedata_height_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    ImageDataPrivate *idpriv = (ImageDataPrivate *)priv->node;
    return JS_NewUint32(ctx, idpriv->height);
}

JSValue wisp_imagedata_data_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    ImageDataPrivate *idpriv = (ImageDataPrivate *)priv->node;
    return JS_DupValue(ctx, idpriv->data);
}

static JSValue create_imagedata_object(JSContext *ctx, uint32_t w, uint32_t h, JSValue data)
{
    ImageDataPrivate *idpriv = calloc(1, sizeof(*idpriv));
    if (!idpriv) {
        JS_FreeValue(ctx, data);
        return JS_ThrowOutOfMemory(ctx);
    }
    idpriv->width = w;
    idpriv->height = h;
    idpriv->data = data; // Takes ownership

    JSValue obj = JS_NewObjectClass(ctx, qjs_imagedata_class_id);
    if (JS_IsException(obj)) {
        JS_FreeValue(ctx, idpriv->data);
        free(idpriv);
        return obj;
    }

    QJSNodePrivate *qpriv = calloc(1, sizeof(*qpriv));
    if (!qpriv) {
        JS_FreeValue(ctx, idpriv->data);
        free(idpriv);
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    qpriv->magic = QJS_DOM_MAGIC;
    qpriv->node = idpriv;
    qpriv->ctx = ctx;
    qpriv->is_dom_node = false;
    JS_SetOpaque(obj, qpriv);

    return obj;
}

JSValue wisp_imagedata_constructor_0_impl(JSContext *ctx, uint32_t sw, uint32_t sh)
{
    if (sw == 0 || sh == 0) return JS_ThrowRangeError(ctx, "Invalid dimensions");
    size_t size = (size_t)sw * sh * 4;
    uint8_t *buf = calloc(1, size);
    if (!buf) return JS_ThrowOutOfMemory(ctx);

    JSValue array_buf = JS_NewArrayBuffer(ctx, buf, size, imagedata_free_buffer, NULL, false);
    if (JS_IsException(array_buf)) {
        free(buf);
        return array_buf;
    }
    JSValue data = JS_NewTypedArray(ctx, 1, &array_buf, JS_TYPED_ARRAY_UINT8C);
    JS_FreeValue(ctx, array_buf);
    if (JS_IsException(data)) return data;

    return create_imagedata_object(ctx, sw, sh, data);
}

JSValue wisp_imagedata_constructor_1_impl(JSContext *ctx, JSValue data, uint32_t sw, uint32_t sh)
{
    return create_imagedata_object(ctx, sw, sh, JS_DupValue(ctx, data));
}

int qjs_init_imagedata(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_imagedata_class_id == 0) {
        JS_NewClassID(rt, &qjs_imagedata_class_id);
    }

    if (!JS_IsRegisteredClass(rt, qjs_imagedata_class_id)) {
        JS_NewClass(rt, qjs_imagedata_class_id, &wisp_imagedata_class);
    }

    qjs_init_imagedata_gen(ctx);
    return 0;
}
