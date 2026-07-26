#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <dom/core/characterdata.h>
#include "JSCharacterData.gen.h"

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

JSValue wisp_characterdata_data_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;

    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &wisp_shm_dom->node_strings[(uint64_t)(uintptr_t)priv->node];
            return JS_NewString(ctx, sns->value);
        }
        return JS_NULL;
    }

    dom_string *val = NULL;
    dom_characterdata_get_data(priv->node, &val);
    if (val) {
        JSValue res = JS_NewStringLen(ctx, (const char *)dom_string_data(val), dom_string_byte_length(val));
        dom_string_unref(val);
        return res;
    }
    return JS_NULL;
}

JSValue wisp_characterdata_data_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;

    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &wisp_shm_dom->node_strings[(uint64_t)(uintptr_t)priv->node];
            strncpy(sns->value, value, SHM_DOM_STRING_MAX - 1);
            sns->value[SHM_DOM_STRING_MAX - 1] = '\0';
        }
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_SET_NODE_VALUE, (uint64_t)(uintptr_t)priv->node, 0, 0, NULL, value);
        return JS_UNDEFINED;
    }

    dom_string *ds;
    dom_string_create((const uint8_t *)value, strlen(value), &ds);
    dom_characterdata_set_data(priv->node, ds);
    dom_string_unref(ds);
    return JS_UNDEFINED;
}

JSValue wisp_characterdata_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);

    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &wisp_shm_dom->node_strings[(uint64_t)(uintptr_t)priv->node];
            return JS_NewInt32(ctx, (int32_t)strlen(sns->value));
        }
        return JS_NewInt32(ctx, 0);
    }

    dom_ulong length = 0;
    dom_characterdata_get_length(priv->node, &length);
    return JS_NewInt32(ctx, (int32_t)length);
}
