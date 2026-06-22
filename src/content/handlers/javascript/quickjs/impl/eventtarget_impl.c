#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSEventTarget.gen.h"

JSValue wisp_eventtarget_addEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, void * callback, bool capture)
{
    if (!priv || !priv->node || !callback) return JS_UNDEFINED;

    dom_string *type_dom = NULL;
    dom_string_create((const uint8_t *)type, strlen(type), &type_dom);

    struct jsthread *thread = JS_GetContextOpaque(ctx);
    /* Note: callback is currently the raw LibDOM pointer if it was a DOM object,
     * but for addEventListener it's likely a JS function.
     * Our generator passed 'void *' but since it wasn't a DOM object, it's probably NULL.
     * Wait, if it wasn't a DOM object, it stays JSValue in the generator?
     * No, I changed it to void * and qjs_get_dom_priv.
     * Let me fix the generator to pass JSValue for 'any' or unknown types.
     */
    NSLOG(wisp, DEBUG, "addEventListener called (stub)");

    dom_string_unref(type_dom);
    return JS_UNDEFINED;
}

JSValue wisp_eventtarget_removeEventListener_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, void * callback, bool capture)
{
    return JS_UNDEFINED;
}

JSValue wisp_eventtarget_dispatchEvent_impl(JSContext *ctx, QJSNodePrivate *priv, void * event)
{
    if (!priv || !priv->node || !event) return JS_FALSE;
    struct jsthread *thread = JS_GetContextOpaque(ctx);
    bool success = js_fire_event(thread, "click", NULL, (dom_node *)priv->node);
    return JS_NewBool(ctx, success);
}

int qjs_init_eventtarget(JSContext *ctx)
{
    return qjs_init_eventtarget_gen(ctx);
}
