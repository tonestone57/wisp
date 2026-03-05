import re

with open("src/content/handlers/javascript/quickjs/event_target.c", "r") as f:
    content = f.read()

# I will add the dom listener code to js_addEventListener.

new_code = """
#include "event_target.h"
#include <wisp/utils/log.h>
#include "quickjs.h"
#include <stdlib.h>
#include <string.h>

#include "utils/libdom.h"
extern void *qjs_get_document_priv(JSContext *ctx);
extern bool js_dom_event_add_listener(void *thread, struct dom_document *document, struct dom_node *node, struct dom_string *event_type_dom, void *js_funcval);

JSValue js_addEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc >= 2) {
        const char *type = JS_ToCString(ctx, argv[0]);
        bool is_func = JS_IsFunction(ctx, argv[1]);
        NSLOG(wisp, INFO, "addEventListener('%s', %s)", type, is_func ? "function" : "non-function");

        if (is_func) {
            JSValue listeners = JS_GetPropertyStr(ctx, this_val, "__listeners");
            if (JS_IsUndefined(listeners)) {
                listeners = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, this_val, "__listeners", JS_DupValue(ctx, listeners));
            }

            JSValue type_listeners = JS_GetPropertyStr(ctx, listeners, type);
            if (JS_IsUndefined(type_listeners)) {
                type_listeners = JS_NewArray(ctx);
                JS_SetPropertyStr(ctx, listeners, type, JS_DupValue(ctx, type_listeners));
            }

            JSValue push_func = JS_GetPropertyStr(ctx, type_listeners, "push");
            JSValue ret = JS_Call(ctx, push_func, type_listeners, 1, &argv[1]);
            JS_FreeValue(ctx, ret);

            JS_FreeValue(ctx, push_func);
            JS_FreeValue(ctx, type_listeners);
            JS_FreeValue(ctx, listeners);

            /* Register with LibDOM if this is the global object or a known DOM node */
            void *thread = JS_GetContextOpaque(ctx);
            struct dom_document *doc = qjs_get_document_priv(ctx);
            JSValue global_obj = JS_GetGlobalObject(ctx);

            /* For now, just register on the document if this is the global object. */
            if (JS_VALUE_GET_PTR(this_val) == JS_VALUE_GET_PTR(global_obj) && doc) {
                struct dom_node *node = (struct dom_node *)doc;
                dom_string *type_dom = NULL;
                dom_string_create((const uint8_t *)type, strlen(type), &type_dom);

                js_dom_event_add_listener(thread, doc, node, type_dom, (void *)&argv[1]);

                dom_string_unref(type_dom);
            }
            JS_FreeValue(ctx, global_obj);
        }
        JS_FreeCString(ctx, type);
    }
    return JS_UNDEFINED;
}
"""

content = re.sub(r'#include "event_target\.h".*?return JS_UNDEFINED;\n\}', new_code, content, flags=re.DOTALL)

with open("src/content/handlers/javascript/quickjs/event_target.c", "w") as f:
    f.write(content)
