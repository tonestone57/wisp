/* Implementation for Console */
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSConsole.gen.h"

static JSValue wisp_console_log_internal(JSContext *ctx, const char *msg, void * subst, const char *level)
{
    if (msg) {
        NSLOG(wisp, INFO, "Console [%s]: %s", level, msg);
    }
    return JS_UNDEFINED;
}

JSValue wisp_console_debug_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, void * subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "DEBUG");
}

JSValue wisp_console_error_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, void * subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "ERROR");
}

JSValue wisp_console_info_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, void * subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "INFO");
}

JSValue wisp_console_log_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, void * subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "LOG");
}

JSValue wisp_console_warn_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, void * subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "WARN");
}

JSValue wisp_console_trace_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_console_log_internal(ctx, "trace() called", NULL, "TRACE");
}

JSValue wisp_console_group_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_console_groupCollapsed_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_console_groupEnd_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_console_time_impl(JSContext *ctx, QJSNodePrivate *priv, const char * timerName) { return JS_UNDEFINED; }
JSValue wisp_console_timeEnd_impl(JSContext *ctx, QJSNodePrivate *priv, const char * timerName) { return JS_UNDEFINED; }
JSValue wisp_console_dir_impl(JSContext *ctx, QJSNodePrivate *priv, void * object) { return JS_UNDEFINED; }

int qjs_init_console(JSContext *ctx)
{
    /* Initialize the class and prototype using the generated function */
    qjs_init_console_gen(ctx);

    /* Add the "console" property to the global object. */
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue console = qjs_new_console(ctx, NULL, false);
    if (JS_IsException(console)) {
        NSLOG(wisp, ERROR, "Failed to create console object");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }
    JS_SetPropertyStr(ctx, global_obj, "console", console);
    JS_FreeValue(ctx, global_obj);
    return 0;
}
