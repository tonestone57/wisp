#include "quickjs.h"

/* Implementation for Window */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "dom_bridge.h"
#include "crypto.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
JSClassID qjs_window_class_id;

static void js_window_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_window_class_id);
    if (priv) {
        free(priv);
    }
}

static JSValue js_window_captureEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_releaseEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_getComputedStyle(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_matchMedia(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_moveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_moveBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_resizeTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_resizeBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_scroll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_scrollTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_scrollBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_alert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_confirm(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_TRUE; }
static JSValue js_window_prompt(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_atob(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_btoa(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_requestAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NewInt32(ctx, 1); }
static JSValue js_window_cancelAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_setTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_clearTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_setInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }
static JSValue js_window_clearInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_UNDEFINED; }
static JSValue js_window_createImageBitmap(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) { return JS_NULL; }

#define WINDOW_EVENT_GETSET(name) \
static JSValue js_window_on##name##_get(JSContext *ctx, JSValueConst this_val) { return JS_NULL; } \
static JSValue js_window_on##name##_set(JSContext *ctx, JSValueConst this_val, JSValueConst val) { return JS_UNDEFINED; }

WINDOW_EVENT_GETSET(abort)
WINDOW_EVENT_GETSET(autocomplete)
WINDOW_EVENT_GETSET(autocompleteerror)
WINDOW_EVENT_GETSET(blur)
WINDOW_EVENT_GETSET(cancel)
WINDOW_EVENT_GETSET(canplay)
WINDOW_EVENT_GETSET(canplaythrough)
WINDOW_EVENT_GETSET(change)
WINDOW_EVENT_GETSET(click)
WINDOW_EVENT_GETSET(close)
WINDOW_EVENT_GETSET(contextmenu)
WINDOW_EVENT_GETSET(cuechange)
WINDOW_EVENT_GETSET(dblclick)
WINDOW_EVENT_GETSET(drag)
WINDOW_EVENT_GETSET(dragend)
WINDOW_EVENT_GETSET(dragenter)
WINDOW_EVENT_GETSET(dragexit)
WINDOW_EVENT_GETSET(dragleave)
WINDOW_EVENT_GETSET(dragover)
WINDOW_EVENT_GETSET(dragstart)
WINDOW_EVENT_GETSET(drop)
WINDOW_EVENT_GETSET(durationchange)
WINDOW_EVENT_GETSET(emptied)
WINDOW_EVENT_GETSET(ended)
WINDOW_EVENT_GETSET(error)
WINDOW_EVENT_GETSET(focus)
WINDOW_EVENT_GETSET(input)
WINDOW_EVENT_GETSET(invalid)
WINDOW_EVENT_GETSET(keydown)
WINDOW_EVENT_GETSET(keypress)
WINDOW_EVENT_GETSET(keyup)
WINDOW_EVENT_GETSET(load)
WINDOW_EVENT_GETSET(loadeddata)
WINDOW_EVENT_GETSET(loadedmetadata)
WINDOW_EVENT_GETSET(loadstart)
WINDOW_EVENT_GETSET(mousedown)
WINDOW_EVENT_GETSET(mouseenter)
WINDOW_EVENT_GETSET(mouseleave)
WINDOW_EVENT_GETSET(mousemove)
WINDOW_EVENT_GETSET(mouseout)
WINDOW_EVENT_GETSET(mouseover)
WINDOW_EVENT_GETSET(mouseup)
WINDOW_EVENT_GETSET(wheel)
WINDOW_EVENT_GETSET(pause)
WINDOW_EVENT_GETSET(play)
WINDOW_EVENT_GETSET(playing)
WINDOW_EVENT_GETSET(progress)
WINDOW_EVENT_GETSET(ratechange)
WINDOW_EVENT_GETSET(reset)
WINDOW_EVENT_GETSET(resize)
WINDOW_EVENT_GETSET(scroll)
WINDOW_EVENT_GETSET(seeked)
WINDOW_EVENT_GETSET(seeking)
WINDOW_EVENT_GETSET(select)
WINDOW_EVENT_GETSET(show)
WINDOW_EVENT_GETSET(sort)
WINDOW_EVENT_GETSET(stalled)
WINDOW_EVENT_GETSET(submit)
WINDOW_EVENT_GETSET(suspend)
WINDOW_EVENT_GETSET(timeupdate)
WINDOW_EVENT_GETSET(toggle)
WINDOW_EVENT_GETSET(volumechange)
WINDOW_EVENT_GETSET(waiting)
WINDOW_EVENT_GETSET(afterprint)
WINDOW_EVENT_GETSET(beforeprint)
WINDOW_EVENT_GETSET(beforeunload)
WINDOW_EVENT_GETSET(hashchange)
WINDOW_EVENT_GETSET(languagechange)
WINDOW_EVENT_GETSET(message)
WINDOW_EVENT_GETSET(offline)
WINDOW_EVENT_GETSET(online)
WINDOW_EVENT_GETSET(pagehide)
WINDOW_EVENT_GETSET(pageshow)
WINDOW_EVENT_GETSET(popstate)
WINDOW_EVENT_GETSET(storage)
WINDOW_EVENT_GETSET(unload)

static JSValue js_window_sessionStorage_get(JSContext *ctx, JSValueConst this_val) { return JS_NULL; }
static JSValue js_window_localStorage_get(JSContext *ctx, JSValueConst this_val) { return JS_NULL; }

#include "window.inc"
int qjs_init_window(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_window_class_id == 0) JS_NewClassID(rt, &qjs_window_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_window_class_id)) JS_NewClass(rt, qjs_window_class_id, &js_window_class);

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyFunctionList(ctx, global_obj, js_window_proto_funcs, sizeof(js_window_proto_funcs) / sizeof(js_window_proto_funcs[0]));

    /* Set self references */
    JS_SetPropertyStr(ctx, global_obj, "window", JS_DupValue(ctx, global_obj));
    JS_SetPropertyStr(ctx, global_obj, "self", JS_DupValue(ctx, global_obj));

    JS_FreeValue(ctx, global_obj);
    return 0;
}

JSValue qjs_new_window(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_GetGlobalObject(ctx);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->node = node; priv->is_dom_node = is_dom_node;
    JS_SetOpaque(obj, priv); return obj;
}
