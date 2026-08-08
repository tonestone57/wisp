#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSConsole.gen.h"

static JSValue wisp_console_log_internal(JSContext *ctx, const char *msg, JSValue subst, const char *level)
{
    if (msg) {
        NSLOG(wisp, INFO, "Console [%s]: %s", level, msg);
    }

    return JS_UNDEFINED;
}

JSValue wisp_console_assert_impl(JSContext *ctx, QJSNodePrivate *priv, bool condition, JSValue subst)
{
    if (!condition) {
        JSValue len_val = JS_GetPropertyStr(ctx, subst, "length");
        int32_t len = 0;
        JS_ToInt32(ctx, &len, len_val);
        JS_FreeValue(ctx, len_val);

        if (len > 0) {
            JSValue first_arg = JS_GetPropertyUint32(ctx, subst, 0);
            const char *first_msg = JS_ToCString(ctx, first_arg);
            JS_FreeValue(ctx, first_arg);

            char *msg = NULL;
            if (first_msg) {
                size_t msg_len = strlen(first_msg) + 32;
                msg = malloc(msg_len);
                if (msg) {
                    snprintf(msg, msg_len, "Assertion failed: %s", first_msg);
                }
                JS_FreeCString(ctx, first_msg);
            }

            // Create a new subst array without the first element
            JSValue rest_subst = JS_NewArray(ctx);
            for (int i = 1; i < len; i++) {
                JSValue arg = JS_GetPropertyUint32(ctx, subst, i);
                JS_SetPropertyUint32(ctx, rest_subst, i - 1, arg);
            }

            wisp_console_log_internal(ctx, msg ? msg : "Assertion failed", rest_subst, "ASSERT_ERROR");
            JS_FreeValue(ctx, rest_subst);
            if (msg) free(msg);
        } else {
            wisp_console_log_internal(ctx, "Assertion failed", JS_UNDEFINED, "ASSERT_ERROR");
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_console_debug_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, JSValue subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "DEBUG");
}

JSValue wisp_console_error_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, JSValue subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "ERROR");
}

JSValue wisp_console_info_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, JSValue subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "INFO");
}

JSValue wisp_console_log_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, JSValue subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "LOG");
}

JSValue wisp_console_warn_impl(JSContext *ctx, QJSNodePrivate *priv, const char * msg, JSValue subst)
{
    return wisp_console_log_internal(ctx, msg, subst, "WARN");
}

JSValue wisp_console_trace_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_console_log_internal(ctx, "trace() called", JS_UNDEFINED, "TRACE");
}

JSValue wisp_console_group_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_console_groupCollapsed_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_console_groupEnd_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_console_time_impl(JSContext *ctx, QJSNodePrivate *priv, const char * timerName) { return JS_UNDEFINED; }
JSValue wisp_console_timeEnd_impl(JSContext *ctx, QJSNodePrivate *priv, const char * timerName) { return JS_UNDEFINED; }
JSValue wisp_console_dir_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue object) { return JS_UNDEFINED; }

int qjs_init_console(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_console_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_console_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_console_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_console_class_id, JS_DupValue(ctx, proto));
    }
    JS_FreeValue(ctx, proto);

    /* Add the "console" property to the global object. */
    JSValue console = qjs_new_console(ctx, NULL, false);
    if (JS_IsException(console)) {
        NSLOG(wisp, ERROR, "Failed to create console object");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }
    if (JS_DefinePropertyValueStr(ctx, global_obj, "console", console, JS_PROP_C_W_E) < 0) {
        NSLOG(wisp, ERROR, "Failed to define console property");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }

    /* Mark as initialized */
    if (JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_console_init", JS_TRUE, 0) < 0) {
        NSLOG(wisp, ERROR, "Failed to define __wisp_console_init property");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }
    JS_FreeValue(ctx, global_obj);

    return 0;
}

void qjs_console_cleanup(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue console = JS_GetPropertyStr(ctx, global_obj, "console");
    if (JS_IsObject(console)) {
        /* In browser environments, console usually stays until context dies.
         * For unit tests that create/destroy context manually, we might need
         * to explicitly null the global property to drop the reference. */
        JS_SetPropertyStr(ctx, global_obj, "console", JS_UNDEFINED);
    }
    JS_FreeValue(ctx, console);
    JS_FreeValue(ctx, global_obj);
}
