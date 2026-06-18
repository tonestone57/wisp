/* Implementation for Console */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"

#include "console.inc"

static void js_console_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_console_class_id);
    if (priv) {
        free(priv);
    }
}

static JSValue js_console_log_internal(JSContext *ctx, int argc, JSValueConst *argv, const char *level)
{
    for (int i = 0; i < argc; i++) {
        const char *str = JS_ToCString(ctx, argv[i]);
        if (str) {
            NSLOG(wisp, INFO, "Console [%s]: %s", level, str);
            JS_FreeCString(ctx, str);
        }
    }
    return JS_UNDEFINED;
}

static JSValue js_console_debug(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "DEBUG");
}

static JSValue js_console_error(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "ERROR");
}

static JSValue js_console_info(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "INFO");
}

static JSValue js_console_log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "LOG");
}

static JSValue js_console_warn(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "WARN");
}

static JSValue js_console_dir(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "DIR");
}

static JSValue js_console_dirxml(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "DIRXML");
}

static JSValue js_console_trace(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return js_console_log_internal(ctx, argc, argv, "TRACE");
}

static JSValue js_console_group(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_groupCollapsed(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_groupEnd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_time(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_timeEnd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_timeStamp(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_profile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_profileEnd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_assert(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc > 0 && !JS_ToBool(ctx, argv[0])) {
        js_console_log_internal(ctx, argc - 1, argv + 1, "ASSERT FAILED");
    }
    return JS_UNDEFINED;
}

static JSValue js_console_count(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_markTimeline(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_timeline(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

static JSValue js_console_timelineEnd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    return JS_UNDEFINED;
}

int qjs_init_console(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_console_class_id == 0) JS_NewClassID(rt, &qjs_console_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_console_class_id)) JS_NewClass(rt, qjs_console_class_id, &js_console_class);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyFunctionList(ctx, proto, js_console_proto_funcs, sizeof(js_console_proto_funcs) / sizeof(js_console_proto_funcs[0]));
    JS_SetClassProto(ctx, qjs_console_class_id, proto);

    JSValue console_obj = JS_NewObjectClass(ctx, qjs_console_class_id);
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global_obj, "console", console_obj);
    JS_FreeValue(ctx, global_obj);

    return 0;
}

JSValue qjs_new_console(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_console_class_id);
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) return JS_ThrowOutOfMemory(ctx);
    priv->node = node; priv->is_dom_node = is_dom_node;
    JS_SetOpaque(obj, priv); return obj;
}
