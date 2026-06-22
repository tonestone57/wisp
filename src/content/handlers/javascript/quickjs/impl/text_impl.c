#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "utils/libdom.h"

JSValue wisp_text_splitText_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t offset)
{
    struct dom_text *result;
    dom_text_split_text((dom_text *)priv->node, offset, &result);
    if (result) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

JSValue wisp_text_wholeText_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct dom_string *text;
    dom_text_get_whole_text((dom_text *)priv->node, &text);
    if (text) {
        JSValue val = JS_NewString(ctx, (const char *)dom_string_data(text));
        dom_string_unref(text);
        return val;
    }
    return JS_NULL;
}
