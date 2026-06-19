/* Implementation for Window */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"

#include "window.inc"

static void js_window_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_window_class_id);
    if (priv) {
        free(priv);
    }
}

static JSValue js_window_captureEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_releaseEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_getComputedStyle(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.getComputedStyle() called (stub)");
    return JS_NULL;
}

static JSValue js_window_matchMedia(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.matchMedia() called (stub)");
    return JS_NULL;
}

static JSValue js_window_moveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_moveBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_resizeTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_resizeBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_scroll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_scrollTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_scrollBy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_alert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc > 0) {
        const char *str = JS_ToCString(ctx, argv[0]);
        if (str) {
            NSLOG(wisp, INFO, "Alert: %s", str);
            JS_FreeCString(ctx, str);
        }
    }
    return JS_UNDEFINED;
}

static JSValue js_window_confirm(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_TRUE;
}

static JSValue js_window_prompt(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_atob(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_btoa(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NULL;
}

static JSValue js_window_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_window_requestAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_NewInt32(ctx, 1);
}

static JSValue js_window_cancelAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

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

static JSValue js_window_setTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_setTimeout(ctx, this_val, argc, argv);
}

static JSValue js_window_setInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_setInterval(ctx, this_val, argc, argv);
}

static JSValue js_window_clearTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_clearTimeout(ctx, this_val, argc, argv);
}

static JSValue js_window_clearInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_clearInterval(ctx, this_val, argc, argv);
}

static JSValue js_window_createImageBitmap(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.createImageBitmap() called (stub)");
    return JS_NULL;
}

static JSValue js_window_localStorage_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Window.localStorage getter called (stub)");
    return JS_UNDEFINED;
}

static JSValue js_window_sessionStorage_get(JSContext *ctx, JSValueConst this_val)
{
    NSLOG(wisp, DEBUG, "Window.sessionStorage getter called (stub)");
    return JS_UNDEFINED;
}

/* Event handler stubs */
#define WINDOW_EVENT_STUB(name) \
static JSValue js_window_on##name##_get(JSContext *ctx, JSValueConst this_val) { return JS_NULL; } \
static JSValue js_window_on##name##_set(JSContext *ctx, JSValueConst this_val, JSValueConst val) { return JS_UNDEFINED; }

WINDOW_EVENT_STUB(abort)
WINDOW_EVENT_STUB(autocomplete)
WINDOW_EVENT_STUB(autocompleteerror)
WINDOW_EVENT_STUB(blur)
WINDOW_EVENT_STUB(cancel)
WINDOW_EVENT_STUB(canplay)
WINDOW_EVENT_STUB(canplaythrough)
WINDOW_EVENT_STUB(change)
WINDOW_EVENT_STUB(click)
WINDOW_EVENT_STUB(close)
WINDOW_EVENT_STUB(contextmenu)
WINDOW_EVENT_STUB(cuechange)
WINDOW_EVENT_STUB(dblclick)
WINDOW_EVENT_STUB(drag)
WINDOW_EVENT_STUB(dragend)
WINDOW_EVENT_STUB(dragenter)
WINDOW_EVENT_STUB(dragexit)
WINDOW_EVENT_STUB(dragleave)
WINDOW_EVENT_STUB(dragover)
WINDOW_EVENT_STUB(dragstart)
WINDOW_EVENT_STUB(drop)
WINDOW_EVENT_STUB(durationchange)
WINDOW_EVENT_STUB(emptied)
WINDOW_EVENT_STUB(ended)
WINDOW_EVENT_STUB(error)
WINDOW_EVENT_STUB(focus)
WINDOW_EVENT_STUB(input)
WINDOW_EVENT_STUB(invalid)
WINDOW_EVENT_STUB(keydown)
WINDOW_EVENT_STUB(keypress)
WINDOW_EVENT_STUB(keyup)
WINDOW_EVENT_STUB(load)
WINDOW_EVENT_STUB(loadeddata)
WINDOW_EVENT_STUB(loadedmetadata)
WINDOW_EVENT_STUB(loadstart)
WINDOW_EVENT_STUB(mousedown)
WINDOW_EVENT_STUB(mouseenter)
WINDOW_EVENT_STUB(mouseleave)
WINDOW_EVENT_STUB(mousemove)
WINDOW_EVENT_STUB(mouseout)
WINDOW_EVENT_STUB(mouseover)
WINDOW_EVENT_STUB(mouseup)
WINDOW_EVENT_STUB(wheel)
WINDOW_EVENT_STUB(pause)
WINDOW_EVENT_STUB(play)
WINDOW_EVENT_STUB(playing)
WINDOW_EVENT_STUB(progress)
WINDOW_EVENT_STUB(ratechange)
WINDOW_EVENT_STUB(reset)
WINDOW_EVENT_STUB(resize)
WINDOW_EVENT_STUB(scroll)
WINDOW_EVENT_STUB(seeked)
WINDOW_EVENT_STUB(seeking)
WINDOW_EVENT_STUB(select)
WINDOW_EVENT_STUB(show)
WINDOW_EVENT_STUB(sort)
WINDOW_EVENT_STUB(stalled)
WINDOW_EVENT_STUB(submit)
WINDOW_EVENT_STUB(suspend)
WINDOW_EVENT_STUB(timeupdate)
WINDOW_EVENT_STUB(toggle)
WINDOW_EVENT_STUB(waiting)
WINDOW_EVENT_STUB(afterprint)
WINDOW_EVENT_STUB(beforeprint)
WINDOW_EVENT_STUB(beforeunload)
WINDOW_EVENT_STUB(hashchange)
WINDOW_EVENT_STUB(languagechange)
WINDOW_EVENT_STUB(message)
WINDOW_EVENT_STUB(offline)
WINDOW_EVENT_STUB(online)
WINDOW_EVENT_STUB(pagehide)
WINDOW_EVENT_STUB(pageshow)
WINDOW_EVENT_STUB(popstate)
WINDOW_EVENT_STUB(storage)
WINDOW_EVENT_STUB(unload)
WINDOW_EVENT_STUB(volumechange)
