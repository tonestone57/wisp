#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include "JSDOMTokenList.gen.h"

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

#define MAX_TOKENS 128

typedef struct {
    char *tokens[MAX_TOKENS];
    int count;
} TokenList;

static TokenList get_tokens(dom_element *el)
{
    TokenList tl = { .count = 0 };
    const char *class_str = NULL;
    size_t len = 0;
    dom_string *class_dom = NULL;

    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)el);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)el];
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, "class")) {
                    class_str = wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].value);
                    if (class_str) {
                        len = strlen(class_str);
                    }
                    break;
                }
            }
        }
    } else {
        dom_string *name_dom = NULL;
        dom_string_create((const uint8_t *)"class", 5, &name_dom);
        if (!name_dom) return tl;
        dom_element_get_attribute(el, name_dom, &class_dom);
        dom_string_unref(name_dom);

        if (!class_dom) return tl;

        class_str = (const char *)dom_string_data(class_dom);
        len = dom_string_byte_length(class_dom);
    }

    if (!class_str) return tl;

    // Parse spaces
    size_t start = 0;
    while (start < len) {
        // Skip leading whitespace
        while (start < len && (class_str[start] == ' ' || class_str[start] == '\t' ||
                               class_str[start] == '\n' || class_str[start] == '\r' ||
                               class_str[start] == '\f')) {
            start++;
        }
        if (start >= len) break;

        size_t end = start;
        while (end < len && !(class_str[end] == ' ' || class_str[end] == '\t' ||
                              class_str[end] == '\n' || class_str[end] == '\r' ||
                              class_str[end] == '\f')) {
            end++;
        }

        size_t token_len = end - start;
        if (token_len > 0 && tl.count < MAX_TOKENS) {
            tl.tokens[tl.count] = malloc(token_len + 1);
            if (tl.tokens[tl.count]) {
                memcpy(tl.tokens[tl.count], class_str + start, token_len);
                tl.tokens[tl.count][token_len] = '\0';
                tl.count++;
            }
        }
        start = end;
    }

    if (class_dom) {
        dom_string_unref(class_dom);
    }
    return tl;
}

static void set_tokens(dom_element *el, TokenList *tl)
{
    // Calculate required buffer size
    size_t size = 0;
    for (int i = 0; i < tl->count; i++) {
        size += strlen(tl->tokens[i]) + 1; // token + space
    }

    char *buf = malloc(size > 0 ? size : 1);
    if (!buf) return;
    buf[0] = '\0';

    size_t pos = 0;
    for (int i = 0; i < tl->count; i++) {
        size_t token_len = strlen(tl->tokens[i]);
        memcpy(buf + pos, tl->tokens[i], token_len);
        pos += token_len;
        if (i < tl->count - 1) {
            buf[pos] = ' ';
            pos++;
        }
    }
    buf[pos] = '\0';

    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)el);
        WispStringRef name_ref = wisp_shm_alloc_string(wisp_shm_dom, "class");
        WispStringRef value_ref = wisp_shm_alloc_string(wisp_shm_dom, buf);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)el];
            bool found = false;
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, "class")) {
                    sns->attrs[i].value = value_ref;
                    found = true;
                    break;
                }
            }
            if (!found && sns->attr_count < WISP_SHM_MAX_ATTRIBUTES) {
                uint32_t i = sns->attr_count++;
                sns->attrs[i].name = name_ref;
                sns->attrs[i].value = value_ref;
            }
        }
        shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_SET_ATTRIBUTE, (uint64_t)(uintptr_t)el, 0, 0, "class", buf);
    } else {
        dom_string *name_dom = NULL;
        dom_string_create((const uint8_t *)"class", 5, &name_dom);
        dom_string *val_dom = NULL;
        dom_string_create((const uint8_t *)buf, pos, &val_dom);

        if (name_dom && val_dom) {
            dom_element_set_attribute(el, name_dom, val_dom);
        }

        if (name_dom) dom_string_unref(name_dom);
        if (val_dom) dom_string_unref(val_dom);
    }
    free(buf);
}

static void free_tokens(TokenList *tl)
{
    for (int i = 0; i < tl->count; i++) {
        free(tl->tokens[i]);
    }
}

JSValue wisp_domtokenlist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    TokenList tl = get_tokens((dom_element *)priv->node);
    int count = tl.count;
    free_tokens(&tl);
    return JS_NewInt32(ctx, count);
}

JSValue wisp_domtokenlist_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    if (!priv || !priv->node) return JS_NULL;
    TokenList tl = get_tokens((dom_element *)priv->node);
    JSValue res = JS_NULL;
    if (index < (uint32_t)tl.count) {
        res = JS_NewString(ctx, tl.tokens[index]);
    }
    free_tokens(&tl);
    return res;
}

JSValue wisp_domtokenlist_contains_impl(JSContext *ctx, QJSNodePrivate *priv, const char * token)
{
    if (!priv || !priv->node || !token) return JS_FALSE;
    TokenList tl = get_tokens((dom_element *)priv->node);
    bool found = false;
    for (int i = 0; i < tl.count; i++) {
        if (strcmp(tl.tokens[i], token) == 0) {
            found = true;
            break;
        }
    }
    free_tokens(&tl);
    return JS_NewBool(ctx, found);
}

JSValue wisp_domtokenlist_add_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue tokens)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    TokenList tl = get_tokens((dom_element *)priv->node);

    uint32_t len = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, tokens, "length");
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    bool changed = false;
    for (uint32_t i = 0; i < len; i++) {
        JSValue item = JS_GetPropertyUint32(ctx, tokens, i);
        const char *token = JS_ToCString(ctx, item);
        if (token && strlen(token) > 0) {
            bool found = false;
            for (int j = 0; j < tl.count; j++) {
                if (strcmp(tl.tokens[j], token) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found && tl.count < MAX_TOKENS) {
                char *dup_tok = strdup(token);
                if (dup_tok) {
                    tl.tokens[tl.count] = dup_tok;
                    tl.count++;
                    changed = true;
                }
            }
        }
        if (token) JS_FreeCString(ctx, token);
        JS_FreeValue(ctx, item);
    }

    if (changed) {
        set_tokens((dom_element *)priv->node, &tl);
    }
    free_tokens(&tl);
    return JS_UNDEFINED;
}

JSValue wisp_domtokenlist_remove_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue tokens)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    TokenList tl = get_tokens((dom_element *)priv->node);

    uint32_t len = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, tokens, "length");
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    bool changed = false;
    for (uint32_t i = 0; i < len; i++) {
        JSValue item = JS_GetPropertyUint32(ctx, tokens, i);
        const char *token = JS_ToCString(ctx, item);
        if (token) {
            for (int j = 0; j < tl.count; j++) {
                if (strcmp(tl.tokens[j], token) == 0) {
                    free(tl.tokens[j]);
                    // Shift remainder of tokens left
                    for (int k = j; k < tl.count - 1; k++) {
                        tl.tokens[k] = tl.tokens[k + 1];
                    }
                    tl.count--;
                    changed = true;
                    j--; // re-check index after shift
                }
            }
        }
        if (token) JS_FreeCString(ctx, token);
        JS_FreeValue(ctx, item);
    }

    if (changed) {
        set_tokens((dom_element *)priv->node, &tl);
    }
    free_tokens(&tl);
    return JS_UNDEFINED;
}

JSValue wisp_domtokenlist_toString_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    if (wisp_is_js_process) {
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn) {
            WispNodeStrings *sns = &shm_dom_get_node_strings(wisp_shm_dom)[(uint64_t)(uintptr_t)priv->node];
            uint32_t limit = sns->attr_count < WISP_SHM_MAX_ATTRIBUTES ? sns->attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t i = 0; i < limit; i++) {
                if (wisp_string_ref_caseeq(wisp_shm_dom, sns->attrs[i].name, "class")) {
                    return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, sns->attrs[i].value));
                }
            }
        }
        return JS_NewString(ctx, "");
    }
    dom_string *class_dom = NULL;
    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)"class", 5, &name_dom);
    if (!name_dom) return JS_NewString(ctx, "");
    dom_element_get_attribute((dom_element *)priv->node, name_dom, &class_dom);
    dom_string_unref(name_dom);

    if (!class_dom) return JS_NewString(ctx, "");
    JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(class_dom), dom_string_byte_length(class_dom));
    dom_string_unref(class_dom);
    return val;
}

JSValue wisp_domtokenlist_toggle_impl(JSContext *ctx, QJSNodePrivate *priv, const char * token, bool force)
{
    // Overridden by custom_domtokenlist_toggle below
    return JS_FALSE;
}

static JSValue custom_domtokenlist_toggle(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_EXCEPTION;
    if (argc < 1) return JS_ThrowTypeError(ctx, "token is required");

    const char *token = JS_ToCString(ctx, argv[0]);
    if (!token) return JS_EXCEPTION;

    TokenList tl = get_tokens((dom_element *)priv->node);
    bool found = false;
    int found_idx = -1;
    for (int i = 0; i < tl.count; i++) {
        if (strcmp(tl.tokens[i], token) == 0) {
            found = true;
            found_idx = i;
            break;
        }
    }

    bool result_present = false;
    bool has_force = (argc > 1);
    bool force_val = has_force ? JS_ToBool(ctx, argv[1]) : false;

    if (found) {
        if (has_force && force_val) {
            // keep it
            result_present = true;
        } else {
            // remove it
            free(tl.tokens[found_idx]);
            for (int k = found_idx; k < tl.count - 1; k++) {
                tl.tokens[k] = tl.tokens[k + 1];
            }
            tl.count--;
            result_present = false;
            set_tokens((dom_element *)priv->node, &tl);
        }
    } else {
        if (has_force && !force_val) {
            // do nothing
            result_present = false;
        } else {
            // add it
            if (tl.count < MAX_TOKENS) {
                tl.tokens[tl.count] = strdup(token);
                tl.count++;
                result_present = true;
                set_tokens((dom_element *)priv->node, &tl);
            }
        }
    }

    free_tokens(&tl);
    JS_FreeCString(ctx, token);
    return JS_NewBool(ctx, result_present);
}

JSValue qjs_new_domtokenlist(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_domtokenlist_class_id);
    if (JS_IsException(obj)) return obj;
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    priv->magic = QJS_DOM_MAGIC;
    priv->node = node;
    priv->is_dom_node = is_dom_node;
    priv->ctx = ctx;
    if (!wisp_is_js_process && is_dom_node && node) dom_node_ref((dom_node *)node);
    JS_SetOpaque(obj, priv);

    /* Wrap in proxy to support indexed token access */
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue make_proxy_fn = JS_GetPropertyStr(ctx, global_obj, "__wisp_make_domtokenlist_proxy");
    if (JS_IsFunction(ctx, make_proxy_fn)) {
        JSValue proxy_obj = JS_Call(ctx, make_proxy_fn, JS_UNDEFINED, 1, &obj);
        JS_FreeValue(ctx, obj);
        obj = proxy_obj;
    }
    JS_FreeValue(ctx, make_proxy_fn);
    JS_FreeValue(ctx, global_obj);

    return obj;
}

int qjs_init_domtokenlist(JSContext *ctx)
{
    qjs_init_domtokenlist_gen(ctx);

    /* Define __wisp_make_domtokenlist_proxy */
    JSValue global_obj = JS_GetGlobalObject(ctx);
    const char *proxy_js =
        "globalThis.__wisp_make_domtokenlist_proxy = function(tokenList) {\n"
        "    return new Proxy(tokenList, {\n"
        "        get(target, prop) {\n"
        "            if (typeof prop !== 'symbol') {\n"
        "                let idx = Number(prop);\n"
        "                if (Number.isInteger(idx) && idx >= 0) {\n"
        "                    return target.item(idx);\n"
        "                }\n"
        "            }\n"
        "            let val = target[prop];\n"
        "            if (typeof val === 'function') {\n"
        "                return val.bind(target);\n"
        "            }\n"
        "            return val;\n"
        "        }\n"
        "    });\n"
        "};";
    JSValue eval_res = JS_Eval(ctx, proxy_js, strlen(proxy_js), "<domtokenlist_proxy_init>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, eval_res);

    JSValue proto = JS_GetClassProto(ctx, qjs_domtokenlist_class_id);
    if (JS_IsObject(proto)) {
        JSValue toggle_fn = JS_NewCFunction2(ctx, custom_domtokenlist_toggle, "toggle", 1, JS_CFUNC_generic, 0);
        JS_SetPropertyStr(ctx, proto, "toggle", toggle_fn);
    }
    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
