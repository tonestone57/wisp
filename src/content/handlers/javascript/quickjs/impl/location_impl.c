#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <libwapcaplet/libwapcaplet.h>
#include "JSLocation.gen.h"
#include "qjs_internal.h"

struct nsurl;
extern const char *nsurl_access(const struct nsurl *url);
extern struct nsurl *content_get_url(void *c);

static struct nsurl *get_location_nsurl(JSContext *ctx)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->doc_priv) {
        return content_get_url((struct content *)t->doc_priv);
    }
    return NULL;
}

JSValue wisp_location_href_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        return JS_NewString(ctx, nsurl_access(url));
    }
    return JS_NewString(ctx, "about:blank");
}

JSValue wisp_location_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *scheme_lwc = nsurl_get_component(url, NSURL_SCHEME);
        if (scheme_lwc) {
            const char *data = lwc_string_data(scheme_lwc);
            size_t len = lwc_string_length(scheme_lwc);
            char *buf = malloc(len + 2);
            if (buf) {
                memcpy(buf, data, len);
                buf[len] = ':';
                buf[len + 1] = '\0';
                JSValue res = JS_NewString(ctx, buf);
                free(buf);
                lwc_string_unref(scheme_lwc);
                return res;
            }
            lwc_string_unref(scheme_lwc);
        }
    }
    return JS_NewString(ctx, "about:");
}

JSValue wisp_location_hostname_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *host_lwc = nsurl_get_component(url, NSURL_HOST);
        if (host_lwc) {
            const char *data = lwc_string_data(host_lwc);
            size_t len = lwc_string_length(host_lwc);
            JSValue res = JS_NewStringLen(ctx, data, len);
            lwc_string_unref(host_lwc);
            return res;
        }
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_location_port_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *port_lwc = nsurl_get_component(url, NSURL_PORT);
        if (port_lwc) {
            const char *data = lwc_string_data(port_lwc);
            size_t len = lwc_string_length(port_lwc);
            JSValue res = JS_NewStringLen(ctx, data, len);
            lwc_string_unref(port_lwc);
            return res;
        }
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_location_host_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *host_lwc = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port_lwc = nsurl_get_component(url, NSURL_PORT);
        if (host_lwc) {
            const char *h_data = lwc_string_data(host_lwc);
            size_t h_len = lwc_string_length(host_lwc);
            if (port_lwc) {
                const char *p_data = lwc_string_data(port_lwc);
                size_t p_len = lwc_string_length(port_lwc);
                if (p_len > 0) {
                    char *buf = malloc(h_len + p_len + 2);
                    if (buf) {
                        memcpy(buf, h_data, h_len);
                        buf[h_len] = ':';
                        memcpy(buf + h_len + 1, p_data, p_len);
                        buf[h_len + p_len + 1] = '\0';
                        JSValue res = JS_NewString(ctx, buf);
                        free(buf);
                        lwc_string_unref(host_lwc);
                        lwc_string_unref(port_lwc);
                        return res;
                    }
                }
                lwc_string_unref(port_lwc);
            }
            JSValue res = JS_NewStringLen(ctx, h_data, h_len);
            lwc_string_unref(host_lwc);
            return res;
        }
        if (port_lwc) lwc_string_unref(port_lwc);
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_location_pathname_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *path_lwc = nsurl_get_component(url, NSURL_PATH);
        if (path_lwc) {
            const char *data = lwc_string_data(path_lwc);
            size_t len = lwc_string_length(path_lwc);
            if (len == 0 || data[0] != '/') {
                char *buf = malloc(len + 2);
                if (buf) {
                    buf[0] = '/';
                    memcpy(buf + 1, data, len);
                    buf[len + 1] = '\0';
                    JSValue res = JS_NewString(ctx, buf);
                    free(buf);
                    lwc_string_unref(path_lwc);
                    return res;
                }
            } else {
                JSValue res = JS_NewStringLen(ctx, data, len);
                lwc_string_unref(path_lwc);
                return res;
            }
            lwc_string_unref(path_lwc);
        }
    }
    return JS_NewString(ctx, "/");
}

JSValue wisp_location_search_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *query_lwc = nsurl_get_component(url, NSURL_QUERY);
        if (query_lwc) {
            const char *data = lwc_string_data(query_lwc);
            size_t len = lwc_string_length(query_lwc);
            if (len > 0) {
                char *buf = malloc(len + 2);
                if (buf) {
                    buf[0] = '?';
                    memcpy(buf + 1, data, len);
                    buf[len + 1] = '\0';
                    JSValue res = JS_NewString(ctx, buf);
                    free(buf);
                    lwc_string_unref(query_lwc);
                    return res;
                }
            }
            lwc_string_unref(query_lwc);
        }
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_location_hash_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *frag_lwc = nsurl_get_component(url, NSURL_FRAGMENT);
        if (frag_lwc) {
            const char *data = lwc_string_data(frag_lwc);
            size_t len = lwc_string_length(frag_lwc);
            if (len > 0) {
                char *buf = malloc(len + 2);
                if (buf) {
                    buf[0] = '#';
                    memcpy(buf + 1, data, len);
                    buf[len + 1] = '\0';
                    JSValue res = JS_NewString(ctx, buf);
                    free(buf);
                    lwc_string_unref(frag_lwc);
                    return res;
                }
            }
            lwc_string_unref(frag_lwc);
        }
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_location_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        lwc_string *scheme_lwc = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host_lwc = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port_lwc = nsurl_get_component(url, NSURL_PORT);
        if (scheme_lwc && host_lwc) {
            const char *s_data = lwc_string_data(scheme_lwc);
            size_t s_len = lwc_string_length(scheme_lwc);
            const char *h_data = lwc_string_data(host_lwc);
            size_t h_len = lwc_string_length(host_lwc);
            size_t p_len = port_lwc ? lwc_string_length(port_lwc) : 0;
            const char *p_data = port_lwc ? lwc_string_data(port_lwc) : "";

            size_t buf_len = s_len + 3 + h_len + (p_len > 0 ? 1 + p_len : 0);
            char *buf = malloc(buf_len + 1);
            if (buf) {
                char *ptr = buf;
                memcpy(ptr, s_data, s_len); ptr += s_len;
                memcpy(ptr, "://", 3); ptr += 3;
                memcpy(ptr, h_data, h_len); ptr += h_len;
                if (p_len > 0) {
                    *ptr = ':'; ptr++;
                    memcpy(ptr, p_data, p_len); ptr += p_len;
                }
                *ptr = '\0';
                JSValue res = JS_NewString(ctx, buf);
                free(buf);
                lwc_string_unref(scheme_lwc);
                lwc_string_unref(host_lwc);
                if (port_lwc) lwc_string_unref(port_lwc);
                return res;
            }
        }
        if (scheme_lwc) lwc_string_unref(scheme_lwc);
        if (host_lwc) lwc_string_unref(host_lwc);
        if (port_lwc) lwc_string_unref(port_lwc);
    }
    return JS_NewString(ctx, "null");
}

static JSValue js_location_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    return qjs_new_location(ctx, NULL, false);
}

int qjs_init_location(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_location_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_location_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_location_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_location_class_id, JS_DupValue(ctx, proto));
    }

    JSValue loc = qjs_new_location(ctx, NULL, false);
    JSValue ctor = JS_NewCFunction2(ctx, js_location_constructor, "Location", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_FreeValue(ctx, ctor);
    JS_FreeValue(ctx, proto);
    JS_DefinePropertyValueStr(ctx, global_obj, "location", loc, JS_PROP_C_W_E);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_location_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}