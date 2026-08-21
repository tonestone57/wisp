#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSURLSearchParams.gen.h"

JSClassID qjs_urlsearchparams_class_id;

struct url_param {
    char *name;
    char *value;
};

struct url_search_params_data {
    struct url_param *params;
    size_t count;
    size_t capacity;
};

static void urlsearchparams_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_urlsearchparams_class_id);
    if (priv) {
        struct url_search_params_data *data = priv->node;
        if (data) {
            for (size_t i = 0; i < data->count; i++) {
                free(data->params[i].name);
                free(data->params[i].value);
            }
            free(data->params);
            free(data);
        }
        free(priv);
    }
}

static JSClassDef wisp_urlsearchparams_class = {
    "URLSearchParams",
    .finalizer = urlsearchparams_finalizer,
};

JSValue wisp_urlsearchparams_constructor_impl(JSContext *ctx, JSValue init)
{
    struct url_search_params_data *data = calloc(1, sizeof(struct url_search_params_data));
    if (!data) return JS_ThrowOutOfMemory(ctx);

    if (JS_IsString(init)) {
        const char *str = JS_ToCString(ctx, init);
        if (!str) {
            free(data);
            return JS_ThrowOutOfMemory(ctx);
        }
        {
            const char *p = str;
            if (*p == '?') p++;
            while (*p) {
                const char *amp = strchr(p, '&');
                if (!amp) amp = p + strlen(p);

                const char *eq = strchr(p, '=');
                if (eq && eq < amp) {
                    char *name = strndup(p, eq - p);
                    char *value = strndup(eq + 1, amp - (eq + 1));
                    if (name && value) {
                        if (data->count >= data->capacity) {
                            size_t new_cap = data->capacity ? data->capacity * 2 : 8;
                            struct url_param *new_params = realloc(data->params, new_cap * sizeof(struct url_param));
                            if (!new_params) {
                                free(name); free(value);
                                JS_FreeCString(ctx, str);
                                for (size_t k = 0; k < data->count; k++) {
                                    free(data->params[k].name);
                                    free(data->params[k].value);
                                }
                                free(data->params); free(data);
                                return JS_ThrowOutOfMemory(ctx);
                            }
                            data->params = new_params;
                            data->capacity = new_cap;
                        }
                        data->params[data->count].name = name;
                        data->params[data->count].value = value;
                        data->count++;
                    } else {
                        free(name);
                        free(value);
                    }
                } else {
                    char *name = strndup(p, amp - p);
                    char *value = strdup("");
                    if (name && value) {
                        if (data->count >= data->capacity) {
                            size_t new_cap = data->capacity ? data->capacity * 2 : 8;
                            struct url_param *new_params = realloc(data->params, new_cap * sizeof(struct url_param));
                            if (!new_params) {
                                free(name); free(value);
                                JS_FreeCString(ctx, str);
                                for (size_t k = 0; k < data->count; k++) {
                                    free(data->params[k].name);
                                    free(data->params[k].value);
                                }
                                free(data->params); free(data);
                                return JS_ThrowOutOfMemory(ctx);
                            }
                            data->params = new_params;
                            data->capacity = new_cap;
                        }
                        data->params[data->count].name = name;
                        data->params[data->count].value = value;
                        data->count++;
                    } else {
                        free(name);
                        free(value);
                    }
                }

                if (*amp == '\0') break;
                p = amp + 1;
            }
            JS_FreeCString(ctx, str);
        }
    }

    return qjs_new_urlsearchparams(ctx, data, false);
}

JSValue wisp_urlsearchparams_append_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name, const char * value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    struct url_search_params_data *data = priv->node;
    if (name && value) {
        if (data->count >= data->capacity) {
            size_t new_cap = data->capacity ? data->capacity * 2 : 8;
            struct url_param *new_params = realloc(data->params, new_cap * sizeof(struct url_param));
            if (!new_params) return JS_ThrowOutOfMemory(ctx);
            data->params = new_params;
            data->capacity = new_cap;
        }
        char *n = strdup(name);
        char *v = strdup(value);
        if (!n || !v) {
            free(n);
            free(v);
            return JS_ThrowOutOfMemory(ctx);
        }
        data->params[data->count].name = n;
        data->params[data->count].value = v;
        data->count++;
    }
    return JS_UNDEFINED;
}

JSValue wisp_urlsearchparams_delete_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_UNDEFINED;
    struct url_search_params_data *data = priv->node;
    size_t i = 0;
    while (i < data->count) {
        if (strcmp(data->params[i].name, name) == 0) {
            free(data->params[i].name);
            free(data->params[i].value);
            for (size_t j = i; j < data->count - 1; j++) {
                data->params[j] = data->params[j + 1];
            }
            data->count--;
        } else {
            i++;
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_urlsearchparams_get_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_NULL;
    struct url_search_params_data *data = priv->node;
    for (size_t i = 0; i < data->count; i++) {
        if (strcmp(data->params[i].name, name) == 0) {
            return JS_NewString(ctx, data->params[i].value);
        }
    }
    return JS_NULL;
}

JSValue wisp_urlsearchparams_getAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    JSValue arr = JS_NewArray(ctx);
    if (!priv || !priv->node || !name) return arr;
    struct url_search_params_data *data = priv->node;
    uint32_t idx = 0;
    for (size_t i = 0; i < data->count; i++) {
        if (strcmp(data->params[i].name, name) == 0) {
            JS_SetPropertyUint32(ctx, arr, idx++, JS_NewString(ctx, data->params[i].value));
        }
    }
    return arr;
}

JSValue wisp_urlsearchparams_has_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_FALSE;
    struct url_search_params_data *data = priv->node;
    for (size_t i = 0; i < data->count; i++) {
        if (strcmp(data->params[i].name, name) == 0) {
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

JSValue wisp_urlsearchparams_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name, const char * value)
{
    if (!priv || !priv->node || !name || !value) return JS_UNDEFINED;
    struct url_search_params_data *data = priv->node;
    bool found = false;
    for (size_t i = 0; i < data->count; i++) {
        if (strcmp(data->params[i].name, name) == 0) {
            if (!found) {
                char *new_val = strdup(value);
                if (!new_val) return JS_ThrowOutOfMemory(ctx);
                free(data->params[i].value);
                data->params[i].value = new_val;
                found = true;
            } else {
                // Delete duplicate keys if we set again
                free(data->params[i].name);
                free(data->params[i].value);
                for (size_t j = i; j < data->count - 1; j++) {
                    data->params[j] = data->params[j + 1];
                }
                data->count--;
                i--; // Adjust index
            }
        }
    }
    if (!found) {
        wisp_urlsearchparams_append_impl(ctx, priv, name, value);
    }
    return JS_UNDEFINED;
}

JSValue wisp_urlsearchparams_toString_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct url_search_params_data *data = priv->node;
    if (data->count == 0) return JS_NewString(ctx, "");

    // Estimate length and cache strlen results
    size_t stack_lens[64];
    size_t *lens = stack_lens;
    if (data->count * 2 > 64) {
        lens = malloc(data->count * 2 * sizeof(size_t));
        if (!lens) return JS_ThrowOutOfMemory(ctx);
    }

    size_t total_len = 0;
    for (size_t i = 0; i < data->count; i++) {
        size_t n_len = strlen(data->params[i].name);
        size_t v_len = strlen(data->params[i].value);
        lens[i * 2] = n_len;
        lens[i * 2 + 1] = v_len;
        total_len += n_len + v_len + 2; // '=' and '&'
    }

    char *buf = malloc(total_len + 1);
    if (!buf) {
        if (lens != stack_lens) free(lens);
        return JS_ThrowOutOfMemory(ctx);
    }
    buf[0] = '\0';
    size_t curr = 0;
    for (size_t i = 0; i < data->count; i++) {
        if (i > 0) {
            buf[curr++] = '&';
        }
        memcpy(buf + curr, data->params[i].name, lens[i * 2]);
        curr += lens[i * 2];
        buf[curr++] = '=';
        memcpy(buf + curr, data->params[i].value, lens[i * 2 + 1]);
        curr += lens[i * 2 + 1];
    }
    if (lens != stack_lens) free(lens);
    buf[curr] = '\0';
    JSValue res = JS_NewString(ctx, buf);
    free(buf);
    return res;
}

int qjs_init_urlsearchparams(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_urlsearchparams_class_id == 0) {
        JS_NewClassID(rt, &qjs_urlsearchparams_class_id);
    }

    if (!JS_IsRegisteredClass(rt, qjs_urlsearchparams_class_id)) {
        JS_NewClass(rt, qjs_urlsearchparams_class_id, &wisp_urlsearchparams_class);
    }

    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_urlsearchparams_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_urlsearchparams_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_urlsearchparams_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_urlsearchparams_class_id, JS_DupValue(ctx, proto));
    }
    JS_FreeValue(ctx, proto);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_urlsearchparams_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
