#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "quickjs.h"
#include "timers.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "JSDOMImplementation.gen.h"
#include <wisp/utils/nsurl.h>
#include <libwapcaplet/libwapcaplet.h>
#include <wisp/utils/shm_dom.h>
#include <wisp/utils/corestrings.h>
#include <wisp/content/handlers/html/private.h>
#include <wisp/content/handlers/html/form_internal.h>
#include <wisp/browser_window.h>
#include "desktop/browser_private.h"
#include <wisp/utils/ipc.h>

struct nsurl;
extern const char *nsurl_access(const struct nsurl *url);
extern nserror nsurl_create(const char *const url_s, struct nsurl **url);
extern struct nsurl *get_location_nsurl(JSContext *ctx);
extern JSValue wisp_window_location_get_impl(JSContext *ctx, QJSNodePrivate *priv);

extern bool wisp_is_js_process;
extern shm_dom_t *wisp_shm_dom;

extern JSValue wisp_node_textContent_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_node_textContent_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value);

// Forward declarations of core element helper functions
JSValue wisp_element_getAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName);
JSValue wisp_element_setAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName, const char * value);
JSValue wisp_element_removeAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName);
JSValue wisp_element_hasAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName);

// Helper to retrieve document base URL (copied from location_impl.c)
static struct nsurl *get_doc_base_url(JSContext *ctx)
{
    return get_location_nsurl(ctx);
}

extern bool js_dom_event_add_listener(jsthread *thread, struct dom_document *document, struct dom_node *node,
    struct dom_string *event_type_dom, JSValue js_funcval);
extern bool js_dom_event_remove_listener(jsthread *thread, struct dom_document *document, struct dom_node *node,
    struct dom_string *event_type_dom, JSValue js_funcval);

static void helper_set_event_handler(JSContext *ctx, QJSNodePrivate *priv, const char *prop_name, const char *event_name, JSValue value) {
    struct jsthread *thread = JS_GetContextOpaque(ctx);
    if (!thread) return;
    if (!priv || (!priv->node && priv != &thread->global_window_priv)) return;

    if (wisp_is_js_process) {
        // In the companion process, we cannot attach native LibDOM event listeners,
        // because native event targets do not exist (the node pointer is just an integer ID).
        // The event system handles storing this via QuickJS object properties already.
        JSValue wrapper;
        if (priv == &thread->global_window_priv) {
            wrapper = JS_GetGlobalObject(ctx);
        } else {
            wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
        }
        if (JS_IsObject(wrapper)) {
            char prop_buf[64];
            snprintf(prop_buf, sizeof(prop_buf), "__%s_func", prop_name);
            JS_SetPropertyStr(ctx, wrapper, prop_buf, JS_DupValue(ctx, value));
        }
        JS_FreeValue(ctx, wrapper);
        return;
    }

    JSValue wrapper;
    if (priv == &thread->global_window_priv) {
        wrapper = JS_GetGlobalObject(ctx);
    } else {
        wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    }

    bool is_real_dom_node = priv->is_dom_node || (thread && priv == &thread->global_window_priv);

    if (JS_IsObject(wrapper)) {
        char prop_buf[64];
        snprintf(prop_buf, sizeof(prop_buf), "__%s_func", prop_name);
        JSValue oldVal = JS_GetPropertyStr(ctx, wrapper, prop_buf);

        if (is_real_dom_node) {
            if (!JS_IsUndefined(oldVal) && !JS_IsNull(oldVal)) {
                struct dom_string *type_dom = NULL;
                dom_string_create((const uint8_t *)event_name, strlen(event_name), &type_dom);
                if (type_dom) {
                    js_dom_event_remove_listener(thread, qjs_thread_get_document(thread), (struct dom_node *)priv->node, type_dom, oldVal);
                    dom_string_unref(type_dom);
                }
            }
        }
        JS_FreeValue(ctx, oldVal);

        JS_SetPropertyStr(ctx, wrapper, prop_buf, JS_DupValue(ctx, value));

        if (is_real_dom_node) {
            if (JS_IsFunction(ctx, value)) {
                struct dom_string *type_dom = NULL;
                dom_string_create((const uint8_t *)event_name, strlen(event_name), &type_dom);
                if (type_dom) {
                    js_dom_event_add_listener(thread, qjs_thread_get_document(thread), (struct dom_node *)priv->node, type_dom, value);
                    dom_string_unref(type_dom);
                }
            }
        }
    }
    JS_FreeValue(ctx, wrapper);
}

static JSValue helper_get_event_handler(JSContext *ctx, QJSNodePrivate *priv, const char *prop_name) {
    struct jsthread *thread = JS_GetContextOpaque(ctx);
    if (!thread) return JS_NULL;
    if (!priv || (!priv->node && priv != &thread->global_window_priv)) return JS_NULL;

    JSValue wrapper;
    if (priv == &thread->global_window_priv) {
        wrapper = JS_GetGlobalObject(ctx);
    } else {
        wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    }

    if (JS_IsObject(wrapper)) {
        char prop_buf[64];
        snprintf(prop_buf, sizeof(prop_buf), "__%s_func", prop_name);
        JSValue val = JS_GetPropertyStr(ctx, wrapper, prop_buf);
        JS_FreeValue(ctx, wrapper);
        return val;
    }
    JS_FreeValue(ctx, wrapper);
    return JS_NULL;
}

// -----------------------------------------------------------------------------
// HTMLAnchorElement Implementation (16 stubs)
// -----------------------------------------------------------------------------

// Helper to get fully resolved URL for an anchor element
static struct nsurl *get_anchor_resolved_url(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue href_val = wisp_element_getAttribute_impl(ctx, priv, "href");
    const char *href_str = NULL;
    bool free_href = false;
    if (JS_IsString(href_val)) {
        href_str = JS_ToCString(ctx, href_val);
        if (href_str) {
            free_href = true;
        }
    }
    if (!href_str) {
        href_str = "";
    }

    struct nsurl *base_url = get_doc_base_url(ctx);
    struct nsurl *fallback_base = NULL;
    if (!base_url) {
        nsurl_create("http://localhost/", &fallback_base);
        base_url = fallback_base;
    }

    struct nsurl *resolved_url = NULL;
    if (base_url) {
        nserror err = nsurl_join(base_url, href_str, &resolved_url);
        if (err != NSERROR_OK || !resolved_url) {
            nsurl_create("http://localhost/", &resolved_url);
        }
    } else {
        nsurl_create("http://localhost/", &resolved_url);
    }

    if (fallback_base) {
        nsurl_unref(fallback_base);
    }

    if (free_href) {
        JS_FreeCString(ctx, href_str);
    }
    JS_FreeValue(ctx, href_val);

    return resolved_url;
}

// Helper to set a modified nsurl back on the element's "href" attribute
static void set_anchor_resolved_url(JSContext *ctx, QJSNodePrivate *priv, struct nsurl *url)
{
    if (url) {
        wisp_element_setAttribute_impl(ctx, priv, "href", nsurl_access(url));
    }
}

JSValue wisp_htmlanchorelement_href_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        JSValue res = JS_NewString(ctx, nsurl_access(url));
        nsurl_unref(url);
        return res;
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlanchorelement_href_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_htmlanchorelement_href_impl(ctx, priv);
}

JSValue wisp_htmlanchorelement_href_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    wisp_element_setAttribute_impl(ctx, priv, "href", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        if (scheme) {
            const char *data = lwc_string_data(scheme);
            size_t len = lwc_string_length(scheme);
            char *buf = malloc(len + 2);
            JSValue res;
            if (buf) {
                memcpy(buf, data, len);
                buf[len] = ':';
                buf[len + 1] = '\0';
                res = JS_NewString(ctx, buf);
                free(buf);
            } else {
                res = JS_NewString(ctx, ":");
            }
            lwc_string_unref(scheme);
            nsurl_unref(url);
            return res;
        }
        nsurl_unref(url);
    }
    return JS_NewString(ctx, ":");
}

JSValue wisp_htmlanchorelement_protocol_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);

        if (scheme) lwc_string_unref(scheme);
        size_t val_len = strlen(value);
        size_t clean_len = (val_len > 0 && value[val_len - 1] == ':') ? val_len - 1 : val_len;
        lwc_intern_string(value, clean_len, &scheme);

        struct nsurl *new_url = NULL;
        nsurl_create_from_components_str(scheme, host, port, path, query, frag, &new_url);
        if (new_url) {
            set_anchor_resolved_url(ctx, priv, new_url);
            nsurl_unref(new_url);
        }

        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        if (path) lwc_string_unref(path);
        if (query) lwc_string_unref(query);
        if (frag) lwc_string_unref(frag);
        nsurl_unref(url);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_host_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        if (host) {
            const char *h_data = lwc_string_data(host);
            size_t h_len = lwc_string_length(host);
            if (port && lwc_string_length(port) > 0) {
                const char *p_data = lwc_string_data(port);
                size_t p_len = lwc_string_length(port);
                char *buf = malloc(h_len + p_len + 2);
                if (buf) {
                    memcpy(buf, h_data, h_len);
                    buf[h_len] = ':';
                    memcpy(buf + h_len + 1, p_data, p_len);
                    buf[h_len + p_len + 1] = '\0';
                    JSValue res = JS_NewString(ctx, buf);
                    free(buf);
                    lwc_string_unref(host);
                    lwc_string_unref(port);
                    nsurl_unref(url);
                    return res;
                }
            } else {
                JSValue res = JS_NewStringLen(ctx, h_data, h_len);
                lwc_string_unref(host);
                if (port) lwc_string_unref(port);
                nsurl_unref(url);
                return res;
            }
        }
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        nsurl_unref(url);
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlanchorelement_host_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);

        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        host = NULL;
        port = NULL;

        const char *rbracket = strchr(value, ']');
        const char *colon = strrchr(value, ':');

        if (colon && (!rbracket || colon > rbracket)) {
            lwc_intern_string(value, colon - value, &host);
            lwc_intern_string(colon + 1, strlen(colon + 1), &port);
        } else {
            lwc_intern_string(value, strlen(value), &host);
        }

        struct nsurl *new_url = NULL;
        nsurl_create_from_components_str(scheme, host, port, path, query, frag, &new_url);
        if (new_url) {
            set_anchor_resolved_url(ctx, priv, new_url);
            nsurl_unref(new_url);
        }

        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        if (path) lwc_string_unref(path);
        if (query) lwc_string_unref(query);
        if (frag) lwc_string_unref(frag);
        nsurl_unref(url);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_hostname_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        if (host) {
            JSValue res = JS_NewStringLen(ctx, lwc_string_data(host), lwc_string_length(host));
            lwc_string_unref(host);
            nsurl_unref(url);
            return res;
        }
        nsurl_unref(url);
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlanchorelement_hostname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);

        if (host) lwc_string_unref(host);
        lwc_intern_string(value, strlen(value), &host);

        struct nsurl *new_url = NULL;
        nsurl_create_from_components_str(scheme, host, port, path, query, frag, &new_url);
        if (new_url) {
            set_anchor_resolved_url(ctx, priv, new_url);
            nsurl_unref(new_url);
        }

        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        if (path) lwc_string_unref(path);
        if (query) lwc_string_unref(query);
        if (frag) lwc_string_unref(frag);
        nsurl_unref(url);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_port_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        if (port) {
            JSValue res = JS_NewStringLen(ctx, lwc_string_data(port), lwc_string_length(port));
            lwc_string_unref(port);
            nsurl_unref(url);
            return res;
        }
        nsurl_unref(url);
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlanchorelement_port_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);

        if (port) lwc_string_unref(port);
        lwc_intern_string(value, strlen(value), &port);

        struct nsurl *new_url = NULL;
        nsurl_create_from_components_str(scheme, host, port, path, query, frag, &new_url);
        if (new_url) {
            set_anchor_resolved_url(ctx, priv, new_url);
            nsurl_unref(new_url);
        }

        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        if (path) lwc_string_unref(path);
        if (query) lwc_string_unref(query);
        if (frag) lwc_string_unref(frag);
        nsurl_unref(url);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_pathname_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        if (path) {
            const char *data = lwc_string_data(path);
            size_t len = lwc_string_length(path);
            if (len == 0 || data[0] != '/') {
                char *buf = malloc(len + 2);
                if (buf) {
                    buf[0] = '/';
                    memcpy(buf + 1, data, len);
                    buf[len + 1] = '\0';
                    JSValue res = JS_NewString(ctx, buf);
                    free(buf);
                    lwc_string_unref(path);
                    nsurl_unref(url);
                    return res;
                }
            } else {
                JSValue res = JS_NewStringLen(ctx, data, len);
                lwc_string_unref(path);
                nsurl_unref(url);
                return res;
            }
            lwc_string_unref(path);
        }
        nsurl_unref(url);
    }
    return JS_NewString(ctx, "/");
}

JSValue wisp_htmlanchorelement_pathname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);

        if (path) lwc_string_unref(path);
        lwc_intern_string(value, strlen(value), &path);

        struct nsurl *new_url = NULL;
        nsurl_create_from_components_str(scheme, host, port, path, query, frag, &new_url);
        if (new_url) {
            set_anchor_resolved_url(ctx, priv, new_url);
            nsurl_unref(new_url);
        }

        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        if (path) lwc_string_unref(path);
        if (query) lwc_string_unref(query);
        if (frag) lwc_string_unref(frag);
        nsurl_unref(url);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_search_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        if (query && lwc_string_length(query) > 0) {
            const char *data = lwc_string_data(query);
            size_t len = lwc_string_length(query);
            char *buf = malloc(len + 2);
            if (buf) {
                buf[0] = '?';
                memcpy(buf + 1, data, len);
                buf[len + 1] = '\0';
                JSValue res = JS_NewString(ctx, buf);
                free(buf);
                lwc_string_unref(query);
                nsurl_unref(url);
                return res;
            }
            lwc_string_unref(query);
        } else if (query) {
            lwc_string_unref(query);
        }
        nsurl_unref(url);
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlanchorelement_search_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);

        if (query) lwc_string_unref(query);
        if (value[0] == '?') {
            lwc_intern_string(value + 1, strlen(value + 1), &query);
        } else {
            lwc_intern_string(value, strlen(value), &query);
        }

        struct nsurl *new_url = NULL;
        nsurl_create_from_components_str(scheme, host, port, path, query, frag, &new_url);
        if (new_url) {
            set_anchor_resolved_url(ctx, priv, new_url);
            nsurl_unref(new_url);
        }

        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        if (path) lwc_string_unref(path);
        if (query) lwc_string_unref(query);
        if (frag) lwc_string_unref(frag);
        nsurl_unref(url);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_hash_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);
        if (frag && lwc_string_length(frag) > 0) {
            const char *data = lwc_string_data(frag);
            size_t len = lwc_string_length(frag);
            char *buf = malloc(len + 2);
            if (buf) {
                buf[0] = '#';
                memcpy(buf + 1, data, len);
                buf[len + 1] = '\0';
                JSValue res = JS_NewString(ctx, buf);
                free(buf);
                lwc_string_unref(frag);
                nsurl_unref(url);
                return res;
            }
            lwc_string_unref(frag);
        } else if (frag) {
            lwc_string_unref(frag);
        }
        nsurl_unref(url);
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlanchorelement_hash_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        lwc_string *path = nsurl_get_component(url, NSURL_PATH);
        lwc_string *query = nsurl_get_component(url, NSURL_QUERY);
        lwc_string *frag = nsurl_get_component(url, NSURL_FRAGMENT);

        if (frag) lwc_string_unref(frag);
        if (value[0] == '#') {
            lwc_intern_string(value + 1, strlen(value + 1), &frag);
        } else {
            lwc_intern_string(value, strlen(value), &frag);
        }

        struct nsurl *new_url = NULL;
        nsurl_create_from_components_str(scheme, host, port, path, query, frag, &new_url);
        if (new_url) {
            set_anchor_resolved_url(ctx, priv, new_url);
            nsurl_unref(new_url);
        }

        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        if (path) lwc_string_unref(path);
        if (query) lwc_string_unref(query);
        if (frag) lwc_string_unref(frag);
        nsurl_unref(url);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlanchorelement_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_anchor_resolved_url(ctx, priv);
    if (url) {
        lwc_string *scheme = nsurl_get_component(url, NSURL_SCHEME);
        lwc_string *host = nsurl_get_component(url, NSURL_HOST);
        lwc_string *port = nsurl_get_component(url, NSURL_PORT);
        if (scheme && host) {
            const char *s_data = lwc_string_data(scheme);
            size_t s_len = lwc_string_length(scheme);
            const char *h_data = lwc_string_data(host);
            size_t h_len = lwc_string_length(host);

            // Omit standard default ports for http (80) & https (443)
            bool include_port = false;
            if (port && lwc_string_length(port) > 0) {
                const char *p_data = lwc_string_data(port);
                if (!((strcmp(s_data, "http") == 0 && strcmp(p_data, "80") == 0) ||
                      (strcmp(s_data, "https") == 0 && strcmp(p_data, "443") == 0))) {
                    include_port = true;
                }
            }

            size_t p_len = include_port ? lwc_string_length(port) : 0;
            const char *p_data = include_port ? lwc_string_data(port) : "";

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
                lwc_string_unref(scheme);
                lwc_string_unref(host);
                if (port) lwc_string_unref(port);
                nsurl_unref(url);
                return res;
            }
        }
        if (scheme) lwc_string_unref(scheme);
        if (host) lwc_string_unref(host);
        if (port) lwc_string_unref(port);
        nsurl_unref(url);
    }
    return JS_NewString(ctx, "null");
}

// -----------------------------------------------------------------------------
// Direct C-based getters and setters for HTMLAnchorElement properties
// -----------------------------------------------------------------------------

static JSValue js_anchor_href_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_href_get_impl(ctx, priv);
}
static JSValue js_anchor_href_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_href_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_protocol_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_protocol_get_impl(ctx, priv);
}
static JSValue js_anchor_protocol_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_protocol_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_host_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_host_get_impl(ctx, priv);
}
static JSValue js_anchor_host_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_host_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_hostname_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_hostname_get_impl(ctx, priv);
}
static JSValue js_anchor_hostname_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_hostname_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_port_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_port_get_impl(ctx, priv);
}
static JSValue js_anchor_port_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_port_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_pathname_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_pathname_get_impl(ctx, priv);
}
static JSValue js_anchor_pathname_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_pathname_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_search_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_search_get_impl(ctx, priv);
}
static JSValue js_anchor_search_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_search_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_hash_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_hash_get_impl(ctx, priv);
}
static JSValue js_anchor_hash_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    if (argc < 1) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    const char *val = JS_ToCString(ctx, argv[0]);
    if (!val) return JS_EXCEPTION;
    JSValue res = wisp_htmlanchorelement_hash_set_impl(ctx, priv, val);
    JS_FreeCString(ctx, val);
    return res;
}

static JSValue js_anchor_origin_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);
    if (!priv || !priv->node) return JS_UNDEFINED;
    return wisp_htmlanchorelement_origin_get_impl(ctx, priv);
}

static void define_anchor_property(JSContext *ctx, JSValueConst proto, const char *name, JSValue getter, JSValue setter) {
    JSAtom atom = JS_NewAtom(ctx, name);
    JS_DefinePropertyGetSet(ctx, proto, atom, getter, setter, JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_FreeAtom(ctx, atom);
}

// -----------------------------------------------------------------------------
// Override of the weak qjs_init_htmlanchorelement to inject property definitions
// -----------------------------------------------------------------------------

extern JSClassID qjs_htmlelement_class_id;
int qjs_init_htmlanchorelement_gen(JSContext *ctx);

int qjs_init_htmlanchorelement(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_htmlanchorelement_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_htmlanchorelement_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_htmlanchorelement_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_htmlanchorelement_class_id, JS_DupValue(ctx, proto));
    }
    JSValue htmlel_proto = JS_GetClassProto(ctx, qjs_htmlelement_class_id);
    if (JS_IsObject(proto) && JS_IsObject(htmlel_proto)) JS_SetPrototype(ctx, proto, htmlel_proto);
    JS_FreeValue(ctx, htmlel_proto);

    define_anchor_property(ctx, proto, "href",
                           JS_NewCFunction(ctx, js_anchor_href_get, "get href", 0),
                           JS_NewCFunction(ctx, js_anchor_href_set, "set href", 1));
    define_anchor_property(ctx, proto, "protocol",
                           JS_NewCFunction(ctx, js_anchor_protocol_get, "get protocol", 0),
                           JS_NewCFunction(ctx, js_anchor_protocol_set, "set protocol", 1));
    define_anchor_property(ctx, proto, "host",
                           JS_NewCFunction(ctx, js_anchor_host_get, "get host", 0),
                           JS_NewCFunction(ctx, js_anchor_host_set, "set host", 1));
    define_anchor_property(ctx, proto, "hostname",
                           JS_NewCFunction(ctx, js_anchor_hostname_get, "get hostname", 0),
                           JS_NewCFunction(ctx, js_anchor_hostname_set, "set hostname", 1));
    define_anchor_property(ctx, proto, "port",
                           JS_NewCFunction(ctx, js_anchor_port_get, "get port", 0),
                           JS_NewCFunction(ctx, js_anchor_port_set, "set port", 1));
    define_anchor_property(ctx, proto, "pathname",
                           JS_NewCFunction(ctx, js_anchor_pathname_get, "get pathname", 0),
                           JS_NewCFunction(ctx, js_anchor_pathname_set, "set pathname", 1));
    define_anchor_property(ctx, proto, "search",
                           JS_NewCFunction(ctx, js_anchor_search_get, "get search", 0),
                           JS_NewCFunction(ctx, js_anchor_search_set, "set search", 1));
    define_anchor_property(ctx, proto, "hash",
                           JS_NewCFunction(ctx, js_anchor_hash_get, "get hash", 0),
                           JS_NewCFunction(ctx, js_anchor_hash_set, "set hash", 1));
    define_anchor_property(ctx, proto, "origin",
                           JS_NewCFunction(ctx, js_anchor_origin_get, "get origin", 0),
                           JS_UNDEFINED);

    JS_FreeValue(ctx, proto);

    /* Mark as initialized */
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_htmlanchorelement_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}

// -----------------------------------------------------------------------------
// HTMLInputElement Implementation (10 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlinputelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "value");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlinputelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "value", value);
}

JSValue wisp_htmlinputelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "type");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "text");
    }
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        if (str) {
            static const char *valid_types[] = {
                "button", "checkbox", "color", "date", "datetime-local", "email",
                "file", "hidden", "image", "month", "number", "password",
                "radio", "range", "reset", "search", "submit", "tel", "text",
                "time", "url", "week"
            };
            size_t num_types = sizeof(valid_types) / sizeof(valid_types[0]);
            bool is_valid = false;
            for (size_t i = 0; i < num_types; i++) {
                if (strcasecmp(str, valid_types[i]) == 0) {
                    is_valid = true;
                    JS_FreeCString(ctx, str);
                    JS_FreeValue(ctx, val);
                    return JS_NewString(ctx, valid_types[i]);
                }
            }
            JS_FreeCString(ctx, str);
            JS_FreeValue(ctx, val);
            return JS_NewString(ctx, "text");
        }
    }
    return val;
}

JSValue wisp_htmlinputelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "type", value ? value : "text");
}

JSValue wisp_htmlinputelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "name");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlinputelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "name", value);
}

JSValue wisp_htmlinputelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "disabled");
}

JSValue wisp_htmlinputelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "disabled", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "disabled");
    }
}

// -----------------------------------------------------------------------------
// Core WebIDL Attribute Helpers
// -----------------------------------------------------------------------------

static int32_t get_element_int_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name, int32_t default_val)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, name);
    int32_t res_val = default_val;
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        if (str && strlen(str) > 0) {
            res_val = atoi(str);
        }
        if (str) JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, val);
    return res_val;
}

static void set_element_int_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name, int32_t value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    wisp_element_setAttribute_impl(ctx, priv, name, buf);
}

static JSValue get_element_str_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name, const char *default_val)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, name);
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, default_val);
    }
    return val;
}

static void set_element_str_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name, const char *value)
{
    wisp_element_setAttribute_impl(ctx, priv, name, value);
}

static JSValue get_element_bool_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name)
{
    return wisp_element_hasAttribute_impl(ctx, priv, name);
}

static void set_element_bool_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name, bool value)
{
    if (value) {
        wisp_element_setAttribute_impl(ctx, priv, name, "");
    } else {
        wisp_element_removeAttribute_impl(ctx, priv, name);
    }
}

static JSValue get_element_form_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    JSValue form_attr = get_element_str_attr(ctx, priv, "form", NULL);
    if (JS_IsString(form_attr)) {
        const char *form_id = JS_ToCString(ctx, form_attr);
        if (form_id && form_id[0] != '\0') {
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue doc_val = JS_GetPropertyStr(ctx, global, "document");
            JSValue get_el = JS_GetPropertyStr(ctx, doc_val, "getElementById");
            JSValue id_str = JS_NewString(ctx, form_id);
            JSValue target = JS_Call(ctx, get_el, doc_val, 1, &id_str);
            JS_FreeValue(ctx, id_str);
            JS_FreeValue(ctx, get_el);
            JS_FreeValue(ctx, doc_val);
            JS_FreeValue(ctx, global);
            JS_FreeCString(ctx, form_id);
            JS_FreeValue(ctx, form_attr);
            if (!JS_IsException(target) && !JS_IsNull(target) && !JS_IsUndefined(target)) {
                return target;
            }
            JS_FreeValue(ctx, target);
        } else {
            if (form_id) JS_FreeCString(ctx, form_id);
            JS_FreeValue(ctx, form_attr);
        }
    } else {
        JS_FreeValue(ctx, form_attr);
    }
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t our_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            uint32_t curr_id = nodes_arr[our_id].parent_id;
            while (curr_id != nodes_arr[curr_id].parent_id) {
                if (nodes_arr[curr_id].node_type == 1 &&
                    wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[curr_id].tag_name, "form")) {
                    return qjs_wrap_node(ctx, (dom_node *)(uintptr_t)curr_id);
                }
                curr_id = nodes_arr[curr_id].parent_id;
            }
        }
        return JS_NULL;
    }
    dom_node *curr = (dom_node *)priv->node;
    dom_node_ref(curr);
    while (curr) {
        dom_node *parent = NULL;
        dom_node_get_parent_node(curr, &parent);
        dom_node_unref(curr);
        curr = parent;
        if (curr) {
            dom_string *tag_name = NULL;
            dom_node_get_node_name(curr, &tag_name);
            if (tag_name) {
                if (strcasecmp((const char *)dom_string_data(tag_name), "form") == 0) {
                    dom_string_unref(tag_name);
                    JSValue form_val = qjs_wrap_node(ctx, curr);
                    dom_node_unref(curr);
                    return form_val;
                }
                dom_string_unref(tag_name);
            }
        }
    }
    return JS_NULL;
}

static bool is_form_control(const char *tag)
{
    if (!tag) return false;
    return (strcasecmp(tag, "input") == 0 ||
            strcasecmp(tag, "button") == 0 ||
            strcasecmp(tag, "select") == 0 ||
            strcasecmp(tag, "textarea") == 0);
}

static void find_form_controls_libdom(JSContext *ctx, dom_node *parent, JSValue arr, uint32_t *count)
{
    dom_node *child = NULL;
    dom_node_get_first_child(parent, &child);
    while (child) {
        dom_node_type type;
        dom_node_get_node_type(child, &type);
        if (type == DOM_ELEMENT_NODE) {
            dom_string *tag_name = NULL;
            dom_node_get_node_name(child, &tag_name);
            if (tag_name) {
                const char *tag_str = (const char *)dom_string_data(tag_name);
                if (is_form_control(tag_str)) {
                    JS_SetPropertyUint32(ctx, arr, (*count)++, qjs_wrap_node(ctx, child));
                }
                dom_string_unref(tag_name);
            }
            find_form_controls_libdom(ctx, child, arr, count);
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
}

// -----------------------------------------------------------------------------
// HTMLElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlelement_blur_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_click_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv) return JS_UNDEFINED;
    struct jsthread *thread = JS_GetContextOpaque(ctx);
    if (!thread) return JS_UNDEFINED;

    if (wisp_is_js_process || !priv->is_dom_node || !priv->node) {
        JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
        if (JS_IsObject(wrapper)) {
            JSValue dispatch = JS_GetPropertyStr(ctx, wrapper, "dispatchEvent");
            if (JS_IsFunction(ctx, dispatch)) {
                JSValue evt_ctor = JS_GetPropertyStr(ctx, wrapper, "Event");
                if (JS_IsUndefined(evt_ctor)) {
                    JSValue global = JS_GetGlobalObject(ctx);
                    evt_ctor = JS_GetPropertyStr(ctx, global, "Event");
                    JS_FreeValue(ctx, global);
                }
                JSValue init = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, init, "bubbles", JS_TRUE);
                JS_SetPropertyStr(ctx, init, "cancelable", JS_TRUE);
                JSValue type_val = JS_NewString(ctx, "click");
                JSValue args[2] = { type_val, init };
                JSValue evt = JS_CallConstructor(ctx, evt_ctor, 2, args);
                JS_FreeValue(ctx, type_val);
                JS_FreeValue(ctx, init);
                JS_FreeValue(ctx, evt_ctor);
                if (!JS_IsException(evt)) {
                    JSValue ret = JS_Call(ctx, dispatch, wrapper, 1, &evt);
                    JS_FreeValue(ctx, ret);
                    JS_FreeValue(ctx, evt);
                }
            }
            JS_FreeValue(ctx, dispatch);
        }
        JS_FreeValue(ctx, wrapper);
        return JS_UNDEFINED;
    }

    js_fire_event(thread, "click", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_focus_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_title_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "title", "");
}

JSValue wisp_htmlelement_title_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "title", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_lang_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "lang", "");
}

JSValue wisp_htmlelement_lang_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "lang", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_dir_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "dir", "");
}

JSValue wisp_htmlelement_dir_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "dir", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_hidden_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "hidden");
}

JSValue wisp_htmlelement_hidden_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "hidden", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_tabIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "tabindex", -1));
}

JSValue wisp_htmlelement_tabIndex_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "tabindex", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLIFrameElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmliframeelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "name", "");
}

JSValue wisp_htmliframeelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmliframeelement_sandbox_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    extern JSValue qjs_new_domtokenlist(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_domtokenlist(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmliframeelement_contentDocument_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NULL;
}

JSValue wisp_htmliframeelement_contentWindow_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NULL;
}

// -----------------------------------------------------------------------------
// HTMLFormElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlformelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "name", "");
}

JSValue wisp_htmlformelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlformelement_reset_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (wisp_is_js_process) return JS_UNDEFINED;
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLFormElement target");
    dom_node *doc = NULL;
    dom_node_get_owner_document((dom_node *)priv->node, (dom_document **)&doc);
    if (doc) {
        struct html_content *htmlc = NULL;
        dom_node_get_user_data((dom_node *)doc, corestring_dom___ns_key_html_content_data, (void **)&htmlc);
        dom_node_unref((dom_node *)doc);
        if (htmlc) {
            struct form *f = NULL;
            for (f = htmlc->forms; f != NULL; f = f->prev) {
                if (f->node == priv->node) break;
            }
            if (f && f->controls) {
                for (struct form_control *c = f->controls; c != NULL; c = c->next) {
                    if (c->initial_value) {
                        form_gadget_update_value(c, c->initial_value);
                    }
                }
            }
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlformelement_submit_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (wisp_is_js_process) return JS_UNDEFINED;
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLFormElement target");
    dom_node *doc = NULL;
    dom_node_get_owner_document((dom_node *)priv->node, (dom_document **)&doc);
    if (doc) {
        struct html_content *htmlc = NULL;
        dom_node_get_user_data((dom_node *)doc, corestring_dom___ns_key_html_content_data, (void **)&htmlc);
        dom_node_unref((dom_node *)doc);
        if (htmlc) {
            struct form *f = NULL;
            for (f = htmlc->forms; f != NULL; f = f->prev) {
                if (f->node == priv->node) break;
            }
            if (f) {
                struct nsurl *page_url = content_get_url((struct content *)htmlc);
                if (page_url && htmlc->bw) form_submit(page_url, htmlc->bw, f, NULL);
            }
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlformelement_elements_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewArray(ctx);
    JSValue arr = JS_NewArray(ctx);
    uint32_t count = 0;

    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t form_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].node_type == 1) { // Element node
                    const char *tag = wisp_string_ref_data(wisp_shm_dom, strings_arr[i].tag_name);
                    if (is_form_control(tag)) {
                        uint32_t curr = nodes_arr[i].parent_id;
                        while (curr != nodes_arr[curr].parent_id) {
                            if (curr == form_id) {
                                JS_SetPropertyUint32(ctx, arr, count++, qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i));
                                break;
                            }
                            curr = nodes_arr[curr].parent_id;
                        }
                    }
                }
            }
        }
        return arr;
    }

    find_form_controls_libdom(ctx, (dom_node *)priv->node, arr, &count);
    return arr;
}

JSValue wisp_htmlformelement_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue arr = wisp_htmlformelement_elements_get_impl(ctx, priv);
    JSValue len_val = JS_GetPropertyStr(ctx, arr, "length");
    JS_FreeValue(ctx, arr);
    return len_val;
}

// -----------------------------------------------------------------------------
// HTMLTextAreaElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmltextareaelement_placeholder_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "placeholder", "");
}

JSValue wisp_htmltextareaelement_placeholder_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "placeholder", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_readOnly_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "readonly");
}

JSValue wisp_htmltextareaelement_readOnly_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "readonly", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_required_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "required");
}

JSValue wisp_htmltextareaelement_required_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "required", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_cols_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "cols", 20));
}

JSValue wisp_htmltextareaelement_cols_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    set_element_int_attr(ctx, priv, "cols", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_rows_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "rows", 2));
}

JSValue wisp_htmltextareaelement_rows_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    set_element_int_attr(ctx, priv, "rows", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_maxLength_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "maxlength", -1));
}

JSValue wisp_htmltextareaelement_maxLength_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "maxlength", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_minLength_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "minlength", -1));
}

JSValue wisp_htmltextareaelement_minLength_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "minlength", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "textarea");
}

// -----------------------------------------------------------------------------
// HTMLInputElement Implementation (Additional stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlinputelement_maxLength_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "maxlength", -1));
}

JSValue wisp_htmlinputelement_maxLength_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "maxlength", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_minLength_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "minlength", -1));
}

JSValue wisp_htmlinputelement_minLength_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "minlength", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_pattern_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "pattern", "");
}

JSValue wisp_htmlinputelement_pattern_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "pattern", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_form_impl(ctx, priv);
}

// -----------------------------------------------------------------------------
// HTMLButtonElement Implementation (Additional stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlbuttonelement_autofocus_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "autofocus");
}

JSValue wisp_htmlbuttonelement_autofocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "autofocus", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbuttonelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_form_impl(ctx, priv);
}

// -----------------------------------------------------------------------------
// HTMLBodyElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlbodyelement_background_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "background", "");
}

JSValue wisp_htmlbodyelement_background_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "background", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbodyelement_bgColor_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "bgcolor", "");
}

JSValue wisp_htmlbodyelement_bgColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "bgcolor", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbodyelement_text_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "text", "");
}

JSValue wisp_htmlbodyelement_text_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "text", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLOptionElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmloptionelement_defaultSelected_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_defaultSelected_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "disabled");
}

JSValue wisp_htmloptionelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "disabled", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "disabled");
    }
}

JSValue wisp_htmloptionelement_selected_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_selected_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_text_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_text_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }

JSValue wisp_htmloptionelement_label_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue label_val = wisp_element_getAttribute_impl(ctx, priv, "label");
    if (JS_IsNull(label_val) || JS_IsUndefined(label_val)) {
        return wisp_htmloptionelement_text_get_impl(ctx, priv);
    }
    return label_val;
}

JSValue wisp_htmloptionelement_label_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "label", value);
}

JSValue wisp_htmloptionelement_index_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    int idx = 0;

    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t our_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            uint32_t parent_id = nodes_arr[our_id].parent_id;
            if (parent_id != our_id) {
                for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                    if (nodes_arr[i].parent_id == parent_id && nodes_arr[i].node_type == 1 &&
                        wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "option")) {
                        if (i == our_id) {
                            return JS_NewInt32(ctx, idx);
                        }
                        idx++;
                    }
                }
            }
        }
        return JS_NewInt32(ctx, 0);
    }

    dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    if (parent) {
        dom_node *child = NULL;
        dom_node_get_first_child(parent, &child);
        while (child) {
            dom_string *tag_name = NULL;
            dom_node_get_node_name(child, &tag_name);
            if (tag_name) {
                if (strcasecmp((const char *)dom_string_data(tag_name), "option") == 0) {
                    if (child == (dom_node *)priv->node) {
                        dom_string_unref(tag_name);
                        dom_node_unref(child);
                        dom_node_unref(parent);
                        return JS_NewInt32(ctx, idx);
                    }
                    idx++;
                }
                dom_string_unref(tag_name);
            }
            dom_node *next = NULL;
            dom_node_get_next_sibling(child, &next);
            dom_node_unref(child);
            child = next;
        }
        dom_node_unref(parent);
    }
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_htmloptionelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t our_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            uint32_t curr_id = nodes_arr[our_id].parent_id;
            while (curr_id != nodes_arr[curr_id].parent_id) {
                if (nodes_arr[curr_id].node_type == 1 &&
                    wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[curr_id].tag_name, "form")) {
                    return qjs_wrap_node(ctx, (dom_node *)(uintptr_t)curr_id);
                }
                curr_id = nodes_arr[curr_id].parent_id;
            }
        }
        return JS_NULL;
    }
    dom_node *curr = (dom_node *)priv->node;
    dom_node_ref(curr);
    while (curr) {
        dom_node *parent = NULL;
        dom_node_get_parent_node(curr, &parent);
        dom_node_unref(curr);
        curr = parent;
        if (curr) {
            dom_string *tag_name = NULL;
            dom_node_get_node_name(curr, &tag_name);
            if (tag_name) {
                if (strcasecmp((const char *)dom_string_data(tag_name), "form") == 0) {
                    dom_string_unref(tag_name);
                    JSValue form_val = qjs_wrap_node(ctx, curr);
                    dom_node_unref(curr);
                    return form_val;
                }
                dom_string_unref(tag_name);
            }
        }
    }
    return JS_NULL;
}

JSValue wisp_htmloptionelement_Option_impl(JSContext *ctx, const char * text, const char * value, bool defaultSelected, bool selected)
{
    if (wisp_is_js_process) {
        JSValue global_obj = JS_GetGlobalObject(ctx);
        JSValue document = JS_GetPropertyStr(ctx, global_obj, "document");
        JSValue create_element = JS_GetPropertyStr(ctx, document, "createElement");
        JSValue tag = JS_NewString(ctx, "option");
        JSValue opt = JS_Call(ctx, create_element, document, 1, &tag);
        JS_FreeValue(ctx, tag);
        JS_FreeValue(ctx, create_element);
        JS_FreeValue(ctx, document);
        JS_FreeValue(ctx, global_obj);

        if (JS_IsException(opt)) return opt;

        if (text && strlen(text) > 0) {
            JSValue text_val = JS_NewString(ctx, text);
            JS_SetPropertyStr(ctx, opt, "text", text_val);
        }
        if (value && strlen(value) > 0) {
            JSValue val_val = JS_NewString(ctx, value);
            JS_SetPropertyStr(ctx, opt, "value", val_val);
        }
        if (defaultSelected) {
            JS_SetPropertyStr(ctx, opt, "defaultSelected", JS_TRUE);
        }
        if (selected) {
            JS_SetPropertyStr(ctx, opt, "selected", JS_TRUE);
        }
        return opt;
    }

    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_NULL;
    struct dom_document *doc = qjs_thread_get_document(t);
    if (!doc) return JS_NULL;

    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)"option", 6, &name_dom);
    struct dom_element *result = NULL;
    dom_document_create_element(doc, name_dom, &result);
    dom_string_unref(name_dom);

    if (result) {
        JSValue opt = qjs_wrap_node(ctx, (dom_node *)result);
        QJSNodePrivate *priv = qjs_get_dom_priv(ctx, opt);

        if (text && strlen(text) > 0) {
            wisp_htmloptionelement_text_set_impl(ctx, priv, text);
        }
        if (value && strlen(value) > 0) {
            wisp_htmloptionelement_value_set_impl(ctx, priv, value);
        }
        if (defaultSelected) {
            wisp_htmloptionelement_defaultSelected_set_impl(ctx, priv, defaultSelected);
        }
        if (selected) {
            wisp_htmloptionelement_selected_set_impl(ctx, priv, selected);
        }

        dom_node_unref((dom_node *)result);
        return opt;
    }
    return JS_NULL;
}

// -----------------------------------------------------------------------------
// HTMLSelectElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlselectelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewString(ctx, "");

    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t our_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].parent_id == our_id && nodes_arr[i].node_type == 1 &&
                    wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "option")) {
                    bool is_sel = false;
                    uint32_t limit1 = strings_arr[i].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings_arr[i].attr_count : WISP_SHM_MAX_ATTRIBUTES;
                    for (uint32_t j = 0; j < limit1; j++) {
                        if (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].attrs[j].name, "selected")) {
                            is_sel = true;
                            break;
                        }
                    }
                    if (is_sel) {
                        uint32_t limit2 = strings_arr[i].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings_arr[i].attr_count : WISP_SHM_MAX_ATTRIBUTES;
                        for (uint32_t j = 0; j < limit2; j++) {
                            if (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].attrs[j].name, "value")) {
                                return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, strings_arr[i].attrs[j].value));
                            }
                        }
                        for (uint32_t j = 1; j < wisp_shm_dom->node_count; j++) {
                            if (nodes_arr[j].parent_id == i && nodes_arr[j].node_type == 3) {
                                return JS_NewString(ctx, wisp_string_ref_data(wisp_shm_dom, strings_arr[j].value));
                            }
                        }
                        return JS_NewString(ctx, "");
                    }
                }
            }
        }
        return JS_NewString(ctx, "");
    }

    dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    while (child) {
        dom_string *tag_name = NULL;
        dom_node_get_node_name(child, &tag_name);
        if (tag_name) {
            if (strcasecmp((const char *)dom_string_data(tag_name), "option") == 0) {
                dom_string_unref(tag_name);
                dom_string *attr_name = NULL;
                dom_string_create((const uint8_t *)"selected", 8, &attr_name);
                bool has_sel = false;
                dom_element_has_attribute((dom_element *)child, attr_name, &has_sel);
                dom_string_unref(attr_name);
                if (has_sel) {
                    dom_string *val_dom = NULL;
                    dom_string_create((const uint8_t *)"value", 5, &attr_name);
                    dom_element_get_attribute((dom_element *)child, attr_name, &val_dom);
                    dom_string_unref(attr_name);
                    if (val_dom) {
                        JSValue res = JS_NewStringLen(ctx, (const char *)dom_string_data(val_dom), dom_string_byte_length(val_dom));
                        dom_string_unref(val_dom);
                        dom_node_unref(child);
                        return res;
                    }
                    dom_string *text = NULL;
                    dom_node_get_text_content(child, &text);
                    if (text) {
                        JSValue res = JS_NewStringLen(ctx, (const char *)dom_string_data(text), dom_string_byte_length(text));
                        dom_string_unref(text);
                        dom_node_unref(child);
                        return res;
                    }
                    dom_node_unref(child);
                    return JS_NewString(ctx, "");
                }
            } else {
                dom_string_unref(tag_name);
            }
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlselectelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    if (!priv || !priv->node || !value) return JS_UNDEFINED;

    if (wisp_is_js_process) {
        return wisp_element_setAttribute_impl(ctx, priv, "value", value);
    }

    dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    while (child) {
        dom_string *tag_name = NULL;
        dom_node_get_node_name(child, &tag_name);
        if (tag_name) {
            if (strcasecmp((const char *)dom_string_data(tag_name), "option") == 0) {
                dom_string_unref(tag_name);
                bool match = false;

                dom_string *attr_name = NULL;
                dom_string *val_dom = NULL;
                dom_string_create((const uint8_t *)"value", 5, &attr_name);
                dom_element_get_attribute((dom_element *)child, attr_name, &val_dom);
                dom_string_unref(attr_name);
                if (val_dom) {
                    if (strcmp((const char *)dom_string_data(val_dom), value) == 0) {
                        match = true;
                    }
                    dom_string_unref(val_dom);
                } else {
                    dom_string *text = NULL;
                    dom_node_get_text_content(child, &text);
                    if (text) {
                        if (strcmp((const char *)dom_string_data(text), value) == 0) {
                            match = true;
                        }
                        dom_string_unref(text);
                    }
                }

                dom_string_create((const uint8_t *)"selected", 8, &attr_name);
                if (match) {
                    dom_element_set_attribute((dom_element *)child, attr_name, attr_name);
                } else {
                    dom_element_remove_attribute((dom_element *)child, attr_name);
                }
                dom_string_unref(attr_name);
            } else {
                dom_string_unref(tag_name);
            }
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlselectelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "disabled");
}

JSValue wisp_htmlselectelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "disabled", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "disabled");
    }
}

JSValue wisp_htmlselectelement_autofocus_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "autofocus");
}

JSValue wisp_htmlselectelement_autofocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "autofocus", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "autofocus");
    }
}

JSValue wisp_htmlselectelement_required_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "required");
}

JSValue wisp_htmlselectelement_required_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "required", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "required");
    }
}

JSValue wisp_htmlselectelement_autocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "autocomplete");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlselectelement_autocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "autocomplete", value);
}

JSValue wisp_htmlselectelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "name");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlselectelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "name", value);
}

JSValue wisp_htmlselectelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    bool multi = JS_ToBool(ctx, wisp_element_hasAttribute_impl(ctx, priv, "multiple"));
    return JS_NewString(ctx, multi ? "select-multiple" : "select-one");
}

JSValue wisp_htmlselectelement_multiple_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "multiple");
}

JSValue wisp_htmlselectelement_multiple_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "multiple", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "multiple");
    }
}

JSValue wisp_htmlselectelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_htmloptionelement_form_get_impl(ctx, priv);
}

JSValue wisp_htmlselectelement_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_length_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_selectedIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_selectedIndex_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_options_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) { return JS_UNDEFINED; }

extern JSValue wisp_node_appendChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node);
extern JSValue wisp_node_removeChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * child);

JSValue wisp_htmlselectelement_add_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue element, JSValue before) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_remove_0_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_remove_1_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) { return JS_UNDEFINED; }

JSValue wisp_htmlselectelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_htmlselectelement_selectedOptions_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_htmlselectelement_size_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_htmlselectelement_size_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlselectelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__customValidity", "");
}

JSValue wisp_htmlselectelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmlselectelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_htmlselectelement___setter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index, void * option) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlselectelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlselectelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlselectelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlselectelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlselectelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLInputElement Additional Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlinputelement_placeholder_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "placeholder");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlinputelement_placeholder_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "placeholder", value);
}

JSValue wisp_htmlinputelement_readOnly_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "readonly");
}

JSValue wisp_htmlinputelement_readOnly_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "readonly", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "readonly");
    }
}

JSValue wisp_htmlinputelement_required_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "required");
}

JSValue wisp_htmlinputelement_required_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "required", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "required");
    }
}

JSValue wisp_htmlinputelement_autocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "autocomplete");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlinputelement_autocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "autocomplete", value);
}

JSValue wisp_htmlinputelement_autofocus_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "autofocus");
}

JSValue wisp_htmlinputelement_autofocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "autofocus", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "autofocus");
    }
}

// -----------------------------------------------------------------------------
// HTMLButtonElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlbuttonelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "disabled");
}

JSValue wisp_htmlbuttonelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "disabled", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "disabled");
    }
}

JSValue wisp_htmlbuttonelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "type");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "submit");
    }
    return val;
}

JSValue wisp_htmlbuttonelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "type", value);
}

JSValue wisp_htmlbuttonelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "value");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlbuttonelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "value", value);
}

JSValue wisp_htmlbuttonelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "name");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlbuttonelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "name", value);
}

// -----------------------------------------------------------------------------
// HTMLFormElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlformelement_action_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "action");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlformelement_action_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "action", value);
}

JSValue wisp_htmlformelement_method_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "method");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "get");
    }
    return val;
}

JSValue wisp_htmlformelement_method_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "method", value);
}

JSValue wisp_htmlformelement_target_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "target");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlformelement_target_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "target", value);
}

// -----------------------------------------------------------------------------
// HTMLLinkElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmllinkelement_href_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "href");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmllinkelement_href_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "href", value);
}

JSValue wisp_htmllinkelement_rel_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "rel");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmllinkelement_rel_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "rel", value);
}

JSValue wisp_htmllinkelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "type");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmllinkelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "type", value);
}

JSValue wisp_htmllinkelement_media_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "media");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmllinkelement_media_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "media", value);
}

// -----------------------------------------------------------------------------
// HTMLStyleElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlstyleelement_media_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "media");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlstyleelement_media_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "media", value);
}

JSValue wisp_htmlstyleelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "type");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "text/css");
    }
    return val;
}

JSValue wisp_htmlstyleelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "type", value);
}

// -----------------------------------------------------------------------------
// HTMLMetaElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlmetaelement_content_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "content");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlmetaelement_content_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "content", value);
}

JSValue wisp_htmlmetaelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "name");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlmetaelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "name", value);
}

JSValue wisp_htmlmetaelement_httpEquiv_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "http-equiv");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlmetaelement_httpEquiv_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "http-equiv", value);
}

JSValue wisp_htmlmetaelement_scheme_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "scheme");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmlmetaelement_scheme_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "scheme", value);
}

// -----------------------------------------------------------------------------
// History Implementation
// -----------------------------------------------------------------------------

JSValue wisp_history_back_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_UNDEFINED;
}

JSValue wisp_history_forward_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_UNDEFINED;
}

JSValue wisp_history_go_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t delta)
{
    return JS_UNDEFINED;
}

JSValue wisp_history_pushState_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue data, const char * title, const char * url)
{
    return JS_UNDEFINED;
}

JSValue wisp_history_replaceState_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue data, const char * title, const char * url)
{
    return JS_UNDEFINED;
}

JSValue wisp_history_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, 1);
}

JSValue wisp_history_state_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NULL;
}

// -----------------------------------------------------------------------------
// Location Implementation (Methods)
// -----------------------------------------------------------------------------

JSValue wisp_location_assign_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url)
{
    if (!url || strlen(url) == 0) return JS_UNDEFINED;
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_UNDEFINED;

    if (!wisp_is_js_process) {
        if (t->win_priv && t->win_priv != t->doc_priv) {
            struct browser_window *bw = (struct browser_window *)t->win_priv;
            struct nsurl *base_url = get_location_nsurl(ctx);
            struct nsurl *target_url = NULL;
            nserror err = NSERROR_BAD_URL;
            if (base_url) {
                err = nsurl_join(base_url, url, &target_url);
            }
            if (err != NSERROR_OK) {
                err = nsurl_create(url, &target_url);
            }
            if (err == NSERROR_OK && target_url) {
                bw->js_navigated = true;
                browser_window_navigate(bw, target_url, base_url, BW_NAVIGATE_HISTORY, NULL, NULL, NULL);
                nsurl_unref(target_url);
            }
        } else {
            if (t->location_url) {
                nsurl_unref(t->location_url);
                t->location_url = NULL;
            }
            nsurl_create(url, &t->location_url);
        }
    } else {
        extern wisp_ipc_handle *ipc_main;
        if (ipc_main) {
            wisp_ipc_msg req;
            req.type = WISP_IPC_MSG_NAVIGATE;
            req.length = (uint32_t)strlen(url) + 1;
            req.data = (uint8_t *)strdup(url);
            if (req.data) {
                wisp_ipc_send(ipc_main, &req);
                free(req.data);

                wisp_ipc_set_blocking(ipc_main, true);
                wisp_ipc_msg resp;
                while (wisp_ipc_recv(ipc_main, &resp) == NSERROR_OK) {
                    if (resp.type == WISP_IPC_MSG_DOM_RESPONSE) {
                        wisp_ipc_msg_free(&resp);
                        break;
                    }
                    wisp_ipc_msg_free(&resp);
                }
                wisp_ipc_set_blocking(ipc_main, false);
            }
        }
    }
    return JS_UNDEFINED;
}

JSValue wisp_location_replace_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url)
{
    return wisp_location_assign_impl(ctx, priv, url);
}

JSValue wisp_location_reload_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct nsurl *url = get_location_nsurl(ctx);
    if (url) {
        const char *url_str = nsurl_access(url);
        return wisp_location_assign_impl(ctx, priv, url_str);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_checked_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "checked");
}

JSValue wisp_htmlinputelement_checked_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "checked", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "checked");
    }
}

// -----------------------------------------------------------------------------
// HTMLIFrameElement Implementation (6 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmliframeelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "src");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmliframeelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "src", value);
}

JSValue wisp_htmliframeelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "width");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmliframeelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "width", value);
}

JSValue wisp_htmliframeelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "height");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmliframeelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "height", value);
}

// -----------------------------------------------------------------------------
// HTMLTextAreaElement Implementation (6 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltextareaelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "value");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmltextareaelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "value", value);
}

JSValue wisp_htmltextareaelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "name");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    return val;
}

JSValue wisp_htmltextareaelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "name", value);
}

JSValue wisp_htmltextareaelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "disabled");
}

JSValue wisp_htmltextareaelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "disabled", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "disabled");
    }
}


// -----------------------------------------------------------------------------
// HTMLDivElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmldivelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "align", "");
}

JSValue wisp_htmldivelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLParagraphElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlparagraphelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "align", "");
}

JSValue wisp_htmlparagraphelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLHeadingElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlheadingelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "align", "");
}

JSValue wisp_htmlheadingelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLBRElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlbrelement_clear_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "clear", "");
}

JSValue wisp_htmlbrelement_clear_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "clear", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLHRElement Implementation (10 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlhrelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "align", "");
}

JSValue wisp_htmlhrelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlhrelement_color_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "color", "");
}

JSValue wisp_htmlhrelement_color_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "color", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlhrelement_noShade_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "noshade");
}

JSValue wisp_htmlhrelement_noShade_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "noshade", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlhrelement_size_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "size", "");
}

JSValue wisp_htmlhrelement_size_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "size", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlhrelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "width", "");
}

JSValue wisp_htmlhrelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLPreElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlpreelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "width", 0));
}

JSValue wisp_htmlpreelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLQuoteElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlquoteelement_cite_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "cite", "");
}

JSValue wisp_htmlquoteelement_cite_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "cite", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLOListElement Implementation (8 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlolistelement_compact_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "compact");
}

JSValue wisp_htmlolistelement_compact_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "compact", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// Double attribute helpers
// -----------------------------------------------------------------------------

static double get_element_double_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name, double default_val)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, name);
    double res_val = default_val;
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        if (str && strlen(str) > 0) {
            res_val = atof(str);
        }
        if (str) JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, val);
    return res_val;
}

static void set_element_double_attr(JSContext *ctx, QJSNodePrivate *priv, const char *name, double value)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", value);
    wisp_element_setAttribute_impl(ctx, priv, name, buf);
}

// -----------------------------------------------------------------------------
// HTMLVideoElement Implementation (8 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlvideoelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "height", 0));
}
JSValue wisp_htmlvideoelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "height", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlvideoelement_poster_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "poster", "");
}
JSValue wisp_htmlvideoelement_poster_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "poster", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlvideoelement_videoHeight_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}
JSValue wisp_htmlvideoelement_videoWidth_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}
JSValue wisp_htmlvideoelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "width", 0));
}
JSValue wisp_htmlvideoelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLSourceElement Implementation (10 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlsourceelement_media_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "media", "");
}
JSValue wisp_htmlsourceelement_media_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "media", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlsourceelement_sizes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "sizes", "");
}
JSValue wisp_htmlsourceelement_sizes_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "sizes", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlsourceelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "src", "");
}
JSValue wisp_htmlsourceelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "src", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlsourceelement_srcset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "srcset", "");
}
JSValue wisp_htmlsourceelement_srcset_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "srcset", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlsourceelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "type", "");
}
JSValue wisp_htmlsourceelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLStyleElement Implementation (5 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlstyleelement_nonce_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "nonce", "");
}
JSValue wisp_htmlstyleelement_nonce_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "nonce", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlstyleelement_scoped_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "scoped");
}
JSValue wisp_htmlstyleelement_scoped_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "scoped", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlstyleelement_sheet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// -----------------------------------------------------------------------------
// HTMLAreaElement Implementation (40 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlareaelement_alt_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "alt", "");
}
JSValue wisp_htmlareaelement_alt_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "alt", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_coords_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "coords", "");
}
JSValue wisp_htmlareaelement_coords_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "coords", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_download_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "download", "");
}
JSValue wisp_htmlareaelement_download_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "download", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_hash_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "hash", "");
}
JSValue wisp_htmlareaelement_hash_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "hash", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_host_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "host", "");
}
JSValue wisp_htmlareaelement_host_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "host", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_hostname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "hostname", "");
}
JSValue wisp_htmlareaelement_hostname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "hostname", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_href_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "href", "");
}
JSValue wisp_htmlareaelement_hreflang_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "hreflang", "");
}
JSValue wisp_htmlareaelement_hreflang_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "hreflang", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_noHref_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "nohref");
}
JSValue wisp_htmlareaelement_noHref_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "nohref", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "origin", "");
}
JSValue wisp_htmlareaelement_password_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "password", "");
}
JSValue wisp_htmlareaelement_password_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "password", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_pathname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "pathname", "");
}
JSValue wisp_htmlareaelement_pathname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "pathname", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_ping_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "ping", "");
}
JSValue wisp_htmlareaelement_port_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "port", "");
}
JSValue wisp_htmlareaelement_port_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "port", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "protocol", "");
}
JSValue wisp_htmlareaelement_protocol_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "protocol", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_relList_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmlareaelement_rel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "rel", "");
}
JSValue wisp_htmlareaelement_rel_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "rel", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_search_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "search", "");
}
JSValue wisp_htmlareaelement_search_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "search", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_shape_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "shape", "");
}
JSValue wisp_htmlareaelement_shape_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "shape", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_target_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "target", "");
}
JSValue wisp_htmlareaelement_target_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "target", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "type", "");
}
JSValue wisp_htmlareaelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlareaelement_username_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "username", "");
}
JSValue wisp_htmlareaelement_username_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "username", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLMapElement Implementation (3 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlmapelement_areas_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmlmapelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "name", "");
}
JSValue wisp_htmlmapelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLFontElement Implementation (6 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlfontelement_color_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "color", "");
}
JSValue wisp_htmlfontelement_color_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "color", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlfontelement_face_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "face", "");
}
JSValue wisp_htmlfontelement_face_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "face", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlfontelement_size_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "size", "");
}
JSValue wisp_htmlfontelement_size_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "size", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLFrameElement Implementation (16 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlframeelement_contentDocument_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmlframeelement_contentWindow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmlframeelement_frameBorder_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "frameborder", "");
}
JSValue wisp_htmlframeelement_frameBorder_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "frameborder", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframeelement_longDesc_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "longdesc", "");
}
JSValue wisp_htmlframeelement_longDesc_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "longdesc", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframeelement_marginHeight_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "marginheight", "");
}
JSValue wisp_htmlframeelement_marginHeight_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "marginheight", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframeelement_marginWidth_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "marginwidth", "");
}
JSValue wisp_htmlframeelement_marginWidth_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "marginwidth", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframeelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "name", "");
}
JSValue wisp_htmlframeelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframeelement_noResize_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "noresize");
}
JSValue wisp_htmlframeelement_noResize_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "noresize", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframeelement_scrolling_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "scrolling", "");
}
JSValue wisp_htmlframeelement_scrolling_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "scrolling", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframeelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "src", "");
}
JSValue wisp_htmlframeelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "src", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLFrameSetElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlframesetelement_cols_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "cols", "");
}
JSValue wisp_htmlframesetelement_cols_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "cols", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlframesetelement_rows_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "rows", "");
}
JSValue wisp_htmlframesetelement_rows_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "rows", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLLegendElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmllegendelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmllegendelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLProgressElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlprogresselement_max_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "max", 1.0));
}
JSValue wisp_htmlprogresselement_max_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "max", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlprogresselement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "value", 0.0));
}
JSValue wisp_htmlprogresselement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "value", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLCollection Implementation (custom dynamic collection type wrapping)
// -----------------------------------------------------------------------------

typedef struct {
    QJSNodePrivate base;
    char type_name[32];
} QJSHTMLCollectionPrivate;

extern JSClassID qjs_htmlcollection_class_id;

JSValue qjs_new_htmlcollection(JSContext *ctx, void *node, bool is_dom_node)
{
    JSValue obj = JS_NewObjectClass(ctx, qjs_htmlcollection_class_id);
    if (JS_IsException(obj)) return obj;
    QJSHTMLCollectionPrivate *priv = calloc(1, sizeof(QJSHTMLCollectionPrivate));
    if (!priv) {
        JS_FreeValue(ctx, obj);
        return JS_ThrowOutOfMemory(ctx);
    }
    priv->base.magic = QJS_DOM_MAGIC;
    priv->base.node = node;
    priv->base.is_dom_node = is_dom_node;
    priv->base.ctx = ctx;
    strncpy(priv->type_name, "children", sizeof(priv->type_name) - 1); // Default is children
    priv->type_name[sizeof(priv->type_name) - 1] = '\0';
    if (!wisp_is_js_process && is_dom_node && node) dom_node_ref((dom_node *)node);
    JS_SetOpaque(obj, priv);
    return obj;
}

JSValue qjs_new_htmlcollection_with_type(JSContext *ctx, void *node, bool is_dom_node, const char *type)
{
    JSValue obj = qjs_new_htmlcollection(ctx, node, is_dom_node);
    if (!JS_IsException(obj)) {
        QJSHTMLCollectionPrivate *priv = (QJSHTMLCollectionPrivate *)JS_GetOpaque(obj, qjs_htmlcollection_class_id);
        if (priv && type) {
            strncpy(priv->type_name, type, sizeof(priv->type_name) - 1);
        }
    }
    return obj;
}

static void collect_elements_libdom(dom_node *parent, const char *type, dom_node **list, int *count, int max_count)
{
    dom_node *child = NULL;
    dom_node_get_first_child(parent, &child);
    while (child) {
        dom_node_type node_type;
        dom_node_get_node_type(child, &node_type);
        if (node_type == DOM_ELEMENT_NODE) {
            bool match = false;
            dom_string *tag_dom = NULL;
            dom_node_get_node_name(child, &tag_dom);
            if (tag_dom) {
                const char *tag = (const char *)dom_string_data(tag_dom);
                if (strcmp(type, "children") == 0) {
                    match = true;
                } else if (strcmp(type, "images") == 0 && strcasecmp(tag, "img") == 0) {
                    match = true;
                } else if (strcmp(type, "forms") == 0 && strcasecmp(tag, "form") == 0) {
                    match = true;
                } else if (strcmp(type, "scripts") == 0 && strcasecmp(tag, "script") == 0) {
                    match = true;
                } else if (strcmp(type, "plugins") == 0 && (strcasecmp(tag, "embed") == 0 || strcasecmp(tag, "object") == 0)) {
                    match = true;
                } else if (strcmp(type, "embeds") == 0 && strcasecmp(tag, "embed") == 0) {
                    match = true;
                } else if (strcmp(type, "applets") == 0 && strcasecmp(tag, "applet") == 0) {
                    match = true;
                } else if (strcmp(type, "links") == 0 && (strcasecmp(tag, "a") == 0 || strcasecmp(tag, "area") == 0)) {
                    dom_string *href_dom = NULL;
                    dom_string_create((const uint8_t *)"href", 4, &href_dom);
                    bool has_href = false;
                    dom_element_has_attribute((dom_element *)child, href_dom, &has_href);
                    dom_string_unref(href_dom);
                    if (has_href) match = true;
                } else if (strcmp(type, "anchors") == 0 && strcasecmp(tag, "a") == 0) {
                    dom_string *name_dom = NULL;
                    dom_string_create((const uint8_t *)"name", 4, &name_dom);
                    bool has_name = false;
                    dom_element_has_attribute((dom_element *)child, name_dom, &has_name);
                    dom_string_unref(name_dom);
                    if (has_name) match = true;
                }
                dom_string_unref(tag_dom);
            }
            if (match) {
                if (list) {
                    if (*count < max_count) {
                        dom_node_ref(child);
                        list[*count] = child;
                        (*count)++;
                    }
                } else {
                    (*count)++;
                }
            }
            if (strcmp(type, "children") != 0) {
                collect_elements_libdom(child, type, list, count, max_count);
            }
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
}

static void collect_elements_shm(uint32_t parent_id, const char *type, uint32_t *list, int *count, int max_count)
{
    if (!wisp_shm_dom) return;
    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    WispNodeStrings *strings = shm_dom_get_node_strings(wisp_shm_dom);

    for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
        if (nodes[i].node_type != 1) continue;

        bool is_descendant = false;
        if (strcmp(type, "children") == 0) {
            is_descendant = (nodes[i].parent_id == parent_id);
        } else {
            uint32_t curr = nodes[i].parent_id;
            while (curr != 0 && curr != nodes[curr].parent_id) {
                if (curr == parent_id) {
                    is_descendant = true;
                    break;
                }
                curr = nodes[curr].parent_id;
            }
        }

        if (is_descendant) {
            bool match = false;
            const char *tag = wisp_string_ref_data(wisp_shm_dom, strings[i].tag_name);
            if (strcmp(type, "children") == 0) {
                match = true;
            } else if (strcmp(type, "images") == 0 && strcasecmp(tag, "img") == 0) {
                match = true;
            } else if (strcmp(type, "forms") == 0 && strcasecmp(tag, "form") == 0) {
                match = true;
            } else if (strcmp(type, "scripts") == 0 && strcasecmp(tag, "script") == 0) {
                match = true;
            } else if (strcmp(type, "plugins") == 0 && (strcasecmp(tag, "embed") == 0 || strcasecmp(tag, "object") == 0)) {
                match = true;
            } else if (strcmp(type, "embeds") == 0 && strcasecmp(tag, "embed") == 0) {
                match = true;
            } else if (strcmp(type, "applets") == 0 && strcasecmp(tag, "applet") == 0) {
                match = true;
            } else if (strcmp(type, "links") == 0 && (strcasecmp(tag, "a") == 0 || strcasecmp(tag, "area") == 0)) {
                uint32_t limit = strings[i].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings[i].attr_count : WISP_SHM_MAX_ATTRIBUTES;
                for (uint32_t j = 0; j < limit; j++) {
                    if (wisp_string_ref_caseeq(wisp_shm_dom, strings[i].attrs[j].name, "href")) {
                        match = true;
                        break;
                    }
                }
            } else if (strcmp(type, "anchors") == 0 && strcasecmp(tag, "a") == 0) {
                uint32_t limit = strings[i].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings[i].attr_count : WISP_SHM_MAX_ATTRIBUTES;
                for (uint32_t j = 0; j < limit; j++) {
                    if (wisp_string_ref_caseeq(wisp_shm_dom, strings[i].attrs[j].name, "name")) {
                        match = true;
                        break;
                    }
                }
            }

            if (match) {
                if (list) {
                    if (*count < max_count) {
                        list[*count] = i;
                        (*count)++;
                    }
                } else {
                    (*count)++;
                }
            }
        }
    }
}

JSValue wisp_htmlcollection_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    QJSHTMLCollectionPrivate *cpriv = (QJSHTMLCollectionPrivate *)priv;
    int count = 0;
    if (wisp_is_js_process) {
        uint32_t parent_id = (uint32_t)(uintptr_t)priv->node;
        collect_elements_shm(parent_id, cpriv->type_name, NULL, &count, 10000);
    } else {
        collect_elements_libdom((dom_node *)priv->node, cpriv->type_name, NULL, &count, 10000);
    }
    return JS_NewInt32(ctx, count);
}

JSValue wisp_htmlcollection_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    if (!priv || !priv->node) return JS_NULL;
    QJSHTMLCollectionPrivate *cpriv = (QJSHTMLCollectionPrivate *)priv;
    int count = 0;
    if (wisp_is_js_process) {
        uint32_t parent_id = (uint32_t)(uintptr_t)priv->node;
        uint32_t *list = malloc(sizeof(uint32_t) * 1000);
        if (!list) return JS_ThrowOutOfMemory(ctx);
        collect_elements_shm(parent_id, cpriv->type_name, list, &count, 1000);
        JSValue res = JS_NULL;
        if (index < (uint32_t)count) {
            res = qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)list[index]);
        }
        free(list);
        return res;
    } else {
        dom_node **list = malloc(sizeof(dom_node *) * 1000);
        if (!list) return JS_ThrowOutOfMemory(ctx);
        collect_elements_libdom((dom_node *)priv->node, cpriv->type_name, list, &count, 1000);
        JSValue res = JS_NULL;
        if (index < (uint32_t)count) {
            res = qjs_wrap_node(ctx, list[index]);
        }
        for (int i = 0; i < count; i++) {
            dom_node_unref(list[i]);
        }
        free(list);
        return res;
    }
}

JSValue wisp_htmlcollection_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_NULL;
    QJSHTMLCollectionPrivate *cpriv = (QJSHTMLCollectionPrivate *)priv;
    int count = 0;
    if (wisp_is_js_process) {
        uint32_t parent_id = (uint32_t)(uintptr_t)priv->node;
        uint32_t *list = malloc(sizeof(uint32_t) * 1000);
        if (!list) return JS_ThrowOutOfMemory(ctx);
        collect_elements_shm(parent_id, cpriv->type_name, list, &count, 1000);
        JSValue res = JS_NULL;
        WispNodeStrings *strings = shm_dom_get_node_strings(wisp_shm_dom);
        for (int i = 0; i < count; i++) {
            uint32_t node_id = list[i];
            bool match = false;
            uint32_t limit = strings[node_id].attr_count < WISP_SHM_MAX_ATTRIBUTES ? strings[node_id].attr_count : WISP_SHM_MAX_ATTRIBUTES;
            for (uint32_t j = 0; j < limit; j++) {
                if ((wisp_string_ref_caseeq(wisp_shm_dom, strings[node_id].attrs[j].name, "id") ||
                     wisp_string_ref_caseeq(wisp_shm_dom, strings[node_id].attrs[j].name, "name")) &&
                    wisp_string_ref_caseeq(wisp_shm_dom, strings[node_id].attrs[j].value, name)) {
                    match = true;
                    break;
                }
            }
            if (match) {
                res = qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)node_id);
                break;
            }
        }
        free(list);
        return res;
    } else {
        dom_node **list = malloc(sizeof(dom_node *) * 1000);
        if (!list) return JS_ThrowOutOfMemory(ctx);
        collect_elements_libdom((dom_node *)priv->node, cpriv->type_name, list, &count, 1000);
        JSValue res = JS_NULL;
        for (int i = 0; i < count; i++) {
            dom_string *id_dom = NULL;
            dom_string *name_dom = NULL;
            dom_string *attr_id = NULL;
            dom_string_create((const uint8_t *)"id", 2, &attr_id);
            dom_element_get_attribute((dom_element *)list[i], attr_id, &id_dom);
            dom_string_unref(attr_id);

            dom_string_create((const uint8_t *)"name", 4, &attr_id);
            dom_element_get_attribute((dom_element *)list[i], attr_id, &name_dom);
            dom_string_unref(attr_id);

            bool match = false;
            if (id_dom && strcasecmp((const char *)dom_string_data(id_dom), name) == 0) {
                match = true;
            }
            if (!match && name_dom && strcasecmp((const char *)dom_string_data(name_dom), name) == 0) {
                match = true;
            }
            if (id_dom) dom_string_unref(id_dom);
            if (name_dom) dom_string_unref(name_dom);

            if (match) {
                res = qjs_wrap_node(ctx, list[i]);
                break;
            }
        }
        for (int i = 0; i < count; i++) {
            dom_node_unref(list[i]);
        }
        free(list);
        return res;
    }
}

// -----------------------------------------------------------------------------
// NodeList Implementation (childNodes)
// -----------------------------------------------------------------------------

static void collect_nodes_libdom(dom_node *parent, dom_node **list, int *count, int max_count)
{
    dom_node *child = NULL;
    dom_node_get_first_child(parent, &child);
    while (child) {
        if (list) {
            if (*count < max_count) {
                dom_node_ref(child);
                list[*count] = child;
                (*count)++;
            }
        } else {
            (*count)++;
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
}

static void collect_nodes_shm(uint32_t parent_id, uint32_t *list, int *count, int max_count)
{
    if (!wisp_shm_dom) return;
    WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
    WispCompactNode *parent = &nodes[parent_id];
    uint32_t curr_id = parent->first_child_id;
    while (curr_id != 0) {
        if (list) {
            if (*count < max_count) {
                list[*count] = curr_id;
                (*count)++;
            }
        } else {
            (*count)++;
        }
        curr_id = nodes[curr_id].next_sibling_id;
    }
}

JSValue wisp_nodelist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    int count = 0;
    if (wisp_is_js_process) {
        collect_nodes_shm((uint32_t)(uintptr_t)priv->node, NULL, &count, 10000);
    } else {
        collect_nodes_libdom((dom_node *)priv->node, NULL, &count, 10000);
    }
    return JS_NewInt32(ctx, count);
}

JSValue wisp_nodelist_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    if (!priv || !priv->node) return JS_NULL;
    int count = 0;
    if (wisp_is_js_process) {
        uint32_t *list = malloc(sizeof(uint32_t) * 1000);
        if (!list) return JS_ThrowOutOfMemory(ctx);
        collect_nodes_shm((uint32_t)(uintptr_t)priv->node, list, &count, 1000);
        JSValue res = JS_NULL;
        if (index < (uint32_t)count) {
            res = qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)list[index]);
        }
        free(list);
        return res;
    } else {
        dom_node **list = malloc(sizeof(dom_node *) * 1000);
        if (!list) return JS_ThrowOutOfMemory(ctx);
        collect_nodes_libdom((dom_node *)priv->node, list, &count, 1000);
        JSValue res = JS_NULL;
        if (index < (uint32_t)count) {
            res = qjs_wrap_node(ctx, list[index]);
        }
        for (int i = 0; i < count; i++) {
            dom_node_unref(list[i]);
        }
        free(list);
        return res;
    }
}

// -----------------------------------------------------------------------------
// StyleSheetList Implementation
// -----------------------------------------------------------------------------

JSValue wisp_stylesheetlist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_stylesheetlist_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    return JS_NULL;
}

// -----------------------------------------------------------------------------
// ChildNode & ParentNode Implementation
// -----------------------------------------------------------------------------

JSValue wisp_childnode_remove_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (wisp_is_js_process) {
        WispCompactNode *nodes = shm_dom_get_nodes(wisp_shm_dom);
        WispCompactNode *sn = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
        if (sn && sn->parent_id != 0) {
            shm_mutation_enqueue(wisp_shm_dom, SHM_MUTATION_REMOVE_CHILD, sn->parent_id, (uint64_t)(uintptr_t)priv->node, 0, NULL, NULL);
            WispCompactNode *parent_shm = find_shm_node(wisp_shm_dom, sn->parent_id);
            if (parent_shm) {
                if (parent_shm->first_child_id == (uint32_t)(uintptr_t)priv->node) {
                    parent_shm->first_child_id = sn->next_sibling_id;
                }
            }
            if (sn->prev_sibling_id != 0) {
                WispCompactNode *prev = find_shm_node(wisp_shm_dom, sn->prev_sibling_id);
                if (prev) prev->next_sibling_id = sn->next_sibling_id;
            }
            if (sn->next_sibling_id != 0) {
                WispCompactNode *next = find_shm_node(wisp_shm_dom, sn->next_sibling_id);
                if (next) next->prev_sibling_id = sn->prev_sibling_id;
            }
            sn->parent_id = 0;
            sn->prev_sibling_id = 0;
            sn->next_sibling_id = 0;
        }
        return JS_UNDEFINED;
    }
    dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    if (parent) {
        dom_node *removed = NULL;
        dom_node_remove_child(parent, (dom_node *)priv->node, &removed);
        dom_node_unref(parent);
        if (removed) dom_node_unref(removed);
    }
    return JS_UNDEFINED;
}

JSValue wisp_parentnode_append_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    QJSNodePrivate *node_priv = qjs_get_dom_priv(ctx, nodes);
    if (node_priv && node_priv->node) {
        extern JSValue wisp_node_appendChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node);
        wisp_node_appendChild_impl(ctx, priv, node_priv->node);
    }
    return JS_UNDEFINED;
}

JSValue wisp_parentnode_prepend_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes)
{
    if (!priv || !priv->node) return JS_UNDEFINED;
    QJSNodePrivate *node_priv = qjs_get_dom_priv(ctx, nodes);
    if (node_priv && node_priv->node) {
        extern JSValue wisp_node_insertBefore_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, void * child);
        if (wisp_is_js_process) {
            WispCompactNode *parent = find_shm_node(wisp_shm_dom, (uint64_t)(uintptr_t)priv->node);
            uint32_t first_id = parent ? parent->first_child_id : 0;
            wisp_node_insertBefore_impl(ctx, priv, node_priv->node, first_id ? (void*)(uintptr_t)first_id : NULL);
        } else {
            dom_node *first = NULL;
            dom_node_get_first_child((dom_node *)priv->node, &first);
            wisp_node_insertBefore_impl(ctx, priv, node_priv->node, first);
            if (first) dom_node_unref(first);
        }
    }
    return JS_UNDEFINED;
}

extern JSValue wisp_element_nextElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_element_previousElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv);

JSValue wisp_nondocumenttypechildnode_nextElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_nextElementSibling_get_impl(ctx, priv);
}

JSValue wisp_nondocumenttypechildnode_previousElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_previousElementSibling_get_impl(ctx, priv);
}

// -----------------------------------------------------------------------------
// Helper Utilities for Table Element Implementations
// -----------------------------------------------------------------------------

static JSValue create_element_helper(JSContext *ctx, const char *tag_name)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue document = JS_GetPropertyStr(ctx, global_obj, "document");
    JSValue create_element = JS_GetPropertyStr(ctx, document, "createElement");
    JSValue tag = JS_NewString(ctx, tag_name);
    JSValue el = JS_Call(ctx, create_element, document, 1, &tag);
    JS_FreeValue(ctx, tag);
    JS_FreeValue(ctx, create_element);
    JS_FreeValue(ctx, document);
    JS_FreeValue(ctx, global_obj);
    return el;
}

static JSValue call_js_method_1(JSContext *ctx, JSValue this_val, const char *method_name, JSValue arg)
{
    JSValue method = JS_GetPropertyStr(ctx, this_val, method_name);
    JSValue res = JS_Call(ctx, method, this_val, 1, &arg);
    JS_FreeValue(ctx, method);
    JS_FreeValue(ctx, arg);
    return res;
}

static JSValue get_first_child_by_tag(JSContext *ctx, JSValue parent, const char *tag)
{
    JSValue children = JS_GetPropertyStr(ctx, parent, "children");
    if (JS_IsException(children)) return JS_NULL;
    uint32_t len = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, children, "length");
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    for (uint32_t i = 0; i < len; i++) {
        JSValue child = JS_GetPropertyUint32(ctx, children, i);
        JSValue tag_val = JS_GetPropertyStr(ctx, child, "tagName");
        if (JS_IsString(tag_val)) {
            const char *tag_str = JS_ToCString(ctx, tag_val);
            if (tag_str && strcasecmp(tag_str, tag) == 0) {
                JS_FreeCString(ctx, tag_str);
                JS_FreeValue(ctx, tag_val);
                JS_FreeValue(ctx, children);
                return child;
            }
            if (tag_str) JS_FreeCString(ctx, tag_str);
        }
        JS_FreeValue(ctx, tag_val);
        JS_FreeValue(ctx, child);
    }
    JS_FreeValue(ctx, children);
    return JS_NULL;
}

static JSValue get_children_by_tags(JSContext *ctx, JSValue parent, const char **tags, int num_tags)
{
    JSValue arr = JS_NewArray(ctx);
    JSValue children = JS_GetPropertyStr(ctx, parent, "children");
    if (JS_IsException(children)) return arr;
    uint32_t len = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, children, "length");
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    uint32_t count = 0;
    for (uint32_t i = 0; i < len; i++) {
        JSValue child = JS_GetPropertyUint32(ctx, children, i);
        JSValue tag_val = JS_GetPropertyStr(ctx, child, "tagName");
        if (JS_IsString(tag_val)) {
            const char *tag_str = JS_ToCString(ctx, tag_val);
            if (tag_str) {
                bool match = false;
                for (int j = 0; j < num_tags; j++) {
                    if (strcasecmp(tag_str, tags[j]) == 0) {
                        match = true;
                        break;
                    }
                }
                if (match) {
                    JS_SetPropertyUint32(ctx, arr, count++, JS_DupValue(ctx, child));
                }
                JS_FreeCString(ctx, tag_str);
            }
        }
        JS_FreeValue(ctx, tag_val);
        JS_FreeValue(ctx, child);
    }
    JS_FreeValue(ctx, children);
    return arr;
}

static bool is_same_node_helper(JSContext *ctx, JSValue v1, JSValue v2)
{
    QJSNodePrivate *p1 = qjs_get_dom_priv(ctx, v1);
    QJSNodePrivate *p2 = qjs_get_dom_priv(ctx, v2);
    if (p1 && p2 && p1->node == p2->node) {
        return true;
    }
    return JS_VALUE_GET_PTR(v1) == JS_VALUE_GET_PTR(v2);
}

static JSValue get_table_rows(JSContext *ctx, JSValue table)
{
    JSValue arr = JS_NewArray(ctx);
    JSValue children = JS_GetPropertyStr(ctx, table, "children");
    if (JS_IsException(children)) return arr;
    uint32_t len = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, children, "length");
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    uint32_t count = 0;
    for (uint32_t i = 0; i < len; i++) {
        JSValue child = JS_GetPropertyUint32(ctx, children, i);
        JSValue tag_val = JS_GetPropertyStr(ctx, child, "tagName");
        if (JS_IsString(tag_val)) {
            const char *tag_str = JS_ToCString(ctx, tag_val);
            if (tag_str) {
                if (strcasecmp(tag_str, "TR") == 0) {
                    JS_SetPropertyUint32(ctx, arr, count++, JS_DupValue(ctx, child));
                } else if (strcasecmp(tag_str, "THEAD") == 0 || strcasecmp(tag_str, "TBODY") == 0 || strcasecmp(tag_str, "TFOOT") == 0) {
                    JSValue sec_children = JS_GetPropertyStr(ctx, child, "children");
                    if (!JS_IsException(sec_children)) {
                        uint32_t sec_len = 0;
                        JSValue sec_len_val = JS_GetPropertyStr(ctx, sec_children, "length");
                        JS_ToUint32(ctx, &sec_len, sec_len_val);
                        JS_FreeValue(ctx, sec_len_val);
                        for (uint32_t k = 0; k < sec_len; k++) {
                            JSValue sec_child = JS_GetPropertyUint32(ctx, sec_children, k);
                            JSValue sec_tag_val = JS_GetPropertyStr(ctx, sec_child, "tagName");
                            if (JS_IsString(sec_tag_val)) {
                                const char *sec_tag_str = JS_ToCString(ctx, sec_tag_val);
                                if (sec_tag_str && strcasecmp(sec_tag_str, "TR") == 0) {
                                    JS_SetPropertyUint32(ctx, arr, count++, JS_DupValue(ctx, sec_child));
                                }
                                if (sec_tag_str) JS_FreeCString(ctx, sec_tag_str);
                            }
                            JS_FreeValue(ctx, sec_tag_val);
                            JS_FreeValue(ctx, sec_child);
                        }
                        JS_FreeValue(ctx, sec_children);
                    }
                }
                JS_FreeCString(ctx, tag_str);
            }
        }
        JS_FreeValue(ctx, tag_val);
        JS_FreeValue(ctx, child);
    }
    JS_FreeValue(ctx, children);
    return arr;
}

static JSValue table_insert_row_helper(JSContext *ctx, JSValue parent, int32_t index)
{
    JSValue row = create_element_helper(ctx, "tr");
    if (JS_IsException(row)) return row;

    JSValue rows = get_table_rows(ctx, parent);
    uint32_t num_rows = 0;
    JSValue num_rows_val = JS_GetPropertyStr(ctx, rows, "length");
    JS_ToUint32(ctx, &num_rows, num_rows_val);
    JS_FreeValue(ctx, num_rows_val);

    if (index == -1 || index >= (int32_t)num_rows) {
        JSValue tbodies = get_children_by_tags(ctx, parent, (const char *[]){"TBODY"}, 1);
        uint32_t num_tbodies = 0;
        JSValue num_tbodies_val = JS_GetPropertyStr(ctx, tbodies, "length");
        JS_ToUint32(ctx, &num_tbodies, num_tbodies_val);
        JS_FreeValue(ctx, num_tbodies_val);

        JSValue target_parent;
        if (num_tbodies > 0) {
            target_parent = JS_GetPropertyUint32(ctx, tbodies, num_tbodies - 1);
        } else {
            target_parent = create_element_helper(ctx, "tbody");
            call_js_method_1(ctx, parent, "appendChild", JS_DupValue(ctx, target_parent));
        }
        JS_FreeValue(ctx, tbodies);

        call_js_method_1(ctx, target_parent, "appendChild", JS_DupValue(ctx, row));
        JS_FreeValue(ctx, target_parent);
    } else {
        JSValue target_row = JS_GetPropertyUint32(ctx, rows, index);
        JSValue target_parent = JS_GetPropertyStr(ctx, target_row, "parentNode");

        JSValue insert_before = JS_GetPropertyStr(ctx, target_parent, "insertBefore");
        JSValueConst args[2] = { row, target_row };
        JSValue res = JS_Call(ctx, insert_before, target_parent, 2, args);
        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, insert_before);

        JS_FreeValue(ctx, target_parent);
        JS_FreeValue(ctx, target_row);
    }

    JS_FreeValue(ctx, rows);
    return row;
}

static JSValue table_delete_row_helper(JSContext *ctx, JSValue parent, int32_t index)
{
    JSValue rows = get_table_rows(ctx, parent);
    uint32_t num_rows = 0;
    JSValue num_rows_val = JS_GetPropertyStr(ctx, rows, "length");
    JS_ToUint32(ctx, &num_rows, num_rows_val);
    JS_FreeValue(ctx, num_rows_val);

    int32_t target_idx = index;
    if (target_idx == -1) {
        target_idx = (int32_t)num_rows - 1;
    }

    if (target_idx >= 0 && target_idx < (int32_t)num_rows) {
        JSValue target_row = JS_GetPropertyUint32(ctx, rows, target_idx);
        JSValue target_parent = JS_GetPropertyStr(ctx, target_row, "parentNode");
        call_js_method_1(ctx, target_parent, "removeChild", target_row);
        JS_FreeValue(ctx, target_parent);
        JS_FreeValue(ctx, target_row);
    }
    JS_FreeValue(ctx, rows);
    return JS_UNDEFINED;
}

static JSValue section_insert_row_helper(JSContext *ctx, JSValue section, int32_t index)
{
    JSValue row = create_element_helper(ctx, "tr");
    if (JS_IsException(row)) return row;

    JSValue rows = get_children_by_tags(ctx, section, (const char *[]){"TR"}, 1);
    uint32_t num_rows = 0;
    JSValue num_rows_val = JS_GetPropertyStr(ctx, rows, "length");
    JS_ToUint32(ctx, &num_rows, num_rows_val);
    JS_FreeValue(ctx, num_rows_val);

    if (index == -1 || index >= (int32_t)num_rows) {
        call_js_method_1(ctx, section, "appendChild", JS_DupValue(ctx, row));
    } else {
        JSValue target_row = JS_GetPropertyUint32(ctx, rows, index);
        JSValue insert_before = JS_GetPropertyStr(ctx, section, "insertBefore");
        JSValueConst args[2] = { row, target_row };
        JSValue res = JS_Call(ctx, insert_before, section, 2, args);
        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, insert_before);
        JS_FreeValue(ctx, target_row);
    }
    JS_FreeValue(ctx, rows);
    return row;
}

static JSValue section_delete_row_helper(JSContext *ctx, JSValue section, int32_t index)
{
    JSValue rows = get_children_by_tags(ctx, section, (const char *[]){"TR"}, 1);
    uint32_t num_rows = 0;
    JSValue num_rows_val = JS_GetPropertyStr(ctx, rows, "length");
    JS_ToUint32(ctx, &num_rows, num_rows_val);
    JS_FreeValue(ctx, num_rows_val);

    int32_t target_idx = index;
    if (target_idx == -1) {
        target_idx = (int32_t)num_rows - 1;
    }

    if (target_idx >= 0 && target_idx < (int32_t)num_rows) {
        JSValue target_row = JS_GetPropertyUint32(ctx, rows, target_idx);
        call_js_method_1(ctx, section, "removeChild", target_row);
        JS_FreeValue(ctx, target_row);
    }
    JS_FreeValue(ctx, rows);
    return JS_UNDEFINED;
}

static JSValue row_insert_cell_helper(JSContext *ctx, JSValue row, int32_t index)
{
    JSValue cell = create_element_helper(ctx, "td");
    if (JS_IsException(cell)) return cell;

    JSValue cells = get_children_by_tags(ctx, row, (const char *[]){"TD", "TH"}, 2);
    uint32_t num_cells = 0;
    JSValue num_cells_val = JS_GetPropertyStr(ctx, cells, "length");
    JS_ToUint32(ctx, &num_cells, num_cells_val);
    JS_FreeValue(ctx, num_cells_val);

    if (index == -1 || index >= (int32_t)num_cells) {
        call_js_method_1(ctx, row, "appendChild", JS_DupValue(ctx, cell));
    } else {
        JSValue target_cell = JS_GetPropertyUint32(ctx, cells, index);
        JSValue insert_before = JS_GetPropertyStr(ctx, row, "insertBefore");
        JSValueConst args[2] = { cell, target_cell };
        JSValue res = JS_Call(ctx, insert_before, row, 2, args);
        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, insert_before);
        JS_FreeValue(ctx, target_cell);
    }
    JS_FreeValue(ctx, cells);
    return cell;
}

static JSValue row_delete_cell_helper(JSContext *ctx, JSValue row, int32_t index)
{
    JSValue cells = get_children_by_tags(ctx, row, (const char *[]){"TD", "TH"}, 2);
    uint32_t num_cells = 0;
    JSValue num_cells_val = JS_GetPropertyStr(ctx, cells, "length");
    JS_ToUint32(ctx, &num_cells, num_cells_val);
    JS_FreeValue(ctx, num_cells_val);

    int32_t target_idx = index;
    if (target_idx == -1) {
        target_idx = (int32_t)num_cells - 1;
    }

    if (target_idx >= 0 && target_idx < (int32_t)num_cells) {
        JSValue target_cell = JS_GetPropertyUint32(ctx, cells, target_idx);
        call_js_method_1(ctx, row, "removeChild", target_cell);
        JS_FreeValue(ctx, target_cell);
    }
    JS_FreeValue(ctx, cells);
    return JS_UNDEFINED;
}

static int32_t get_cell_index_helper(JSContext *ctx, JSValue cell)
{
    JSValue parent = JS_GetPropertyStr(ctx, cell, "parentNode");
    if (JS_IsException(parent) || JS_IsNull(parent) || JS_IsUndefined(parent)) {
        JS_FreeValue(ctx, parent);
        return -1;
    }
    JSValue cells = get_children_by_tags(ctx, parent, (const char *[]){"TD", "TH"}, 2);
    uint32_t num_cells = 0;
    JSValue num_cells_val = JS_GetPropertyStr(ctx, cells, "length");
    JS_ToUint32(ctx, &num_cells, num_cells_val);
    JS_FreeValue(ctx, num_cells_val);

    int32_t idx = -1;
    for (uint32_t i = 0; i < num_cells; i++) {
        JSValue c = JS_GetPropertyUint32(ctx, cells, i);
        if (is_same_node_helper(ctx, c, cell)) {
            idx = (int32_t)i;
            JS_FreeValue(ctx, c);
            break;
        }
        JS_FreeValue(ctx, c);
    }
    JS_FreeValue(ctx, cells);
    JS_FreeValue(ctx, parent);
    return idx;
}

static int32_t get_row_index_helper(JSContext *ctx, JSValue row)
{
    JSValue table = JS_GetPropertyStr(ctx, row, "parentNode");
    while (!JS_IsNull(table) && !JS_IsUndefined(table)) {
        JSValue tag_val = JS_GetPropertyStr(ctx, table, "tagName");
        if (JS_IsString(tag_val)) {
            const char *tag_str = JS_ToCString(ctx, tag_val);
            if (tag_str && strcasecmp(tag_str, "TABLE") == 0) {
                JS_FreeCString(ctx, tag_str);
                JS_FreeValue(ctx, tag_val);
                break;
            }
            if (tag_str) JS_FreeCString(ctx, tag_str);
        }
        JS_FreeValue(ctx, tag_val);
        JSValue next_parent = JS_GetPropertyStr(ctx, table, "parentNode");
        JS_FreeValue(ctx, table);
        table = next_parent;
    }

    if (JS_IsNull(table) || JS_IsUndefined(table)) {
        JS_FreeValue(ctx, table);
        return -1;
    }

    JSValue rows = get_table_rows(ctx, table);
    uint32_t num_rows = 0;
    JSValue num_rows_val = JS_GetPropertyStr(ctx, rows, "length");
    JS_ToUint32(ctx, &num_rows, num_rows_val);
    JS_FreeValue(ctx, num_rows_val);

    int32_t idx = -1;
    for (uint32_t i = 0; i < num_rows; i++) {
        JSValue r = JS_GetPropertyUint32(ctx, rows, i);
        if (is_same_node_helper(ctx, r, row)) {
            idx = (int32_t)i;
            JS_FreeValue(ctx, r);
            break;
        }
        JS_FreeValue(ctx, r);
    }
    JS_FreeValue(ctx, rows);
    JS_FreeValue(ctx, table);
    return idx;
}

static int32_t get_section_row_index_helper(JSContext *ctx, JSValue row)
{
    JSValue section = JS_GetPropertyStr(ctx, row, "parentNode");
    if (JS_IsNull(section) || JS_IsUndefined(section)) {
        JS_FreeValue(ctx, section);
        return -1;
    }

    JSValue rows = get_children_by_tags(ctx, section, (const char *[]){"TR"}, 1);
    uint32_t num_rows = 0;
    JSValue num_rows_val = JS_GetPropertyStr(ctx, rows, "length");
    JS_ToUint32(ctx, &num_rows, num_rows_val);
    JS_FreeValue(ctx, num_rows_val);

    int32_t idx = -1;
    for (uint32_t i = 0; i < num_rows; i++) {
        JSValue r = JS_GetPropertyUint32(ctx, rows, i);
        if (is_same_node_helper(ctx, r, row)) {
            idx = (int32_t)i;
            JS_FreeValue(ctx, r);
            break;
        }
        JS_FreeValue(ctx, r);
    }
    JS_FreeValue(ctx, rows);
    JS_FreeValue(ctx, section);
    return idx;
}

// -----------------------------------------------------------------------------
// HTMLTableElement Implementation (25 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltableelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmltableelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_bgColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "bgcolor", "");
}
JSValue wisp_htmltableelement_bgColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "bgcolor", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_border_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "border", "");
}
JSValue wisp_htmltableelement_border_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "border", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_cellPadding_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "cellpadding", "");
}
JSValue wisp_htmltableelement_cellPadding_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "cellpadding", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_cellSpacing_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "cellspacing", "");
}
JSValue wisp_htmltableelement_cellSpacing_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "cellspacing", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_frame_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "frame", "");
}
JSValue wisp_htmltableelement_frame_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "frame", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_rules_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "rules", "");
}
JSValue wisp_htmltableelement_rules_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "rules", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_summary_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "summary", "");
}
JSValue wisp_htmltableelement_summary_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "summary", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "width", "");
}
JSValue wisp_htmltableelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_sortable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "sortable");
}
JSValue wisp_htmltableelement_sortable_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "sortable", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltableelement_stopSorting_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmltableelement_caption_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue cap = get_first_child_by_tag(ctx, table, "CAPTION");
    JS_FreeValue(ctx, table);
    return cap;
}
JSValue wisp_htmltableelement_caption_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue old_cap = get_first_child_by_tag(ctx, table, "CAPTION");
    if (!JS_IsNull(old_cap)) {
        call_js_method_1(ctx, table, "removeChild", old_cap);
    } else {
        JS_FreeValue(ctx, old_cap);
    }
    if (value) {
        JSValue new_cap = qjs_wrap_node(ctx, (dom_node *)value);
        call_js_method_1(ctx, table, "appendChild", new_cap);
    }
    JS_FreeValue(ctx, table);
    return JS_UNDEFINED;
}

JSValue wisp_htmltableelement_tHead_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue thead = get_first_child_by_tag(ctx, table, "THEAD");
    JS_FreeValue(ctx, table);
    return thead;
}
JSValue wisp_htmltableelement_tHead_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue old_thead = get_first_child_by_tag(ctx, table, "THEAD");
    if (!JS_IsNull(old_thead)) {
        call_js_method_1(ctx, table, "removeChild", old_thead);
    } else {
        JS_FreeValue(ctx, old_thead);
    }
    if (value) {
        JSValue new_thead = qjs_wrap_node(ctx, (dom_node *)value);
        call_js_method_1(ctx, table, "appendChild", new_thead);
    }
    JS_FreeValue(ctx, table);
    return JS_UNDEFINED;
}

JSValue wisp_htmltableelement_tFoot_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue tfoot = get_first_child_by_tag(ctx, table, "TFOOT");
    JS_FreeValue(ctx, table);
    return tfoot;
}
JSValue wisp_htmltableelement_tFoot_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue old_tfoot = get_first_child_by_tag(ctx, table, "TFOOT");
    if (!JS_IsNull(old_tfoot)) {
        call_js_method_1(ctx, table, "removeChild", old_tfoot);
    } else {
        JS_FreeValue(ctx, old_tfoot);
    }
    if (value) {
        JSValue new_tfoot = qjs_wrap_node(ctx, (dom_node *)value);
        call_js_method_1(ctx, table, "appendChild", new_tfoot);
    }
    JS_FreeValue(ctx, table);
    return JS_UNDEFINED;
}

JSValue wisp_htmltableelement_createCaption_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue cap = get_first_child_by_tag(ctx, table, "CAPTION");
    if (!JS_IsNull(cap)) {
        JS_FreeValue(ctx, table);
        return cap;
    }
    JS_FreeValue(ctx, cap);

    JSValue new_cap = create_element_helper(ctx, "caption");
    if (JS_IsException(new_cap)) {
        JS_FreeValue(ctx, table);
        return new_cap;
    }

    JSValue first_child = JS_GetPropertyStr(ctx, table, "firstChild");
    if (JS_IsNull(first_child) || JS_IsUndefined(first_child)) {
        call_js_method_1(ctx, table, "appendChild", JS_DupValue(ctx, new_cap));
    } else {
        JSValue insert_before = JS_GetPropertyStr(ctx, table, "insertBefore");
        JSValueConst args[2] = { new_cap, first_child };
        JSValue res = JS_Call(ctx, insert_before, table, 2, args);
        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, insert_before);
    }
    JS_FreeValue(ctx, first_child);
    JS_FreeValue(ctx, table);
    return new_cap;
}

JSValue wisp_htmltableelement_deleteCaption_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue cap = get_first_child_by_tag(ctx, table, "CAPTION");
    if (!JS_IsNull(cap)) {
        call_js_method_1(ctx, table, "removeChild", cap);
    } else {
        JS_FreeValue(ctx, cap);
    }
    JS_FreeValue(ctx, table);
    return JS_UNDEFINED;
}

JSValue wisp_htmltableelement_createTHead_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue thead = get_first_child_by_tag(ctx, table, "THEAD");
    if (!JS_IsNull(thead)) {
        JS_FreeValue(ctx, table);
        return thead;
    }
    JS_FreeValue(ctx, thead);

    JSValue new_thead = create_element_helper(ctx, "thead");
    if (JS_IsException(new_thead)) {
        JS_FreeValue(ctx, table);
        return new_thead;
    }

    JSValue children = JS_GetPropertyStr(ctx, table, "children");
    uint32_t len = 0;
    if (!JS_IsException(children)) {
        JSValue len_val = JS_GetPropertyStr(ctx, children, "length");
        JS_ToUint32(ctx, &len, len_val);
        JS_FreeValue(ctx, len_val);
    }

    JSValue ref_child = JS_NULL;
    for (uint32_t i = 0; i < len; i++) {
        JSValue child = JS_GetPropertyUint32(ctx, children, i);
        JSValue tag_val = JS_GetPropertyStr(ctx, child, "tagName");
        if (JS_IsString(tag_val)) {
            const char *tag_str = JS_ToCString(ctx, tag_val);
            if (tag_str && strcasecmp(tag_str, "CAPTION") != 0 && strcasecmp(tag_str, "COLGROUP") != 0) {
                ref_child = child;
                JS_FreeCString(ctx, tag_str);
                JS_FreeValue(ctx, tag_val);
                break;
            }
            if (tag_str) JS_FreeCString(ctx, tag_str);
        }
        JS_FreeValue(ctx, tag_val);
        JS_FreeValue(ctx, child);
    }
    if (!JS_IsException(children)) JS_FreeValue(ctx, children);

    if (JS_IsNull(ref_child)) {
        call_js_method_1(ctx, table, "appendChild", JS_DupValue(ctx, new_thead));
    } else {
        JSValue insert_before = JS_GetPropertyStr(ctx, table, "insertBefore");
        JSValueConst args[2] = { new_thead, ref_child };
        JSValue res = JS_Call(ctx, insert_before, table, 2, args);
        JS_FreeValue(ctx, res);
        JS_FreeValue(ctx, insert_before);
        JS_FreeValue(ctx, ref_child);
    }

    JS_FreeValue(ctx, table);
    return new_thead;
}

JSValue wisp_htmltableelement_deleteTHead_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue thead = get_first_child_by_tag(ctx, table, "THEAD");
    if (!JS_IsNull(thead)) {
        call_js_method_1(ctx, table, "removeChild", thead);
    } else {
        JS_FreeValue(ctx, thead);
    }
    JS_FreeValue(ctx, table);
    return JS_UNDEFINED;
}

JSValue wisp_htmltableelement_createTFoot_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue tfoot = get_first_child_by_tag(ctx, table, "TFOOT");
    if (!JS_IsNull(tfoot)) {
        JS_FreeValue(ctx, table);
        return tfoot;
    }
    JS_FreeValue(ctx, tfoot);

    JSValue new_tfoot = create_element_helper(ctx, "tfoot");
    if (JS_IsException(new_tfoot)) {
        JS_FreeValue(ctx, table);
        return new_tfoot;
    }

    call_js_method_1(ctx, table, "appendChild", JS_DupValue(ctx, new_tfoot));
    JS_FreeValue(ctx, table);
    return new_tfoot;
}

JSValue wisp_htmltableelement_deleteTFoot_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue tfoot = get_first_child_by_tag(ctx, table, "TFOOT");
    if (!JS_IsNull(tfoot)) {
        call_js_method_1(ctx, table, "removeChild", tfoot);
    } else {
        JS_FreeValue(ctx, tfoot);
    }
    JS_FreeValue(ctx, table);
    return JS_UNDEFINED;
}

JSValue wisp_htmltableelement_createTBody_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue new_tbody = create_element_helper(ctx, "tbody");
    if (JS_IsException(new_tbody)) {
        JS_FreeValue(ctx, table);
        return new_tbody;
    }
    call_js_method_1(ctx, table, "appendChild", JS_DupValue(ctx, new_tbody));
    JS_FreeValue(ctx, table);
    return new_tbody;
}

JSValue wisp_htmltableelement_rows_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue rows = get_table_rows(ctx, table);
    JS_FreeValue(ctx, table);
    return rows;
}

JSValue wisp_htmltableelement_tBodies_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue tbodies = get_children_by_tags(ctx, table, (const char *[]){"TBODY"}, 1);
    JS_FreeValue(ctx, table);
    return tbodies;
}

JSValue wisp_htmltableelement_insertRow_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue row = table_insert_row_helper(ctx, table, index);
    JS_FreeValue(ctx, table);
    return row;
}

JSValue wisp_htmltableelement_deleteRow_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) {
    JSValue table = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue res = table_delete_row_helper(ctx, table, index);
    JS_FreeValue(ctx, table);
    return res;
}

// -----------------------------------------------------------------------------
// HTMLTableRowElement Implementation (14 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltablerowelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmltablerowelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablerowelement_bgColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "bgcolor", "");
}
JSValue wisp_htmltablerowelement_bgColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "bgcolor", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablerowelement_ch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "char", "");
}
JSValue wisp_htmltablerowelement_ch_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "char", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablerowelement_chOff_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "charoff", "");
}
JSValue wisp_htmltablerowelement_chOff_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "charoff", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablerowelement_vAlign_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "valign", "");
}
JSValue wisp_htmltablerowelement_vAlign_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "valign", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltablerowelement_cells_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue row = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue cells = get_children_by_tags(ctx, row, (const char *[]){"TD", "TH"}, 2);
    JS_FreeValue(ctx, row);
    return cells;
}

JSValue wisp_htmltablerowelement_rowIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue row = qjs_wrap_node(ctx, (dom_node *)priv->node);
    int32_t idx = get_row_index_helper(ctx, row);
    JS_FreeValue(ctx, row);
    return JS_NewInt32(ctx, idx);
}

JSValue wisp_htmltablerowelement_sectionRowIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue row = qjs_wrap_node(ctx, (dom_node *)priv->node);
    int32_t idx = get_section_row_index_helper(ctx, row);
    JS_FreeValue(ctx, row);
    return JS_NewInt32(ctx, idx);
}

JSValue wisp_htmltablerowelement_insertCell_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) {
    JSValue row = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue cell = row_insert_cell_helper(ctx, row, index);
    JS_FreeValue(ctx, row);
    return cell;
}

JSValue wisp_htmltablerowelement_deleteCell_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) {
    JSValue row = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue res = row_delete_cell_helper(ctx, row, index);
    JS_FreeValue(ctx, row);
    return res;
}

// -----------------------------------------------------------------------------
// HTMLTableCellElement Implementation (21 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltablecellelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmltablecellelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_axis_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "axis", "");
}
JSValue wisp_htmltablecellelement_axis_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "axis", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_bgColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "bgcolor", "");
}
JSValue wisp_htmltablecellelement_bgColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "bgcolor", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_ch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "char", "");
}
JSValue wisp_htmltablecellelement_ch_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "char", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_chOff_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "charoff", "");
}
JSValue wisp_htmltablecellelement_chOff_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "charoff", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "height", "");
}
JSValue wisp_htmltablecellelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "height", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "width", "");
}
JSValue wisp_htmltablecellelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_vAlign_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "valign", "");
}
JSValue wisp_htmltablecellelement_vAlign_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "valign", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_noWrap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "nowrap");
}
JSValue wisp_htmltablecellelement_noWrap_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "nowrap", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_colSpan_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "colspan", 1));
}
JSValue wisp_htmltablecellelement_colSpan_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "colspan", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_rowSpan_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "rowspan", 1));
}
JSValue wisp_htmltablecellelement_rowSpan_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "rowspan", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecellelement_headers_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "headers", "");
}
JSValue wisp_htmltablecellelement_cellIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue cell = qjs_wrap_node(ctx, (dom_node *)priv->node);
    int32_t idx = get_cell_index_helper(ctx, cell);
    JS_FreeValue(ctx, cell);
    return JS_NewInt32(ctx, idx);
}

// -----------------------------------------------------------------------------
// HTMLTableSectionElement Implementation (12 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltablesectionelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmltablesectionelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablesectionelement_ch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "char", "");
}
JSValue wisp_htmltablesectionelement_ch_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "char", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablesectionelement_chOff_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "charoff", "");
}
JSValue wisp_htmltablesectionelement_chOff_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "charoff", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablesectionelement_vAlign_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "valign", "");
}
JSValue wisp_htmltablesectionelement_vAlign_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "valign", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltablesectionelement_rows_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue section = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue rows = get_children_by_tags(ctx, section, (const char *[]){"TR"}, 1);
    JS_FreeValue(ctx, section);
    return rows;
}

JSValue wisp_htmltablesectionelement_insertRow_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) {
    JSValue section = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue row = section_insert_row_helper(ctx, section, index);
    JS_FreeValue(ctx, section);
    return row;
}

JSValue wisp_htmltablesectionelement_deleteRow_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) {
    JSValue section = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue res = section_delete_row_helper(ctx, section, index);
    JS_FreeValue(ctx, section);
    return res;
}

// -----------------------------------------------------------------------------
// HTMLTableColElement Implementation (12 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltablecolelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmltablecolelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecolelement_ch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "char", "");
}
JSValue wisp_htmltablecolelement_ch_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "char", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecolelement_chOff_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "charoff", "");
}
JSValue wisp_htmltablecolelement_chOff_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "charoff", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecolelement_vAlign_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "valign", "");
}
JSValue wisp_htmltablecolelement_vAlign_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "valign", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecolelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "width", "");
}
JSValue wisp_htmltablecolelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmltablecolelement_span_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "span", 1));
}
JSValue wisp_htmltablecolelement_span_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "span", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLTableCaptionElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltablecaptionelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmltablecaptionelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlolistelement_reversed_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "reversed");
}

JSValue wisp_htmlolistelement_reversed_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "reversed", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlolistelement_start_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "start", 1));
}

JSValue wisp_htmlolistelement_start_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "start", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlolistelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "type", "");
}

JSValue wisp_htmlolistelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLUListElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlulistelement_compact_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "compact");
}

JSValue wisp_htmlulistelement_compact_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "compact", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLHtmlElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlhtmlelement_version_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "version", "");
}

JSValue wisp_htmlhtmlelement_version_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "version", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLModElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlmodelement_cite_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "cite", "");
}

JSValue wisp_htmlmodelement_cite_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "cite", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmodelement_dateTime_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "datetime", "");
}

JSValue wisp_htmlmodelement_dateTime_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "datetime", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLBaseElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlbaseelement_href_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "href", "");
}

JSValue wisp_htmlbaseelement_href_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "href", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbaseelement_target_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "target", "");
}

JSValue wisp_htmlbaseelement_target_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "target", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLTitleElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltitleelement_text_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_node_textContent_get_impl(ctx, priv);
}

JSValue wisp_htmltitleelement_text_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_node_textContent_set_impl(ctx, priv, value);
}

// -----------------------------------------------------------------------------
// HTMLDataElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmldataelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "value", "");
}

JSValue wisp_htmldataelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "value", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLTimeElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltimeelement_dateTime_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "datetime", "");
}

JSValue wisp_htmltimeelement_dateTime_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "datetime", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLLabelElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

static JSValue get_element_labels_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewArray(ctx);
    JSValue labels_arr = JS_NewArray(ctx);
    uint32_t idx = 0;
    JSValue id_val = get_element_str_attr(ctx, priv, "id", NULL);
    if (JS_IsString(id_val)) {
        const char *id_str = JS_ToCString(ctx, id_val);
        if (id_str && id_str[0] != '\0') {
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue doc_val = JS_GetPropertyStr(ctx, global, "document");
            JSValue qsa = JS_GetPropertyStr(ctx, doc_val, "querySelectorAll");
            if (JS_IsFunction(ctx, qsa)) {
                size_t sel_len = strlen(id_str) + 32;
                char *sel = malloc(sel_len);
                if (sel) {
                    snprintf(sel, sel_len, "label[for=\"%s\"]", id_str);
                    JSValue sel_val = JS_NewString(ctx, sel);
                    JSValue matched = JS_Call(ctx, qsa, doc_val, 1, &sel_val);
                    JS_FreeValue(ctx, sel_val);
                    free(sel);
                    if (!JS_IsException(matched) && JS_IsObject(matched)) {
                        JSValue len_val = JS_GetPropertyStr(ctx, matched, "length");
                        int32_t len = 0;
                        if (JS_IsNumber(len_val)) JS_ToInt32(ctx, &len, len_val);
                        JS_FreeValue(ctx, len_val);
                        for (int i = 0; i < len; i++) {
                            JSValue item = JS_GetPropertyUint32(ctx, matched, i);
                            JS_SetPropertyUint32(ctx, labels_arr, idx++, item);
                        }
                    }
                    JS_FreeValue(ctx, matched);
                }
            }
            JS_FreeValue(ctx, qsa);
            JS_FreeValue(ctx, doc_val);
            JS_FreeValue(ctx, global);
            JS_FreeCString(ctx, id_str);
        } else if (id_str) {
            JS_FreeCString(ctx, id_str);
        }
    }
    JS_FreeValue(ctx, id_val);
    return labels_arr;
}

JSValue wisp_htmllabelelement_control_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NULL;
    JSValue for_val = get_element_str_attr(ctx, priv, "for", NULL);
    if (JS_IsString(for_val)) {
        const char *for_str = JS_ToCString(ctx, for_val);
        if (for_str && for_str[0] != '\0') {
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue doc_val = JS_GetPropertyStr(ctx, global, "document");
            JSValue get_el = JS_GetPropertyStr(ctx, doc_val, "getElementById");
            JSValue id_str = JS_NewString(ctx, for_str);
            JSValue target = JS_Call(ctx, get_el, doc_val, 1, &id_str);
            JS_FreeValue(ctx, id_str);
            JS_FreeValue(ctx, get_el);
            JS_FreeValue(ctx, doc_val);
            JS_FreeValue(ctx, global);
            JS_FreeCString(ctx, for_str);
            JS_FreeValue(ctx, for_val);
            if (!JS_IsException(target) && !JS_IsNull(target) && !JS_IsUndefined(target)) {
                return target;
            }
            JS_FreeValue(ctx, target);
        } else {
            if (for_str) JS_FreeCString(ctx, for_str);
            JS_FreeValue(ctx, for_val);
        }
    } else {
        JS_FreeValue(ctx, for_val);
    }
    JSValue label_obj = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue qs = JS_GetPropertyStr(ctx, label_obj, "querySelector");
    if (JS_IsFunction(ctx, qs)) {
        JSValue sel = JS_NewString(ctx, "input, select, textarea, button");
        JSValue target = JS_Call(ctx, qs, label_obj, 1, &sel);
        JS_FreeValue(ctx, sel);
        JS_FreeValue(ctx, qs);
        JS_FreeValue(ctx, label_obj);
        if (!JS_IsException(target) && !JS_IsNull(target) && !JS_IsUndefined(target)) {
            return target;
        }
        JS_FreeValue(ctx, target);
    } else {
        JS_FreeValue(ctx, qs);
        JS_FreeValue(ctx, label_obj);
    }
    return JS_NULL;
}

JSValue wisp_htmllabelelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_form_impl(ctx, priv);
}

JSValue wisp_htmllabelelement_htmlFor_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "for", "");
}

JSValue wisp_htmllabelelement_htmlFor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "for", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLOptGroupElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmloptgroupelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "disabled");
}

JSValue wisp_htmloptgroupelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "disabled", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmloptgroupelement_label_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "label", "");
}

JSValue wisp_htmloptgroupelement_label_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "label", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLMenuElement Implementation (6 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlmenuelement_compact_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "compact");
}

JSValue wisp_htmlmenuelement_compact_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "compact", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuelement_label_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "label", "");
}

JSValue wisp_htmlmenuelement_label_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "label", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "type", "");
}

JSValue wisp_htmlmenuelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLDetailsElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmldetailselement_open_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "open");
}

JSValue wisp_htmldetailselement_open_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "open", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLMenuItemElement Implementation (15 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlmenuitemelement_checked_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "checked");
}

JSValue wisp_htmlmenuitemelement_checked_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "checked", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuitemelement_command_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NULL;
}

JSValue wisp_htmlmenuitemelement_default_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "default");
}

JSValue wisp_htmlmenuitemelement_default_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "default", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuitemelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "disabled");
}

JSValue wisp_htmlmenuitemelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "disabled", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuitemelement_icon_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "icon", "");
}

JSValue wisp_htmlmenuitemelement_icon_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "icon", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuitemelement_label_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "label", "");
}

JSValue wisp_htmlmenuitemelement_label_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "label", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuitemelement_radiogroup_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "radiogroup", "");
}

JSValue wisp_htmlmenuitemelement_radiogroup_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "radiogroup", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmenuitemelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "type", "");
}

JSValue wisp_htmlmenuitemelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLUListElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlulistelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "type", "");
}

JSValue wisp_htmlulistelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLLIElement Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmllielement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_str_attr(ctx, priv, "type", "");
}

JSValue wisp_htmllielement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmllielement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "value", 0));
}

JSValue wisp_htmllielement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    set_element_int_attr(ctx, priv, "value", value);
    return JS_UNDEFINED;
}


// -----------------------------------------------------------------------------
// HTMLDListElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmldlistelement_compact_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return get_element_bool_attr(ctx, priv, "compact");
}

JSValue wisp_htmldlistelement_compact_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    set_element_bool_attr(ctx, priv, "compact", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLImageElement Additional Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlimageelement_srcset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "srcset", "");
}

JSValue wisp_htmlimageelement_srcset_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "srcset", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_sizes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "sizes", "");
}

JSValue wisp_htmlimageelement_sizes_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "sizes", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_crossOrigin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "crossorigin", "");
}

JSValue wisp_htmlimageelement_crossOrigin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "crossorigin", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_lowsrc_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "lowsrc", "");
}

JSValue wisp_htmlimageelement_lowsrc_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "lowsrc", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlimageelement_currentSrc_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "src", "");
}

// -----------------------------------------------------------------------------
// ValidityState Implementation
// -----------------------------------------------------------------------------

JSValue wisp_validitystate_badInput_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}


static bool check_custom_error(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return false;
    JSValue msg = get_element_str_attr(ctx, priv, "__customValidity", "");
    bool has_error = false;
    if (JS_IsString(msg)) {
        const char *str = JS_ToCString(ctx, msg);
        if (str && strlen(str) > 0) has_error = true;
        if (str) JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, msg);
    return has_error;
}

JSValue wisp_validitystate_customError_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewBool(ctx, check_custom_error(ctx, priv));
}


JSValue wisp_validitystate_patternMismatch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_validitystate_rangeOverflow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_validitystate_rangeUnderflow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_validitystate_stepMismatch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_validitystate_tooLong_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_validitystate_tooShort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_validitystate_typeMismatch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}


static bool check_value_missing(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return false;
    JSValue required_val = get_element_bool_attr(ctx, priv, "required");
    bool required = JS_ToBool(ctx, required_val);
    JS_FreeValue(ctx, required_val);
    if (!required) return false;

    // get value
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "value");
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        bool missing = (str == NULL || strlen(str) == 0);
        if (str) JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, val);
        return missing;
    }
    JS_FreeValue(ctx, val);
    return true; // if no value attribute, value is missing for required field
}

JSValue wisp_validitystate_valueMissing_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewBool(ctx, check_value_missing(ctx, priv));
}

JSValue wisp_validitystate_valid_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (check_value_missing(ctx, priv)) return JS_FALSE;
    if (check_custom_error(ctx, priv)) return JS_FALSE;
    return JS_TRUE;
}




// -----------------------------------------------------------------------------
// HTMLFieldSetElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmlfieldsetelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "disabled");
}

JSValue wisp_htmlfieldsetelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "disabled", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlfieldsetelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_form_impl(ctx, priv);
}

JSValue wisp_htmlfieldsetelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "name", "");
}

JSValue wisp_htmlfieldsetelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlfieldsetelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "fieldset");
}

JSValue wisp_htmlfieldsetelement_elements_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_htmlfieldsetelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_htmlfieldsetelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmlfieldsetelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__customValidity", "");
}

JSValue wisp_htmlfieldsetelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlfieldsetelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlfieldsetelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlfieldsetelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlfieldsetelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLOutputElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmloutputelement_htmlFor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_domtokenlist(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_domtokenlist(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmloutputelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_form_impl(ctx, priv);
}

JSValue wisp_htmloutputelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "name", "");
}

JSValue wisp_htmloutputelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmloutputelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "output");
}

JSValue wisp_htmloutputelement_defaultValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "defaultValue", "");
}

JSValue wisp_htmloutputelement_defaultValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "defaultValue", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmloutputelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "value", "");
}

JSValue wisp_htmloutputelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "value", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmloutputelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_htmloutputelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmloutputelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__customValidity", "");
}

JSValue wisp_htmloutputelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_htmloutputelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmloutputelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmloutputelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmloutputelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmloutputelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLInputElement Additional WebIDL Implementations
// -----------------------------------------------------------------------------

JSValue wisp_htmlinputelement_stepUp_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t n) {
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLInputElement target");
    double val = 0.0;
    JSValue num_val = wisp_htmlinputelement_valueAsNumber_get_impl(ctx, priv);
    JS_ToFloat64(ctx, &val, num_val);
    JS_FreeValue(ctx, num_val);
    val += n;

    char buf[64];
    snprintf(buf, sizeof(buf), "%g", val);
    return wisp_htmlinputelement_value_set_impl(ctx, priv, buf);
}

JSValue wisp_htmlinputelement_stepDown_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t n) {
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLInputElement target");
    double val = 0.0;
    JSValue num_val = wisp_htmlinputelement_valueAsNumber_get_impl(ctx, priv);
    JS_ToFloat64(ctx, &val, num_val);
    JS_FreeValue(ctx, num_val);
    val -= n;

    char buf[64];
    snprintf(buf, sizeof(buf), "%g", val);
    return wisp_htmlinputelement_value_set_impl(ctx, priv, buf);
}

JSValue wisp_htmlinputelement_select_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLInputElement target");
    set_element_int_attr(ctx, priv, "selectionstart", 0);
    set_element_int_attr(ctx, priv, "selectionend", 999999);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_setRangeText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * replacement) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_setSelectionRange_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t start, uint32_t end, const char * direction) {
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLInputElement target");
    set_element_int_attr(ctx, priv, "selectionstart", start);
    set_element_int_attr(ctx, priv, "selectionend", end);
    if (direction) {
        set_element_str_attr(ctx, priv, "selectiondirection", direction);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_dirName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "dirname", "");
}

JSValue wisp_htmlinputelement_dirName_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "dirname", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_formAction_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formaction", "");
}

JSValue wisp_htmlinputelement_formAction_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formaction", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_formEnctype_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formenctype", "");
}

JSValue wisp_htmlinputelement_formEnctype_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formenctype", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_formMethod_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formmethod", "");
}

JSValue wisp_htmlinputelement_formMethod_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formmethod", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_formNoValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "formnovalidate");
}

JSValue wisp_htmlinputelement_formNoValidate_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "formnovalidate", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_formTarget_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formtarget", "");
}

JSValue wisp_htmlinputelement_formTarget_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formtarget", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "height", 0));
}

JSValue wisp_htmlinputelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "height", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "width", 0));
}

JSValue wisp_htmlinputelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_indeterminate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "indeterminate");
}

JSValue wisp_htmlinputelement_indeterminate_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "indeterminate", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_list_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlinputelement_max_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "max", "");
}

JSValue wisp_htmlinputelement_max_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "max", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_min_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "min", "");
}

JSValue wisp_htmlinputelement_min_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "min", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_step_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "step", "");
}

JSValue wisp_htmlinputelement_step_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "step", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_valueAsDate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    JSValue val = wisp_htmlinputelement_value_get_impl(ctx, priv);
    if (!JS_IsString(val)) {
        JS_FreeValue(ctx, val);
        return JS_NULL;
    }
    const char *str = JS_ToCString(ctx, val);
    if (!str || str[0] == '\0') {
        if (str) JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, val);
        return JS_NULL;
    }
    int year = 0, month = 0, day = 0;
    if (sscanf(str, "%d-%d-%d", &year, &month, &day) == 3) {
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, val);
        struct tm tm = {0};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        time_t t = timegm(&tm);
        double epoch_ms = (double)t * 1000.0;
        return JS_NewDate(ctx, epoch_ms);
    }
    JS_FreeCString(ctx, str);
    JS_FreeValue(ctx, val);
    return JS_NULL;
}

JSValue wisp_htmlinputelement_valueAsDate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (JS_IsNull(value) || JS_IsUndefined(value)) {
        return wisp_htmlinputelement_value_set_impl(ctx, priv, "");
    }
    double epoch_ms = 0.0;
    if (JS_ToFloat64(ctx, &epoch_ms, value) < 0 || isnan(epoch_ms)) {
        return wisp_htmlinputelement_value_set_impl(ctx, priv, "");
    }
    time_t sec = (time_t)(epoch_ms / 1000.0);
    struct tm tm;
    if (gmtime_r(&sec, &tm) == NULL) {
        return wisp_htmlinputelement_value_set_impl(ctx, priv, "");
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return wisp_htmlinputelement_value_set_impl(ctx, priv, buf);
}

JSValue wisp_htmlinputelement_valueAsNumber_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewFloat64(ctx, NAN);
    JSValue val = wisp_htmlinputelement_value_get_impl(ctx, priv);
    double d = NAN;
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        if (str && str[0] != '\0') {
            int year = 0, month = 0, day = 0;
            if (sscanf(str, "%d-%d-%d", &year, &month, &day) == 3) {
                struct tm tm = {0};
                tm.tm_year = year - 1900;
                tm.tm_mon = month - 1;
                tm.tm_mday = day;
                time_t t = timegm(&tm);
                d = (double)t * 1000.0;
            } else {
                char *endptr = NULL;
                double parsed_d = strtod(str, &endptr);
                if (endptr && *endptr == '\0') {
                    d = parsed_d;
                } else {
                    d = NAN;
                }
            }
        }
        if (str) JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, val);
    return JS_NewFloat64(ctx, d);
}

JSValue wisp_htmlinputelement_valueAsNumber_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    if (isnan(value)) {
        return wisp_htmlinputelement_value_set_impl(ctx, priv, "");
    }
    char buf[64];
    snprintf(buf, sizeof(buf), "%g", value);
    return wisp_htmlinputelement_value_set_impl(ctx, priv, buf);
}

JSValue wisp_htmlinputelement_selectionStart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "selectionstart", 0));
}

JSValue wisp_htmlinputelement_selectionStart_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "selectionstart", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_selectionEnd_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "selectionend", 0));
}

JSValue wisp_htmlinputelement_selectionEnd_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "selectionend", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_selectionDirection_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "selectiondirection", "none");
}

JSValue wisp_htmlinputelement_selectionDirection_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "selectiondirection", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmlinputelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__customValidity", "");
}

JSValue wisp_htmlinputelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlinputelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlinputelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlinputelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlinputelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlinputelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLTextAreaElement Additional WebIDL Implementations
// -----------------------------------------------------------------------------

JSValue wisp_htmltextareaelement_select_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_setRangeText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * replacement) {
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_setSelectionRange_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t start, uint32_t end, const char * direction) {
    set_element_int_attr(ctx, priv, "selectionstart", start);
    set_element_int_attr(ctx, priv, "selectionend", end);
    if (direction) {
        set_element_str_attr(ctx, priv, "selectiondirection", direction);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_selectionStart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "selectionstart", 0));
}

JSValue wisp_htmltextareaelement_selectionStart_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "selectionstart", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_selectionEnd_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "selectionend", 0));
}

JSValue wisp_htmltextareaelement_selectionEnd_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "selectionend", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_selectionDirection_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "selectiondirection", "none");
}

JSValue wisp_htmltextareaelement_selectionDirection_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "selectiondirection", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmltextareaelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__customValidity", "");
}

JSValue wisp_htmltextareaelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmltextareaelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmltextareaelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmltextareaelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmltextareaelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmltextareaelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLButtonElement Additional WebIDL Implementations
// -----------------------------------------------------------------------------

JSValue wisp_htmlbuttonelement_formAction_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formaction", "");
}

JSValue wisp_htmlbuttonelement_formAction_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formaction", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbuttonelement_formEnctype_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formenctype", "");
}

JSValue wisp_htmlbuttonelement_formEnctype_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formenctype", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbuttonelement_formMethod_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formmethod", "");
}

JSValue wisp_htmlbuttonelement_formMethod_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formmethod", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbuttonelement_formNoValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "formnovalidate");
}

JSValue wisp_htmlbuttonelement_formNoValidate_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "formnovalidate", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbuttonelement_formTarget_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "formtarget", "");
}

JSValue wisp_htmlbuttonelement_formTarget_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "formtarget", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlbuttonelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmlbuttonelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__customValidity", "");
}

JSValue wisp_htmlbuttonelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlbuttonelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlbuttonelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlbuttonelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlbuttonelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlbuttonelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// Document Additional Implementations
// -----------------------------------------------------------------------------

JSValue wisp_document_documentURI_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue wisp_document_URL_get_impl(JSContext *ctx, QJSNodePrivate *priv);
    return wisp_document_URL_get_impl(ctx, priv);
}

JSValue wisp_document_lastModified_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "01/01/2027 00:00:00");
}

JSValue wisp_document_designMode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "off");
    JSValue doc_elem = wisp_document_documentElement_get_impl(ctx, priv);
    if (JS_IsObject(doc_elem)) {
        QJSNodePrivate *elem_priv = qjs_get_dom_priv(ctx, doc_elem);
        if (elem_priv) {
            JSValue val = wisp_element_getAttribute_impl(ctx, elem_priv, "designMode");
            if (JS_IsString(val)) {
                const char *str = JS_ToCString(ctx, val);
                JS_FreeValue(ctx, val);
                if (str) {
                    bool is_on = (strcasecmp(str, "on") == 0);
                    JS_FreeCString(ctx, str);
                    JS_FreeValue(ctx, doc_elem);
                    return JS_NewString(ctx, is_on ? "on" : "off");
                }
            } else {
                JS_FreeValue(ctx, val);
            }
        }
        JS_FreeValue(ctx, doc_elem);
    } else {
        JS_FreeValue(ctx, doc_elem);
    }
    return JS_NewString(ctx, "off");
}

JSValue wisp_document_designMode_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    JSValue doc_elem = wisp_document_documentElement_get_impl(ctx, priv);
    if (JS_IsObject(doc_elem)) {
        QJSNodePrivate *elem_priv = qjs_get_dom_priv(ctx, doc_elem);
        if (elem_priv) {
            if (strcasecmp(value, "on") == 0) {
                wisp_element_setAttribute_impl(ctx, elem_priv, "designMode", "on");
            } else if (strcasecmp(value, "off") == 0) {
                wisp_element_setAttribute_impl(ctx, elem_priv, "designMode", "off");
            }
        }
        JS_FreeValue(ctx, doc_elem);
    } else {
        JS_FreeValue(ctx, doc_elem);
    }
    return JS_UNDEFINED;
}

JSValue wisp_document_hasFocus_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_document_hidden_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_document_visibilityState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "visible");
}

// -----------------------------------------------------------------------------
// HTMLDataListElement Implementation
// -----------------------------------------------------------------------------

JSValue wisp_htmldatalistelement_options_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    extern JSValue qjs_new_htmlcollection_with_type(JSContext *ctx, void *node, bool is_dom_node, const char *type);
    return qjs_new_htmlcollection_with_type(ctx, priv->node, priv->is_dom_node, "children");
}

// -----------------------------------------------------------------------------
// MediaError Implementation
// -----------------------------------------------------------------------------

JSValue wisp_mediaerror_code_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 4);
}
typedef struct WispVTTCue {
    double startTime;
    double endTime;
    char *text;
    char *id;
    bool pauseOnExit;
    struct WispTextTrack *track;
} WispVTTCue;

typedef struct WispTextTrackCueList {
    WispVTTCue **cues;
    uint32_t count;
    uint32_t capacity;
} WispTextTrackCueList;

typedef struct WispTextTrack {
    char *kind;
    char *label;
    char *language;
    char *id;
    char *mode;
    WispTextTrackCueList *cues;
} WispTextTrack;

typedef struct WispTextTrackList {
    WispTextTrack **tracks;
    uint32_t count;
    uint32_t capacity;
} WispTextTrackList;

typedef struct WispMediaTracksEntry {
    void *node;
    WispTextTrackList *track_list;
    struct WispMediaTracksEntry *next;
} WispMediaTracksEntry;

static WispMediaTracksEntry *g_media_tracks_head = NULL;

static WispTextTrackList *get_or_create_media_tracks(void *node) {
    if (!node) return NULL;
    WispMediaTracksEntry *curr = g_media_tracks_head;
    while (curr) {
        if (curr->node == node) return curr->track_list;
        curr = curr->next;
    }
    WispMediaTracksEntry *entry = calloc(1, sizeof(WispMediaTracksEntry));
    if (!entry) return NULL;
    entry->node = node;
    entry->track_list = calloc(1, sizeof(WispTextTrackList));
    entry->next = g_media_tracks_head;
    g_media_tracks_head = entry;
    return entry->track_list;
}

JSValue wisp_htmlmediaelement_textTracks_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    WispTextTrackList *tl = get_or_create_media_tracks(priv->node);
    if (!tl) return JS_NULL;
    extern JSValue qjs_new_texttracklist(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_texttracklist(ctx, tl, false);
}

JSValue wisp_htmlmediaelement_addTextTrack_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue kind, const char * label, const char * language) {
    if (!priv || !priv->node) return JS_NULL;
    WispTextTrackList *tl = get_or_create_media_tracks(priv->node);
    if (!tl) return JS_NULL;

    WispTextTrack *track = calloc(1, sizeof(WispTextTrack));
    if (!track) return JS_NULL;

    const char *kind_str = "subtitles";
    const char *allocated_kind = NULL;
    if (JS_IsString(kind)) {
        allocated_kind = JS_ToCString(ctx, kind);
        if (allocated_kind) kind_str = allocated_kind;
    }

    track->kind = strdup(kind_str);
    if (allocated_kind) JS_FreeCString(ctx, allocated_kind);

    track->label = strdup(label ? label : "");
    track->language = strdup(language ? language : "");
    track->id = strdup("");
    track->mode = strdup("showing");
    track->cues = calloc(1, sizeof(WispTextTrackCueList));

    if (tl->count >= tl->capacity) {
        uint32_t new_cap = tl->capacity ? tl->capacity * 2 : 4;
        WispTextTrack **new_tracks = realloc(tl->tracks, new_cap * sizeof(WispTextTrack *));
        if (new_tracks) {
            tl->tracks = new_tracks;
            tl->capacity = new_cap;
        }
    }
    if (tl->count < tl->capacity) {
        tl->tracks[tl->count++] = track;
    }

    extern JSValue qjs_new_texttrack(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_texttrack(ctx, track, false);
}

JSValue wisp_htmlmediaelement_canPlayType_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type) {
    if (!type || !*type) {
        return JS_NewString(ctx, "");
    }

    char mime[128] = {0};
    const char *semicolon = strchr(type, ';');
    size_t mime_len = semicolon ? (size_t)(semicolon - type) : strlen(type);
    if (mime_len >= sizeof(mime)) mime_len = sizeof(mime) - 1;
    strncpy(mime, type, mime_len);
    mime[mime_len] = '\0';

    // Trim trailing whitespace from mime
    while (mime_len > 0 && (mime[mime_len - 1] == ' ' || mime[mime_len - 1] == '\t')) {
        mime[--mime_len] = '\0';
    }

    // Convert mime to lowercase
    for (size_t i = 0; mime[i]; i++) {
        if (mime[i] >= 'A' && mime[i] <= 'Z') mime[i] += 32;
    }

    bool is_mp4 = (strcmp(mime, "video/mp4") == 0 || strcmp(mime, "audio/mp4") == 0 || strcmp(mime, "audio/x-m4a") == 0 || strcmp(mime, "audio/m4a") == 0);
    bool is_webm = (strcmp(mime, "video/webm") == 0 || strcmp(mime, "audio/webm") == 0);
    bool is_ogg = (strcmp(mime, "video/ogg") == 0 || strcmp(mime, "audio/ogg") == 0 || strcmp(mime, "application/ogg") == 0);
    bool is_mp3 = (strcmp(mime, "audio/mpeg") == 0 || strcmp(mime, "audio/mp3") == 0);
    bool is_aac = (strcmp(mime, "audio/aac") == 0);
    bool is_wav = (strcmp(mime, "audio/wav") == 0 || strcmp(mime, "audio/x-wav") == 0);
    bool is_opus = (strcmp(mime, "audio/opus") == 0);
    bool is_flac = (strcmp(mime, "audio/flac") == 0);
    bool is_vp8 = (strcmp(mime, "video/vp8") == 0 || strcmp(mime, "video/x-vp8") == 0 || is_webm || is_ogg);
    bool is_vp9 = (strcmp(mime, "video/vp9") == 0 || strcmp(mime, "video/x-vp9") == 0 || is_webm || is_mp4);
    bool is_av1 = (strcmp(mime, "video/av1") == 0 || strcmp(mime, "video/x-av1") == 0 || is_mp4 || is_webm);
    bool is_av2 = (strcmp(mime, "video/av2") == 0 || strcmp(mime, "video/x-av2") == 0 || is_mp4 || is_webm);

    if (!is_mp4 && !is_webm && !is_ogg && !is_mp3 && !is_aac && !is_wav && !is_opus && !is_flac && !is_vp8 && !is_vp9 && !is_av1 && !is_av2) {
        return JS_NewString(ctx, "");
    }

    // Check for codecs parameter
    const char *codecs_ptr = strstr(type, "codecs=");
    if (!codecs_ptr) {
        return JS_NewString(ctx, "maybe");
    }

    codecs_ptr += 7; // skip "codecs="
    if (*codecs_ptr == '"' || *codecs_ptr == '\'') codecs_ptr++;

    char codecs_buf[256] = {0};
    size_t c_idx = 0;
    while (*codecs_ptr && *codecs_ptr != '"' && *codecs_ptr != '\'' && *codecs_ptr != ';' && c_idx < sizeof(codecs_buf) - 1) {
        codecs_buf[c_idx++] = *codecs_ptr++;
    }
    codecs_buf[c_idx] = '\0';

    // Parse comma-separated codecs
    char *token = strtok(codecs_buf, ", ");
    bool all_codecs_ok = true;
    int codec_count = 0;

    while (token) {
        codec_count++;
        for (size_t i = 0; token[i]; i++) {
            if (token[i] >= 'A' && token[i] <= 'Z') token[i] += 32;
        }

        bool ok = false;
        if (strncmp(token, "avc1", 4) == 0 || strncmp(token, "avc3", 4) == 0 || strcmp(token, "h264") == 0) ok = is_mp4;
        else if (strncmp(token, "mp4a", 4) == 0 || strcmp(token, "aac") == 0) ok = (is_mp4 || is_aac);
        else if (strncmp(token, "vp8", 3) == 0 || strncmp(token, "vp08", 4) == 0) ok = is_vp8;
        else if (strncmp(token, "vp9", 3) == 0 || strncmp(token, "vp09", 4) == 0) ok = is_vp9;
        else if (strncmp(token, "av01", 4) == 0 || strcmp(token, "av1") == 0) ok = is_av1;
        else if (strncmp(token, "av02", 4) == 0 || strcmp(token, "av2") == 0) ok = is_av2;
        else if (strcmp(token, "theora") == 0) ok = is_ogg;
        else if (strcmp(token, "vorbis") == 0) ok = (is_webm || is_ogg);
        else if (strcmp(token, "opus") == 0) ok = (is_webm || is_ogg || is_opus || is_mp4);
        else if (strcmp(token, "flac") == 0) ok = (is_flac || is_ogg || is_mp4);
        else if (strcmp(token, "mp3") == 0) ok = (is_mp3 || is_mp4);

        if (!ok) {
            all_codecs_ok = false;
            break;
        }
        token = strtok(NULL, ", ");
    }

    if (codec_count > 0 && all_codecs_ok) {
        return JS_NewString(ctx, "probably");
    } else if (codec_count > 0 && !all_codecs_ok) {
        return JS_NewString(ctx, "");
    }

    return JS_NewString(ctx, "maybe");
}

JSValue wisp_htmlmediaelement_fastSeek_impl(JSContext *ctx, QJSNodePrivate *priv, double time) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_getStartDate_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_load_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_pause_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_play_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_audioTracks_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "audiotracks", "");
}

JSValue wisp_htmlmediaelement_autoplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "autoplay");
}

JSValue wisp_htmlmediaelement_autoplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "autoplay", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_buffered_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlmediaelement_controller_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "controller", "");
}

JSValue wisp_htmlmediaelement_controller_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    // Stub setter for htmlmediaelement.controller
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_controls_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "controls");
}

JSValue wisp_htmlmediaelement_controls_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "controls", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_crossOrigin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "crossorigin", "");
}

JSValue wisp_htmlmediaelement_crossOrigin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "crossorigin", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_currentSrc_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "currentsrc", "");
}

JSValue wisp_htmlmediaelement_currentTime_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "currenttime", 0.0));
}

JSValue wisp_htmlmediaelement_currentTime_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "currenttime", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_defaultMuted_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "defaultmuted");
}

JSValue wisp_htmlmediaelement_defaultMuted_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "defaultmuted", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_defaultPlaybackRate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "defaultplaybackrate", 0.0));
}

JSValue wisp_htmlmediaelement_defaultPlaybackRate_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "defaultplaybackrate", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_duration_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "duration", 0.0));
}

JSValue wisp_htmlmediaelement_ended_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "ended");
}

JSValue wisp_htmlmediaelement_error_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlmediaelement_loop_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "loop");
}

JSValue wisp_htmlmediaelement_loop_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "loop", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_mediaGroup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "mediagroup", "");
}

JSValue wisp_htmlmediaelement_mediaGroup_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "mediagroup", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_muted_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "muted");
}

JSValue wisp_htmlmediaelement_muted_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "muted", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_networkState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "networkstate", 0));
}

JSValue wisp_htmlmediaelement_paused_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "paused");
}

JSValue wisp_htmlmediaelement_playbackRate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "playbackrate", 0.0));
}

JSValue wisp_htmlmediaelement_playbackRate_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "playbackrate", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_played_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "played", "");
}

JSValue wisp_htmlmediaelement_preload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "preload", "");
}

JSValue wisp_htmlmediaelement_preload_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "preload", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "readystate", 0));
}

JSValue wisp_htmlmediaelement_seekable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "seekable", "");
}

JSValue wisp_htmlmediaelement_seeking_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "seeking");
}

JSValue wisp_htmlmediaelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "src", "");
}

JSValue wisp_htmlmediaelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "src", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_srcObject_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlmediaelement_srcObject_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    // Stub setter for htmlmediaelement.srcObject
    return JS_UNDEFINED;
}


JSValue wisp_htmlmediaelement_videoTracks_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "videotracks", "");
}

JSValue wisp_htmlmediaelement_volume_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "volume", 0.0));
}

JSValue wisp_htmlmediaelement_volume_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "volume", value);
    return JS_UNDEFINED;
}

JSValue wisp_mouseevent_getModifierState_impl(JSContext *ctx, QJSNodePrivate *priv, const char * keyArg) {
    return JS_FALSE;
}

JSValue wisp_mouseevent_initMouseEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, void * viewArg, int32_t detailArg, int32_t screenXArg, int32_t screenYArg, int32_t clientXArg, int32_t clientYArg, bool ctrlKeyArg, bool altKeyArg, bool shiftKeyArg, bool metaKeyArg, int16_t buttonArg, void * relatedTargetArg) {
    return JS_UNDEFINED;
}

JSValue wisp_mouseevent_altKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_mouseevent_button_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_mouseevent_buttons_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_mouseevent_clientX_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_mouseevent_clientY_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_mouseevent_ctrlKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_mouseevent_metaKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_mouseevent_region_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_mouseevent_relatedTarget_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_mouseevent_screenX_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_mouseevent_screenY_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_mouseevent_shiftKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_constructor_impl(JSContext *ctx, const char * typeArg, JSValue keyboardEventInitDict) {
    return JS_UNDEFINED;
}

JSValue wisp_keyboardevent_getModifierState_impl(JSContext *ctx, QJSNodePrivate *priv, const char * keyArg) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_initKeyboardEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, void * viewArg, const char * keyArg, uint32_t locationArg, const char * modifiersListArg, bool repeat, const char * locale) {
    return JS_UNDEFINED;
}

JSValue wisp_keyboardevent_altKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_charCode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_keyboardevent_code_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_keyboardevent_ctrlKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_isComposing_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_key_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_keyboardevent_keyCode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_keyboardevent_location_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_keyboardevent_metaKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_repeat_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_shiftKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_keyboardevent_which_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_wheelevent_constructor_impl(JSContext *ctx, const char * typeArg, JSValue wheelEventInitDict) {
    return JS_UNDEFINED;
}

JSValue wisp_wheelevent_initWheelEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, void * viewArg, int32_t detailArg, int32_t screenXArg, int32_t screenYArg, int32_t clientXArg, int32_t clientYArg, int16_t buttonArg, void * relatedTargetArg, const char * modifiersListArg, double deltaXArg, double deltaYArg, double deltaZArg, uint32_t deltaMode) {
    return JS_UNDEFINED;
}

JSValue wisp_wheelevent_deltaMode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_wheelevent_deltaX_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_wheelevent_deltaY_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_wheelevent_deltaZ_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_focusevent_constructor_impl(JSContext *ctx, const char * typeArg, JSValue focusEventInitDict) {
    return JS_UNDEFINED;
}

JSValue wisp_focusevent_initFocusEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, void * viewArg, int32_t detailArg, void * relatedTargetArg) {
    return JS_UNDEFINED;
}

JSValue wisp_focusevent_relatedTarget_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlcanvaselement_probablySupportsContext_impl(JSContext *ctx, QJSNodePrivate *priv, const char * contextId, JSValue arguments) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlcanvaselement_setContext_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue context) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlcanvaselement_toBlob_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue callback, const char * type, JSValue arguments) {
    if (JS_IsFunction(ctx, callback)) {
        JSValue blob = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, blob, "size", JS_NewInt32(ctx, 0));
        JS_SetPropertyStr(ctx, blob, "type", JS_NewString(ctx, type ? type : "image/png"));
        JSValue ret = JS_Call(ctx, callback, JS_UNDEFINED, 1, &blob);
        JS_FreeValue(ctx, ret);
        JS_FreeValue(ctx, blob);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlcanvaselement_toDataURL_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, JSValue arguments) {
    return JS_NewString(ctx, "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==");
}

JSValue wisp_htmlcanvaselement_transferControlToProxy_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmldialogelement_close_impl(JSContext *ctx, QJSNodePrivate *priv, const char * returnValue) {
    set_element_bool_attr(ctx, priv, "open", false);
    return JS_UNDEFINED;
}

JSValue wisp_htmldialogelement_show_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue anchor) {
    set_element_bool_attr(ctx, priv, "open", true);
    return JS_UNDEFINED;
}

JSValue wisp_htmldialogelement_showModal_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue anchor) {
    set_element_bool_attr(ctx, priv, "open", true);
    return JS_UNDEFINED;
}

JSValue wisp_htmldialogelement_open_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "open");
}

JSValue wisp_htmldialogelement_open_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "open", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmldialogelement_returnValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "returnvalue", "");
}

JSValue wisp_htmldialogelement_returnValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "returnvalue", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltemplateelement_content_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue self_val = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue cached = JS_GetPropertyStr(ctx, self_val, "__wisp_template_content_cached");
    if (!JS_IsUndefined(cached) && !JS_IsNull(cached)) {
        JS_FreeValue(ctx, self_val);
        JS_FreeValue(ctx, global);
        return cached;
    }
    JS_FreeValue(ctx, cached);

    // Create document fragment
    JSValue doc_val = JS_GetPropertyStr(ctx, global, "document");
    JSValue frag_val = JS_NULL;
    if (JS_IsObject(doc_val)) {
        JSValue createFrag = JS_GetPropertyStr(ctx, doc_val, "createDocumentFragment");
        if (JS_IsFunction(ctx, createFrag)) {
            frag_val = JS_Call(ctx, createFrag, doc_val, 0, NULL);
        }
        JS_FreeValue(ctx, createFrag);
    }
    JS_FreeValue(ctx, doc_val);

    if (JS_IsObject(frag_val)) {
        if (!wisp_is_js_process) {
            // Direct LibDOM node operations: reparent child nodes from template element to fragment
            extern JSClassID qjs_documentfragment_class_id;
            dom_node *template_node = (dom_node *)priv->node;
            dom_node *frag_node = NULL;
            QJSNodePrivate *frag_priv = (QJSNodePrivate *)JS_GetOpaque(frag_val, qjs_documentfragment_class_id);
            if (!frag_priv) {
                frag_priv = (QJSNodePrivate *)JS_GetOpaque(frag_val, qjs_node_class_id);
            }
            if (frag_priv && frag_priv->node) {
                frag_node = (dom_node *)frag_priv->node;
            }

            if (frag_node && template_node) {
                dom_node *child = NULL;
                dom_node_get_first_child(template_node, &child);
                while (child != NULL) {
                    dom_node *next = NULL;
                    dom_node_get_next_sibling(child, &next);
                    dom_node *appended = NULL;
                    dom_node_append_child(frag_node, child, &appended);
                    if (appended) dom_node_unref(appended);
                    dom_node_unref(child);
                    child = next;
                }
            }
        } else {
            // JS process (SHM DOM) mode: move child nodes using JS DOM methods
            JSValue childNodes = JS_GetPropertyStr(ctx, self_val, "childNodes");
            if (JS_IsObject(childNodes)) {
                JSValue length_val = JS_GetPropertyStr(ctx, childNodes, "length");
                int len = 0;
                JS_ToInt32(ctx, &len, length_val);
                JS_FreeValue(ctx, length_val);

                JSValue appendChild = JS_GetPropertyStr(ctx, frag_val, "appendChild");
                if (JS_IsFunction(ctx, appendChild)) {
                    while (len > 0) {
                        JSValue first_child = JS_GetPropertyUint32(ctx, childNodes, 0);
                        if (JS_IsObject(first_child)) {
                            JSValue res = JS_Call(ctx, appendChild, frag_val, 1, &first_child);
                            JS_FreeValue(ctx, res);
                        }
                        JS_FreeValue(ctx, first_child);

                        JSValue new_len_val = JS_GetPropertyStr(ctx, childNodes, "length");
                        int new_len = 0;
                        JS_ToInt32(ctx, &new_len, new_len_val);
                        JS_FreeValue(ctx, new_len_val);
                        if (new_len >= len) {
                            break;
                        }
                        len = new_len;
                    }
                }
                JS_FreeValue(ctx, appendChild);
            }
            JS_FreeValue(ctx, childNodes);
        }

        // Cache the fragment on the template element
        JS_SetPropertyStr(ctx, self_val, "__wisp_template_content_cached", JS_DupValue(ctx, frag_val));
    }
    JS_FreeValue(ctx, self_val);
    JS_FreeValue(ctx, global);
    return frag_val;
}

JSValue wisp_htmlmeterelement_high_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    double max_val = get_element_double_attr(ctx, priv, "max", 1.0);
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "high", max_val));
}

JSValue wisp_htmlmeterelement_high_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "high", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmeterelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_nodelist(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_nodelist(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmlmeterelement_low_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    double min_val = get_element_double_attr(ctx, priv, "min", 0.0);
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "low", min_val));
}

JSValue wisp_htmlmeterelement_low_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "low", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmeterelement_max_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "max", 1.0));
}

JSValue wisp_htmlmeterelement_max_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "max", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmeterelement_min_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "min", 0.0));
}

JSValue wisp_htmlmeterelement_min_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "min", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmeterelement_optimum_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    double min_val = get_element_double_attr(ctx, priv, "min", 0.0);
    double max_val = get_element_double_attr(ctx, priv, "max", 1.0);
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "optimum", (min_val + max_val) / 2.0));
}

JSValue wisp_htmlmeterelement_optimum_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "optimum", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmeterelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "value", 0.0));
}

JSValue wisp_htmlmeterelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "value", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlprogresselement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_nodelist(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_nodelist(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmlprogresselement_position_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    double max_val = get_element_double_attr(ctx, priv, "max", 1.0);
    double val = get_element_double_attr(ctx, priv, "value", 0.0);
    if (max_val <= 0.0) return JS_NewFloat64(ctx, -1.0);
    double pos = val / max_val;
    if (pos < 0.0) pos = 0.0;
    if (pos > 1.0) pos = 1.0;
    return JS_NewFloat64(ctx, pos);
}

JSValue wisp_htmltrackelement_default_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "default", "");
}

JSValue wisp_htmltrackelement_default_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "default", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltrackelement_kind_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "kind", "");
}

JSValue wisp_htmltrackelement_kind_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "kind", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltrackelement_label_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "label", "");
}

JSValue wisp_htmltrackelement_label_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "label", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltrackelement_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "readystate", 0));
}

JSValue wisp_htmltrackelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "src", "");
}

JSValue wisp_htmltrackelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "src", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltrackelement_srclang_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "srclang", "");
}

JSValue wisp_htmltrackelement_srclang_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "srclang", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltrackelement_track_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "track", "");
}

#include <ctype.h>

struct style_property {
    char *name;
    char *value;
};

static char *camel_to_kebab(const char *str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char *res = malloc(len * 2 + 1);
    if (!res) return NULL;
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (isupper((unsigned char)str[i])) {
            res[j++] = '-';
            res[j++] = tolower((unsigned char)str[i]);
        } else {
            res[j++] = str[i];
        }
    }
    res[j] = '\0';
    return res;
}

static int parse_style_attribute(const char *style_str, struct style_property *props, int max_props) {
    if (!style_str) return 0;
    int count = 0;
    char *str = strdup(style_str);
    if (!str) return 0;
    char *saveptr1 = NULL;
    char *decl = strtok_r(str, ";", &saveptr1);
    while (decl && count < max_props) {
        char *colon = strchr(decl, ':');
        if (colon) {
            *colon = '\0';
            char *name = decl;
            char *val = colon + 1;
            while (isspace((unsigned char)*name)) name++;
            char *name_end = name + strlen(name) - 1;
            while (name_end >= name && isspace((unsigned char)*name_end)) {
                *name_end = '\0';
                name_end--;
            }
            while (isspace((unsigned char)*val)) val++;
            char *val_end = val + strlen(val) - 1;
            while (val_end >= val && isspace((unsigned char)*val_end)) {
                *val_end = '\0';
                val_end--;
            }
            if (*name && *val) {
                for (char *p = name; *p; p++) *p = tolower((unsigned char)*p);
                char *prop_name = strdup(name);
                char *prop_val = strdup(val);
                if (prop_name && prop_val) {
                    props[count].name = prop_name;
                    props[count].value = prop_val;
                    count++;
                } else {
                    free(prop_name);
                    free(prop_val);
                }
            }
        }
        decl = strtok_r(NULL, ";", &saveptr1);
    }
    free(str);
    return count;
}

static void free_style_properties(struct style_property *props, int count) {
    for (int i = 0; i < count; i++) {
        free(props[i].name);
        free(props[i].value);
    }
}

#include <ctype.h>
static bool has_important_priority(const char *val) {
    if (!val) return false;
    const char *p = val;
    while (*p) {
        if (*p == '!') {
            const char *temp_p = p + 1;
            while (isspace((unsigned char)*temp_p)) temp_p++;
            if (strncasecmp(temp_p, "important", 9) == 0) {
                temp_p += 9;
                while (isspace((unsigned char)*temp_p)) temp_p++;
                if (*temp_p == '\0') return true;
            }
        }
        p++;
    }
    return false;
}

static char *get_clean_value(const char *val) {
    if (!val) return NULL;
    char *dup = strdup(val);
    if (!dup) return NULL;
    char *p = dup;
    while (*p) {
        if (*p == '!') {
            char *start = p;
            char *temp_p = p + 1;
            while (isspace((unsigned char)*temp_p)) temp_p++;
            if (strncasecmp(temp_p, "important", 9) == 0) {
                temp_p += 9;
                while (isspace((unsigned char)*temp_p)) temp_p++;
                if (*temp_p == '\0') {
                    if (start > dup) {
                        start--;
                        while (start > dup && isspace((unsigned char)*start)) start--;
                        if (!isspace((unsigned char)*start)) {
                            *(start + 1) = '\0';
                        } else {
                            *start = '\0';
                        }
                    } else {
                        *dup = '\0';
                    }
                    return dup;
                }
            }
        }
        p++;
    }
    return dup;
}


static char *create_style_string(struct style_property *props, int count) {
    if (count <= 0 || !props) return strdup("");
    size_t stack_name_lens[64];
    size_t stack_val_lens[64];
    size_t *name_lens = count <= 64 ? stack_name_lens : malloc(count * sizeof(size_t));
    size_t *val_lens = count <= 64 ? stack_val_lens : malloc(count * sizeof(size_t));

    if (!name_lens || !val_lens) {
        if (count > 64) {
            free(name_lens);
            free(val_lens);
        }
        return NULL;
    }

    size_t total_len = 0;
    for (int i = 0; i < count; i++) {
        name_lens[i] = props[i].name ? strlen(props[i].name) : 0;
        val_lens[i] = props[i].value ? strlen(props[i].value) : 0;
        total_len += name_lens[i] + 2 + val_lens[i] + 2;
    }

    char *buf = malloc(total_len + 1);
    if (!buf) {
        if (count > 64) {
            free(name_lens);
            free(val_lens);
        }
        return NULL;
    }

    char *ptr = buf;
    for (int i = 0; i < count; i++) {
        size_t name_len = name_lens[i];
        size_t val_len = val_lens[i];
        if (props[i].name && name_len > 0) {
            memcpy(ptr, props[i].name, name_len);
            ptr += name_len;
        }
        memcpy(ptr, ": ", 2);
        ptr += 2;
        if (props[i].value && val_len > 0) {
            memcpy(ptr, props[i].value, val_len);
            ptr += val_len;
        }
        memcpy(ptr, "; ", 2);
        ptr += 2;
    }
    *ptr = '\0';

    if (count > 64) {
        free(name_lens);
        free(val_lens);
    }

    return buf;
}

static void serialize_style_properties(JSContext *ctx, QJSNodePrivate *priv, struct style_property *props, int count) {
    char *buf = create_style_string(props, count);
    if (buf) {
        JSValue dummy = wisp_element_setAttribute_impl(ctx, priv, "style", buf);
        JS_FreeValue(ctx, dummy);
        free(buf);
    }
}

JSValue wisp_cssstyledeclaration_getPropertyPriority_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property) {
    if (!priv || !priv->node || !property) return JS_NewString(ctx, "");
    char *kebab = camel_to_kebab(property);
    JSValue style_attr = wisp_element_getAttribute_impl(ctx, priv, "style");
    if (JS_IsNull(style_attr) || JS_IsUndefined(style_attr)) {
        free(kebab);
        return JS_NewString(ctx, "");
    }
    const char *style_str = JS_ToCString(ctx, style_attr);
    struct style_property props[256];
    int count = parse_style_attribute(style_str, props, 256);
    JS_FreeCString(ctx, style_str);
    JS_FreeValue(ctx, style_attr);
    JSValue result = JS_NewString(ctx, "");
    for (int i = 0; i < count; i++) {
        if (strcasecmp(props[i].name, kebab) == 0) {
            if (has_important_priority(props[i].value)) {
                JS_FreeValue(ctx, result);
                result = JS_NewString(ctx, "important");
            }
            break;
        }
    }
    free_style_properties(props, count);
    free(kebab);
    return result;
}

JSValue wisp_cssstyledeclaration_getPropertyValue_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property) {
    if (!priv || !priv->node || !property) return JS_NewString(ctx, "");
    char *kebab = camel_to_kebab(property);
    JSValue style_attr = wisp_element_getAttribute_impl(ctx, priv, "style");
    if (JS_IsNull(style_attr) || JS_IsUndefined(style_attr)) {
        free(kebab);
        return JS_NewString(ctx, "");
    }
    const char *style_str = JS_ToCString(ctx, style_attr);
    struct style_property props[256];
    int count = parse_style_attribute(style_str, props, 256);
    JS_FreeCString(ctx, style_str);
    JS_FreeValue(ctx, style_attr);
    JSValue result = JS_NewString(ctx, "");
    for (int i = 0; i < count; i++) {
        if (strcasecmp(props[i].name, kebab) == 0) {
            char *clean_val = get_clean_value(props[i].value);
            JS_FreeValue(ctx, result);
            result = JS_NewString(ctx, clean_val);
            free(clean_val);
            break;
        }
    }
    free_style_properties(props, count);
    free(kebab);
    return result;
}

JSValue wisp_cssstyledeclaration_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    JSValue style_attr = wisp_element_getAttribute_impl(ctx, priv, "style");
    if (JS_IsNull(style_attr) || JS_IsUndefined(style_attr)) {
        return JS_NewString(ctx, "");
    }
    const char *style_str = JS_ToCString(ctx, style_attr);
    struct style_property props[256];
    int count = parse_style_attribute(style_str, props, 256);
    JS_FreeCString(ctx, style_str);
    JS_FreeValue(ctx, style_attr);
    JSValue result = JS_NewString(ctx, "");
    if (index < (uint32_t)count) {
        JS_FreeValue(ctx, result);
        result = JS_NewString(ctx, props[index].name);
    }
    free_style_properties(props, count);
    return result;
}

JSValue wisp_cssstyledeclaration_removeProperty_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property) {
    if (!priv || !priv->node || !property) return JS_NewString(ctx, "");
    char *kebab = camel_to_kebab(property);
    JSValue style_attr = wisp_element_getAttribute_impl(ctx, priv, "style");
    const char *style_str = NULL;
    if (!JS_IsNull(style_attr) && !JS_IsUndefined(style_attr)) {
        style_str = JS_ToCString(ctx, style_attr);
    }
    struct style_property props[256];
    int count = parse_style_attribute(style_str, props, 256);
    if (style_str) {
        JS_FreeCString(ctx, style_str);
    }
    JS_FreeValue(ctx, style_attr);
    JSValue removed_val = JS_NewString(ctx, "");
    for (int i = 0; i < count; i++) {
        if (strcasecmp(props[i].name, kebab) == 0) {
            char *clean = get_clean_value(props[i].value);
            JS_FreeValue(ctx, removed_val);
            removed_val = JS_NewString(ctx, clean);
            free(clean);
            free(props[i].name);
            free(props[i].value);
            props[i] = props[--count];
            break;
        }
    }
    serialize_style_properties(ctx, priv, props, count);
    free_style_properties(props, count);
    free(kebab);
    return removed_val;
}

JSValue wisp_cssstyledeclaration_setProperty_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property, const char * value, const char * priority) {
    if (!priv || !priv->node || !property) return JS_UNDEFINED;
    char *kebab = camel_to_kebab(property);
    JSValue style_attr = wisp_element_getAttribute_impl(ctx, priv, "style");
    const char *style_str = NULL;
    if (!JS_IsNull(style_attr) && !JS_IsUndefined(style_attr)) {
        style_str = JS_ToCString(ctx, style_attr);
    }
    struct style_property props[256];
    int count = parse_style_attribute(style_str, props, 256);
    if (style_str) {
        JS_FreeCString(ctx, style_str);
    }
    JS_FreeValue(ctx, style_attr);
    char *final_val = NULL;
    if (value && *value) {
        if (priority && strcasecmp(priority, "important") == 0) {
            size_t needed = strlen(value) + 12;
            final_val = malloc(needed);
            if (final_val) {
                snprintf(final_val, needed, "%s !important", value);
            }
        } else {
            final_val = strdup(value);
        }
    }
    bool found = false;
    for (int i = 0; i < count; i++) {
        if (strcasecmp(props[i].name, kebab) == 0) {
            free(props[i].value);
            if (final_val) {
                props[i].value = final_val;
                final_val = NULL;
            } else {
                props[i].value = NULL;
                free(props[i].name);
                props[i] = props[--count];
            }
            found = true;
            break;
        }
    }
    if (!found && final_val && count < 256) {
        char *pname = strdup(kebab);
        if (pname) {
            props[count].name = pname;
            props[count].value = final_val;
            final_val = NULL;
            count++;
        }
    }
    serialize_style_properties(ctx, priv, props, count);
    if (final_val) free(final_val);
    free_style_properties(props, count);
    free(kebab);
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_setPropertyPriority_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property, const char * priority) {
    if (!priv || !priv->node || !property) return JS_UNDEFINED;
    JSValue current_val = wisp_cssstyledeclaration_getPropertyValue_impl(ctx, priv, property);
    const char *val_str = JS_ToCString(ctx, current_val);
    wisp_cssstyledeclaration_setProperty_impl(ctx, priv, property, val_str, priority);
    if (val_str) JS_FreeCString(ctx, val_str);
    JS_FreeValue(ctx, current_val);
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_setPropertyValue_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property, const char * value) {
    if (!priv || !priv->node || !property) return JS_UNDEFINED;
    JSValue current_pri = wisp_cssstyledeclaration_getPropertyPriority_impl(ctx, priv, property);
    const char *pri_str = JS_ToCString(ctx, current_pri);
    wisp_cssstyledeclaration_setProperty_impl(ctx, priv, property, value, pri_str);
    if (pri_str) JS_FreeCString(ctx, pri_str);
    JS_FreeValue(ctx, current_pri);
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_cssFloat_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "none");
}

JSValue wisp_cssstyledeclaration_cssFloat_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_cssText_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "style");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return JS_NewString(ctx, "");
    }
    const char *style_str = JS_ToCString(ctx, val);
    struct style_property props[256];
    int count = parse_style_attribute(style_str, props, 256);
    JS_FreeCString(ctx, style_str);
    JS_FreeValue(ctx, val);

    char *buf = create_style_string(props, count);
    JSValue result = JS_NewString(ctx, buf);
    free(buf);
    free_style_properties(props, count);
    return result;
}

JSValue wisp_cssstyledeclaration_cssText_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    if (!priv || !priv->node) return JS_UNDEFINED;

    struct style_property props[256];
    int count = parse_style_attribute(value ? value : "", props, 256);

    char *buf = create_style_string(props, count);

    JSValue dummy = wisp_element_setAttribute_impl(ctx, priv, "style", buf);
    JS_FreeValue(ctx, dummy);
    free(buf);
    free_style_properties(props, count);
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_dashed_attribute_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_cssstyledeclaration_dashed_attribute_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    JSValue style_attr = wisp_element_getAttribute_impl(ctx, priv, "style");
    if (JS_IsNull(style_attr) || JS_IsUndefined(style_attr)) {
        return JS_NewInt32(ctx, 0);
    }
    const char *style_str = JS_ToCString(ctx, style_attr);
    struct style_property props[256];
    int count = parse_style_attribute(style_str, props, 256);
    JS_FreeCString(ctx, style_str);
    JS_FreeValue(ctx, style_attr);
    free_style_properties(props, count);
    return JS_NewInt32(ctx, count);
}

JSValue wisp_cssstyledeclaration_parentRule_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlelement_forceSpellCheck_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_accessKey_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "accesskey", "");
}

JSValue wisp_htmlelement_accessKey_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "accesskey", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_accessKeyLabel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "accesskeylabel", "");
}

JSValue wisp_htmlelement_commandChecked_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "commandchecked", "");
}

JSValue wisp_htmlelement_commandDisabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "commanddisabled", "");
}

JSValue wisp_htmlelement_commandHidden_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "commandhidden", "");
}

JSValue wisp_htmlelement_commandIcon_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "commandicon", "");
}

JSValue wisp_htmlelement_commandLabel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "commandlabel", "");
}

JSValue wisp_htmlelement_commandType_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "commandtype", "");
}

JSValue wisp_htmlelement_contentEditable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "inherit");
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "contenteditable");
    if (!JS_IsString(val)) {
        JS_FreeValue(ctx, val);
        return JS_NewString(ctx, "inherit");
    }
    const char *str = JS_ToCString(ctx, val);
    JS_FreeValue(ctx, val);
    if (!str) return JS_NewString(ctx, "inherit");

    JSValue res;
    if (strcasecmp(str, "true") == 0 || str[0] == '\0' || strcasecmp(str, "contenteditable") == 0) {
        res = JS_NewString(ctx, "true");
    } else if (strcasecmp(str, "false") == 0) {
        res = JS_NewString(ctx, "false");
    } else {
        res = JS_NewString(ctx, "inherit");
    }
    JS_FreeCString(ctx, str);
    return res;
}

JSValue wisp_htmlelement_contentEditable_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    if (strcasecmp(value, "inherit") == 0) {
        wisp_element_removeAttribute_impl(ctx, priv, "contenteditable");
    } else if (strcasecmp(value, "true") == 0 || value[0] == '\0') {
        set_element_str_attr(ctx, priv, "contenteditable", "true");
    } else if (strcasecmp(value, "false") == 0) {
        set_element_str_attr(ctx, priv, "contenteditable", "false");
    } else {
        return JS_ThrowSyntaxError(ctx, "The contentEditable attribute value must be 'true', 'false', or 'inherit'");
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_contextMenu_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "contextmenu", "");
}

JSValue wisp_htmlelement_contextMenu_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    // Stub setter for htmlelement.contextMenu
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_dataset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_draggable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_FALSE;

    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "draggable");
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        JS_FreeValue(ctx, val);
        if (str) {
            if (strcasecmp(str, "true") == 0) {
                JS_FreeCString(ctx, str);
                return JS_TRUE;
            }
            if (strcasecmp(str, "false") == 0) {
                JS_FreeCString(ctx, str);
                return JS_FALSE;
            }
            JS_FreeCString(ctx, str);
        }
    } else {
        JS_FreeValue(ctx, val);
    }

    // Default 'auto' behavior: true for <a> with href or <img> with src
    JSValue tag_val = wisp_element_tagName_get_impl(ctx, priv);
    if (JS_IsString(tag_val)) {
        const char *tag = JS_ToCString(ctx, tag_val);
        JS_FreeValue(ctx, tag_val);
        if (tag) {
            if (strcasecmp(tag, "A") == 0) {
                JS_FreeCString(ctx, tag);
                JSValue href_val = wisp_element_getAttribute_impl(ctx, priv, "href");
                bool has_href = JS_IsString(href_val) && !JS_IsNull(href_val);
                JS_FreeValue(ctx, href_val);
                return has_href ? JS_TRUE : JS_FALSE;
            }
            if (strcasecmp(tag, "IMG") == 0) {
                JS_FreeCString(ctx, tag);
                JSValue src_val = wisp_element_getAttribute_impl(ctx, priv, "src");
                bool has_src = JS_IsString(src_val) && !JS_IsNull(src_val);
                JS_FreeValue(ctx, src_val);
                return has_src ? JS_TRUE : JS_FALSE;
            }
            JS_FreeCString(ctx, tag);
        }
    } else {
        JS_FreeValue(ctx, tag_val);
    }

    return JS_FALSE;
}

JSValue wisp_htmlelement_draggable_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    set_element_str_attr(ctx, priv, "draggable", value ? "true" : "false");
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_dropzone_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "dropzone", "");
}

JSValue wisp_htmlelement_isContentEditable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_FALSE;

    QJSNodePrivate *curr = priv;
    JSValue curr_val = JS_UNDEFINED;

    while (curr && curr->node) {
        JSValue val = wisp_element_getAttribute_impl(ctx, curr, "contenteditable");
        if (JS_IsString(val)) {
            const char *str = JS_ToCString(ctx, val);
            JS_FreeValue(ctx, val);
            if (str) {
                if (strcasecmp(str, "true") == 0 || str[0] == '\0' || strcasecmp(str, "contenteditable") == 0) {
                    JS_FreeCString(ctx, str);
                    JS_FreeValue(ctx, curr_val);
                    return JS_TRUE;
                }
                if (strcasecmp(str, "false") == 0) {
                    JS_FreeCString(ctx, str);
                    JS_FreeValue(ctx, curr_val);
                    return JS_FALSE;
                }
                JS_FreeCString(ctx, str);
            }
        } else {
            JS_FreeValue(ctx, val);
        }

        // Get parent element
        JSValue next_parent_val = wisp_node_parentElement_get_impl(ctx, curr);
        JS_FreeValue(ctx, curr_val);
        curr_val = next_parent_val;

        if (JS_IsNull(curr_val) || JS_IsUndefined(curr_val)) {
            JS_FreeValue(ctx, curr_val);
            curr_val = JS_UNDEFINED;
            break;
        }

        curr = qjs_get_dom_priv(ctx, curr_val);
        if (!curr) {
            JS_FreeValue(ctx, curr_val);
            curr_val = JS_UNDEFINED;
            break;
        }
    }
    JS_FreeValue(ctx, curr_val);

    // Check document designMode
    JSValue doc_val = wisp_node_ownerDocument_get_impl(ctx, priv);
    if (JS_IsObject(doc_val)) {
        QJSNodePrivate *doc_priv = qjs_get_dom_priv(ctx, doc_val);
        if (doc_priv) {
            JSValue dm_val = wisp_document_designMode_get_impl(ctx, doc_priv);
            if (JS_IsString(dm_val)) {
                const char *dm_str = JS_ToCString(ctx, dm_val);
                JS_FreeValue(ctx, dm_val);
                if (dm_str) {
                    bool is_on = (strcasecmp(dm_str, "on") == 0);
                    JS_FreeCString(ctx, dm_str);
                    JS_FreeValue(ctx, doc_val);
                    if (is_on) return JS_TRUE;
                    return JS_FALSE;
                }
            } else {
                JS_FreeValue(ctx, dm_val);
            }
        }
        JS_FreeValue(ctx, doc_val);
    } else {
        JS_FreeValue(ctx, doc_val);
    }

    return JS_FALSE;
}

JSValue wisp_htmlelement_spellcheck_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "spellcheck");
}

JSValue wisp_htmlelement_spellcheck_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "spellcheck", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_translate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "translate");
}

JSValue wisp_htmlelement_translate_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "translate", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLMarqueeElement Implementation (32 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlmarqueeelement_start_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_stop_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_behavior_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "behavior", "scroll");
}

JSValue wisp_htmlmarqueeelement_behavior_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "behavior", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_bgColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "bgcolor", "");
}

JSValue wisp_htmlmarqueeelement_bgColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "bgcolor", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_direction_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "direction", "left");
}

JSValue wisp_htmlmarqueeelement_direction_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "direction", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "height", "");
}

JSValue wisp_htmlmarqueeelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "height", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_hspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "hspace", 0));
}

JSValue wisp_htmlmarqueeelement_hspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "hspace", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_loop_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "loop", -1));
}

JSValue wisp_htmlmarqueeelement_loop_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value) {
    set_element_int_attr(ctx, priv, "loop", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_onbounce_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbounce");
}

JSValue wisp_htmlmarqueeelement_onbounce_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbounce", "bounce", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_onfinish_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onfinish");
}

JSValue wisp_htmlmarqueeelement_onfinish_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onfinish", "finish", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_onstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstart");
}

JSValue wisp_htmlmarqueeelement_onstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstart", "start", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_scrollAmount_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "scrollamount", 6));
}

JSValue wisp_htmlmarqueeelement_scrollAmount_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "scrollamount", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_scrollDelay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "scrolldelay", 85));
}

JSValue wisp_htmlmarqueeelement_scrollDelay_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "scrolldelay", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_trueSpeed_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "truespeed");
}

JSValue wisp_htmlmarqueeelement_trueSpeed_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "truespeed", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_vspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "vspace", 0));
}

JSValue wisp_htmlmarqueeelement_vspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "vspace", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "width", "");
}

JSValue wisp_htmlmarqueeelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLAppletElement Implementation (22 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlappletelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}

JSValue wisp_htmlappletelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_alt_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "alt", "");
}

JSValue wisp_htmlappletelement_alt_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "alt", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_archive_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "archive", "");
}

JSValue wisp_htmlappletelement_archive_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "archive", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_code_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "code", "");
}

JSValue wisp_htmlappletelement_code_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "code", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_codeBase_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "codebase", "");
}

JSValue wisp_htmlappletelement_codeBase_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "codebase", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "height", "");
}

JSValue wisp_htmlappletelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "height", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_hspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "hspace", 0));
}

JSValue wisp_htmlappletelement_hspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "hspace", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "name", "");
}

JSValue wisp_htmlappletelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_object_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "object", "");
}

JSValue wisp_htmlappletelement_object_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "object", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_vspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "vspace", 0));
}

JSValue wisp_htmlappletelement_vspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "vspace", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlappletelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "width", "");
}

JSValue wisp_htmlappletelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLDirectoryElement Implementation (2 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmldirectoryelement_compact_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "compact");
}

JSValue wisp_htmldirectoryelement_compact_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "compact", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// MimeType Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_mimetype_description_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "MimeType Description");
}

JSValue wisp_mimetype_enabledPlugin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_mimetype_suffixes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "html");
}

JSValue wisp_mimetype_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "text/html");
}

// -----------------------------------------------------------------------------
// Plugin Implementation (6 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_plugin_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_NULL;
}

JSValue wisp_plugin_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_NULL;
}

JSValue wisp_plugin_description_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "Wisp Plugin");
}

JSValue wisp_plugin_filename_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "libwisp.so");
}

JSValue wisp_plugin_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_plugin_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "Wisp Standard Plugin");
}

// -----------------------------------------------------------------------------
// PluginArray Implementation (4 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_pluginarray_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_NULL;
}

JSValue wisp_pluginarray_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_NULL;
}

JSValue wisp_pluginarray_refresh_impl(JSContext *ctx, QJSNodePrivate *priv, bool reload) {
    return JS_UNDEFINED;
}

JSValue wisp_pluginarray_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// -----------------------------------------------------------------------------
// MimeTypeArray Implementation (3 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_mimetypearray_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_NULL;
}

JSValue wisp_mimetypearray_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_NULL;
}

JSValue wisp_mimetypearray_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Forward declarations of navigator functions
extern JSValue wisp_navigator_cookieEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_navigator_userAgent_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_navigator_appCodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_navigator_appName_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_navigator_appVersion_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_navigator_platform_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_navigator_product_get_impl(JSContext *ctx, QJSNodePrivate *priv);
extern JSValue wisp_navigator_language_get_impl(JSContext *ctx, QJSNodePrivate *priv);

// -----------------------------------------------------------------------------
// NavigatorPlugins Implementation (3 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_navigatorplugins_javaEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_navigatorplugins_mimeTypes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern int qjs_init_mimetypearray(JSContext *ctx);
    extern JSValue qjs_new_mimetypearray(JSContext *ctx, void *node, bool is_dom_node);
    qjs_init_mimetypearray(ctx);
    return qjs_new_mimetypearray(ctx, NULL, false);
}

JSValue wisp_navigatorplugins_plugins_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern int qjs_init_pluginarray(JSContext *ctx);
    extern JSValue qjs_new_pluginarray(JSContext *ctx, void *node, bool is_dom_node);
    qjs_init_pluginarray(ctx);
    return qjs_new_pluginarray(ctx, NULL, false);
}

// -----------------------------------------------------------------------------
// DrawingStyle Implementation (20 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_drawingstyle_getLineDash_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_drawingstyle_setLineDash_impl(JSContext *ctx, QJSNodePrivate *priv, double segments) {
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_direction_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__drawing_direction", "ltr");
}

JSValue wisp_drawingstyle_direction_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "__drawing_direction", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_font_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__drawing_font", "10px sans-serif");
}

JSValue wisp_drawingstyle_font_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "__drawing_font", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_lineCap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__drawing_linecap", "butt");
}

JSValue wisp_drawingstyle_lineCap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "__drawing_linecap", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_lineDashOffset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "__drawing_linedashoffset", 0.0));
}

JSValue wisp_drawingstyle_lineDashOffset_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "__drawing_linedashoffset", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_lineJoin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__drawing_linejoin", "miter");
}

JSValue wisp_drawingstyle_lineJoin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "__drawing_linejoin", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_lineWidth_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "__drawing_linewidth", 1.0));
}

JSValue wisp_drawingstyle_lineWidth_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "__drawing_linewidth", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_miterLimit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "__drawing_miterlimit", 10.0));
}

JSValue wisp_drawingstyle_miterLimit_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "__drawing_miterlimit", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_textAlign_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__drawing_textalign", "start");
}

JSValue wisp_drawingstyle_textAlign_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "__drawing_textalign", value);
    return JS_UNDEFINED;
}

JSValue wisp_drawingstyle_textBaseline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__drawing_textbaseline", "alphabetic");
}

JSValue wisp_drawingstyle_textBaseline_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "__drawing_textbaseline", value);
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// TextMetrics Implementation (12 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_textmetrics_actualBoundingBoxAscent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_actualBoundingBoxDescent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_actualBoundingBoxLeft_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_actualBoundingBoxRight_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_alphabeticBaseline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_emHeightAscent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_emHeightDescent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_fontBoundingBoxAscent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_fontBoundingBoxDescent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_hangingBaseline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_ideographicBaseline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

JSValue wisp_textmetrics_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, 0.0);
}

// -----------------------------------------------------------------------------
// HTMLObjectElement Implementation (42 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmlobjectelement_getSVGDocument_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlobjectelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlobjectelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlobjectelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlobjectelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

JSValue wisp_htmlobjectelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}

JSValue wisp_htmlobjectelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_archive_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "archive", "");
}

JSValue wisp_htmlobjectelement_archive_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "archive", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_border_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "border", "");
}

JSValue wisp_htmlobjectelement_border_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "border", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_code_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "code", "");
}

JSValue wisp_htmlobjectelement_code_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "code", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_codeBase_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "codebase", "");
}

JSValue wisp_htmlobjectelement_codeBase_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "codebase", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_codeType_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "codetype", "");
}

JSValue wisp_htmlobjectelement_codeType_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "codetype", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_contentDocument_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlobjectelement_contentWindow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlobjectelement_data_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "data", "");
}

JSValue wisp_htmlobjectelement_data_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "data", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_declare_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "declare");
}

JSValue wisp_htmlobjectelement_declare_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "declare", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_form_impl(ctx, priv);
}

JSValue wisp_htmlobjectelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "height", "");
}

JSValue wisp_htmlobjectelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "height", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_hspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "hspace", 0));
}

JSValue wisp_htmlobjectelement_hspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "hspace", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "name", "");
}

JSValue wisp_htmlobjectelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_standby_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "standby", "");
}

JSValue wisp_htmlobjectelement_standby_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "standby", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "type", "");
}

JSValue wisp_htmlobjectelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_typeMustMatch_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "typemustmatch");
}

JSValue wisp_htmlobjectelement_typeMustMatch_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "typemustmatch", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_useMap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "usemap", "");
}

JSValue wisp_htmlobjectelement_useMap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "usemap", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "__customValidity", "");
}

JSValue wisp_htmlobjectelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

JSValue wisp_htmlobjectelement_vspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, get_element_int_attr(ctx, priv, "vspace", 0));
}

JSValue wisp_htmlobjectelement_vspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    set_element_int_attr(ctx, priv, "vspace", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "width", "");
}

JSValue wisp_htmlobjectelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlobjectelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// -----------------------------------------------------------------------------
// HTMLTextAreaElement Implementation (14 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_htmltextareaelement_autocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "autocomplete", "");
}

JSValue wisp_htmltextareaelement_autocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "autocomplete", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_autofocus_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "autofocus");
}

JSValue wisp_htmltextareaelement_autofocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "autofocus", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_dirName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "dirname", "");
}

JSValue wisp_htmltextareaelement_dirName_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "dirname", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_form_impl(ctx, priv);
}

JSValue wisp_htmltextareaelement_inputMode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "inputmode", "");
}

JSValue wisp_htmltextareaelement_inputMode_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "inputmode", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_wrap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "wrap", "");
}

JSValue wisp_htmltextareaelement_wrap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "wrap", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmltextareaelement_textLength_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue text = wisp_htmltextareaelement_value_get_impl(ctx, priv);
    if (JS_IsString(text)) {
        const char *str = JS_ToCString(ctx, text);
        int32_t len = str ? strlen(str) : 0;
        if (str) JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, text);
        return JS_NewInt32(ctx, len);
    }
    JS_FreeValue(ctx, text);
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_htmltextareaelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}


// =============================================================================
// WAVE 5 - 150+ WEBIDL STUBS IMPLEMENTATION & INTEGRATION (169 stubs)
// =============================================================================

// Forward declarations
extern JSValue qjs_new_url(JSContext *ctx, void *node, bool is_dom_node);
extern JSValue qjs_new_urlsearchparams(JSContext *ctx, void *node, bool is_dom_node);

// 1. StorageEvent Implementation (5 stubs)
JSValue wisp_storageevent_key_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_storageevent_oldValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_storageevent_newValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_storageevent_url_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_storageevent_storageArea_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 2. CloseEvent Implementation (3 stubs + constructor)
JSValue wisp_closeevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_NewObject(ctx);
}
JSValue wisp_closeevent_wasClean_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}
JSValue wisp_closeevent_code_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}
JSValue wisp_closeevent_reason_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// 3. MessagePort Implementation (5 stubs)
JSValue wisp_messageport_postMessage_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue message, JSValue transfer) {
    return JS_UNDEFINED;
}
JSValue wisp_messageport_start_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}
JSValue wisp_messageport_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}
JSValue wisp_messageport_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}
JSValue wisp_messageport_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// 4. BroadcastChannel Implementation (5 stubs + constructor)
typedef struct WispBroadcastChannelPrivate {
    char *name;
} WispBroadcastChannelPrivate;

extern JSClassID qjs_broadcastchannel_class_id;
static void js_broadcastchannel_finalizer_manual(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_broadcastchannel_class_id);
    if (priv) {
        if (priv->node) {
            WispBroadcastChannelPrivate *bcp = (WispBroadcastChannelPrivate *)priv->node;
            free(bcp->name);
            free(bcp);
        }
        free(priv);
    }
}

static JSClassDef js_broadcastchannel_class_manual = {
    "BroadcastChannel",
    .finalizer = js_broadcastchannel_finalizer_manual,
};

int qjs_init_broadcastchannel(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_broadcastchannel_class_id == 0) JS_NewClassID(rt, &qjs_broadcastchannel_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_broadcastchannel_class_id)) {
        JS_NewClass(rt, qjs_broadcastchannel_class_id, &js_broadcastchannel_class_manual);
    }
    extern int qjs_init_broadcastchannel_gen(JSContext *ctx);
    return qjs_init_broadcastchannel_gen(ctx);
}

extern JSValue qjs_new_broadcastchannel(JSContext *ctx, void *node, bool is_dom_node);

JSValue wisp_broadcastchannel_constructor_impl(JSContext *ctx, const char * name) {
    WispBroadcastChannelPrivate *bcp = calloc(1, sizeof(WispBroadcastChannelPrivate));
    if (bcp && name) {
        bcp->name = strdup(name);
    }
    return qjs_new_broadcastchannel(ctx, bcp, false);
}
JSValue wisp_broadcastchannel_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}
JSValue wisp_broadcastchannel_postMessage_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue message) {
    return JS_UNDEFINED;
}
JSValue wisp_broadcastchannel_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    WispBroadcastChannelPrivate *bcp = (WispBroadcastChannelPrivate *)priv->node;
    return JS_NewString(ctx, bcp->name ? bcp->name : "");
}
JSValue wisp_broadcastchannel_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}
JSValue wisp_broadcastchannel_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// 5. HTMLButtonElement Implementation (3 stubs)
JSValue wisp_htmlbuttonelement_menu_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmlbuttonelement_menu_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    return JS_UNDEFINED;
}

// 6. HTMLLegendElement Implementation (1 stub)
JSValue wisp_htmllegendelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_form_impl(ctx, priv);
}

// 7. HTMLInputElement Implementation (10 stubs)
JSValue wisp_htmlinputelement_files_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmlinputelement_inputMode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "inputmode", "");
}
JSValue wisp_htmlinputelement_inputMode_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "inputmode", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlinputelement_multiple_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "multiple");
}
JSValue wisp_htmlinputelement_multiple_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "multiple", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlinputelement_valueLow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "valuelow", 0.0));
}
JSValue wisp_htmlinputelement_valueLow_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "valuelow", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlinputelement_valueHigh_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "valuehigh", 0.0));
}
JSValue wisp_htmlinputelement_valueHigh_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "valuehigh", value);
    return JS_UNDEFINED;
}

// 8. HTMLFormElement Implementation (9 stubs)
JSValue wisp_htmlformelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLFormElement target");
    JSValue wrapper = qjs_wrap_node(ctx, (struct dom_node *)priv->node);
    JSValue elements = JS_GetPropertyStr(ctx, wrapper, "elements");
    JS_FreeValue(ctx, wrapper);
    bool all_valid = true;
    if (JS_IsObject(elements)) {
        JSValue length_val = JS_GetPropertyStr(ctx, elements, "length");
        uint32_t len = 0;
        if (JS_IsNumber(length_val)) {
            JS_ToUint32(ctx, &len, length_val);
        }
        JS_FreeValue(ctx, length_val);
        for (uint32_t i = 0; i < len; i++) {
            JSValue el = JS_GetPropertyUint32(ctx, elements, i);
            JSValue check = JS_GetPropertyStr(ctx, el, "checkValidity");
            if (JS_IsFunction(ctx, check)) {
                JSValue ret = JS_Call(ctx, check, el, 0, NULL);
                if (JS_IsBool(ret) && !JS_ToBool(ctx, ret)) {
                    all_valid = false;
                }
                JS_FreeValue(ctx, ret);
            }
            JS_FreeValue(ctx, check);
            JS_FreeValue(ctx, el);
        }
    }
    JS_FreeValue(ctx, elements);
    return JS_NewBool(ctx, all_valid);
}
JSValue wisp_htmlformelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue wrapper = qjs_wrap_node(ctx, (struct dom_node *)priv->node);
    JSValue elements = JS_GetPropertyStr(ctx, wrapper, "elements");
    JS_FreeValue(ctx, wrapper);
    bool all_valid = true;
    if (JS_IsObject(elements)) {
        JSValue length_val = JS_GetPropertyStr(ctx, elements, "length");
        uint32_t len = 0;
        if (JS_IsNumber(length_val)) {
            JS_ToUint32(ctx, &len, length_val);
        }
        JS_FreeValue(ctx, length_val);
        for (uint32_t i = 0; i < len; i++) {
            JSValue el = JS_GetPropertyUint32(ctx, elements, i);
            JSValue check = JS_GetPropertyStr(ctx, el, "reportValidity");
            if (JS_IsFunction(ctx, check)) {
                JSValue ret = JS_Call(ctx, check, el, 0, NULL);
                if (JS_IsBool(ret) && !JS_ToBool(ctx, ret)) {
                    all_valid = false;
                }
                JS_FreeValue(ctx, ret);
            }
            JS_FreeValue(ctx, check);
            JS_FreeValue(ctx, el);
        }
    }
    JS_FreeValue(ctx, elements);
    return JS_NewBool(ctx, all_valid);
}
JSValue wisp_htmlformelement_requestAutocomplete_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}
JSValue wisp_htmlformelement_requestSubmit_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue submitter) {
    if (!priv || !priv->node) return JS_ThrowTypeError(ctx, "Invalid HTMLFormElement target");
    JSValue valid = wisp_htmlformelement_checkValidity_impl(ctx, priv);
    bool is_valid = true;
    if (JS_IsBool(valid)) {
        is_valid = JS_ToBool(ctx, valid);
    }
    JS_FreeValue(ctx, valid);
    if (!is_valid) {
        return JS_UNDEFINED;
    }
    return wisp_htmlformelement_submit_impl(ctx, priv);
}
JSValue wisp_htmlformelement_autocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "autocomplete", "");
}
JSValue wisp_htmlformelement_autocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "autocomplete", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlformelement_encoding_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "enctype", "");
}
JSValue wisp_htmlformelement_encoding_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "enctype", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlformelement_noValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "novalidate");
}
JSValue wisp_htmlformelement_noValidate_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "novalidate", value);
    return JS_UNDEFINED;
}

// 9. URL Implementation (23 stubs)

// Custom manual initializer/finalizer to prevent nsurl leak for URL objects
extern JSClassID qjs_url_class_id;
static void js_url_finalizer_manual(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_url_class_id);
    if (priv) {
        if (priv->magic == QJS_DOM_MAGIC && priv->node) {
            nsurl_unref((struct nsurl *)priv->node);
        }
        free(priv);
    }
}

static JSClassDef js_url_class_manual = {
    "URL",
    .finalizer = js_url_finalizer_manual,
};

int qjs_init_url(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_url_class_id == 0) JS_NewClassID(rt, &qjs_url_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_url_class_id)) {
        JS_NewClass(rt, qjs_url_class_id, &js_url_class_manual);
    }
    extern int qjs_init_url_gen(JSContext *ctx);
    return qjs_init_url_gen(ctx);
}

JSValue wisp_url_constructor_impl(JSContext *ctx, const char * url, const char * base) {
    struct nsurl *u = NULL;
    if (base && strlen(base) > 0) {
        struct nsurl *b_url = NULL;
        nsurl_create(base, &b_url);
        if (b_url) {
            nsurl_join(b_url, url ? url : "", &u);
            nsurl_unref(b_url);
        } else {
            nsurl_create(url ? url : "", &u);
        }
    } else {
        nsurl_create(url ? url : "", &u);
    }
    return qjs_new_url(ctx, u, false);
}
JSValue wisp_url_href_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    return JS_NewString(ctx, nsurl_access(u));
}
JSValue wisp_url_hash_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *frag = nsurl_get_component(u, NSURL_FRAGMENT);
    if (frag) {
        char buf[512];
        snprintf(buf, sizeof(buf), "#%s", lwc_string_data(frag));
        lwc_string_unref(frag);
        return JS_NewString(ctx, buf);
    }
    return JS_NewString(ctx, "");
}
JSValue wisp_url_hash_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_host_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *host = nsurl_get_component(u, NSURL_HOST);
    lwc_string *port = nsurl_get_component(u, NSURL_PORT);
    if (host) {
        char buf[512];
        if (port) {
            snprintf(buf, sizeof(buf), "%s:%s", lwc_string_data(host), lwc_string_data(port));
            lwc_string_unref(port);
        } else {
            snprintf(buf, sizeof(buf), "%s", lwc_string_data(host));
        }
        lwc_string_unref(host);
        return JS_NewString(ctx, buf);
    }
    return JS_NewString(ctx, "");
}
JSValue wisp_url_host_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_hostname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *host = nsurl_get_component(u, NSURL_HOST);
    if (host) {
        JSValue res = JS_NewString(ctx, lwc_string_data(host));
        lwc_string_unref(host);
        return res;
    }
    return JS_NewString(ctx, "");
}
JSValue wisp_url_hostname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *scheme = nsurl_get_component(u, NSURL_SCHEME);
    lwc_string *host = nsurl_get_component(u, NSURL_HOST);
    lwc_string *port = nsurl_get_component(u, NSURL_PORT);
    if (scheme && host) {
        char buf[512];
        if (port) {
            snprintf(buf, sizeof(buf), "%s://%s:%s", lwc_string_data(scheme), lwc_string_data(host), lwc_string_data(port));
            lwc_string_unref(port);
        } else {
            snprintf(buf, sizeof(buf), "%s://%s", lwc_string_data(scheme), lwc_string_data(host));
        }
        lwc_string_unref(scheme);
        lwc_string_unref(host);
        return JS_NewString(ctx, buf);
    }
    if (scheme) lwc_string_unref(scheme);
    if (host) lwc_string_unref(host);
    if (port) lwc_string_unref(port);
    return JS_NewString(ctx, "");
}
JSValue wisp_url_password_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_url_password_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_pathname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *path = nsurl_get_component(u, NSURL_PATH);
    if (path) {
        JSValue res = JS_NewString(ctx, lwc_string_data(path));
        lwc_string_unref(path);
        return res;
    }
    return JS_NewString(ctx, "");
}
JSValue wisp_url_pathname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_port_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *port = nsurl_get_component(u, NSURL_PORT);
    if (port) {
        JSValue res = JS_NewString(ctx, lwc_string_data(port));
        lwc_string_unref(port);
        return res;
    }
    return JS_NewString(ctx, "");
}
JSValue wisp_url_port_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "http:");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *scheme = nsurl_get_component(u, NSURL_SCHEME);
    if (scheme) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s:", lwc_string_data(scheme));
        lwc_string_unref(scheme);
        return JS_NewString(ctx, buf);
    }
    return JS_NewString(ctx, "http:");
}
JSValue wisp_url_protocol_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_search_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    struct nsurl *u = (struct nsurl *)priv->node;
    lwc_string *query = nsurl_get_component(u, NSURL_QUERY);
    if (query) {
        char buf[512];
        snprintf(buf, sizeof(buf), "?%s", lwc_string_data(query));
        lwc_string_unref(query);
        return JS_NewString(ctx, buf);
    }
    return JS_NewString(ctx, "");
}
JSValue wisp_url_search_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_searchParams_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return qjs_new_urlsearchparams(ctx, NULL, false);
}
JSValue wisp_url_searchParams_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    return JS_UNDEFINED;
}
JSValue wisp_url_username_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_url_username_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// 10. HTMLEmbedElement Implementation (13 stubs)
JSValue wisp_htmlembedelement___legacycaller___impl(JSContext *ctx, QJSNodePrivate *priv, JSValue arguments) {
    return JS_UNDEFINED;
}
JSValue wisp_htmlembedelement_getSVGDocument_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmlembedelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmlembedelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlembedelement_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "height", "");
}
JSValue wisp_htmlembedelement_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "height", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlembedelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "name", "");
}
JSValue wisp_htmlembedelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "name", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlembedelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "src", "");
}
JSValue wisp_htmlembedelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "src", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlembedelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "type", "");
}
JSValue wisp_htmlembedelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlembedelement_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "width", "");
}
JSValue wisp_htmlembedelement_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "width", value);
    return JS_UNDEFINED;
}

// 11. HTMLIFrameElement Implementation (7 stubs)
JSValue wisp_htmliframeelement_getSVGDocument_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmliframeelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "align", "");
}
JSValue wisp_htmliframeelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "align", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmliframeelement_allowFullscreen_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "allowfullscreen");
}
JSValue wisp_htmliframeelement_allowFullscreen_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "allowfullscreen", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmliframeelement_seamless_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "seamless");
}
JSValue wisp_htmliframeelement_seamless_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "seamless", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmliframeelement_srcdoc_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "srcdoc", "");
}
JSValue wisp_htmliframeelement_srcdoc_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "srcdoc", value);
    return JS_UNDEFINED;
}

// 12. HTMLAnchorElement Implementation (12 stubs)
JSValue wisp_htmlanchorelement_download_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "download", "");
}
JSValue wisp_htmlanchorelement_download_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "download", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlanchorelement_ping_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "ping", "");
}
JSValue wisp_htmlanchorelement_ping_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "ping", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlanchorelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "type", "");
}
JSValue wisp_htmlanchorelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "type", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmlanchorelement_text_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_node_textContent_get_impl(ctx, priv);
}
JSValue wisp_htmlanchorelement_text_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return wisp_node_textContent_set_impl(ctx, priv, value);
}
JSValue wisp_htmlanchorelement_username_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_htmlanchorelement_username_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_htmlanchorelement_password_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_htmlanchorelement_password_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_htmlanchorelement_relList_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_domtokenlist(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_domtokenlist(ctx, priv->node, priv->is_dom_node);
}

// 13. HTMLLinkElement Implementation (5 stubs)
JSValue wisp_htmllinkelement_crossOrigin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "crossorigin", "");
}
JSValue wisp_htmllinkelement_crossOrigin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "crossorigin", value);
    return JS_UNDEFINED;
}
JSValue wisp_htmllinkelement_relList_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_domtokenlist(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_domtokenlist(ctx, priv->node, priv->is_dom_node);
}
JSValue wisp_htmllinkelement_sizes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_htmllinkelement_sheet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 14. HTMLOptionsCollection Implementation (5 stubs)
JSValue wisp_htmloptionscollection___setter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index, void * option) {
    return JS_UNDEFINED;
}
JSValue wisp_htmloptionscollection_add_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue element, JSValue before) {
    return JS_UNDEFINED;
}
JSValue wisp_htmloptionscollection_remove_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index) {
    return JS_UNDEFINED;
}
JSValue wisp_htmloptionscollection_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}
JSValue wisp_htmloptionscollection_length_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    return JS_UNDEFINED;
}
JSValue wisp_htmloptionscollection_selectedIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, -1);
}
JSValue wisp_htmloptionscollection_selectedIndex_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value) {
    return JS_UNDEFINED;
}

// 15. HTMLAllCollection Implementation (4 stubs)
JSValue wisp_htmlallcollection_item_0_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_NULL;
}
JSValue wisp_htmlallcollection_item_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_NULL;
}
JSValue wisp_htmlallcollection_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_NULL;
}
JSValue wisp_htmlallcollection_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// 16. RadioNodeList Implementation (2 stubs)
JSValue wisp_radionodelist_value_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_radionodelist_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// 17. HTMLFormControlsCollection Implementation (1 stub)
JSValue wisp_htmlformcontrolscollection_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_NULL;
}

// 18. ProcessingInstruction Implementation (2 stubs)
JSValue wisp_processinginstruction_sheet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_processinginstruction_target_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// 19. XMLSerializer Implementation (1 stub + constructor)
JSValue wisp_xmlserializer_constructor_impl(JSContext *ctx) {
    extern JSValue qjs_new_xmlserializer(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_xmlserializer(ctx, NULL, false);
}
JSValue wisp_xmlserializer_serializeToString_impl(JSContext *ctx, QJSNodePrivate *priv, void * root) {
    return JS_NewString(ctx, "");
}

// 20. XMLDocument Implementation (1 stub)
JSValue wisp_xmldocument_load_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url) {
    return JS_TRUE;
}

// 21. TimeRanges Implementation (3 stubs)
JSValue wisp_timeranges_end_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_NewInt32(ctx, 0);
}
JSValue wisp_timeranges_start_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_NewInt32(ctx, 0);
}
JSValue wisp_timeranges_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// 22. MessageChannel Implementation (2 stubs + constructor)
JSValue wisp_messagechannel_constructor_impl(JSContext *ctx) {
    return JS_NewObject(ctx);
}
JSValue wisp_messagechannel_port1_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_messagechannel_port2_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 23. BeforeUnloadEvent Implementation (2 stubs)
JSValue wisp_beforeunloadevent_returnValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_beforeunloadevent_returnValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// 24. HashChangeEvent Implementation (2 stubs + constructor)
JSValue wisp_hashchangeevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_NewObject(ctx);
}
JSValue wisp_hashchangeevent_newURL_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_hashchangeevent_oldURL_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// 25. TreeWalker Implementation (12 stubs)
JSValue wisp_treewalker_firstChild_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_lastChild_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_nextNode_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_nextSibling_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_parentNode_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_previousNode_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_previousSibling_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_currentNode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_currentNode_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    return JS_UNDEFINED;
}
JSValue wisp_treewalker_filter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_root_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_treewalker_whatToShow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// 26. NodeIterator Implementation (8 stubs)
JSValue wisp_nodeiterator_detach_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}
JSValue wisp_nodeiterator_nextNode_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_nodeiterator_previousNode_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_nodeiterator_filter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_nodeiterator_pointerBeforeReferenceNode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}
JSValue wisp_nodeiterator_referenceNode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_nodeiterator_root_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_nodeiterator_whatToShow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// 27. PseudoElement Implementation (4 stubs)
JSValue wisp_pseudoelement_cascadedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_pseudoelement_defaultStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_pseudoelement_rawComputedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_pseudoelement_usedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 28. ImageBitmap Implementation (2 stubs)
JSValue wisp_imagebitmap_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}
JSValue wisp_imagebitmap_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// 29. BarProp Implementation (1 stub)
JSValue wisp_barprop_visible_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

// 30. Touch Implementation (1 stub)
JSValue wisp_touch_region_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}


// 33. AutocompleteErrorEvent Implementation (1 stub + constructor)
JSValue wisp_autocompleteerrorevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_NewObject(ctx);
}
JSValue wisp_autocompleteerrorevent_reason_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// 34. TrackEvent Implementation (1 stub + constructor)
JSValue wisp_trackevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_NewObject(ctx);
}
JSValue wisp_trackevent_track_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 35. RelatedEvent Implementation (1 stub + constructor)
JSValue wisp_relatedevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_NewObject(ctx);
}
JSValue wisp_relatedevent_relatedTarget_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 36. PageTransitionEvent Implementation (1 stub + constructor)
JSValue wisp_pagetransitionevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_NewObject(ctx);
}
JSValue wisp_pagetransitionevent_persisted_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// 37. PopStateEvent Implementation (1 stub + constructor)
JSValue wisp_popstateevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_NewObject(ctx);
}
JSValue wisp_popstateevent_state_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// =============================================================================
// AUTOMATICALLY GENERATED WEBIDL STUB OVERRIDES (165 STUBS)
// =============================================================================

// Overrides: method | MutationEvent::initMutationEvent();
JSValue wisp_mutationevent_initMutationEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, void * relatedNodeArg, const char * prevValueArg, const char * newValueArg, const char * attrNameArg, uint16_t attrChangeArg) {
    return JS_UNDEFINED;
}

// Overrides: getter | MutationEvent::relatedNode(user);
JSValue wisp_mutationevent_relatedNode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MutationEvent::prevValue(string);
JSValue wisp_mutationevent_prevValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MutationEvent::newValue(string);
JSValue wisp_mutationevent_newValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MutationEvent::attrName(string);
JSValue wisp_mutationevent_attrName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MutationEvent::attrChange(unsigned short);
JSValue wisp_mutationevent_attrChange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | UIEvent::initUIEvent();
JSValue wisp_uievent_initUIEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, void * viewArg, int32_t detailArg) {
    return JS_UNDEFINED;
}

// Overrides: getter | UIEvent::view(user);
JSValue wisp_uievent_view_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | UIEvent::detail(long);
JSValue wisp_uievent_detail_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | CompositionEvent::initCompositionEvent();
JSValue wisp_compositionevent_initCompositionEvent_impl(JSContext *ctx, QJSNodePrivate *priv, const char * typeArg, bool bubblesArg, bool cancelableArg, void * viewArg, const char * dataArg, const char * locale) {
    return JS_UNDEFINED;
}

// Overrides: getter | CompositionEvent::data(string);
JSValue wisp_compositionevent_data_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | CSSMarginRule::name(string);

// Overrides: getter | CSSMarginRule::style(user);




// Overrides: getter | StyleSheet::parentStyleSheet(user);
JSValue wisp_stylesheet_parentStyleSheet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | StyleSheet::title(string);
JSValue wisp_stylesheet_title_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | StyleSheet::media(user);
JSValue wisp_stylesheet_media_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | StyleSheet::disabled(boolean);
JSValue wisp_stylesheet_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// Overrides: setter | StyleSheet::disabled(boolean);
JSValue wisp_stylesheet_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: method | MediaList::item();
JSValue wisp_medialist_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

// Overrides: method | MediaList::appendMedium();
JSValue wisp_medialist_appendMedium_impl(JSContext *ctx, QJSNodePrivate *priv, const char * medium) {
    return JS_UNDEFINED;
}

// Overrides: method | MediaList::deleteMedium();
JSValue wisp_medialist_deleteMedium_impl(JSContext *ctx, QJSNodePrivate *priv, const char * medium) {
    return JS_UNDEFINED;
}

// Overrides: getter | MediaList::length(unsigned long);
JSValue wisp_medialist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Element::getAttributeNS();
JSValue wisp_element_getAttributeNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName) {
    return JS_NewString(ctx, "");
}

// Overrides: method | Element::setAttributeNS();
JSValue wisp_element_setAttributeNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * name, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::removeAttributeNS();
JSValue wisp_element_removeAttributeNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::hasAttributeNS();
JSValue wisp_element_hasAttributeNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::getAttributeNode();
JSValue wisp_element_getAttributeNode_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_NULL;
}

// Overrides: method | Element::getAttributeNodeNS();
JSValue wisp_element_getAttributeNodeNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName) {
    return JS_NULL;
}

// Overrides: method | Element::setAttributeNode();
JSValue wisp_element_setAttributeNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * attr) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::setAttributeNodeNS();
JSValue wisp_element_setAttributeNodeNS_impl(JSContext *ctx, QJSNodePrivate *priv, void * attr) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::removeAttributeNode();
JSValue wisp_element_removeAttributeNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * attr) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::closest();
JSValue wisp_element_closest_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::getElementsByTagNameNS();
JSValue wisp_element_getElementsByTagNameNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName) {
    return JS_NewString(ctx, "");
}

// Overrides: method | Element::insertAdjacentHTML();

// Overrides: method | Element::pseudo();
JSValue wisp_element_pseudo_impl(JSContext *ctx, QJSNodePrivate *priv, const char * pseudoElt) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::query();
JSValue wisp_element_query_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::queryAll();
JSValue wisp_element_queryAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::before();
JSValue wisp_element_before_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::after();
JSValue wisp_element_after_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | Element::replaceWith();
JSValue wisp_element_replaceWith_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: getter | Element::cascadedStyle(user);
JSValue wisp_element_cascadedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | Element::defaultStyle(user);
JSValue wisp_element_defaultStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | Element::rawComputedStyle(user);
JSValue wisp_element_rawComputedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | Element::usedStyle(user);
JSValue wisp_element_usedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerLocation::origin(user);
JSValue wisp_workerlocation_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::protocol(user);
JSValue wisp_workerlocation_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::host(user);
JSValue wisp_workerlocation_host_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::hostname(user);
JSValue wisp_workerlocation_hostname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::port(user);
JSValue wisp_workerlocation_port_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::pathname(user);
JSValue wisp_workerlocation_pathname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::search(user);
JSValue wisp_workerlocation_search_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::hash(user);
JSValue wisp_workerlocation_hash_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: method | WorkerNavigator::taintEnabled();
JSValue wisp_workernavigator_taintEnabled_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerNavigator::appCodeName(string);
JSValue wisp_workernavigator_appCodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_appCodeName_get_impl(ctx, priv);
}

// Overrides: getter | WorkerNavigator::appName(string);
JSValue wisp_workernavigator_appName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_appName_get_impl(ctx, priv);
}

// Overrides: getter | WorkerNavigator::appVersion(string);
JSValue wisp_workernavigator_appVersion_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_appVersion_get_impl(ctx, priv);
}

// Overrides: getter | WorkerNavigator::platform(string);
JSValue wisp_workernavigator_platform_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_platform_get_impl(ctx, priv);
}

// Overrides: getter | WorkerNavigator::product(string);
JSValue wisp_workernavigator_product_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_product_get_impl(ctx, priv);
}

// Overrides: getter | WorkerNavigator::productSub(string);
JSValue wisp_workernavigator_productSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "20030107");
}

// Overrides: getter | WorkerNavigator::userAgent(string);
JSValue wisp_workernavigator_userAgent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_userAgent_get_impl(ctx, priv);
}

// Overrides: getter | WorkerNavigator::vendor(string);
JSValue wisp_workernavigator_vendor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "Google Inc.");
}

// Overrides: getter | WorkerNavigator::vendorSub(string);
JSValue wisp_workernavigator_vendorSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::languages(string);
JSValue wisp_workernavigator_languages_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue arr = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, arr, 0, JS_NewString(ctx, "en-US"));
    return arr;
}

// Overrides: getter | WorkerNavigator::onLine(boolean);
JSValue wisp_workernavigator_onLine_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewBool(ctx, 1);
}

// Overrides: getter | SharedWorker::port(user);
JSValue wisp_sharedworker_port_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue port_ctor = JS_GetPropertyStr(ctx, global, "MessagePort");
    JS_FreeValue(ctx, global);
    JSValue port = JS_UNDEFINED;
    if (JS_IsFunction(ctx, port_ctor)) {
        port = JS_CallConstructor(ctx, port_ctor, 0, NULL);
    } else {
        port = JS_NewObject(ctx);
    }
    JS_FreeValue(ctx, port_ctor);
    return port;
}

// Overrides: getter | SharedWorker::onerror(user);
JSValue wisp_sharedworker_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: setter | SharedWorker::onerror(user);
JSValue wisp_sharedworker_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: method | WorkerGlobalScope::setTimeout();
JSValue wisp_workerglobalscope_setTimeout_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return wisp_timer_create(ctx, handler, timeout, arguments, false);
}
JSValue wisp_workerglobalscope_setTimeout_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    JSValue handler_val = JS_NewString(ctx, handler);
    JSValue ret = wisp_timer_create(ctx, handler_val, timeout, arguments, false);
    JS_FreeValue(ctx, handler_val);
    return ret;
}

// Overrides: method | WorkerGlobalScope::clearTimeout();
JSValue wisp_workerglobalscope_clearTimeout_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return wisp_timer_clear(ctx, handle);
}

// Overrides: method | WorkerGlobalScope::setInterval();
JSValue wisp_workerglobalscope_setInterval_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return wisp_timer_create(ctx, handler, timeout, arguments, true);
}
JSValue wisp_workerglobalscope_setInterval_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    JSValue handler_val = JS_NewString(ctx, handler);
    JSValue ret = wisp_timer_create(ctx, handler_val, timeout, arguments, true);
    JS_FreeValue(ctx, handler_val);
    return ret;
}

// Overrides: method | WorkerGlobalScope::clearInterval();
JSValue wisp_workerglobalscope_clearInterval_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return wisp_timer_clear(ctx, handle);
}

// Overrides: method | WorkerGlobalScope::createImageBitmap();
JSValue wisp_workerglobalscope_createImageBitmap_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image) {
    return JS_UNDEFINED;
}
JSValue wisp_workerglobalscope_createImageBitmap_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, int32_t sx, int32_t sy, int32_t sw, int32_t sh) {
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerGlobalScope::location(user);
JSValue wisp_workerglobalscope_location_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerGlobalScope::navigator(user);
JSValue wisp_workerglobalscope_navigator_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerGlobalScope::onerror(user);
JSValue wisp_workerglobalscope_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: setter | WorkerGlobalScope::onerror(user);
JSValue wisp_workerglobalscope_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerGlobalScope::onlanguagechange(user);
JSValue wisp_workerglobalscope_onlanguagechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onlanguagechange");
}

// Overrides: setter | WorkerGlobalScope::onlanguagechange(user);
JSValue wisp_workerglobalscope_onlanguagechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onlanguagechange", "languagechange", value);
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerGlobalScope::onoffline(user);
JSValue wisp_workerglobalscope_onoffline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onoffline");
}

// Overrides: setter | WorkerGlobalScope::onoffline(user);
JSValue wisp_workerglobalscope_onoffline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onoffline", "offline", value);
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerGlobalScope::ononline(user);
JSValue wisp_workerglobalscope_ononline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ononline");
}

// Overrides: setter | WorkerGlobalScope::ononline(user);
JSValue wisp_workerglobalscope_ononline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ononline", "online", value);
    return JS_UNDEFINED;
}

// Overrides: getter | SharedWorkerGlobalScope::name(string);
JSValue wisp_sharedworkerglobalscope_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | SharedWorkerGlobalScope::applicationCache(user);
JSValue wisp_sharedworkerglobalscope_applicationCache_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | SharedWorkerGlobalScope::onconnect(user);
JSValue wisp_sharedworkerglobalscope_onconnect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onconnect");
}

// Overrides: setter | SharedWorkerGlobalScope::onconnect(user);
JSValue wisp_sharedworkerglobalscope_onconnect_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onconnect", "connect", value);
    return JS_UNDEFINED;
}

// Overrides: method | WebSocket::close();
JSValue wisp_websocket_close_impl(JSContext *ctx, QJSNodePrivate *priv, uint16_t code, const char * reason) {
    return JS_UNDEFINED;
}

// Overrides: method | WebSocket::send();
JSValue wisp_websocket_send_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * data) {
    return JS_UNDEFINED;
}
JSValue wisp_websocket_send_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue data) {
    return JS_UNDEFINED;
}
JSValue wisp_websocket_send_2_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue data) {
    return JS_UNDEFINED;
}
JSValue wisp_websocket_send_3_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue data) {
    return JS_UNDEFINED;
}

typedef struct WispWebSocketPrivate {
    char *url;
    char *binaryType;
    uint16_t readyState;
} WispWebSocketPrivate;

extern JSClassID qjs_websocket_class_id;
static void js_websocket_finalizer_manual(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_websocket_class_id);
    if (priv) {
        if (priv->node) {
            WispWebSocketPrivate *wsp = (WispWebSocketPrivate *)priv->node;
            free(wsp->url);
            free(wsp->binaryType);
            free(wsp);
        }
        free(priv);
    }
}

static JSClassDef js_websocket_class_manual = {
    "WebSocket",
    .finalizer = js_websocket_finalizer_manual,
};

int qjs_init_websocket(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_websocket_class_id == 0) JS_NewClassID(rt, &qjs_websocket_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_websocket_class_id)) {
        JS_NewClass(rt, qjs_websocket_class_id, &js_websocket_class_manual);
    }
    extern int qjs_init_websocket_gen(JSContext *ctx);
    return qjs_init_websocket_gen(ctx);
}

typedef struct WispEventSourcePrivate {
    char *url;
    uint16_t readyState;
    bool withCredentials;
} WispEventSourcePrivate;

extern JSClassID qjs_eventsource_class_id;
static void js_eventsource_finalizer_manual(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_eventsource_class_id);
    if (priv) {
        if (priv->node) {
            WispEventSourcePrivate *esp = (WispEventSourcePrivate *)priv->node;
            free(esp->url);
            free(esp);
        }
        free(priv);
    }
}

static JSClassDef js_eventsource_class_manual = {
    "EventSource",
    .finalizer = js_eventsource_finalizer_manual,
};

int qjs_init_eventsource(JSContext *ctx)
{
    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_eventsource_class_id == 0) JS_NewClassID(rt, &qjs_eventsource_class_id);
    if (!JS_IsRegisteredClass(rt, qjs_eventsource_class_id)) {
        JS_NewClass(rt, qjs_eventsource_class_id, &js_eventsource_class_manual);
    }
    extern int qjs_init_eventsource_gen(JSContext *ctx);
    return qjs_init_eventsource_gen(ctx);
}

// Overrides: getter | WebSocket::url(string);
JSValue wisp_websocket_url_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    WispWebSocketPrivate *wsp = (WispWebSocketPrivate *)priv->node;
    return JS_NewString(ctx, wsp->url ? wsp->url : "");
}

// Overrides: getter | WebSocket::readyState(unsigned short);
JSValue wisp_websocket_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    WispWebSocketPrivate *wsp = (WispWebSocketPrivate *)priv->node;
    return JS_NewInt32(ctx, wsp->readyState);
}

// Overrides: getter | WebSocket::bufferedAmount(unsigned long);
JSValue wisp_websocket_bufferedAmount_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: getter | WebSocket::onopen(user);
JSValue wisp_websocket_onopen_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onopen");
}

// Overrides: setter | WebSocket::onopen(user);
JSValue wisp_websocket_onopen_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onopen", "open", value);
    return JS_UNDEFINED;
}

// Overrides: getter | WebSocket::onerror(user);
JSValue wisp_websocket_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: setter | WebSocket::onerror(user);
JSValue wisp_websocket_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: getter | WebSocket::onclose(user);
JSValue wisp_websocket_onclose_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclose");
}

// Overrides: setter | WebSocket::onclose(user);
JSValue wisp_websocket_onclose_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclose", "close", value);
    return JS_UNDEFINED;
}

// Overrides: getter | WebSocket::extensions(string);
JSValue wisp_websocket_extensions_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WebSocket::protocol(string);
JSValue wisp_websocket_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WebSocket::onmessage(user);
JSValue wisp_websocket_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}

// Overrides: setter | WebSocket::onmessage(user);
JSValue wisp_websocket_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// Overrides: getter | WebSocket::binaryType(user);
JSValue wisp_websocket_binaryType_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "blob");
    WispWebSocketPrivate *wsp = (WispWebSocketPrivate *)priv->node;
    return JS_NewString(ctx, wsp->binaryType ? wsp->binaryType : "blob");
}

// Overrides: setter | WebSocket::binaryType(user);
JSValue wisp_websocket_binaryType_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    WispWebSocketPrivate *wsp = (WispWebSocketPrivate *)priv->node;
    const char *str = JS_ToCString(ctx, value);
    if (str) {
        if (strcmp(str, "blob") == 0 || strcmp(str, "arraybuffer") == 0) {
            free(wsp->binaryType);
            wsp->binaryType = strdup(str);
        }
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

// Overrides: method | EventSource::close();
JSValue wisp_eventsource_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (priv && priv->node) {
        WispEventSourcePrivate *esp = (WispEventSourcePrivate *)priv->node;
        esp->readyState = 2; // CLOSED
    }
    return JS_UNDEFINED;
}

// Overrides: getter | EventSource::url(string);
JSValue wisp_eventsource_url_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    WispEventSourcePrivate *esp = (WispEventSourcePrivate *)priv->node;
    return JS_NewString(ctx, esp->url ? esp->url : "");
}

// Overrides: getter | EventSource::withCredentials(boolean);
JSValue wisp_eventsource_withCredentials_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_FALSE;
    WispEventSourcePrivate *esp = (WispEventSourcePrivate *)priv->node;
    return esp->withCredentials ? JS_TRUE : JS_FALSE;
}

// Overrides: getter | EventSource::readyState(unsigned short);
JSValue wisp_eventsource_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    WispEventSourcePrivate *esp = (WispEventSourcePrivate *)priv->node;
    return JS_NewInt32(ctx, esp->readyState);
}

// Overrides: getter | EventSource::onopen(user);
JSValue wisp_eventsource_onopen_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onopen");
}

// Overrides: setter | EventSource::onopen(user);
JSValue wisp_eventsource_onopen_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onopen", "open", value);
    return JS_UNDEFINED;
}

// Overrides: getter | EventSource::onmessage(user);
JSValue wisp_eventsource_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}

// Overrides: setter | EventSource::onmessage(user);
JSValue wisp_eventsource_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// Overrides: getter | EventSource::onerror(user);
JSValue wisp_eventsource_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: setter | EventSource::onerror(user);
JSValue wisp_eventsource_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: method | External::AddSearchProvider();
JSValue wisp_external_AddSearchProvider_impl(JSContext *ctx, QJSNodePrivate *priv, const char * engineURL) {
    return JS_UNDEFINED;
}

// Overrides: method | External::IsSearchProviderInstalled();
JSValue wisp_external_IsSearchProviderInstalled_impl(JSContext *ctx, QJSNodePrivate *priv, const char * engineURL) {
    return JS_FALSE;
}

// Overrides: method | Navigator::registerProtocolHandler();
JSValue wisp_navigator_registerProtocolHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url, const char * title) {
    return JS_UNDEFINED;
}

// Overrides: method | Navigator::registerContentHandler();
JSValue wisp_navigator_registerContentHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url, const char * title) {
    return JS_UNDEFINED;
}

// Overrides: method | Navigator::isProtocolHandlerRegistered();
JSValue wisp_navigator_isProtocolHandlerRegistered_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: method | Navigator::isContentHandlerRegistered();
JSValue wisp_navigator_isContentHandlerRegistered_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: method | Navigator::unregisterProtocolHandler();
JSValue wisp_navigator_unregisterProtocolHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: method | Navigator::unregisterContentHandler();
JSValue wisp_navigator_unregisterContentHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: method | Navigator::yieldForStorageUpdates();
JSValue wisp_navigator_yieldForStorageUpdates_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: getter | Navigator::onLine(boolean);
JSValue wisp_navigator_onLine_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewBool(ctx, 1);
}

// Overrides: getter | Navigator::plugins(user);
JSValue wisp_navigator_plugins_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigatorplugins_plugins_get_impl(ctx, priv);
}

// Overrides: getter | Navigator::mimeTypes(user);
JSValue wisp_navigator_mimeTypes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigatorplugins_mimeTypes_get_impl(ctx, priv);
}

// Overrides: method | ApplicationCache::update();
JSValue wisp_applicationcache_update_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | ApplicationCache::abort();
JSValue wisp_applicationcache_abort_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | ApplicationCache::swapCache();
JSValue wisp_applicationcache_swapCache_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::status(unsigned short);
JSValue wisp_applicationcache_status_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | ApplicationCache::onchecking(user);
JSValue wisp_applicationcache_onchecking_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchecking");
}

// Overrides: setter | ApplicationCache::onchecking(user);
JSValue wisp_applicationcache_onchecking_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchecking", "checking", value);
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onerror(user);
JSValue wisp_applicationcache_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: setter | ApplicationCache::onerror(user);
JSValue wisp_applicationcache_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onnoupdate(user);
JSValue wisp_applicationcache_onnoupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onnoupdate");
}

// Overrides: setter | ApplicationCache::onnoupdate(user);
JSValue wisp_applicationcache_onnoupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onnoupdate", "noupdate", value);
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::ondownloading(user);
JSValue wisp_applicationcache_ondownloading_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondownloading");
}

// Overrides: setter | ApplicationCache::ondownloading(user);
JSValue wisp_applicationcache_ondownloading_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondownloading", "downloading", value);
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onprogress(user);
JSValue wisp_applicationcache_onprogress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onprogress");
}

// Overrides: setter | ApplicationCache::onprogress(user);
JSValue wisp_applicationcache_onprogress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onprogress", "progress", value);
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onupdateready(user);
JSValue wisp_applicationcache_onupdateready_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onupdateready");
}

// Overrides: setter | ApplicationCache::onupdateready(user);
JSValue wisp_applicationcache_onupdateready_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onupdateready", "updateready", value);
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::oncached(user);
JSValue wisp_applicationcache_oncached_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncached");
}

// Overrides: setter | ApplicationCache::oncached(user);
JSValue wisp_applicationcache_oncached_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncached", "cached", value);
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onobsolete(user);
JSValue wisp_applicationcache_onobsolete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onobsolete");
}

// Overrides: setter | ApplicationCache::onobsolete(user);
JSValue wisp_applicationcache_onobsolete_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onobsolete", "obsolete", value);
    return JS_UNDEFINED;
}

// Overrides: getter | Location::ancestorOrigins(string);
JSValue wisp_location_ancestorOrigins_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Location::username(user);
JSValue wisp_location_username_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: setter | Location::password(user);
JSValue wisp_location_password_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::close();
JSValue wisp_window_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::stop();
JSValue wisp_window_stop_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::focus();
JSValue wisp_window_focus_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::blur();
JSValue wisp_window_blur_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::open();
JSValue wisp_window_open_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url, const char * target, const char * features, bool replace) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::confirm();
JSValue wisp_window_confirm_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::prompt();
JSValue wisp_window_prompt_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message, const char * default_val) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::print();
JSValue wisp_window_print_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::showModalDialog();
JSValue wisp_window_showModalDialog_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url, JSValue argument) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::requestAnimationFrame();
JSValue wisp_window_requestAnimationFrame_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue callback) {
    JSValueConst argv[1] = { callback };
    return js_requestAnimationFrame(ctx, JS_UNDEFINED, 1, argv);
}

// Overrides: method | Window::cancelAnimationFrame();
JSValue wisp_window_cancelAnimationFrame_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t handle) {
    JSValue handle_val = JS_NewUint32(ctx, handle);
    JSValueConst argv[1] = { handle_val };
    JSValue res = js_cancelAnimationFrame(ctx, JS_UNDEFINED, 1, argv);
    JS_FreeValue(ctx, handle_val);
    return res;
}

// Overrides: method | Window::postMessage();
JSValue wisp_window_postMessage_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue message, const char * targetOrigin, JSValue transfer) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue window_val = JS_GetPropertyStr(ctx, global, "window");
    if (!JS_IsObject(window_val)) {
        JS_FreeValue(ctx, window_val);
        window_val = JS_DupValue(ctx, global);
    }

    JSValue evt = JS_UNDEFINED;
    JSValue msg_ctor = JS_GetPropertyStr(ctx, global, "MessageEvent");
    if (JS_IsFunction(ctx, msg_ctor)) {
        JSValue init = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, init, "data", JS_DupValue(ctx, message));
        if (targetOrigin) {
            JS_SetPropertyStr(ctx, init, "origin", JS_NewString(ctx, targetOrigin));
        }
        JSValue type_val = JS_NewString(ctx, "message");
        JSValue args[2] = { type_val, init };
        evt = JS_CallConstructor(ctx, msg_ctor, 2, args);
        JS_FreeValue(ctx, type_val);
        JS_FreeValue(ctx, init);
    }
    JS_FreeValue(ctx, msg_ctor);

    if (JS_IsException(evt) || JS_IsUndefined(evt)) {
        JSValue evt_ctor = JS_GetPropertyStr(ctx, global, "Event");
        if (JS_IsFunction(ctx, evt_ctor)) {
            JSValue type_val = JS_NewString(ctx, "message");
            evt = JS_CallConstructor(ctx, evt_ctor, 1, &type_val);
            JS_FreeValue(ctx, type_val);
            if (!JS_IsException(evt) && !JS_IsUndefined(evt)) {
                JS_SetPropertyStr(ctx, evt, "data", JS_DupValue(ctx, message));
            }
        }
        JS_FreeValue(ctx, evt_ctor);
    }

    if (!JS_IsException(evt) && !JS_IsUndefined(evt)) {
        JSValue onmsg = JS_GetPropertyStr(ctx, window_val, "onmessage");
        if (JS_IsFunction(ctx, onmsg)) {
            JSValue ret = JS_Call(ctx, onmsg, window_val, 1, &evt);
            JS_FreeValue(ctx, ret);
        }
        JS_FreeValue(ctx, onmsg);

        JSValue dispatch = JS_GetPropertyStr(ctx, window_val, "dispatchEvent");
        if (JS_IsFunction(ctx, dispatch)) {
            JSValue ret = JS_Call(ctx, dispatch, window_val, 1, &evt);
            JS_FreeValue(ctx, ret);
        }
        JS_FreeValue(ctx, dispatch);
    }

    JS_FreeValue(ctx, evt);
    JS_FreeValue(ctx, window_val);
    JS_FreeValue(ctx, global);
    return JS_UNDEFINED;
}

// Overrides: method | Window::captureEvents();
JSValue wisp_window_captureEvents_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::releaseEvents();
JSValue wisp_window_releaseEvents_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Window::getComputedStyle();
JSValue wisp_window_getComputedStyle_impl(JSContext *ctx, QJSNodePrivate *priv, void * elt, const char * pseudoElt) {
    fprintf(stderr, "DEBUG getComputedStyle elt=%p\n", elt);
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue get_computed_style_fn = JS_GetPropertyStr(ctx, global_obj, "__wisp_get_computed_style_internal");
    if (!JS_IsFunction(ctx, get_computed_style_fn)) {
        JS_FreeValue(ctx, get_computed_style_fn);
        get_computed_style_fn = JS_GetPropertyStr(ctx, global_obj, "getComputedStyle");
    }

    JSValue ret = JS_UNDEFINED;
    if (JS_IsFunction(ctx, get_computed_style_fn)) {
        JSValue el_val = qjs_wrap_node(ctx, (struct dom_node *)elt);
        JSValue pseudo_val = pseudoElt ? JS_NewString(ctx, pseudoElt) : JS_NULL;
        JSValue args[2] = { el_val, pseudo_val };
        ret = JS_Call(ctx, get_computed_style_fn, JS_UNDEFINED, 2, args);
        JS_FreeValue(ctx, el_val);
        JS_FreeValue(ctx, pseudo_val);
    }
    JS_FreeValue(ctx, get_computed_style_fn);
    JS_FreeValue(ctx, global_obj);
    return ret;
}

// Overrides: method | Window::createImageBitmap();
JSValue wisp_window_createImageBitmap_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image) {
    return JS_UNDEFINED;
}

JSValue wisp_window_createImageBitmap_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, int32_t sx, int32_t sy, int32_t sw, int32_t sh) {
    return JS_UNDEFINED;
}

// Overrides: getter | Window::locationbar(user);
JSValue wisp_window_locationbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Window::menubar(user);
JSValue wisp_window_menubar_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Window::personalbar(user);
JSValue wisp_window_personalbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Window::scrollbars(user);
JSValue wisp_window_scrollbars_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Window::statusbar(user);
JSValue wisp_window_statusbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Window::toolbar(user);
JSValue wisp_window_toolbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Window::status(string);
JSValue wisp_window_status_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Window::status(string);
JSValue wisp_window_status_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Window::closed(boolean);
JSValue wisp_window_closed_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Window::frames(user);
JSValue wisp_window_frames_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_GetGlobalObject(ctx);
}

// Overrides: getter | Window::length(unsigned long);
JSValue wisp_window_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: getter | Window::top(user);
JSValue wisp_window_top_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_GetGlobalObject(ctx);
}

// Overrides: getter | Window::opener(any);
JSValue wisp_window_opener_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Window::opener(any);
JSValue wisp_window_opener_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Window::parent(user);
JSValue wisp_window_parent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_GetGlobalObject(ctx);
}

// Overrides: getter | Window::frameElement(user);
JSValue wisp_window_frameElement_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: Window | external (getter)
JSValue wisp_window_external_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: method | Path2D::addPath();
JSValue wisp_path2d_addPath_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, JSValue transformation) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::addPathByStrokingPath();
JSValue wisp_path2d_addPathByStrokingPath_impl(JSContext *ctx, QJSNodePrivate *priv, void * path, void * styles, JSValue transformation) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::addText();
JSValue wisp_path2d_addText_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, void * styles, JSValue transformation, double x, double y, double maxWidth) {
    return JS_UNDEFINED;
}

JSValue wisp_path2d_addText_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, void * styles, JSValue transformation, void * path, double maxWidth) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::addPathByStrokingText();
JSValue wisp_path2d_addPathByStrokingText_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, void * styles, JSValue transformation, double x, double y, double maxWidth) {
    return JS_UNDEFINED;
}

JSValue wisp_path2d_addPathByStrokingText_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * text, void * styles, JSValue transformation, void * path, double maxWidth) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::closePath();
JSValue wisp_path2d_closePath_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::moveTo();
JSValue wisp_path2d_moveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::lineTo();
JSValue wisp_path2d_lineTo_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::quadraticCurveTo();
JSValue wisp_path2d_quadraticCurveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double cpx, double cpy, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::bezierCurveTo();
JSValue wisp_path2d_bezierCurveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::arcTo();
JSValue wisp_path2d_arcTo_0_impl(JSContext *ctx, QJSNodePrivate *priv, double x1, double y1, double x2, double y2, double radius) {
    return JS_UNDEFINED;
}

JSValue wisp_path2d_arcTo_1_impl(JSContext *ctx, QJSNodePrivate *priv, double x1, double y1, double x2, double y2, double radiusX, double radiusY, double rotation) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::rect();
JSValue wisp_path2d_rect_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double w, double h) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::arc();
JSValue wisp_path2d_arc_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double radius, double startAngle, double endAngle, bool anticlockwise) {
    return JS_UNDEFINED;
}

// Overrides: method | Path2D::ellipse();
JSValue wisp_path2d_ellipse_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool anticlockwise) {
    return JS_UNDEFINED;
}



// Overrides: method | CanvasProxy::setContext();
JSValue wisp_canvasproxy_setContext_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue context) {
    return JS_UNDEFINED;
}

// Overrides: method | HTMLTableHeaderCellElement::sort();
JSValue wisp_htmltableheadercellelement_sort_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: getter | HTMLTableHeaderCellElement::scope(string);
JSValue wisp_htmltableheadercellelement_scope_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | HTMLTableHeaderCellElement::scope(string);
JSValue wisp_htmltableheadercellelement_scope_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | HTMLTableHeaderCellElement::abbr(string);
JSValue wisp_htmltableheadercellelement_abbr_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | HTMLTableHeaderCellElement::abbr(string);
JSValue wisp_htmltableheadercellelement_abbr_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | HTMLTableHeaderCellElement::sorted(string);
JSValue wisp_htmltableheadercellelement_sorted_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | HTMLTableHeaderCellElement::sorted(string);
JSValue wisp_htmltableheadercellelement_sorted_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | HTMLTableDataCellElement::abbr(string);
JSValue wisp_htmltabledatacellelement_abbr_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | HTMLTableDataCellElement::abbr(string);
JSValue wisp_htmltabledatacellelement_abbr_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// VTTCue Implementation
JSValue wisp_vttcue_constructor_impl(JSContext *ctx, double startTime, double endTime, const char * text) {
    WispVTTCue *cue = calloc(1, sizeof(WispVTTCue));
    if (!cue) return JS_ThrowOutOfMemory(ctx);
    cue->startTime = startTime;
    cue->endTime = endTime;
    cue->text = strdup(text ? text : "");
    cue->id = strdup("");
    cue->pauseOnExit = false;
    cue->track = NULL;

    extern JSValue qjs_new_vttcue(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_vttcue(ctx, cue, false);
}

JSValue wisp_vttcue_text_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    return JS_NewString(ctx, (cue && cue->text) ? cue->text : "");
}

JSValue wisp_vttcue_text_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    if (cue) {
        free(cue->text);
        cue->text = strdup(value ? value : "");
    }
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackCue::track(user);
JSValue wisp_texttrackcue_track_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    if (cue && cue->track) {
        extern JSValue qjs_new_texttrack(JSContext *ctx, void *node, bool is_dom_node);
        return qjs_new_texttrack(ctx, cue->track, false);
    }
    return JS_NULL;
}

// Overrides: getter | TextTrackCue::id(string);
JSValue wisp_texttrackcue_id_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    return JS_NewString(ctx, (cue && cue->id) ? cue->id : "");
}

// Overrides: setter | TextTrackCue::id(string);
JSValue wisp_texttrackcue_id_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    if (cue) {
        free(cue->id);
        cue->id = strdup(value ? value : "");
    }
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackCue::startTime(double);
JSValue wisp_texttrackcue_startTime_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    return JS_NewFloat64(ctx, cue ? cue->startTime : 0.0);
}

// Overrides: setter | TextTrackCue::startTime(double);
JSValue wisp_texttrackcue_startTime_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    if (cue) cue->startTime = value;
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackCue::endTime(double);
JSValue wisp_texttrackcue_endTime_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    return JS_NewFloat64(ctx, cue ? cue->endTime : 0.0);
}

// Overrides: setter | TextTrackCue::endTime(double);
JSValue wisp_texttrackcue_endTime_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    if (cue) cue->endTime = value;
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackCue::pauseOnExit(boolean);
JSValue wisp_texttrackcue_pauseOnExit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    return JS_NewBool(ctx, cue ? cue->pauseOnExit : false);
}

// Overrides: setter | TextTrackCue::pauseOnExit(boolean);
JSValue wisp_texttrackcue_pauseOnExit_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    WispVTTCue *cue = priv ? (WispVTTCue *)priv->node : NULL;
    if (cue) cue->pauseOnExit = value;
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackCue::onenter(user);
JSValue wisp_texttrackcue_onenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onenter");
}

// Overrides: setter | TextTrackCue::onenter(user);
JSValue wisp_texttrackcue_onenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onenter", "enter", value);
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackCue::onexit(user);
JSValue wisp_texttrackcue_onexit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onexit");
}

// Overrides: setter | TextTrackCue::onexit(user);
JSValue wisp_texttrackcue_onexit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onexit", "exit", value);
    return JS_UNDEFINED;
}

// Overrides: method | TextTrackCueList::getCueById();
JSValue wisp_texttrackcuelist_getCueById_impl(JSContext *ctx, QJSNodePrivate *priv, const char * id) {
    WispTextTrackCueList *cl = priv ? (WispTextTrackCueList *)priv->node : NULL;
    if (cl && id) {
        for (uint32_t i = 0; i < cl->count; i++) {
            if (cl->cues[i]->id && strcmp(cl->cues[i]->id, id) == 0) {
                extern JSValue qjs_new_vttcue(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_vttcue(ctx, cl->cues[i], false);
            }
        }
    }
    return JS_NULL;
}

// Overrides: getter | TextTrackCueList::length(unsigned long);
JSValue wisp_texttrackcuelist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrackCueList *cl = priv ? (WispTextTrackCueList *)priv->node : NULL;
    return JS_NewInt32(ctx, cl ? cl->count : 0);
}

// Overrides: method | TextTrack::addCue();
JSValue wisp_texttrack_addCue_impl(JSContext *ctx, QJSNodePrivate *priv, void * cue_ptr) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    WispVTTCue *cue = (WispVTTCue *)cue_ptr;
    if (!track || !cue) return JS_UNDEFINED;

    if (!track->cues) {
        track->cues = calloc(1, sizeof(WispTextTrackCueList));
    }
    cue->track = track;

    WispTextTrackCueList *cl = track->cues;
    for (uint32_t i = 0; i < cl->count; i++) {
        if (cl->cues[i] == cue) return JS_UNDEFINED;
    }
    if (cl->count >= cl->capacity) {
        uint32_t new_cap = cl->capacity ? cl->capacity * 2 : 4;
        WispVTTCue **new_cues = realloc(cl->cues, new_cap * sizeof(WispVTTCue *));
        if (!new_cues) return JS_UNDEFINED;
        cl->cues = new_cues;
        cl->capacity = new_cap;
    }
    cl->cues[cl->count++] = cue;
    return JS_UNDEFINED;
}

// Overrides: method | TextTrack::removeCue();
JSValue wisp_texttrack_removeCue_impl(JSContext *ctx, QJSNodePrivate *priv, void * cue_ptr) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    WispVTTCue *cue = (WispVTTCue *)cue_ptr;
    if (!track || !cue || !track->cues) return JS_UNDEFINED;

    if (cue->track == track) cue->track = NULL;

    WispTextTrackCueList *cl = track->cues;
    for (uint32_t i = 0; i < cl->count; i++) {
        if (cl->cues[i] == cue) {
            memmove(&cl->cues[i], &cl->cues[i + 1], (cl->count - i - 1) * sizeof(WispVTTCue *));
            cl->count--;
            break;
        }
    }
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrack::kind(user);
JSValue wisp_texttrack_kind_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    return JS_NewString(ctx, (track && track->kind) ? track->kind : "subtitles");
}

// Overrides: getter | TextTrack::label(string);
JSValue wisp_texttrack_label_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    return JS_NewString(ctx, (track && track->label) ? track->label : "");
}

// Overrides: getter | TextTrack::language(string);
JSValue wisp_texttrack_language_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    return JS_NewString(ctx, (track && track->language) ? track->language : "");
}

// Overrides: getter | TextTrack::id(string);
JSValue wisp_texttrack_id_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    return JS_NewString(ctx, (track && track->id) ? track->id : "");
}

// Overrides: getter | TextTrack::inBandMetadataTrackDispatchType(string);
JSValue wisp_texttrack_inBandMetadataTrackDispatchType_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | TextTrack::mode(user);
JSValue wisp_texttrack_mode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    return JS_NewString(ctx, (track && track->mode) ? track->mode : "showing");
}

// Overrides: setter | TextTrack::mode(user);
JSValue wisp_texttrack_mode_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    if (track && JS_IsString(value)) {
        const char *s = JS_ToCString(ctx, value);
        if (s) {
            free(track->mode);
            track->mode = strdup(s);
            JS_FreeCString(ctx, s);
        }
    }
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrack::cues(user);
JSValue wisp_texttrack_cues_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrack *track = priv ? (WispTextTrack *)priv->node : NULL;
    if (!track) return JS_NULL;
    if (!track->cues) {
        track->cues = calloc(1, sizeof(WispTextTrackCueList));
    }
    extern JSValue qjs_new_texttrackcuelist(JSContext *ctx, void *node, bool is_dom_node);
    return qjs_new_texttrackcuelist(ctx, track->cues, false);
}

// Overrides: getter | TextTrack::activeCues(user);
JSValue wisp_texttrack_activeCues_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_texttrack_cues_get_impl(ctx, priv);
}

// Overrides: getter | TextTrack::oncuechange(user);
JSValue wisp_texttrack_oncuechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncuechange");
}

// Overrides: setter | TextTrack::oncuechange(user);
JSValue wisp_texttrack_oncuechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncuechange", "cuechange", value);
    return JS_UNDEFINED;
}

// Overrides: method | TextTrackList::getTrackById();
JSValue wisp_texttracklist_getTrackById_impl(JSContext *ctx, QJSNodePrivate *priv, const char * id) {
    WispTextTrackList *tl = priv ? (WispTextTrackList *)priv->node : NULL;
    if (tl && id) {
        for (uint32_t i = 0; i < tl->count; i++) {
            if (tl->tracks[i]->id && strcmp(tl->tracks[i]->id, id) == 0) {
                extern JSValue qjs_new_texttrack(JSContext *ctx, void *node, bool is_dom_node);
                return qjs_new_texttrack(ctx, tl->tracks[i], false);
            }
        }
    }
    return JS_NULL;
}

// Overrides: getter | TextTrackList::length(unsigned long);
JSValue wisp_texttracklist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispTextTrackList *tl = priv ? (WispTextTrackList *)priv->node : NULL;
    return JS_NewInt32(ctx, tl ? tl->count : 0);
}

// Overrides: getter | TextTrackList::onchange(user);
JSValue wisp_texttracklist_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchange");
}

// Overrides: setter | TextTrackList::onchange(user);
JSValue wisp_texttracklist_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchange", "change", value);
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackList::onaddtrack(user);
JSValue wisp_texttracklist_onaddtrack_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onaddtrack");
}

// Overrides: setter | TextTrackList::onaddtrack(user);
JSValue wisp_texttracklist_onaddtrack_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onaddtrack", "addtrack", value);
    return JS_UNDEFINED;
}

// Overrides: getter | TextTrackList::onremovetrack(user);
JSValue wisp_texttracklist_onremovetrack_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onremovetrack");
}

// Overrides: setter | TextTrackList::onremovetrack(user);
JSValue wisp_texttracklist_onremovetrack_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onremovetrack", "removetrack", value);
    return JS_UNDEFINED;
}

// Overrides: method | MediaController::pause();
JSValue wisp_mediacontroller_pause_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | MediaController::unpause();
JSValue wisp_mediacontroller_unpause_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | MediaController::play();
JSValue wisp_mediacontroller_play_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::readyState(unsigned short);
JSValue wisp_mediacontroller_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MediaController::buffered(user);
JSValue wisp_mediacontroller_buffered_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MediaController::seekable(user);
JSValue wisp_mediacontroller_seekable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MediaController::duration(double);
JSValue wisp_mediacontroller_duration_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MediaController::currentTime(double);
JSValue wisp_mediacontroller_currentTime_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | MediaController::currentTime(double);
JSValue wisp_mediacontroller_currentTime_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::paused(boolean);
JSValue wisp_mediacontroller_paused_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MediaController::playbackState(user);
JSValue wisp_mediacontroller_playbackState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MediaController::played(user);
JSValue wisp_mediacontroller_played_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | MediaController::defaultPlaybackRate(double);
JSValue wisp_mediacontroller_defaultPlaybackRate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | MediaController::defaultPlaybackRate(double);
JSValue wisp_mediacontroller_defaultPlaybackRate_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::playbackRate(double);
JSValue wisp_mediacontroller_playbackRate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | MediaController::playbackRate(double);
JSValue wisp_mediacontroller_playbackRate_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::volume(double);
JSValue wisp_mediacontroller_volume_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | MediaController::volume(double);
JSValue wisp_mediacontroller_volume_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::muted(boolean);
JSValue wisp_mediacontroller_muted_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | MediaController::muted(boolean);
JSValue wisp_mediacontroller_muted_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onemptied(user);
JSValue wisp_mediacontroller_onemptied_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onemptied");
}

// Overrides: setter | MediaController::onemptied(user);
JSValue wisp_mediacontroller_onemptied_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onemptied", "emptied", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onloadedmetadata(user);
JSValue wisp_mediacontroller_onloadedmetadata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadedmetadata");
}

// Overrides: setter | MediaController::onloadedmetadata(user);
JSValue wisp_mediacontroller_onloadedmetadata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadedmetadata", "loadedmetadata", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onloadeddata(user);
JSValue wisp_mediacontroller_onloadeddata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadeddata");
}

// Overrides: setter | MediaController::onloadeddata(user);
JSValue wisp_mediacontroller_onloadeddata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadeddata", "loadeddata", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::oncanplay(user);
JSValue wisp_mediacontroller_oncanplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplay");
}

// Overrides: setter | MediaController::oncanplay(user);
JSValue wisp_mediacontroller_oncanplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplay", "canplay", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::oncanplaythrough(user);
JSValue wisp_mediacontroller_oncanplaythrough_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplaythrough");
}

// Overrides: setter | MediaController::oncanplaythrough(user);
JSValue wisp_mediacontroller_oncanplaythrough_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplaythrough", "canplaythrough", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onplaying(user);
JSValue wisp_mediacontroller_onplaying_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplaying");
}

// Overrides: setter | MediaController::onplaying(user);
JSValue wisp_mediacontroller_onplaying_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplaying", "playing", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onended(user);
JSValue wisp_mediacontroller_onended_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onended");
}

// Overrides: setter | MediaController::onended(user);
JSValue wisp_mediacontroller_onended_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onended", "ended", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onwaiting(user);
JSValue wisp_mediacontroller_onwaiting_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwaiting");
}

// Overrides: setter | MediaController::onwaiting(user);
JSValue wisp_mediacontroller_onwaiting_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwaiting", "waiting", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::ondurationchange(user);
JSValue wisp_mediacontroller_ondurationchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondurationchange");
}

// Overrides: setter | MediaController::ondurationchange(user);
JSValue wisp_mediacontroller_ondurationchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondurationchange", "durationchange", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::ontimeupdate(user);
JSValue wisp_mediacontroller_ontimeupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontimeupdate");
}

// Overrides: setter | MediaController::ontimeupdate(user);
JSValue wisp_mediacontroller_ontimeupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontimeupdate", "timeupdate", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onplay(user);
JSValue wisp_mediacontroller_onplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplay");
}

// Overrides: setter | MediaController::onplay(user);
JSValue wisp_mediacontroller_onplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplay", "play", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onpause(user);
JSValue wisp_mediacontroller_onpause_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpause");
}

// Overrides: setter | MediaController::onpause(user);
JSValue wisp_mediacontroller_onpause_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpause", "pause", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onratechange(user);
JSValue wisp_mediacontroller_onratechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onratechange");
}

// Overrides: setter | MediaController::onratechange(user);
JSValue wisp_mediacontroller_onratechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onratechange", "ratechange", value);
    return JS_UNDEFINED;
}

// Overrides: getter | MediaController::onvolumechange(user);
JSValue wisp_mediacontroller_onvolumechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onvolumechange");
}

// Overrides: setter | MediaController::onvolumechange(user);
JSValue wisp_mediacontroller_onvolumechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onvolumechange", "volumechange", value);
    return JS_UNDEFINED;
}

// Overrides: getter | VideoTrack::id(string);
JSValue wisp_videotrack_id_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}



// Overrides: getter | MediaList::mediaText(string);
// Overrides: setter | MediaList::mediaText(string);
JSValue wisp_medialist_mediaText_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerLocation::href(user);
JSValue wisp_workerlocation_href_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | VideoTrack::kind(string);
JSValue wisp_videotrack_kind_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | VideoTrack::label(string);
JSValue wisp_videotrack_label_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | VideoTrack::language(string);
JSValue wisp_videotrack_language_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | VideoTrack::selected(boolean);
JSValue wisp_videotrack_selected_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | VideoTrack::selected(boolean);
JSValue wisp_videotrack_selected_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: method | VideoTrackList::getTrackById();
JSValue wisp_videotracklist_getTrackById_impl(JSContext *ctx, QJSNodePrivate *priv, const char * id) {
    return JS_UNDEFINED;
}

// Overrides: getter | VideoTrackList::length(unsigned long);
JSValue wisp_videotracklist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | VideoTrackList::selectedIndex(long);
JSValue wisp_videotracklist_selectedIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | VideoTrackList::onchange(user);
JSValue wisp_videotracklist_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchange");
}

// Overrides: setter | VideoTrackList::onchange(user);
JSValue wisp_videotracklist_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchange", "change", value);
    return JS_UNDEFINED;
}

// Overrides: getter | VideoTrackList::onaddtrack(user);
JSValue wisp_videotracklist_onaddtrack_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onaddtrack");
}

// Overrides: setter | VideoTrackList::onaddtrack(user);
JSValue wisp_videotracklist_onaddtrack_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onaddtrack", "addtrack", value);
    return JS_UNDEFINED;
}

// Overrides: getter | VideoTrackList::onremovetrack(user);
JSValue wisp_videotracklist_onremovetrack_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onremovetrack");
}

// Overrides: setter | VideoTrackList::onremovetrack(user);
JSValue wisp_videotracklist_onremovetrack_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onremovetrack", "removetrack", value);
    return JS_UNDEFINED;
}

// Overrides: getter | AudioTrack::id(string);
JSValue wisp_audiotrack_id_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | AudioTrack::kind(string);
JSValue wisp_audiotrack_kind_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | AudioTrack::label(string);
JSValue wisp_audiotrack_label_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | AudioTrack::language(string);
JSValue wisp_audiotrack_language_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | AudioTrack::enabled(boolean);
JSValue wisp_audiotrack_enabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | AudioTrack::enabled(boolean);
JSValue wisp_audiotrack_enabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: method | AudioTrackList::getTrackById();
JSValue wisp_audiotracklist_getTrackById_impl(JSContext *ctx, QJSNodePrivate *priv, const char * id) {
    return JS_UNDEFINED;
}

// Overrides: getter | AudioTrackList::length(unsigned long);
JSValue wisp_audiotracklist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | AudioTrackList::onchange(user);
JSValue wisp_audiotracklist_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchange");
}

// Overrides: setter | AudioTrackList::onchange(user);
JSValue wisp_audiotracklist_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchange", "change", value);
    return JS_UNDEFINED;
}

// Overrides: getter | AudioTrackList::onaddtrack(user);
JSValue wisp_audiotracklist_onaddtrack_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onaddtrack");
}

// Overrides: setter | AudioTrackList::onaddtrack(user);
JSValue wisp_audiotracklist_onaddtrack_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onaddtrack", "addtrack", value);
    return JS_UNDEFINED;
}

// Overrides: getter | AudioTrackList::onremovetrack(user);
JSValue wisp_audiotracklist_onremovetrack_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onremovetrack");
}

// Overrides: setter | AudioTrackList::onremovetrack(user);
JSValue wisp_audiotracklist_onremovetrack_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onremovetrack", "removetrack", value);
    return JS_UNDEFINED;
}

// Handled: callback interface method | NodeFilter::acceptNode();
// Overrides: method | Range::setStart();
JSValue wisp_range_setStart_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, uint32_t offset) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Range::setEnd();
JSValue wisp_range_setEnd_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, uint32_t offset) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Range::setStartBefore();
JSValue wisp_range_setStartBefore_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::setStartAfter();
JSValue wisp_range_setStartAfter_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::setEndBefore();
JSValue wisp_range_setEndBefore_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::setEndAfter();
JSValue wisp_range_setEndAfter_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::collapse();
JSValue wisp_range_collapse_impl(JSContext *ctx, QJSNodePrivate *priv, bool toStart) {
    return JS_FALSE;
}

// Overrides: method | Range::selectNode();
JSValue wisp_range_selectNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::selectNodeContents();
JSValue wisp_range_selectNodeContents_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::compareBoundaryPoints();
JSValue wisp_range_compareBoundaryPoints_impl(JSContext *ctx, QJSNodePrivate *priv, uint16_t how, void * sourceRange) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Range::deleteContents();
JSValue wisp_range_deleteContents_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::extractContents();
JSValue wisp_range_extractContents_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::cloneContents();
JSValue wisp_range_cloneContents_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::insertNode();
JSValue wisp_range_insertNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::surroundContents();
JSValue wisp_range_surroundContents_impl(JSContext *ctx, QJSNodePrivate *priv, void * newParent) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::cloneRange();
JSValue wisp_range_cloneRange_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::detach();
JSValue wisp_range_detach_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Range::isPointInRange();
JSValue wisp_range_isPointInRange_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, uint32_t offset) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Range::comparePoint();
JSValue wisp_range_comparePoint_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, uint32_t offset) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Range::intersectsNode();
JSValue wisp_range_intersectsNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Range::createContextualFragment();
JSValue wisp_range_createContextualFragment_impl(JSContext *ctx, QJSNodePrivate *priv, const char * fragment) {
    return JS_UNDEFINED;
}

// Overrides: getter | Range::startContainer(user);
JSValue wisp_range_startContainer_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Range::startOffset(unsigned long);
JSValue wisp_range_startOffset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Range::endContainer(user);
JSValue wisp_range_endContainer_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Range::endOffset(unsigned long);
JSValue wisp_range_endOffset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Range::collapsed(boolean);
JSValue wisp_range_collapsed_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Range::commonAncestorContainer(user);
JSValue wisp_range_commonAncestorContainer_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: method | CharacterData::substringData();
JSValue wisp_characterdata_substringData_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t offset, uint32_t count) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | CharacterData::appendData();
JSValue wisp_characterdata_appendData_impl(JSContext *ctx, QJSNodePrivate *priv, const char * data) {
    return JS_UNDEFINED;
}

// Overrides: method | CharacterData::insertData();
JSValue wisp_characterdata_insertData_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t offset, const char * data) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | CharacterData::deleteData();
JSValue wisp_characterdata_deleteData_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t offset, uint32_t count) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | CharacterData::replaceData();
JSValue wisp_characterdata_replaceData_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t offset, uint32_t count, const char * data) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | CharacterData::before();
JSValue wisp_characterdata_before_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | CharacterData::after();
JSValue wisp_characterdata_after_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | CharacterData::replaceWith();
JSValue wisp_characterdata_replaceWith_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: getter | CharacterData::previousElementSibling(user);
JSValue wisp_characterdata_previousElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | CharacterData::nextElementSibling(user);
JSValue wisp_characterdata_nextElementSibling_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: method | DOMImplementation::createDocumentType();
JSValue wisp_domimplementation_createDocumentType_impl(JSContext *ctx, QJSNodePrivate *priv, const char * qualifiedName, const char * publicId, const char * systemId) {
    if (wisp_is_js_process) {
        extern uint64_t allocate_virtual_shm_node(uint16_t type, const char *name, const char *value);
        uint64_t virtual_id = allocate_virtual_shm_node(10, qualifiedName ? qualifiedName : "html", NULL);
        if (virtual_id == 0) return JS_NULL;
        if (wisp_shm_dom) {
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            strings_arr[virtual_id].tag_name = wisp_shm_alloc_string(wisp_shm_dom, qualifiedName ? qualifiedName : "html");
            strings_arr[virtual_id].attrs[0].name = wisp_shm_alloc_string(wisp_shm_dom, "publicId");
            strings_arr[virtual_id].attrs[0].value = wisp_shm_alloc_string(wisp_shm_dom, publicId ? publicId : "");
            strings_arr[virtual_id].attrs[1].name = wisp_shm_alloc_string(wisp_shm_dom, "systemId");
            strings_arr[virtual_id].attrs[1].value = wisp_shm_alloc_string(wisp_shm_dom, systemId ? systemId : "");
            strings_arr[virtual_id].attr_count = 2;
        }
        return qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)virtual_id);
    }
    struct dom_document_type *dt = NULL;
    dom_exception err = dom_implementation_create_document_type(
        qualifiedName ? qualifiedName : "html",
        publicId,
        systemId,
        &dt);
    if (err == DOM_NO_ERR && dt) {
        JSValue val = qjs_wrap_node(ctx, (dom_node *)dt);
        dom_node_unref((dom_node *)dt);
        return val;
    }
    return JS_NULL;
}

// Overrides: method | DOMImplementation::createDocument();
JSValue wisp_domimplementation_createDocument_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * qualifiedName, void * doctype) {
    return JS_UNDEFINED;
}

// Overrides: method | DOMImplementation::hasFeature();
JSValue wisp_domimplementation_hasFeature_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

// Overrides: method | Document::getElementsByTagNameNS();
JSValue wisp_document_getElementsByTagNameNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * localName) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::createProcessingInstruction();
JSValue wisp_document_createProcessingInstruction_impl(JSContext *ctx, QJSNodePrivate *priv, const char * target, const char * data) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::importNode();
JSValue wisp_document_importNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * node, bool deep) {
    return JS_FALSE;
}

// Overrides: method | Document::adoptNode();
JSValue wisp_document_adoptNode_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::createAttribute();
JSValue wisp_document_createAttribute_impl(JSContext *ctx, QJSNodePrivate *priv, const char * localName) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::createAttributeNS();
JSValue wisp_document_createAttributeNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::createRange();
JSValue wisp_document_createRange_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::createNodeIterator();
JSValue wisp_document_createNodeIterator_impl(JSContext *ctx, QJSNodePrivate *priv, void * root, uint32_t whatToShow, JSValue filter) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Document::createTreeWalker();
JSValue wisp_document_createTreeWalker_impl(JSContext *ctx, QJSNodePrivate *priv, void * root, uint32_t whatToShow, JSValue filter) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: method | Document::open();
JSValue wisp_document_open_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, const char * replace) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::close();
JSValue wisp_document_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::execCommand();
JSValue wisp_document_execCommand_impl(JSContext *ctx, QJSNodePrivate *priv, const char * commandId, bool showUI, const char * value) {
    if (!commandId) return JS_FALSE;
    if (strcasecmp(commandId, "styleWithCSS") == 0 ||
        strcasecmp(commandId, "useCSS") == 0 ||
        strcasecmp(commandId, "insertHTML") == 0 ||
        strcasecmp(commandId, "insertText") == 0 ||
        strcasecmp(commandId, "bold") == 0 ||
        strcasecmp(commandId, "italic") == 0) {
        return JS_TRUE;
    }
    return JS_FALSE;
}

// Overrides: method | Document::queryCommandEnabled();
JSValue wisp_document_queryCommandEnabled_impl(JSContext *ctx, QJSNodePrivate *priv, const char * commandId) {
    if (!commandId) return JS_FALSE;
    return JS_TRUE;
}

// Overrides: method | Document::queryCommandIndeterm();
JSValue wisp_document_queryCommandIndeterm_impl(JSContext *ctx, QJSNodePrivate *priv, const char * commandId) {
    return JS_FALSE;
}

// Overrides: method | Document::queryCommandState();
JSValue wisp_document_queryCommandState_impl(JSContext *ctx, QJSNodePrivate *priv, const char * commandId) {
    return JS_FALSE;
}

// Overrides: method | Document::queryCommandSupported();
JSValue wisp_document_queryCommandSupported_impl(JSContext *ctx, QJSNodePrivate *priv, const char * commandId) {
    if (!commandId) return JS_FALSE;
    if (strcasecmp(commandId, "bold") == 0 ||
        strcasecmp(commandId, "italic") == 0 ||
        strcasecmp(commandId, "underline") == 0 ||
        strcasecmp(commandId, "copy") == 0 ||
        strcasecmp(commandId, "cut") == 0 ||
        strcasecmp(commandId, "paste") == 0 ||
        strcasecmp(commandId, "selectAll") == 0 ||
        strcasecmp(commandId, "undo") == 0 ||
        strcasecmp(commandId, "redo") == 0 ||
        strcasecmp(commandId, "insertHTML") == 0 ||
        strcasecmp(commandId, "insertText") == 0 ||
        strcasecmp(commandId, "styleWithCSS") == 0 ||
        strcasecmp(commandId, "useCSS") == 0) {
        return JS_TRUE;
    }
    return JS_FALSE;
}

// Overrides: method | Document::queryCommandValue();
JSValue wisp_document_queryCommandValue_impl(JSContext *ctx, QJSNodePrivate *priv, const char * commandId) {
    return JS_NewString(ctx, "");
}

// Overrides: method | Document::clear();
JSValue wisp_document_clear_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::captureEvents();
JSValue wisp_document_captureEvents_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::releaseEvents();
JSValue wisp_document_releaseEvents_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::enableStyleSheetsForSet();
JSValue wisp_document_enableStyleSheetsForSet_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::query();
JSValue wisp_document_query_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::queryAll();
JSValue wisp_document_queryAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: getter | Document::doctype(user);
JSValue wisp_document_doctype_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Document::dir(string);
JSValue wisp_document_dir_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Document::dir(string);
JSValue wisp_document_dir_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Document::cssElementMap(user);
JSValue wisp_document_cssElementMap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Document::commands(user);
JSValue wisp_document_commands_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Document::fgColor(string);
JSValue wisp_document_fgColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Document::fgColor(string);
JSValue wisp_document_fgColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Document::linkColor(string);
JSValue wisp_document_linkColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Document::linkColor(string);
JSValue wisp_document_linkColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Document::vlinkColor(string);
JSValue wisp_document_vlinkColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Document::vlinkColor(string);
JSValue wisp_document_vlinkColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Document::alinkColor(string);
JSValue wisp_document_alinkColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Document::alinkColor(string);
JSValue wisp_document_alinkColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Document::bgColor(string);
JSValue wisp_document_bgColor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Document::bgColor(string);
JSValue wisp_document_bgColor_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

extern JSValue qjs_new_htmlallcollection(JSContext *ctx, void *node, bool is_dom_node);
extern JSValue qjs_new_stylesheetlist(JSContext *ctx, void *node, bool is_dom_node);

// Overrides: getter | Document::all(user);
JSValue wisp_document_all_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return qjs_new_htmlallcollection(ctx, NULL, false);
}

// Overrides: getter | Document::styleSheets(user);
JSValue wisp_document_styleSheets_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return qjs_new_stylesheetlist(ctx, NULL, false);
}

// Overrides: getter | Document::selectedStyleSheetSet(string);
JSValue wisp_document_selectedStyleSheetSet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | Document::selectedStyleSheetSet(string);
JSValue wisp_document_selectedStyleSheetSet_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | Document::lastStyleSheetSet(string);
JSValue wisp_document_lastStyleSheetSet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Document::preferredStyleSheetSet(string);
JSValue wisp_document_preferredStyleSheetSet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Document::styleSheetSets(string);
JSValue wisp_document_styleSheetSets_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | Document::onerror(user);
JSValue wisp_document_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: setter | Document::onerror(user);
JSValue wisp_document_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: method | DocumentType::before();
JSValue wisp_documenttype_before_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | DocumentType::after();
JSValue wisp_documenttype_after_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | DocumentType::replaceWith();
JSValue wisp_documenttype_replaceWith_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: getter | DocumentType::name(string);
JSValue wisp_documenttype_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            WispNodeID id = (WispNodeID)(uintptr_t)priv->node;
            if (id < wisp_shm_dom->node_count) {
                const char *name = wisp_string_ref_data(wisp_shm_dom, strings_arr[id].tag_name);
                return JS_NewString(ctx, name ? name : "");
            }
        }
        return JS_NewString(ctx, "");
    }
    dom_string *name = NULL;
    dom_node_get_node_name((dom_node *)priv->node, &name);
    if (name) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(name), dom_string_byte_length(name));
        dom_string_unref(name);
        return val;
    }
    return JS_NewString(ctx, "");
}

// Overrides: getter | DocumentType::publicId(string);
JSValue wisp_documenttype_publicId_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            WispNodeID id = (WispNodeID)(uintptr_t)priv->node;
            if (id < wisp_shm_dom->node_count) {
                const char *pub = wisp_string_ref_data(wisp_shm_dom, strings_arr[id].attrs[0].value);
                return JS_NewString(ctx, pub ? pub : "");
            }
        }
        return JS_NewString(ctx, "");
    }
    dom_string *pub = NULL;
    dom_document_type_get_public_id((dom_document_type *)priv->node, &pub);
    if (pub) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(pub), dom_string_byte_length(pub));
        dom_string_unref(pub);
        return val;
    }
    return JS_NewString(ctx, "");
}

// Overrides: getter | DocumentType::systemId(string);
JSValue wisp_documenttype_systemId_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NewString(ctx, "");
    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            WispNodeID id = (WispNodeID)(uintptr_t)priv->node;
            if (id < wisp_shm_dom->node_count) {
                const char *sys = wisp_string_ref_data(wisp_shm_dom, strings_arr[id].attrs[1].value);
                return JS_NewString(ctx, sys ? sys : "");
            }
        }
        return JS_NewString(ctx, "");
    }
    dom_string *sys = NULL;
    dom_document_type_get_system_id((dom_document_type *)priv->node, &sys);
    if (sys) {
        JSValue val = JS_NewStringLen(ctx, (const char *)dom_string_data(sys), dom_string_byte_length(sys));
        dom_string_unref(sys);
        return val;
    }
    return JS_NewString(ctx, "");
}

// Overrides: method | DocumentFragment::getElementById();
JSValue wisp_documentfragment_getElementById_impl(JSContext *ctx, QJSNodePrivate *priv, const char * elementId) {
    return JS_NULL;
}

// Overrides: method | DocumentFragment::query();
JSValue wisp_documentfragment_query_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: method | DocumentFragment::queryAll();
JSValue wisp_documentfragment_queryAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: method | DocumentFragment::querySelector();
JSValue wisp_documentfragment_querySelector_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors) {
    return JS_NULL;
}

// Overrides: method | DocumentFragment::querySelectorAll();
JSValue wisp_documentfragment_querySelectorAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors) {
    return JS_UNDEFINED;
}

// Overrides: getter | DocumentFragment::children(user);
JSValue wisp_documentfragment_children_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | DocumentFragment::firstElementChild(user);
JSValue wisp_documentfragment_firstElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | DocumentFragment::lastElementChild(user);
JSValue wisp_documentfragment_lastElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | DocumentFragment::childElementCount(unsigned long);
JSValue wisp_documentfragment_childElementCount_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Handled: callback interface method | EventListener::handleEvent();// Overrides: attribute get | GlobalEventHandlers::onabort;
JSValue wisp_globaleventhandlers_onabort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onabort");
}

// Overrides: attribute set | GlobalEventHandlers::onabort;
JSValue wisp_globaleventhandlers_onabort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onabort", "abort", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onautocomplete;
JSValue wisp_globaleventhandlers_onautocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocomplete");
}

// Overrides: attribute set | GlobalEventHandlers::onautocomplete;
JSValue wisp_globaleventhandlers_onautocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocomplete", "autocomplete", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onautocompleteerror;
JSValue wisp_globaleventhandlers_onautocompleteerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocompleteerror");
}

// Overrides: attribute set | GlobalEventHandlers::onautocompleteerror;
JSValue wisp_globaleventhandlers_onautocompleteerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocompleteerror", "autocompleteerror", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onblur;
JSValue wisp_globaleventhandlers_onblur_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onblur");
}

// Overrides: attribute set | GlobalEventHandlers::onblur;
JSValue wisp_globaleventhandlers_onblur_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onblur", "blur", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::oncancel;
JSValue wisp_globaleventhandlers_oncancel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncancel");
}

// Overrides: attribute set | GlobalEventHandlers::oncancel;
JSValue wisp_globaleventhandlers_oncancel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncancel", "cancel", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::oncanplay;
JSValue wisp_globaleventhandlers_oncanplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplay");
}

// Overrides: attribute set | GlobalEventHandlers::oncanplay;
JSValue wisp_globaleventhandlers_oncanplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplay", "canplay", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::oncanplaythrough;
JSValue wisp_globaleventhandlers_oncanplaythrough_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplaythrough");
}

// Overrides: attribute set | GlobalEventHandlers::oncanplaythrough;
JSValue wisp_globaleventhandlers_oncanplaythrough_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplaythrough", "canplaythrough", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onchange;
JSValue wisp_globaleventhandlers_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchange");
}

// Overrides: attribute set | GlobalEventHandlers::onchange;
JSValue wisp_globaleventhandlers_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchange", "change", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onclick;
JSValue wisp_globaleventhandlers_onclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclick");
}

// Overrides: attribute set | GlobalEventHandlers::onclick;
JSValue wisp_globaleventhandlers_onclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclick", "click", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onclose;
JSValue wisp_globaleventhandlers_onclose_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclose");
}

// Overrides: attribute set | GlobalEventHandlers::onclose;
JSValue wisp_globaleventhandlers_onclose_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclose", "close", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::oncontextmenu;
JSValue wisp_globaleventhandlers_oncontextmenu_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncontextmenu");
}

// Overrides: attribute set | GlobalEventHandlers::oncontextmenu;
JSValue wisp_globaleventhandlers_oncontextmenu_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncontextmenu", "contextmenu", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::oncuechange;
JSValue wisp_globaleventhandlers_oncuechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncuechange");
}

// Overrides: attribute set | GlobalEventHandlers::oncuechange;
JSValue wisp_globaleventhandlers_oncuechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncuechange", "cuechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondblclick;
JSValue wisp_globaleventhandlers_ondblclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondblclick");
}

// Overrides: attribute set | GlobalEventHandlers::ondblclick;
JSValue wisp_globaleventhandlers_ondblclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondblclick", "dblclick", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondrag;
JSValue wisp_globaleventhandlers_ondrag_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrag");
}

// Overrides: attribute set | GlobalEventHandlers::ondrag;
JSValue wisp_globaleventhandlers_ondrag_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrag", "drag", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondragend;
JSValue wisp_globaleventhandlers_ondragend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragend");
}

// Overrides: attribute set | GlobalEventHandlers::ondragend;
JSValue wisp_globaleventhandlers_ondragend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragend", "dragend", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondragenter;
JSValue wisp_globaleventhandlers_ondragenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragenter");
}

// Overrides: attribute set | GlobalEventHandlers::ondragenter;
JSValue wisp_globaleventhandlers_ondragenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragenter", "dragenter", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondragexit;
JSValue wisp_globaleventhandlers_ondragexit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragexit");
}

// Overrides: attribute set | GlobalEventHandlers::ondragexit;
JSValue wisp_globaleventhandlers_ondragexit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragexit", "dragexit", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondragleave;
JSValue wisp_globaleventhandlers_ondragleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragleave");
}

// Overrides: attribute set | GlobalEventHandlers::ondragleave;
JSValue wisp_globaleventhandlers_ondragleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragleave", "dragleave", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondragover;
JSValue wisp_globaleventhandlers_ondragover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragover");
}

// Overrides: attribute set | GlobalEventHandlers::ondragover;
JSValue wisp_globaleventhandlers_ondragover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragover", "dragover", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondragstart;
JSValue wisp_globaleventhandlers_ondragstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragstart");
}

// Overrides: attribute set | GlobalEventHandlers::ondragstart;
JSValue wisp_globaleventhandlers_ondragstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragstart", "dragstart", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondrop;
JSValue wisp_globaleventhandlers_ondrop_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrop");
}

// Overrides: attribute set | GlobalEventHandlers::ondrop;
JSValue wisp_globaleventhandlers_ondrop_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrop", "drop", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ondurationchange;
JSValue wisp_globaleventhandlers_ondurationchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondurationchange");
}

// Overrides: attribute set | GlobalEventHandlers::ondurationchange;
JSValue wisp_globaleventhandlers_ondurationchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondurationchange", "durationchange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onemptied;
JSValue wisp_globaleventhandlers_onemptied_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onemptied");
}

// Overrides: attribute set | GlobalEventHandlers::onemptied;
JSValue wisp_globaleventhandlers_onemptied_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onemptied", "emptied", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onended;
JSValue wisp_globaleventhandlers_onended_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onended");
}

// Overrides: attribute set | GlobalEventHandlers::onended;
JSValue wisp_globaleventhandlers_onended_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onended", "ended", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onerror;
JSValue wisp_globaleventhandlers_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: attribute set | GlobalEventHandlers::onerror;
JSValue wisp_globaleventhandlers_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onfocus;
JSValue wisp_globaleventhandlers_onfocus_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onfocus");
}

// Overrides: attribute set | GlobalEventHandlers::onfocus;
JSValue wisp_globaleventhandlers_onfocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onfocus", "focus", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::oninput;
JSValue wisp_globaleventhandlers_oninput_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninput");
}

// Overrides: attribute set | GlobalEventHandlers::oninput;
JSValue wisp_globaleventhandlers_oninput_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninput", "input", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::oninvalid;
JSValue wisp_globaleventhandlers_oninvalid_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninvalid");
}

// Overrides: attribute set | GlobalEventHandlers::oninvalid;
JSValue wisp_globaleventhandlers_oninvalid_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninvalid", "invalid", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onkeydown;
JSValue wisp_globaleventhandlers_onkeydown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeydown");
}

// Overrides: attribute set | GlobalEventHandlers::onkeydown;
JSValue wisp_globaleventhandlers_onkeydown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeydown", "keydown", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onkeypress;
JSValue wisp_globaleventhandlers_onkeypress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeypress");
}

// Overrides: attribute set | GlobalEventHandlers::onkeypress;
JSValue wisp_globaleventhandlers_onkeypress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeypress", "keypress", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onkeyup;
JSValue wisp_globaleventhandlers_onkeyup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeyup");
}

// Overrides: attribute set | GlobalEventHandlers::onkeyup;
JSValue wisp_globaleventhandlers_onkeyup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeyup", "keyup", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onload;
JSValue wisp_globaleventhandlers_onload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onload");
}

// Overrides: attribute set | GlobalEventHandlers::onload;
JSValue wisp_globaleventhandlers_onload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onload", "load", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onloadeddata;
JSValue wisp_globaleventhandlers_onloadeddata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadeddata");
}

// Overrides: attribute set | GlobalEventHandlers::onloadeddata;
JSValue wisp_globaleventhandlers_onloadeddata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadeddata", "loadeddata", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onloadedmetadata;
JSValue wisp_globaleventhandlers_onloadedmetadata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadedmetadata");
}

// Overrides: attribute set | GlobalEventHandlers::onloadedmetadata;
JSValue wisp_globaleventhandlers_onloadedmetadata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadedmetadata", "loadedmetadata", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onloadstart;
JSValue wisp_globaleventhandlers_onloadstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadstart");
}

// Overrides: attribute set | GlobalEventHandlers::onloadstart;
JSValue wisp_globaleventhandlers_onloadstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadstart", "loadstart", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onmousedown;
JSValue wisp_globaleventhandlers_onmousedown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousedown");
}

// Overrides: attribute set | GlobalEventHandlers::onmousedown;
JSValue wisp_globaleventhandlers_onmousedown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousedown", "mousedown", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onmouseenter;
JSValue wisp_globaleventhandlers_onmouseenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseenter");
}

// Overrides: attribute set | GlobalEventHandlers::onmouseenter;
JSValue wisp_globaleventhandlers_onmouseenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseenter", "mouseenter", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onmouseleave;
JSValue wisp_globaleventhandlers_onmouseleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseleave");
}

// Overrides: attribute set | GlobalEventHandlers::onmouseleave;
JSValue wisp_globaleventhandlers_onmouseleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseleave", "mouseleave", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onmousemove;
JSValue wisp_globaleventhandlers_onmousemove_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousemove");
}

// Overrides: attribute set | GlobalEventHandlers::onmousemove;
JSValue wisp_globaleventhandlers_onmousemove_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousemove", "mousemove", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onmouseout;
JSValue wisp_globaleventhandlers_onmouseout_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseout");
}

// Overrides: attribute set | GlobalEventHandlers::onmouseout;
JSValue wisp_globaleventhandlers_onmouseout_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseout", "mouseout", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onmouseover;
JSValue wisp_globaleventhandlers_onmouseover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseover");
}

// Overrides: attribute set | GlobalEventHandlers::onmouseover;
JSValue wisp_globaleventhandlers_onmouseover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseover", "mouseover", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onmouseup;
JSValue wisp_globaleventhandlers_onmouseup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseup");
}

// Overrides: attribute set | GlobalEventHandlers::onmouseup;
JSValue wisp_globaleventhandlers_onmouseup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseup", "mouseup", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onpause;
JSValue wisp_globaleventhandlers_onpause_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpause");
}

// Overrides: attribute set | GlobalEventHandlers::onpause;
JSValue wisp_globaleventhandlers_onpause_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpause", "pause", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onplay;
JSValue wisp_globaleventhandlers_onplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplay");
}

// Overrides: attribute set | GlobalEventHandlers::onplay;
JSValue wisp_globaleventhandlers_onplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplay", "play", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onplaying;
JSValue wisp_globaleventhandlers_onplaying_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplaying");
}

// Overrides: attribute set | GlobalEventHandlers::onplaying;
JSValue wisp_globaleventhandlers_onplaying_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplaying", "playing", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onprogress;
JSValue wisp_globaleventhandlers_onprogress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onprogress");
}

// Overrides: attribute set | GlobalEventHandlers::onprogress;
JSValue wisp_globaleventhandlers_onprogress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onprogress", "progress", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onratechange;
JSValue wisp_globaleventhandlers_onratechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onratechange");
}

// Overrides: attribute set | GlobalEventHandlers::onratechange;
JSValue wisp_globaleventhandlers_onratechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onratechange", "ratechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onreset;
JSValue wisp_globaleventhandlers_onreset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onreset");
}

// Overrides: attribute set | GlobalEventHandlers::onreset;
JSValue wisp_globaleventhandlers_onreset_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onreset", "reset", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onresize;
JSValue wisp_globaleventhandlers_onresize_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onresize");
}

// Overrides: attribute set | GlobalEventHandlers::onresize;
JSValue wisp_globaleventhandlers_onresize_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onresize", "resize", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onscroll;
JSValue wisp_globaleventhandlers_onscroll_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onscroll");
}

// Overrides: attribute set | GlobalEventHandlers::onscroll;
JSValue wisp_globaleventhandlers_onscroll_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onscroll", "scroll", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onseeked;
JSValue wisp_globaleventhandlers_onseeked_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeked");
}

// Overrides: attribute set | GlobalEventHandlers::onseeked;
JSValue wisp_globaleventhandlers_onseeked_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeked", "seeked", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onseeking;
JSValue wisp_globaleventhandlers_onseeking_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeking");
}

// Overrides: attribute set | GlobalEventHandlers::onseeking;
JSValue wisp_globaleventhandlers_onseeking_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeking", "seeking", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onselect;
JSValue wisp_globaleventhandlers_onselect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onselect");
}

// Overrides: attribute set | GlobalEventHandlers::onselect;
JSValue wisp_globaleventhandlers_onselect_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onselect", "select", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onshow;
JSValue wisp_globaleventhandlers_onshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onshow");
}

// Overrides: attribute set | GlobalEventHandlers::onshow;
JSValue wisp_globaleventhandlers_onshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onshow", "show", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onsort;
JSValue wisp_globaleventhandlers_onsort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsort");
}

// Overrides: attribute set | GlobalEventHandlers::onsort;
JSValue wisp_globaleventhandlers_onsort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsort", "sort", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onstalled;
JSValue wisp_globaleventhandlers_onstalled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstalled");
}

// Overrides: attribute set | GlobalEventHandlers::onstalled;
JSValue wisp_globaleventhandlers_onstalled_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstalled", "stalled", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onsubmit;
JSValue wisp_globaleventhandlers_onsubmit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsubmit");
}

// Overrides: attribute set | GlobalEventHandlers::onsubmit;
JSValue wisp_globaleventhandlers_onsubmit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsubmit", "submit", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onsuspend;
JSValue wisp_globaleventhandlers_onsuspend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsuspend");
}

// Overrides: attribute set | GlobalEventHandlers::onsuspend;
JSValue wisp_globaleventhandlers_onsuspend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsuspend", "suspend", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ontimeupdate;
JSValue wisp_globaleventhandlers_ontimeupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontimeupdate");
}

// Overrides: attribute set | GlobalEventHandlers::ontimeupdate;
JSValue wisp_globaleventhandlers_ontimeupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontimeupdate", "timeupdate", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::ontoggle;
JSValue wisp_globaleventhandlers_ontoggle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontoggle");
}

// Overrides: attribute set | GlobalEventHandlers::ontoggle;
JSValue wisp_globaleventhandlers_ontoggle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontoggle", "toggle", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onvolumechange;
JSValue wisp_globaleventhandlers_onvolumechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onvolumechange");
}

// Overrides: attribute set | GlobalEventHandlers::onvolumechange;
JSValue wisp_globaleventhandlers_onvolumechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onvolumechange", "volumechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onwaiting;
JSValue wisp_globaleventhandlers_onwaiting_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwaiting");
}

// Overrides: attribute set | GlobalEventHandlers::onwaiting;
JSValue wisp_globaleventhandlers_onwaiting_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwaiting", "waiting", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | GlobalEventHandlers::onwheel;
JSValue wisp_globaleventhandlers_onwheel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwheel");
}

// Overrides: attribute set | GlobalEventHandlers::onwheel;
JSValue wisp_globaleventhandlers_onwheel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwheel", "wheel", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::aLink;
JSValue wisp_htmlbodyelement_aLink_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | HTMLBodyElement::aLink;
JSValue wisp_htmlbodyelement_aLink_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::link;
JSValue wisp_htmlbodyelement_link_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | HTMLBodyElement::link;
JSValue wisp_htmlbodyelement_link_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onafterprint;
JSValue wisp_htmlbodyelement_onafterprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onafterprint");
}

// Overrides: attribute set | HTMLBodyElement::onafterprint;
JSValue wisp_htmlbodyelement_onafterprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onafterprint", "afterprint", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onbeforeprint;
JSValue wisp_htmlbodyelement_onbeforeprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeprint");
}

// Overrides: attribute set | HTMLBodyElement::onbeforeprint;
JSValue wisp_htmlbodyelement_onbeforeprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeprint", "beforeprint", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onbeforeunload;
JSValue wisp_htmlbodyelement_onbeforeunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeunload");
}

// Overrides: attribute set | HTMLBodyElement::onbeforeunload;
JSValue wisp_htmlbodyelement_onbeforeunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeunload", "beforeunload", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onhashchange;
JSValue wisp_htmlbodyelement_onhashchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onhashchange");
}

// Overrides: attribute set | HTMLBodyElement::onhashchange;
JSValue wisp_htmlbodyelement_onhashchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onhashchange", "hashchange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onlanguagechange;
JSValue wisp_htmlbodyelement_onlanguagechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onlanguagechange");
}

// Overrides: attribute set | HTMLBodyElement::onlanguagechange;
JSValue wisp_htmlbodyelement_onlanguagechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onlanguagechange", "languagechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onmessage;
JSValue wisp_htmlbodyelement_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}

// Overrides: attribute set | HTMLBodyElement::onmessage;
JSValue wisp_htmlbodyelement_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onoffline;
JSValue wisp_htmlbodyelement_onoffline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onoffline");
}

// Overrides: attribute set | HTMLBodyElement::onoffline;
JSValue wisp_htmlbodyelement_onoffline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onoffline", "offline", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::ononline;
JSValue wisp_htmlbodyelement_ononline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ononline");
}

// Overrides: attribute set | HTMLBodyElement::ononline;
JSValue wisp_htmlbodyelement_ononline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ononline", "online", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onpagehide;
JSValue wisp_htmlbodyelement_onpagehide_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpagehide");
}

// Overrides: attribute set | HTMLBodyElement::onpagehide;
JSValue wisp_htmlbodyelement_onpagehide_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpagehide", "pagehide", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onpageshow;
JSValue wisp_htmlbodyelement_onpageshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpageshow");
}

// Overrides: attribute set | HTMLBodyElement::onpageshow;
JSValue wisp_htmlbodyelement_onpageshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpageshow", "pageshow", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onpopstate;
JSValue wisp_htmlbodyelement_onpopstate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpopstate");
}

// Overrides: attribute set | HTMLBodyElement::onpopstate;
JSValue wisp_htmlbodyelement_onpopstate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpopstate", "popstate", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onstorage;
JSValue wisp_htmlbodyelement_onstorage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstorage");
}

// Overrides: attribute set | HTMLBodyElement::onstorage;
JSValue wisp_htmlbodyelement_onstorage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstorage", "storage", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::onunload;
JSValue wisp_htmlbodyelement_onunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onunload");
}

// Overrides: attribute set | HTMLBodyElement::onunload;
JSValue wisp_htmlbodyelement_onunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onunload", "unload", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | HTMLBodyElement::vLink;
JSValue wisp_htmlbodyelement_vLink_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | HTMLBodyElement::vLink;
JSValue wisp_htmlbodyelement_vLink_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onafterprint;
JSValue wisp_windoweventhandlers_onafterprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onafterprint");
}

// Overrides: attribute set | WindowEventHandlers::onafterprint;
JSValue wisp_windoweventhandlers_onafterprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onafterprint", "afterprint", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onbeforeprint;
JSValue wisp_windoweventhandlers_onbeforeprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeprint");
}

// Overrides: attribute set | WindowEventHandlers::onbeforeprint;
JSValue wisp_windoweventhandlers_onbeforeprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeprint", "beforeprint", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onbeforeunload;
JSValue wisp_windoweventhandlers_onbeforeunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeunload");
}

// Overrides: attribute set | WindowEventHandlers::onbeforeunload;
JSValue wisp_windoweventhandlers_onbeforeunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeunload", "beforeunload", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onhashchange;
JSValue wisp_windoweventhandlers_onhashchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onhashchange");
}

// Overrides: attribute set | WindowEventHandlers::onhashchange;
JSValue wisp_windoweventhandlers_onhashchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onhashchange", "hashchange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onlanguagechange;
JSValue wisp_windoweventhandlers_onlanguagechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onlanguagechange");
}

// Overrides: attribute set | WindowEventHandlers::onlanguagechange;
JSValue wisp_windoweventhandlers_onlanguagechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onlanguagechange", "languagechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onmessage;
JSValue wisp_windoweventhandlers_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}

// Overrides: attribute set | WindowEventHandlers::onmessage;
JSValue wisp_windoweventhandlers_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onoffline;
JSValue wisp_windoweventhandlers_onoffline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onoffline");
}

// Overrides: attribute set | WindowEventHandlers::onoffline;
JSValue wisp_windoweventhandlers_onoffline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onoffline", "offline", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::ononline;
JSValue wisp_windoweventhandlers_ononline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ononline");
}

// Overrides: attribute set | WindowEventHandlers::ononline;
JSValue wisp_windoweventhandlers_ononline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ononline", "online", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onpagehide;
JSValue wisp_windoweventhandlers_onpagehide_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpagehide");
}

// Overrides: attribute set | WindowEventHandlers::onpagehide;
JSValue wisp_windoweventhandlers_onpagehide_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpagehide", "pagehide", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onpageshow;
JSValue wisp_windoweventhandlers_onpageshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpageshow");
}

// Overrides: attribute set | WindowEventHandlers::onpageshow;
JSValue wisp_windoweventhandlers_onpageshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpageshow", "pageshow", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onpopstate;
JSValue wisp_windoweventhandlers_onpopstate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpopstate");
}

// Overrides: attribute set | WindowEventHandlers::onpopstate;
JSValue wisp_windoweventhandlers_onpopstate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpopstate", "popstate", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onstorage;
JSValue wisp_windoweventhandlers_onstorage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstorage");
}

// Overrides: attribute set | WindowEventHandlers::onstorage;
JSValue wisp_windoweventhandlers_onstorage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstorage", "storage", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | WindowEventHandlers::onunload;
JSValue wisp_windoweventhandlers_onunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onunload");
}

// Overrides: attribute set | WindowEventHandlers::onunload;
JSValue wisp_windoweventhandlers_onunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onunload", "unload", value);
    return JS_UNDEFINED;
}

// ============================================================================
// WAVE 4: Manual WebIDL Overrides & Implementations (184 stubs)
// ============================================================================

// Overrides: attribute get | AbstractWorker::onerror (getter);
JSValue wisp_abstractworker_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: attribute set | AbstractWorker::onerror (setter);
JSValue wisp_abstractworker_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: method | AudioTrackList::__getter__();
JSValue wisp_audiotracklist___getter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::direction (getter);
JSValue wisp_canvasdrawingstyles_direction_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::direction (setter);
JSValue wisp_canvasdrawingstyles_direction_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::font (getter);
JSValue wisp_canvasdrawingstyles_font_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::font (setter);
JSValue wisp_canvasdrawingstyles_font_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasDrawingStyles::getLineDash();
JSValue wisp_canvasdrawingstyles_getLineDash_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::lineCap (getter);
JSValue wisp_canvasdrawingstyles_lineCap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::lineCap (setter);
JSValue wisp_canvasdrawingstyles_lineCap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::lineDashOffset (getter);
JSValue wisp_canvasdrawingstyles_lineDashOffset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::lineDashOffset (setter);
JSValue wisp_canvasdrawingstyles_lineDashOffset_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::lineJoin (getter);
JSValue wisp_canvasdrawingstyles_lineJoin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::lineJoin (setter);
JSValue wisp_canvasdrawingstyles_lineJoin_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::lineWidth (getter);
JSValue wisp_canvasdrawingstyles_lineWidth_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::lineWidth (setter);
JSValue wisp_canvasdrawingstyles_lineWidth_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::miterLimit (getter);
JSValue wisp_canvasdrawingstyles_miterLimit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::miterLimit (setter);
JSValue wisp_canvasdrawingstyles_miterLimit_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasDrawingStyles::setLineDash();
JSValue wisp_canvasdrawingstyles_setLineDash_impl(JSContext *ctx, QJSNodePrivate *priv, double segments) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::textAlign (getter);
JSValue wisp_canvasdrawingstyles_textAlign_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::textAlign (setter);
JSValue wisp_canvasdrawingstyles_textAlign_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasDrawingStyles::textBaseline (getter);
JSValue wisp_canvasdrawingstyles_textBaseline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasDrawingStyles::textBaseline (setter);
JSValue wisp_canvasdrawingstyles_textBaseline_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::arc();
JSValue wisp_canvaspathmethods_arc_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double radius, double startAngle, double endAngle, bool anticlockwise) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::arcTo();
JSValue wisp_canvaspathmethods_arcTo_0_impl(JSContext *ctx, QJSNodePrivate *priv, double x1, double y1, double x2, double y2, double radius) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::arcTo();
JSValue wisp_canvaspathmethods_arcTo_1_impl(JSContext *ctx, QJSNodePrivate *priv, double x1, double y1, double x2, double y2, double radiusX, double radiusY, double rotation) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::bezierCurveTo();
JSValue wisp_canvaspathmethods_bezierCurveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double cp1x, double cp1y, double cp2x, double cp2y, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::closePath();
JSValue wisp_canvaspathmethods_closePath_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::ellipse();
JSValue wisp_canvaspathmethods_ellipse_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double radiusX, double radiusY, double rotation, double startAngle, double endAngle, bool anticlockwise) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::lineTo();
JSValue wisp_canvaspathmethods_lineTo_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::moveTo();
JSValue wisp_canvaspathmethods_moveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::quadraticCurveTo();
JSValue wisp_canvaspathmethods_quadraticCurveTo_impl(JSContext *ctx, QJSNodePrivate *priv, double cpx, double cpy, double x, double y) {
    return JS_UNDEFINED;
}

// Overrides: method | CanvasPathMethods::rect();
JSValue wisp_canvaspathmethods_rect_impl(JSContext *ctx, QJSNodePrivate *priv, double x, double y, double w, double h) {
    return JS_UNDEFINED;
}

// Overrides: constructor | CanvasRenderingContext2D::constructor_0;
JSValue wisp_canvasrenderingcontext2d_constructor_0_impl(JSContext *ctx) {
    return JS_UNDEFINED;
}

// Overrides: constructor | CanvasRenderingContext2D::constructor_1;
JSValue wisp_canvasrenderingcontext2d_constructor_1_impl(JSContext *ctx, uint32_t width, uint32_t height) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasRenderingContext2D::height (getter);
JSValue wisp_canvasrenderingcontext2d_height_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasRenderingContext2D::height (setter);
JSValue wisp_canvasrenderingcontext2d_height_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    return JS_UNDEFINED;
}

// Overrides: attribute get | CanvasRenderingContext2D::width (getter);
JSValue wisp_canvasrenderingcontext2d_width_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: attribute set | CanvasRenderingContext2D::width (setter);
JSValue wisp_canvasrenderingcontext2d_width_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    return JS_UNDEFINED;
}

// Overrides: method | CharacterData::remove();
JSValue wisp_characterdata_remove_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | ChildNode::after();
JSValue wisp_childnode_after_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | ChildNode::before();
JSValue wisp_childnode_before_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | ChildNode::replaceWith();
JSValue wisp_childnode_replaceWith_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: constructor | Comment::constructor;
JSValue wisp_comment_constructor_impl(JSContext *ctx, const char * data) {
    return JS_UNDEFINED;
}

// Overrides: constructor | CompositionEvent::constructor;
JSValue wisp_compositionevent_constructor_impl(JSContext *ctx, const char * typeArg, JSValue compositionEventInitDict) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::__getter__();
JSValue wisp_document___getter___impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: method | Document::append();
JSValue wisp_document_append_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

extern JSValue wisp_document_createElement_impl(JSContext *ctx, QJSNodePrivate *priv, const char * localName);

// Overrides: method | Document::createElementNS();
JSValue wisp_document_createElementNS_impl(JSContext *ctx, QJSNodePrivate *priv, const char * namespace, const char * qualifiedName) {
    JSValue elem = wisp_document_createElement_impl(ctx, priv, qualifiedName);
    if (JS_IsObject(elem) && namespace) {
        if (strcmp(namespace, "http://www.w3.org/2000/svg") == 0) {
            JS_DefinePropertyValueStr(ctx, elem, "namespaceURI", JS_NewString(ctx, namespace), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JSValue global = JS_GetGlobalObject(ctx);
            JSValue ctor = JS_UNDEFINED;
            if (qualifiedName && strcasecmp(qualifiedName, "foreignObject") == 0) {
                ctor = JS_GetPropertyStr(ctx, global, "SVGForeignObjectElement");
            } else if (qualifiedName && strcasecmp(qualifiedName, "svg") == 0) {
                ctor = JS_GetPropertyStr(ctx, global, "SVGSVGElement");
            } else {
                ctor = JS_GetPropertyStr(ctx, global, "SVGElement");
            }
            if (JS_IsFunction(ctx, ctor)) {
                JSValue proto = JS_GetPropertyStr(ctx, ctor, "prototype");
                if (JS_IsObject(proto)) {
                    JS_SetPrototype(ctx, elem, proto);
                    JS_FreeValue(ctx, proto);
                } else {
                    JS_FreeValue(ctx, proto);
                }
            }
            JS_FreeValue(ctx, ctor);
            JS_FreeValue(ctx, global);
        } else if (strcmp(namespace, "http://www.w3.org/1998/Math/MathML") == 0) {
            JS_DefinePropertyValueStr(ctx, elem, "namespaceURI", JS_NewString(ctx, namespace), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
        }
    }
    return elem;
}

// Overrides: attribute get | Document::implementation (getter);
JSValue wisp_document_implementation_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    return qjs_new_domimplementation(ctx, priv->node, priv->is_dom_node);
}

// Overrides: attribute get | Document::location (getter);
JSValue wisp_document_location_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_window_location_get_impl(ctx, priv);
}

// Overrides: attribute get | Document::onabort (getter);
JSValue wisp_document_onabort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onabort");
}

// Overrides: attribute set | Document::onabort (setter);
JSValue wisp_document_onabort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onabort", "abort", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onautocomplete (getter);
JSValue wisp_document_onautocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocomplete");
}

// Overrides: attribute set | Document::onautocomplete (setter);
JSValue wisp_document_onautocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocomplete", "autocomplete", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onautocompleteerror (getter);
JSValue wisp_document_onautocompleteerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocompleteerror");
}

// Overrides: attribute set | Document::onautocompleteerror (setter);
JSValue wisp_document_onautocompleteerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocompleteerror", "autocompleteerror", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onblur (getter);
JSValue wisp_document_onblur_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onblur");
}

// Overrides: attribute set | Document::onblur (setter);
JSValue wisp_document_onblur_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onblur", "blur", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::oncancel (getter);
JSValue wisp_document_oncancel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncancel");
}

// Overrides: attribute set | Document::oncancel (setter);
JSValue wisp_document_oncancel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncancel", "cancel", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::oncanplay (getter);
JSValue wisp_document_oncanplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplay");
}

// Overrides: attribute set | Document::oncanplay (setter);
JSValue wisp_document_oncanplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplay", "canplay", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::oncanplaythrough (getter);
JSValue wisp_document_oncanplaythrough_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplaythrough");
}

// Overrides: attribute set | Document::oncanplaythrough (setter);
JSValue wisp_document_oncanplaythrough_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplaythrough", "canplaythrough", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onchange (getter);
JSValue wisp_document_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchange");
}

// Overrides: attribute set | Document::onchange (setter);
JSValue wisp_document_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchange", "change", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onclick (getter);
JSValue wisp_document_onclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclick");
}

// Overrides: attribute set | Document::onclick (setter);
JSValue wisp_document_onclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclick", "click", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onclose (getter);
JSValue wisp_document_onclose_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclose");
}

// Overrides: attribute set | Document::onclose (setter);
JSValue wisp_document_onclose_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclose", "close", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::oncontextmenu (getter);
JSValue wisp_document_oncontextmenu_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncontextmenu");
}

// Overrides: attribute set | Document::oncontextmenu (setter);
JSValue wisp_document_oncontextmenu_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncontextmenu", "contextmenu", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::oncuechange (getter);
JSValue wisp_document_oncuechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncuechange");
}

// Overrides: attribute set | Document::oncuechange (setter);
JSValue wisp_document_oncuechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncuechange", "cuechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondblclick (getter);
JSValue wisp_document_ondblclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondblclick");
}

// Overrides: attribute set | Document::ondblclick (setter);
JSValue wisp_document_ondblclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondblclick", "dblclick", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondrag (getter);
JSValue wisp_document_ondrag_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrag");
}

// Overrides: attribute set | Document::ondrag (setter);
JSValue wisp_document_ondrag_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrag", "drag", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondragend (getter);
JSValue wisp_document_ondragend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragend");
}

// Overrides: attribute set | Document::ondragend (setter);
JSValue wisp_document_ondragend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragend", "dragend", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondragenter (getter);
JSValue wisp_document_ondragenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragenter");
}

// Overrides: attribute set | Document::ondragenter (setter);
JSValue wisp_document_ondragenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragenter", "dragenter", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondragexit (getter);
JSValue wisp_document_ondragexit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragexit");
}

// Overrides: attribute set | Document::ondragexit (setter);
JSValue wisp_document_ondragexit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragexit", "dragexit", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondragleave (getter);
JSValue wisp_document_ondragleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragleave");
}

// Overrides: attribute set | Document::ondragleave (setter);
JSValue wisp_document_ondragleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragleave", "dragleave", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondragover (getter);
JSValue wisp_document_ondragover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragover");
}

// Overrides: attribute set | Document::ondragover (setter);
JSValue wisp_document_ondragover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragover", "dragover", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondragstart (getter);
JSValue wisp_document_ondragstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragstart");
}

// Overrides: attribute set | Document::ondragstart (setter);
JSValue wisp_document_ondragstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragstart", "dragstart", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondrop (getter);
JSValue wisp_document_ondrop_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrop");
}

// Overrides: attribute set | Document::ondrop (setter);
JSValue wisp_document_ondrop_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrop", "drop", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ondurationchange (getter);
JSValue wisp_document_ondurationchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondurationchange");
}

// Overrides: attribute set | Document::ondurationchange (setter);
JSValue wisp_document_ondurationchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondurationchange", "durationchange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onemptied (getter);
JSValue wisp_document_onemptied_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onemptied");
}

// Overrides: attribute set | Document::onemptied (setter);
JSValue wisp_document_onemptied_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onemptied", "emptied", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onended (getter);
JSValue wisp_document_onended_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onended");
}

// Overrides: attribute set | Document::onended (setter);
JSValue wisp_document_onended_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onended", "ended", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onfocus (getter);
JSValue wisp_document_onfocus_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onfocus");
}

// Overrides: attribute set | Document::onfocus (setter);
JSValue wisp_document_onfocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onfocus", "focus", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::oninput (getter);
JSValue wisp_document_oninput_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninput");
}

// Overrides: attribute set | Document::oninput (setter);
JSValue wisp_document_oninput_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninput", "input", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::oninvalid (getter);
JSValue wisp_document_oninvalid_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninvalid");
}

// Overrides: attribute set | Document::oninvalid (setter);
JSValue wisp_document_oninvalid_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninvalid", "invalid", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onkeydown (getter);
JSValue wisp_document_onkeydown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeydown");
}

// Overrides: attribute set | Document::onkeydown (setter);
JSValue wisp_document_onkeydown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeydown", "keydown", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onkeypress (getter);
JSValue wisp_document_onkeypress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeypress");
}

// Overrides: attribute set | Document::onkeypress (setter);
JSValue wisp_document_onkeypress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeypress", "keypress", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onkeyup (getter);
JSValue wisp_document_onkeyup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeyup");
}

// Overrides: attribute set | Document::onkeyup (setter);
JSValue wisp_document_onkeyup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeyup", "keyup", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onload (getter);
JSValue wisp_document_onload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onload");
}

// Overrides: attribute set | Document::onload (setter);
JSValue wisp_document_onload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onload", "load", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onloadeddata (getter);
JSValue wisp_document_onloadeddata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadeddata");
}

// Overrides: attribute set | Document::onloadeddata (setter);
JSValue wisp_document_onloadeddata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadeddata", "loadeddata", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onloadedmetadata (getter);
JSValue wisp_document_onloadedmetadata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadedmetadata");
}

// Overrides: attribute set | Document::onloadedmetadata (setter);
JSValue wisp_document_onloadedmetadata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadedmetadata", "loadedmetadata", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onloadstart (getter);
JSValue wisp_document_onloadstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadstart");
}

// Overrides: attribute set | Document::onloadstart (setter);
JSValue wisp_document_onloadstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadstart", "loadstart", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onmousedown (getter);
JSValue wisp_document_onmousedown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousedown");
}

// Overrides: attribute set | Document::onmousedown (setter);
JSValue wisp_document_onmousedown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousedown", "mousedown", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onmouseenter (getter);
JSValue wisp_document_onmouseenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseenter");
}

// Overrides: attribute set | Document::onmouseenter (setter);
JSValue wisp_document_onmouseenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseenter", "mouseenter", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onmouseleave (getter);
JSValue wisp_document_onmouseleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseleave");
}

// Overrides: attribute set | Document::onmouseleave (setter);
JSValue wisp_document_onmouseleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseleave", "mouseleave", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onmousemove (getter);
JSValue wisp_document_onmousemove_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousemove");
}

// Overrides: attribute set | Document::onmousemove (setter);
JSValue wisp_document_onmousemove_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousemove", "mousemove", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onmouseout (getter);
JSValue wisp_document_onmouseout_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseout");
}

// Overrides: attribute set | Document::onmouseout (setter);
JSValue wisp_document_onmouseout_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseout", "mouseout", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onmouseover (getter);
JSValue wisp_document_onmouseover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseover");
}

// Overrides: attribute set | Document::onmouseover (setter);
JSValue wisp_document_onmouseover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseover", "mouseover", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onmouseup (getter);
JSValue wisp_document_onmouseup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseup");
}

// Overrides: attribute set | Document::onmouseup (setter);
JSValue wisp_document_onmouseup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseup", "mouseup", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onpause (getter);
JSValue wisp_document_onpause_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpause");
}

// Overrides: attribute set | Document::onpause (setter);
JSValue wisp_document_onpause_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpause", "pause", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onplay (getter);
JSValue wisp_document_onplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplay");
}

// Overrides: attribute set | Document::onplay (setter);
JSValue wisp_document_onplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplay", "play", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onplaying (getter);
JSValue wisp_document_onplaying_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplaying");
}

// Overrides: attribute set | Document::onplaying (setter);
JSValue wisp_document_onplaying_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplaying", "playing", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onprogress (getter);
JSValue wisp_document_onprogress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onprogress");
}

// Overrides: attribute set | Document::onprogress (setter);
JSValue wisp_document_onprogress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onprogress", "progress", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onratechange (getter);
JSValue wisp_document_onratechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onratechange");
}

// Overrides: attribute set | Document::onratechange (setter);
JSValue wisp_document_onratechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onratechange", "ratechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onreadystatechange (getter);
JSValue wisp_document_onreadystatechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onreadystatechange");
}

// Overrides: attribute set | Document::onreadystatechange (setter);
JSValue wisp_document_onreadystatechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onreadystatechange", "readystatechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onreset (getter);
JSValue wisp_document_onreset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onreset");
}

// Overrides: attribute set | Document::onreset (setter);
JSValue wisp_document_onreset_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onreset", "reset", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onresize (getter);
JSValue wisp_document_onresize_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onresize");
}

// Overrides: attribute set | Document::onresize (setter);
JSValue wisp_document_onresize_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onresize", "resize", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onscroll (getter);
JSValue wisp_document_onscroll_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onscroll");
}

// Overrides: attribute set | Document::onscroll (setter);
JSValue wisp_document_onscroll_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onscroll", "scroll", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onseeked (getter);
JSValue wisp_document_onseeked_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeked");
}

// Overrides: attribute set | Document::onseeked (setter);
JSValue wisp_document_onseeked_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeked", "seeked", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onseeking (getter);
JSValue wisp_document_onseeking_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeking");
}

// Overrides: attribute set | Document::onseeking (setter);
JSValue wisp_document_onseeking_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeking", "seeking", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onselect (getter);
JSValue wisp_document_onselect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onselect");
}

// Overrides: attribute set | Document::onselect (setter);
JSValue wisp_document_onselect_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onselect", "select", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onshow (getter);
JSValue wisp_document_onshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onshow");
}

// Overrides: attribute set | Document::onshow (setter);
JSValue wisp_document_onshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onshow", "show", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onsort (getter);
JSValue wisp_document_onsort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsort");
}

// Overrides: attribute set | Document::onsort (setter);
JSValue wisp_document_onsort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsort", "sort", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onstalled (getter);
JSValue wisp_document_onstalled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstalled");
}

// Overrides: attribute set | Document::onstalled (setter);
JSValue wisp_document_onstalled_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstalled", "stalled", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onsubmit (getter);
JSValue wisp_document_onsubmit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsubmit");
}

// Overrides: attribute set | Document::onsubmit (setter);
JSValue wisp_document_onsubmit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsubmit", "submit", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onsuspend (getter);
JSValue wisp_document_onsuspend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsuspend");
}

// Overrides: attribute set | Document::onsuspend (setter);
JSValue wisp_document_onsuspend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsuspend", "suspend", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ontimeupdate (getter);
JSValue wisp_document_ontimeupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontimeupdate");
}

// Overrides: attribute set | Document::ontimeupdate (setter);
JSValue wisp_document_ontimeupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontimeupdate", "timeupdate", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::ontoggle (getter);
JSValue wisp_document_ontoggle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontoggle");
}

// Overrides: attribute set | Document::ontoggle (setter);
JSValue wisp_document_ontoggle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontoggle", "toggle", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onvolumechange (getter);
JSValue wisp_document_onvolumechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onvolumechange");
}

// Overrides: attribute set | Document::onvolumechange (setter);
JSValue wisp_document_onvolumechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onvolumechange", "volumechange", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onwaiting (getter);
JSValue wisp_document_onwaiting_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwaiting");
}

// Overrides: attribute set | Document::onwaiting (setter);
JSValue wisp_document_onwaiting_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwaiting", "waiting", value);
    return JS_UNDEFINED;
}

// Overrides: attribute get | Document::onwheel (getter);
JSValue wisp_document_onwheel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwheel");
}

// Overrides: attribute set | Document::onwheel (setter);
JSValue wisp_document_onwheel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwheel", "wheel", value);
    return JS_UNDEFINED;
}

// Overrides: method | Document::prepend();
JSValue wisp_document_prepend_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | DocumentFragment::append();
JSValue wisp_documentfragment_append_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: constructor | DocumentFragment::constructor;
JSValue wisp_documentfragment_constructor_impl(JSContext *ctx) {
    return JS_UNDEFINED;
}

// Overrides: method | DocumentFragment::prepend();
JSValue wisp_documentfragment_prepend_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: method | DocumentType::remove();
JSValue wisp_documenttype_remove_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: method | DOMElementMap::__deleter__();
JSValue wisp_domelementmap___deleter___impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: method | DOMElementMap::__getter__();
JSValue wisp_domelementmap___getter___impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: method | DOMElementMap::__setter__();
JSValue wisp_domelementmap___setter___impl(JSContext *ctx, QJSNodePrivate *priv, const char * name, void * value) {
    return JS_UNDEFINED;
}

// Overrides: method | DOMImplementation::createHTMLDocument();
JSValue wisp_domimplementation_createHTMLDocument_impl(JSContext *ctx, QJSNodePrivate *priv, const char * title) {
    dom_document *doc = NULL;
    dom_exception err = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML, NULL, NULL, NULL, NULL, NULL, &doc);
    if (err != DOM_NO_ERR || !doc) {
        return JS_ThrowInternalError(ctx, "DOMImplementation.createHTMLDocument: Failed to create HTML document");
    }

    // Build the standard html skeleton (html, head, body)
    dom_string *html_s = NULL;
    struct dom_element *html_el = NULL;
    dom_string_create_interned((const uint8_t *)"html", 4, &html_s);
    dom_document_create_element(doc, html_s, &html_el);
    dom_node_append_child((dom_node *)doc, (dom_node *)html_el, NULL);
    dom_string_unref(html_s);

    dom_string *head_s = NULL;
    struct dom_element *head_el = NULL;
    dom_string_create_interned((const uint8_t *)"head", 4, &head_s);
    dom_document_create_element(doc, head_s, &head_el);
    dom_node_append_child((dom_node *)html_el, (dom_node *)head_el, NULL);
    dom_string_unref(head_s);

    dom_string *body_s = NULL;
    struct dom_element *body_el = NULL;
    dom_string_create_interned((const uint8_t *)"body", 4, &body_s);
    dom_document_create_element(doc, body_s, &body_el);
    dom_node_append_child((dom_node *)html_el, (dom_node *)body_el, NULL);
    dom_string_unref(body_s);

    // Optionally create <title> element inside <head> if a title parameter was passed
    if (title && strlen(title) > 0) {
        dom_string *title_s = NULL;
        struct dom_element *title_el = NULL;
        dom_string_create_interned((const uint8_t *)"title", 5, &title_s);
        dom_document_create_element(doc, title_s, &title_el);
        dom_node_append_child((dom_node *)head_el, (dom_node *)title_el, NULL);
        dom_string_unref(title_s);

        dom_string *text_s = NULL;
        struct dom_text *text_node = NULL;
        dom_string_create((const uint8_t *)title, strlen(title), &text_s);
        dom_document_create_text_node(doc, text_s, &text_node);
        dom_node_append_child((dom_node *)title_el, (dom_node *)text_node, NULL);
        dom_node_unref((dom_node *)text_node);
        dom_string_unref(text_s);
        dom_node_unref((dom_node *)title_el);
    }

    // Clean up local element references
    dom_node_unref((dom_node *)head_el);
    dom_node_unref((dom_node *)body_el);
    dom_node_unref((dom_node *)html_el);

    // Link the new document to user data / html content representation if context opaque exists
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->win_priv && t->doc_priv && t->win_priv != t->doc_priv && corestring_dom___ns_key_html_content_data) {
        dom_node_set_user_data((dom_node *)doc, corestring_dom___ns_key_html_content_data, t->doc_priv, NULL, NULL);
    }

    // Wrap the document and return it to JS
    JSValue wrap = qjs_wrap_node(ctx, (dom_node *)doc);
    dom_node_unref((dom_node *)doc);
    return wrap;
}


// =============================================================================
// Automatic strong-symbol WebIDL stub overrides (184 stubs wave)
// =============================================================================

// Overrides: Document | open()
JSValue wisp_document_open_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url, const char * name, const char * features, bool replace) {
    return JS_UNDEFINED;
}

// Overrides: DOMSettableTokenList | value (getter)
JSValue wisp_domsettabletokenlist_value_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: DOMSettableTokenList | value (setter)
JSValue wisp_domsettabletokenlist_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: DOMStringMap | __deleter__()
JSValue wisp_domstringmap___deleter___impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: DOMStringMap | __getter__()
JSValue wisp_domstringmap___getter___impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: DOMStringMap | __setter__()
JSValue wisp_domstringmap___setter___impl(JSContext *ctx, QJSNodePrivate *priv, const char * name, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: DrawingStyle | constructor
JSValue wisp_drawingstyle_constructor_impl(JSContext *ctx, void * scope) {
    return JS_UNDEFINED;
}

// Overrides: Element | append()
JSValue wisp_element_append_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: Element | outerHTML (setter)

// Overrides: Element | prepend()
JSValue wisp_element_prepend_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue nodes) {
    return JS_UNDEFINED;
}

// Overrides: Element | remove()
JSValue wisp_element_remove_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: ElementContentEditable | contentEditable (getter)
JSValue wisp_elementcontenteditable_contentEditable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: ElementContentEditable | contentEditable (setter)
JSValue wisp_elementcontenteditable_contentEditable_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: ElementContentEditable | isContentEditable (getter)
JSValue wisp_elementcontenteditable_isContentEditable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: Event | constructor
JSValue wisp_event_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_UNDEFINED;
}

// Overrides: EventSource | constructor
extern JSValue qjs_new_eventsource(JSContext *ctx, void *node, bool is_dom_node);

JSValue wisp_eventsource_constructor_impl(JSContext *ctx, const char * url, JSValue eventSourceInitDict) {
    WispEventSourcePrivate *esp = calloc(1, sizeof(WispEventSourcePrivate));
    if (esp) {
        if (url) esp->url = strdup(url);
        esp->readyState = 0; // CONNECTING
        if (JS_IsObject(eventSourceInitDict)) {
            JSValue wc = JS_GetPropertyStr(ctx, eventSourceInitDict, "withCredentials");
            if (JS_ToBool(ctx, wc)) {
                esp->withCredentials = true;
            }
            JS_FreeValue(ctx, wc);
        }
    }
    return qjs_new_eventsource(ctx, esp, false);
}

// Overrides: GetStyleUtils | cascadedStyle (getter)
JSValue wisp_getstyleutils_cascadedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: GetStyleUtils | defaultStyle (getter)
JSValue wisp_getstyleutils_defaultStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: GetStyleUtils | rawComputedStyle (getter)
JSValue wisp_getstyleutils_rawComputedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: GetStyleUtils | usedStyle (getter)
JSValue wisp_getstyleutils_usedStyle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | charset (getter)
JSValue wisp_htmlanchorelement_charset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | charset (setter)
JSValue wisp_htmlanchorelement_charset_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAnchorElement | coords (getter)
JSValue wisp_htmlanchorelement_coords_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | coords (setter)
JSValue wisp_htmlanchorelement_coords_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAnchorElement | hreflang (getter)
JSValue wisp_htmlanchorelement_hreflang_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | hreflang (setter)
JSValue wisp_htmlanchorelement_hreflang_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAnchorElement | name (getter)
JSValue wisp_htmlanchorelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | name (setter)
JSValue wisp_htmlanchorelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAnchorElement | rel (getter)
JSValue wisp_htmlanchorelement_rel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | rel (setter)
JSValue wisp_htmlanchorelement_rel_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAnchorElement | rev (getter)
JSValue wisp_htmlanchorelement_rev_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | rev (setter)
JSValue wisp_htmlanchorelement_rev_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAnchorElement | shape (getter)
JSValue wisp_htmlanchorelement_shape_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | shape (setter)
JSValue wisp_htmlanchorelement_shape_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAnchorElement | target (getter)
JSValue wisp_htmlanchorelement_target_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLAnchorElement | target (setter)
JSValue wisp_htmlanchorelement_target_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLAudioElement | Audio
JSValue wisp_htmlaudioelement_Audio_impl(JSContext *ctx, const char * src) {
    extern bool wisp_is_js_process;
    if (wisp_is_js_process) {
        extern JSValue qjs_new_htmlaudioelement(JSContext *ctx, void *node, bool is_dom_node);
        static uint32_t next_dummy_audio_id = 0xf2000000;
        uint32_t dummy_id = next_dummy_audio_id++;
        JSValue val = qjs_new_htmlaudioelement(ctx, (void*)(uintptr_t)dummy_id, false);
        QJSNodePrivate *priv = JS_GetOpaque(val, qjs_htmlaudioelement_class_id);
        if (priv && src && *src) {
            wisp_element_setAttribute_impl(ctx, priv, "src", src);
            wisp_element_setAttribute_impl(ctx, priv, "preload", "auto");
        }
        return val;
    }

    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return JS_NULL;
    struct dom_document *doc = qjs_thread_get_document(t);
    if (!doc) return JS_NULL;

    dom_string *name_dom = NULL;
    dom_string_create((const uint8_t *)"audio", 5, &name_dom);
    struct dom_element *result = NULL;
    dom_document_create_element(doc, name_dom, &result);
    dom_string_unref(name_dom);

    if (result) {
        if (src && *src) {
            dom_string *attr_name = NULL;
            dom_string_create((const uint8_t *)"src", 3, &attr_name);
            dom_string *attr_val = NULL;
            dom_string_create((const uint8_t *)src, strlen(src), &attr_val);
            dom_element_set_attribute(result, attr_name, attr_val);
            dom_string_unref(attr_name);
            dom_string_unref(attr_val);

            dom_string *pl_name = NULL;
            dom_string_create((const uint8_t *)"preload", 7, &pl_name);
            dom_string *pl_val = NULL;
            dom_string_create((const uint8_t *)"auto", 4, &pl_val);
            dom_element_set_attribute(result, pl_name, pl_val);
            dom_string_unref(pl_name);
            dom_string_unref(pl_val);
        }
        JSValue val = qjs_wrap_node(ctx, (dom_node *)result);
        dom_node_unref((dom_node *)result);
        return val;
    }
    return JS_NULL;
}

// Overrides: HTMLButtonElement | labels (getter)
JSValue wisp_htmlbuttonelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLElement | onabort (getter)
JSValue wisp_htmlelement_onabort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onabort");
}

// Overrides: HTMLElement | onabort (setter)
JSValue wisp_htmlelement_onabort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onabort", "abort", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onautocomplete (getter)
JSValue wisp_htmlelement_onautocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocomplete");
}

// Overrides: HTMLElement | onautocomplete (setter)
JSValue wisp_htmlelement_onautocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocomplete", "autocomplete", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onautocompleteerror (getter)
JSValue wisp_htmlelement_onautocompleteerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocompleteerror");
}

// Overrides: HTMLElement | onautocompleteerror (setter)
JSValue wisp_htmlelement_onautocompleteerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocompleteerror", "autocompleteerror", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onblur (getter)
JSValue wisp_htmlelement_onblur_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onblur");
}

// Overrides: HTMLElement | onblur (setter)
JSValue wisp_htmlelement_onblur_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onblur", "blur", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | oncancel (getter)
JSValue wisp_htmlelement_oncancel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncancel");
}

// Overrides: HTMLElement | oncancel (setter)
JSValue wisp_htmlelement_oncancel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncancel", "cancel", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | oncanplay (getter)
JSValue wisp_htmlelement_oncanplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplay");
}

// Overrides: HTMLElement | oncanplay (setter)
JSValue wisp_htmlelement_oncanplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplay", "canplay", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | oncanplaythrough (getter)
JSValue wisp_htmlelement_oncanplaythrough_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplaythrough");
}

// Overrides: HTMLElement | oncanplaythrough (setter)
JSValue wisp_htmlelement_oncanplaythrough_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplaythrough", "canplaythrough", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onchange (getter)
JSValue wisp_htmlelement_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchange");
}

// Overrides: HTMLElement | onchange (setter)
JSValue wisp_htmlelement_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchange", "change", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onclick (getter)
JSValue wisp_htmlelement_onclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclick");
}

// Overrides: HTMLElement | onclick (setter)
JSValue wisp_htmlelement_onclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclick", "click", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onclose (getter)
JSValue wisp_htmlelement_onclose_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclose");
}

// Overrides: HTMLElement | onclose (setter)
JSValue wisp_htmlelement_onclose_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclose", "close", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | oncontextmenu (getter)
JSValue wisp_htmlelement_oncontextmenu_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncontextmenu");
}

// Overrides: HTMLElement | oncontextmenu (setter)
JSValue wisp_htmlelement_oncontextmenu_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncontextmenu", "contextmenu", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | oncuechange (getter)
JSValue wisp_htmlelement_oncuechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncuechange");
}

// Overrides: HTMLElement | oncuechange (setter)
JSValue wisp_htmlelement_oncuechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncuechange", "cuechange", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondblclick (getter)
JSValue wisp_htmlelement_ondblclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondblclick");
}

// Overrides: HTMLElement | ondblclick (setter)
JSValue wisp_htmlelement_ondblclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondblclick", "dblclick", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondrag (getter)
JSValue wisp_htmlelement_ondrag_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrag");
}

// Overrides: HTMLElement | ondrag (setter)
JSValue wisp_htmlelement_ondrag_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrag", "drag", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondragend (getter)
JSValue wisp_htmlelement_ondragend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragend");
}

// Overrides: HTMLElement | ondragend (setter)
JSValue wisp_htmlelement_ondragend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragend", "dragend", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondragenter (getter)
JSValue wisp_htmlelement_ondragenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragenter");
}

// Overrides: HTMLElement | ondragenter (setter)
JSValue wisp_htmlelement_ondragenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragenter", "dragenter", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondragexit (getter)
JSValue wisp_htmlelement_ondragexit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragexit");
}

// Overrides: HTMLElement | ondragexit (setter)
JSValue wisp_htmlelement_ondragexit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragexit", "dragexit", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondragleave (getter)
JSValue wisp_htmlelement_ondragleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragleave");
}

// Overrides: HTMLElement | ondragleave (setter)
JSValue wisp_htmlelement_ondragleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragleave", "dragleave", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondragover (getter)
JSValue wisp_htmlelement_ondragover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragover");
}

// Overrides: HTMLElement | ondragover (setter)
JSValue wisp_htmlelement_ondragover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragover", "dragover", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondragstart (getter)
JSValue wisp_htmlelement_ondragstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragstart");
}

// Overrides: HTMLElement | ondragstart (setter)
JSValue wisp_htmlelement_ondragstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragstart", "dragstart", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondrop (getter)
JSValue wisp_htmlelement_ondrop_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrop");
}

// Overrides: HTMLElement | ondrop (setter)
JSValue wisp_htmlelement_ondrop_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrop", "drop", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ondurationchange (getter)
JSValue wisp_htmlelement_ondurationchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondurationchange");
}

// Overrides: HTMLElement | ondurationchange (setter)
JSValue wisp_htmlelement_ondurationchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondurationchange", "durationchange", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onemptied (getter)
JSValue wisp_htmlelement_onemptied_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onemptied");
}

// Overrides: HTMLElement | onemptied (setter)
JSValue wisp_htmlelement_onemptied_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onemptied", "emptied", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onended (getter)
JSValue wisp_htmlelement_onended_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onended");
}

// Overrides: HTMLElement | onended (setter)
JSValue wisp_htmlelement_onended_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onended", "ended", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onfocus (getter)
JSValue wisp_htmlelement_onfocus_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onfocus");
}

// Overrides: HTMLElement | onfocus (setter)
JSValue wisp_htmlelement_onfocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onfocus", "focus", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | oninput (getter)
JSValue wisp_htmlelement_oninput_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninput");
}

// Overrides: HTMLElement | oninput (setter)
JSValue wisp_htmlelement_oninput_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninput", "input", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | oninvalid (getter)
JSValue wisp_htmlelement_oninvalid_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninvalid");
}

// Overrides: HTMLElement | oninvalid (setter)
JSValue wisp_htmlelement_oninvalid_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninvalid", "invalid", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onkeydown (getter)
JSValue wisp_htmlelement_onkeydown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeydown");
}

// Overrides: HTMLElement | onkeydown (setter)
JSValue wisp_htmlelement_onkeydown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeydown", "keydown", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onkeypress (getter)
JSValue wisp_htmlelement_onkeypress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeypress");
}

// Overrides: HTMLElement | onkeypress (setter)
JSValue wisp_htmlelement_onkeypress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeypress", "keypress", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onkeyup (getter)
JSValue wisp_htmlelement_onkeyup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeyup");
}

// Overrides: HTMLElement | onkeyup (setter)
JSValue wisp_htmlelement_onkeyup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeyup", "keyup", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onloadeddata (getter)
JSValue wisp_htmlelement_onloadeddata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadeddata");
}

// Overrides: HTMLElement | onloadeddata (setter)
JSValue wisp_htmlelement_onloadeddata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadeddata", "loadeddata", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onloadedmetadata (getter)
JSValue wisp_htmlelement_onloadedmetadata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadedmetadata");
}

// Overrides: HTMLElement | onloadedmetadata (setter)
JSValue wisp_htmlelement_onloadedmetadata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadedmetadata", "loadedmetadata", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onloadstart (getter)
JSValue wisp_htmlelement_onloadstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadstart");
}

// Overrides: HTMLElement | onloadstart (setter)
JSValue wisp_htmlelement_onloadstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadstart", "loadstart", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onmousedown (getter)
JSValue wisp_htmlelement_onmousedown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousedown");
}

// Overrides: HTMLElement | onmousedown (setter)
JSValue wisp_htmlelement_onmousedown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousedown", "mousedown", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onmouseenter (getter)
JSValue wisp_htmlelement_onmouseenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseenter");
}

// Overrides: HTMLElement | onmouseenter (setter)
JSValue wisp_htmlelement_onmouseenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseenter", "mouseenter", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onmouseleave (getter)
JSValue wisp_htmlelement_onmouseleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseleave");
}

// Overrides: HTMLElement | onmouseleave (setter)
JSValue wisp_htmlelement_onmouseleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseleave", "mouseleave", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onmousemove (getter)
JSValue wisp_htmlelement_onmousemove_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousemove");
}

// Overrides: HTMLElement | onmousemove (setter)
JSValue wisp_htmlelement_onmousemove_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousemove", "mousemove", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onmouseout (getter)
JSValue wisp_htmlelement_onmouseout_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseout");
}

// Overrides: HTMLElement | onmouseout (setter)
JSValue wisp_htmlelement_onmouseout_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseout", "mouseout", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onmouseover (getter)
JSValue wisp_htmlelement_onmouseover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseover");
}

// Overrides: HTMLElement | onmouseover (setter)
JSValue wisp_htmlelement_onmouseover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseover", "mouseover", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onmouseup (getter)
JSValue wisp_htmlelement_onmouseup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseup");
}

// Overrides: HTMLElement | onmouseup (setter)
JSValue wisp_htmlelement_onmouseup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseup", "mouseup", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onpause (getter)
JSValue wisp_htmlelement_onpause_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpause");
}

// Overrides: HTMLElement | onpause (setter)
JSValue wisp_htmlelement_onpause_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpause", "pause", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onplay (getter)
JSValue wisp_htmlelement_onplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplay");
}

// Overrides: HTMLElement | onplay (setter)
JSValue wisp_htmlelement_onplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplay", "play", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onplaying (getter)
JSValue wisp_htmlelement_onplaying_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplaying");
}

// Overrides: HTMLElement | onplaying (setter)
JSValue wisp_htmlelement_onplaying_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplaying", "playing", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onprogress (getter)
JSValue wisp_htmlelement_onprogress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onprogress");
}

// Overrides: HTMLElement | onprogress (setter)
JSValue wisp_htmlelement_onprogress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onprogress", "progress", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onratechange (getter)
JSValue wisp_htmlelement_onratechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onratechange");
}

// Overrides: HTMLElement | onratechange (setter)
JSValue wisp_htmlelement_onratechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onratechange", "ratechange", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onreset (getter)
JSValue wisp_htmlelement_onreset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onreset");
}

// Overrides: HTMLElement | onreset (setter)
JSValue wisp_htmlelement_onreset_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onreset", "reset", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onresize (getter)
JSValue wisp_htmlelement_onresize_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onresize");
}

// Overrides: HTMLElement | onresize (setter)
JSValue wisp_htmlelement_onresize_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onresize", "resize", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onscroll (getter)
JSValue wisp_htmlelement_onscroll_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onscroll");
}

// Overrides: HTMLElement | onscroll (setter)
JSValue wisp_htmlelement_onscroll_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onscroll", "scroll", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onseeked (getter)
JSValue wisp_htmlelement_onseeked_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeked");
}

// Overrides: HTMLElement | onseeked (setter)
JSValue wisp_htmlelement_onseeked_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeked", "seeked", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onseeking (getter)
JSValue wisp_htmlelement_onseeking_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeking");
}

// Overrides: HTMLElement | onseeking (setter)
JSValue wisp_htmlelement_onseeking_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeking", "seeking", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onselect (getter)
JSValue wisp_htmlelement_onselect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onselect");
}

// Overrides: HTMLElement | onselect (setter)
JSValue wisp_htmlelement_onselect_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onselect", "select", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onshow (getter)
JSValue wisp_htmlelement_onshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onshow");
}

// Overrides: HTMLElement | onshow (setter)
JSValue wisp_htmlelement_onshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onshow", "show", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onsort (getter)
JSValue wisp_htmlelement_onsort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsort");
}

// Overrides: HTMLElement | onsort (setter)
JSValue wisp_htmlelement_onsort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsort", "sort", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onstalled (getter)
JSValue wisp_htmlelement_onstalled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstalled");
}

// Overrides: HTMLElement | onstalled (setter)
JSValue wisp_htmlelement_onstalled_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstalled", "stalled", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onsubmit (getter)
JSValue wisp_htmlelement_onsubmit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsubmit");
}

// Overrides: HTMLElement | onsubmit (setter)
JSValue wisp_htmlelement_onsubmit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsubmit", "submit", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onsuspend (getter)
JSValue wisp_htmlelement_onsuspend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsuspend");
}

// Overrides: HTMLElement | onsuspend (setter)
JSValue wisp_htmlelement_onsuspend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsuspend", "suspend", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ontimeupdate (getter)
JSValue wisp_htmlelement_ontimeupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontimeupdate");
}

// Overrides: HTMLElement | ontimeupdate (setter)
JSValue wisp_htmlelement_ontimeupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontimeupdate", "timeupdate", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | ontoggle (getter)
JSValue wisp_htmlelement_ontoggle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontoggle");
}

// Overrides: HTMLElement | ontoggle (setter)
JSValue wisp_htmlelement_ontoggle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontoggle", "toggle", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onvolumechange (getter)
JSValue wisp_htmlelement_onvolumechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onvolumechange");
}

// Overrides: HTMLElement | onvolumechange (setter)
JSValue wisp_htmlelement_onvolumechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onvolumechange", "volumechange", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onwaiting (getter)
JSValue wisp_htmlelement_onwaiting_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwaiting");
}

// Overrides: HTMLElement | onwaiting (setter)
JSValue wisp_htmlelement_onwaiting_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwaiting", "waiting", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLElement | onwheel (getter)
JSValue wisp_htmlelement_onwheel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwheel");
}

// Overrides: HTMLElement | onwheel (setter)
JSValue wisp_htmlelement_onwheel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwheel", "wheel", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFormElement | __getter__()
JSValue wisp_htmlformelement___getter___0_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

// Overrides: HTMLFormElement | __getter__()
JSValue wisp_htmlformelement___getter___1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: HTMLFormElement | acceptCharset (getter)
JSValue wisp_htmlformelement_acceptCharset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLFormElement | acceptCharset (setter)
JSValue wisp_htmlformelement_acceptCharset_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLFormElement | enctype (getter)
JSValue wisp_htmlformelement_enctype_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLFormElement | enctype (setter)
JSValue wisp_htmlformelement_enctype_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onafterprint (getter)
JSValue wisp_htmlframesetelement_onafterprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onafterprint");
}

// Overrides: HTMLFrameSetElement | onafterprint (setter)
JSValue wisp_htmlframesetelement_onafterprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onafterprint", "afterprint", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onbeforeprint (getter)
JSValue wisp_htmlframesetelement_onbeforeprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeprint");
}

// Overrides: HTMLFrameSetElement | onbeforeprint (setter)
JSValue wisp_htmlframesetelement_onbeforeprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeprint", "beforeprint", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onbeforeunload (getter)
JSValue wisp_htmlframesetelement_onbeforeunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeunload");
}

// Overrides: HTMLFrameSetElement | onbeforeunload (setter)
JSValue wisp_htmlframesetelement_onbeforeunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeunload", "beforeunload", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onhashchange (getter)
JSValue wisp_htmlframesetelement_onhashchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onhashchange");
}

// Overrides: HTMLFrameSetElement | onhashchange (setter)
JSValue wisp_htmlframesetelement_onhashchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onhashchange", "hashchange", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onlanguagechange (getter)
JSValue wisp_htmlframesetelement_onlanguagechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onlanguagechange");
}

// Overrides: HTMLFrameSetElement | onlanguagechange (setter)
JSValue wisp_htmlframesetelement_onlanguagechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onlanguagechange", "languagechange", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onmessage (getter)
JSValue wisp_htmlframesetelement_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}

// Overrides: HTMLFrameSetElement | onmessage (setter)
JSValue wisp_htmlframesetelement_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onoffline (getter)
JSValue wisp_htmlframesetelement_onoffline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onoffline");
}

// Overrides: HTMLFrameSetElement | onoffline (setter)
JSValue wisp_htmlframesetelement_onoffline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onoffline", "offline", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | ononline (getter)
JSValue wisp_htmlframesetelement_ononline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ononline");
}

// Overrides: HTMLFrameSetElement | ononline (setter)
JSValue wisp_htmlframesetelement_ononline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ononline", "online", value);
    return JS_UNDEFINED;
}


// -----------------------------------------------------------------------------
// Alphabetical Sixth Wave - 184 Stubs (2027)
// -----------------------------------------------------------------------------

// Overrides: HTMLFrameSetElement | onpagehide (getter)
JSValue wisp_htmlframesetelement_onpagehide_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpagehide");
}

// Overrides: HTMLFrameSetElement | onpagehide (setter)
JSValue wisp_htmlframesetelement_onpagehide_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpagehide", "pagehide", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onpageshow (getter)
JSValue wisp_htmlframesetelement_onpageshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpageshow");
}

// Overrides: HTMLFrameSetElement | onpageshow (setter)
JSValue wisp_htmlframesetelement_onpageshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpageshow", "pageshow", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onpopstate (getter)
JSValue wisp_htmlframesetelement_onpopstate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpopstate");
}

// Overrides: HTMLFrameSetElement | onpopstate (setter)
JSValue wisp_htmlframesetelement_onpopstate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpopstate", "popstate", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onstorage (getter)
JSValue wisp_htmlframesetelement_onstorage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstorage");
}

// Overrides: HTMLFrameSetElement | onstorage (setter)
JSValue wisp_htmlframesetelement_onstorage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstorage", "storage", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLFrameSetElement | onunload (getter)
JSValue wisp_htmlframesetelement_onunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onunload");
}

// Overrides: HTMLFrameSetElement | onunload (setter)
JSValue wisp_htmlframesetelement_onunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onunload", "unload", value);
    return JS_UNDEFINED;
}

// Overrides: HTMLIFrameElement | frameBorder (getter)
JSValue wisp_htmliframeelement_frameBorder_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLIFrameElement | frameBorder (setter)
JSValue wisp_htmliframeelement_frameBorder_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLIFrameElement | longDesc (getter)
JSValue wisp_htmliframeelement_longDesc_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLIFrameElement | longDesc (setter)
JSValue wisp_htmliframeelement_longDesc_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLIFrameElement | marginHeight (getter)
JSValue wisp_htmliframeelement_marginHeight_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLIFrameElement | marginHeight (setter)
JSValue wisp_htmliframeelement_marginHeight_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLIFrameElement | marginWidth (getter)
JSValue wisp_htmliframeelement_marginWidth_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLIFrameElement | marginWidth (setter)
JSValue wisp_htmliframeelement_marginWidth_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLIFrameElement | scrolling (getter)
JSValue wisp_htmliframeelement_scrolling_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLIFrameElement | scrolling (setter)
JSValue wisp_htmliframeelement_scrolling_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | align (getter)
JSValue wisp_htmlimageelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | align (setter)
JSValue wisp_htmlimageelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | border (getter)
JSValue wisp_htmlimageelement_border_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | border (setter)
JSValue wisp_htmlimageelement_border_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | hspace (getter)
JSValue wisp_htmlimageelement_hspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | hspace (setter)
JSValue wisp_htmlimageelement_hspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | isMap (getter)
JSValue wisp_htmlimageelement_isMap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | isMap (setter)
JSValue wisp_htmlimageelement_isMap_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | longDesc (getter)
JSValue wisp_htmlimageelement_longDesc_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | longDesc (setter)
JSValue wisp_htmlimageelement_longDesc_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | name (getter)
JSValue wisp_htmlimageelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | name (setter)
JSValue wisp_htmlimageelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | useMap (getter)
JSValue wisp_htmlimageelement_useMap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | useMap (setter)
JSValue wisp_htmlimageelement_useMap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLImageElement | vspace (getter)
JSValue wisp_htmlimageelement_vspace_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLImageElement | vspace (setter)
JSValue wisp_htmlimageelement_vspace_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | accept (getter)
JSValue wisp_htmlinputelement_accept_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | accept (setter)
JSValue wisp_htmlinputelement_accept_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | align (getter)
JSValue wisp_htmlinputelement_align_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | align (setter)
JSValue wisp_htmlinputelement_align_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | alt (getter)
JSValue wisp_htmlinputelement_alt_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | alt (setter)
JSValue wisp_htmlinputelement_alt_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | defaultChecked (getter)
JSValue wisp_htmlinputelement_defaultChecked_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | defaultChecked (setter)
JSValue wisp_htmlinputelement_defaultChecked_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | defaultValue (getter)
JSValue wisp_htmlinputelement_defaultValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | defaultValue (setter)
JSValue wisp_htmlinputelement_defaultValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | labels (getter)
JSValue wisp_htmlinputelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_labels_impl(ctx, priv);
}

// Overrides: HTMLInputElement | setRangeText()
JSValue wisp_htmlinputelement_setRangeText_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * replacement) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | setRangeText()
JSValue wisp_htmlinputelement_setRangeText_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * replacement, uint32_t start, uint32_t end, JSValue selectionMode) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | size (getter)
JSValue wisp_htmlinputelement_size_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | size (setter)
JSValue wisp_htmlinputelement_size_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | src (getter)
JSValue wisp_htmlinputelement_src_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | src (setter)
JSValue wisp_htmlinputelement_src_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLInputElement | useMap (getter)
JSValue wisp_htmlinputelement_useMap_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLInputElement | useMap (setter)
JSValue wisp_htmlinputelement_useMap_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLKeygenElement | autofocus (getter)
JSValue wisp_htmlkeygenelement_autofocus_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | autofocus (setter)
JSValue wisp_htmlkeygenelement_autofocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLKeygenElement | challenge (getter)
JSValue wisp_htmlkeygenelement_challenge_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | challenge (setter)
JSValue wisp_htmlkeygenelement_challenge_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLKeygenElement | checkValidity()
JSValue wisp_htmlkeygenelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlkeygenelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

// Overrides: HTMLKeygenElement | disabled (getter)
JSValue wisp_htmlkeygenelement_disabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | disabled (setter)
JSValue wisp_htmlkeygenelement_disabled_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLKeygenElement | form (getter)
JSValue wisp_htmlkeygenelement_form_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | keytype (getter)
JSValue wisp_htmlkeygenelement_keytype_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | keytype (setter)
JSValue wisp_htmlkeygenelement_keytype_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLKeygenElement | labels (getter)
JSValue wisp_htmlkeygenelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | name (getter)
JSValue wisp_htmlkeygenelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | name (setter)
JSValue wisp_htmlkeygenelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLKeygenElement | reportValidity()
JSValue wisp_htmlkeygenelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue validity = wisp_htmlkeygenelement_validity_get_impl(ctx, priv);
    if (!JS_IsUndefined(validity) && !JS_IsNull(validity)) {
        JSValue valid = JS_GetPropertyStr(ctx, validity, "valid");
        int is_valid = JS_ToBool(ctx, valid);
        JS_FreeValue(ctx, valid);
        JS_FreeValue(ctx, validity);
        if (is_valid == 0) {
            struct jsthread *thread = JS_GetContextOpaque(ctx);
            if (thread) {
                js_fire_event(thread, "invalid", qjs_thread_get_document(thread), (struct dom_node *)priv->node);
            }
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

// Overrides: HTMLKeygenElement | setCustomValidity()
JSValue wisp_htmlkeygenelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    set_element_str_attr(ctx, priv, "__customValidity", error ? error : "");
    return JS_UNDEFINED;
}

// Overrides: HTMLKeygenElement | type (getter)
JSValue wisp_htmlkeygenelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | validationMessage (getter)
JSValue wisp_htmlkeygenelement_validationMessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLKeygenElement | validity (getter)
JSValue wisp_htmlkeygenelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue qjs_new_validitystate(JSContext *ctx, void *node, bool is_dom_node);
    if (!priv) return JS_NULL;
    return qjs_new_validitystate(ctx, priv->node, priv->is_dom_node);
}

// Overrides: HTMLKeygenElement | willValidate (getter)
JSValue wisp_htmlkeygenelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLLinkElement | charset (getter)
JSValue wisp_htmllinkelement_charset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLLinkElement | charset (setter)
JSValue wisp_htmllinkelement_charset_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLLinkElement | hreflang (getter)
JSValue wisp_htmllinkelement_hreflang_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLLinkElement | hreflang (setter)
JSValue wisp_htmllinkelement_hreflang_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLLinkElement | rev (getter)
JSValue wisp_htmllinkelement_rev_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLLinkElement | rev (setter)
JSValue wisp_htmllinkelement_rev_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLLinkElement | target (getter)
JSValue wisp_htmllinkelement_target_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLLinkElement | target (setter)
JSValue wisp_htmllinkelement_target_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLObjectElement | __legacycaller__()
JSValue wisp_htmlobjectelement___legacycaller___impl(JSContext *ctx, QJSNodePrivate *priv, JSValue arguments) {
    return JS_UNDEFINED;
}

// Overrides: HTMLParamElement | name (getter)
JSValue wisp_htmlparamelement_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLParamElement | name (setter)
JSValue wisp_htmlparamelement_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLParamElement | type (getter)
JSValue wisp_htmlparamelement_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLParamElement | type (setter)
JSValue wisp_htmlparamelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLParamElement | value (getter)
JSValue wisp_htmlparamelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLParamElement | value (setter)
JSValue wisp_htmlparamelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLParamElement | valueType (getter)
JSValue wisp_htmlparamelement_valueType_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLParamElement | valueType (setter)
JSValue wisp_htmlparamelement_valueType_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLTextAreaElement | defaultValue (getter)
JSValue wisp_htmltextareaelement_defaultValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: HTMLTextAreaElement | defaultValue (setter)
JSValue wisp_htmltextareaelement_defaultValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: HTMLTextAreaElement | setRangeText()
JSValue wisp_htmltextareaelement_setRangeText_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * replacement) {
    return JS_UNDEFINED;
}

// Overrides: HTMLTextAreaElement | setRangeText()
JSValue wisp_htmltextareaelement_setRangeText_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * replacement, uint32_t start, uint32_t end, JSValue selectionMode) {
    return JS_UNDEFINED;
}

// Overrides: ImageBitmapFactories | createImageBitmap()
JSValue wisp_imagebitmapfactories_createImageBitmap_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image) {
    return JS_UNDEFINED;
}

// Overrides: ImageBitmapFactories | createImageBitmap()
JSValue wisp_imagebitmapfactories_createImageBitmap_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, int32_t sx, int32_t sy, int32_t sw, int32_t sh) {
    return JS_UNDEFINED;
}

// Overrides: LinkStyle | sheet (getter)
JSValue wisp_linkstyle_sheet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: Location | hash (setter)
JSValue wisp_location_hash_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Location | host (setter)
JSValue wisp_location_host_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Location | hostname (setter)
JSValue wisp_location_hostname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Location | href()
JSValue wisp_location_href_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: Location | password (getter)
JSValue wisp_location_password_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: Location | pathname (setter)
JSValue wisp_location_pathname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Location | port (setter)
JSValue wisp_location_port_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Location | protocol (setter)
JSValue wisp_location_protocol_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Location | search (setter)
JSValue wisp_location_search_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Location | username (getter)
JSValue wisp_location_username_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: MediaController | constructor
JSValue wisp_mediacontroller_constructor_impl(JSContext *ctx) {
    return JS_UNDEFINED;
}

// Overrides: MutationObserver | constructor
JSValue wisp_mutationobserver_constructor_impl(JSContext *ctx, JSValue callback) {
    return JS_UNDEFINED;
}

// Overrides: Navigator | javaEnabled (getter)
JSValue wisp_navigator_javaEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// Overrides: Navigator | languages (getter)
JSValue wisp_navigator_languages_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue arr = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, arr, 0, JS_NewString(ctx, "en-US"));
    return arr;
}

// Overrides: Navigator | productSub (getter)
JSValue wisp_navigator_productSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "20030107");
}

// Overrides: Navigator | taintEnabled()
JSValue wisp_navigator_taintEnabled_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// Overrides: Navigator | vendor (getter)
JSValue wisp_navigator_vendor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "Google Inc.");
}

// Overrides: Navigator | vendorSub (getter)
JSValue wisp_navigator_vendorSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: NavigatorContentUtils | isContentHandlerRegistered()
JSValue wisp_navigatorcontentutils_isContentHandlerRegistered_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: NavigatorContentUtils | isProtocolHandlerRegistered()
JSValue wisp_navigatorcontentutils_isProtocolHandlerRegistered_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: NavigatorContentUtils | registerContentHandler()
JSValue wisp_navigatorcontentutils_registerContentHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url, const char * title) {
    return JS_UNDEFINED;
}

// Overrides: NavigatorContentUtils | registerProtocolHandler()
JSValue wisp_navigatorcontentutils_registerProtocolHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url, const char * title) {
    return JS_UNDEFINED;
}

// Overrides: NavigatorContentUtils | unregisterContentHandler()
JSValue wisp_navigatorcontentutils_unregisterContentHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mimeType, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: NavigatorContentUtils | unregisterProtocolHandler()
JSValue wisp_navigatorcontentutils_unregisterProtocolHandler_impl(JSContext *ctx, QJSNodePrivate *priv, const char * scheme, const char * url) {
    return JS_UNDEFINED;
}

// Overrides: NavigatorID | appCodeName (getter)
JSValue wisp_navigatorid_appCodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_appCodeName_get_impl(ctx, priv);
}

// Overrides: NavigatorID | appName (getter)
JSValue wisp_navigatorid_appName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_appName_get_impl(ctx, priv);
}

// Overrides: NavigatorID | appVersion (getter)
JSValue wisp_navigatorid_appVersion_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_appVersion_get_impl(ctx, priv);
}

// Overrides: NavigatorID | platform (getter)
JSValue wisp_navigatorid_platform_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_platform_get_impl(ctx, priv);
}

// Overrides: NavigatorID | product (getter)
JSValue wisp_navigatorid_product_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_product_get_impl(ctx, priv);
}

// Overrides: NavigatorID | productSub (getter)
JSValue wisp_navigatorid_productSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "20030107");
}

// Overrides: NavigatorID | taintEnabled()
JSValue wisp_navigatorid_taintEnabled_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// Overrides: NavigatorID | userAgent (getter)
JSValue wisp_navigatorid_userAgent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_userAgent_get_impl(ctx, priv);
}

// Overrides: NavigatorID | vendor (getter)
JSValue wisp_navigatorid_vendor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "Google Inc.");
}

// Overrides: NavigatorID | vendorSub (getter)
JSValue wisp_navigatorid_vendorSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: NavigatorLanguage | language (getter)
JSValue wisp_navigatorlanguage_language_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_language_get_impl(ctx, priv);
}

// Overrides: NavigatorLanguage | languages (getter)
JSValue wisp_navigatorlanguage_languages_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue arr = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, arr, 0, JS_NewString(ctx, "en-US"));
    return arr;
}

// Overrides: NavigatorOnLine | onLine (getter)
JSValue wisp_navigatoronline_onLine_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewBool(ctx, 1);
}

// Overrides: NavigatorStorageUtils | cookieEnabled (getter)
JSValue wisp_navigatorstorageutils_cookieEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_cookieEnabled_get_impl(ctx, priv);
}

// Overrides: NavigatorStorageUtils | yieldForStorageUpdates()
JSValue wisp_navigatorstorageutils_yieldForStorageUpdates_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: NonElementParentNode | getElementById()
JSValue wisp_nonelementparentnode_getElementById_impl(JSContext *ctx, QJSNodePrivate *priv, const char * elementId) {
    return JS_NULL;
}

// Overrides: ParentNode | childElementCount (getter)
JSValue wisp_parentnode_childElementCount_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: ParentNode | children (getter)
JSValue wisp_parentnode_children_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: ParentNode | firstElementChild (getter)
JSValue wisp_parentnode_firstElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: ParentNode | lastElementChild (getter)
JSValue wisp_parentnode_lastElementChild_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: ParentNode | query()
JSValue wisp_parentnode_query_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: ParentNode | queryAll()
JSValue wisp_parentnode_queryAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * relativeSelectors) {
    return JS_UNDEFINED;
}

// Overrides: ParentNode | querySelector()
JSValue wisp_parentnode_querySelector_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors) {
    return JS_NULL;
}

// Overrides: ParentNode | querySelectorAll()
JSValue wisp_parentnode_querySelectorAll_impl(JSContext *ctx, QJSNodePrivate *priv, const char * selectors) {
    return JS_UNDEFINED;
}

// Overrides: Path2D | constructor_0
JSValue wisp_path2d_constructor_0_impl(JSContext *ctx) {
    return JS_UNDEFINED;
}

// Overrides: Path2D | constructor_1
JSValue wisp_path2d_constructor_1_impl(JSContext *ctx, void * path) {
    return JS_UNDEFINED;
}

// Overrides: Path2D | constructor_2
JSValue wisp_path2d_constructor_2_impl(JSContext *ctx, JSValue paths, JSValue fillRule) {
    return JS_UNDEFINED;
}

// Overrides: Path2D | constructor_3
JSValue wisp_path2d_constructor_3_impl(JSContext *ctx, const char * d) {
    return JS_UNDEFINED;
}

// Overrides: Range | toString()
JSValue wisp_range_toString_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: SharedWorker | constructor
JSValue wisp_sharedworker_constructor_impl(JSContext *ctx, const char * scriptURL, const char * name) {
    extern JSClassID qjs_sharedworker_class_id;
    JSValue obj = JS_NewObjectClass(ctx, qjs_sharedworker_class_id);
    if (JS_IsException(obj)) return obj;
    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));
    if (!priv) { JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }
    priv->magic = QJS_DOM_MAGIC; priv->is_dom_node = false; priv->ctx = ctx;
    JS_SetOpaque(obj, priv);

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue port_ctor = JS_GetPropertyStr(ctx, global, "MessagePort");
    JS_FreeValue(ctx, global);

    JSValue port = JS_UNDEFINED;
    if (JS_IsFunction(ctx, port_ctor)) {
        port = JS_CallConstructor(ctx, port_ctor, 0, NULL);
    }
    JS_FreeValue(ctx, port_ctor);

    if (JS_IsUndefined(port) || JS_IsException(port)) {
        JS_FreeValue(ctx, port);
        port = JS_NewObject(ctx);
    }

    JS_SetPropertyStr(ctx, obj, "_port", port);
    return obj;
}

// Overrides: StorageEvent | constructor
JSValue wisp_storageevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_UNDEFINED;
}

// Overrides: Text | constructor
JSValue wisp_text_constructor_impl(JSContext *ctx, const char * data) {
    return JS_UNDEFINED;
}

// Overrides: TextTrackCueList | __getter__()
JSValue wisp_texttrackcuelist___getter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    WispTextTrackCueList *cl = priv ? (WispTextTrackCueList *)priv->node : NULL;
    if (cl && index < cl->count) {
        extern JSValue qjs_new_vttcue(JSContext *ctx, void *node, bool is_dom_node);
        return qjs_new_vttcue(ctx, cl->cues[index], false);
    }
    return JS_UNDEFINED;
}

// Overrides: TextTrackList | __getter__()
JSValue wisp_texttracklist___getter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    WispTextTrackList *tl = priv ? (WispTextTrackList *)priv->node : NULL;
    if (tl && index < tl->count) {
        extern JSValue qjs_new_texttrack(JSContext *ctx, void *node, bool is_dom_node);
        return qjs_new_texttrack(ctx, tl->tracks[index], false);
    }
    return JS_UNDEFINED;
}

// Overrides: UIEvent | constructor
JSValue wisp_uievent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | hash (getter)
JSValue wisp_urlutils_hash_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | hash (setter)
JSValue wisp_urlutils_hash_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | host (getter)
JSValue wisp_urlutils_host_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | host (setter)
JSValue wisp_urlutils_host_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | hostname (getter)
JSValue wisp_urlutils_hostname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | hostname (setter)
JSValue wisp_urlutils_hostname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | href()
JSValue wisp_urlutils_href_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | origin (getter)
JSValue wisp_urlutils_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | password (getter)
JSValue wisp_urlutils_password_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | password (setter)
JSValue wisp_urlutils_password_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | pathname (getter)
JSValue wisp_urlutils_pathname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | pathname (setter)
JSValue wisp_urlutils_pathname_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | port (getter)
JSValue wisp_urlutils_port_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | port (setter)
JSValue wisp_urlutils_port_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | protocol (getter)
JSValue wisp_urlutils_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | protocol (setter)
JSValue wisp_urlutils_protocol_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | search (getter)
JSValue wisp_urlutils_search_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | search (setter)
JSValue wisp_urlutils_search_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtils | username (getter)
JSValue wisp_urlutils_username_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtils | username (setter)
JSValue wisp_urlutils_username_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: URLUtilsReadOnly | hash (getter)
JSValue wisp_urlutilsreadonly_hash_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsReadOnly | host (getter)
JSValue wisp_urlutilsreadonly_host_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsReadOnly | hostname (getter)
JSValue wisp_urlutilsreadonly_hostname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsReadOnly | href()
JSValue wisp_urlutilsreadonly_href_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: URLUtilsReadOnly | origin (getter)
JSValue wisp_urlutilsreadonly_origin_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsReadOnly | pathname (getter)
JSValue wisp_urlutilsreadonly_pathname_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsReadOnly | port (getter)
JSValue wisp_urlutilsreadonly_port_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// =============================================================================
// WAVE 7: Final WebIDL Stubs Implementation (184 stubs)
// =============================================================================

// Overrides: URLUtilsReadOnly | protocol (getter)
JSValue wisp_urlutilsreadonly_protocol_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsReadOnly | search (getter)
JSValue wisp_urlutilsreadonly_search_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsSearchParams | searchParams (getter)
JSValue wisp_urlutilssearchparams_searchParams_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: URLUtilsSearchParams | searchParams (setter)
JSValue wisp_urlutilssearchparams_searchParams_set_impl(JSContext *ctx, QJSNodePrivate *priv, void * value) {
    return JS_UNDEFINED;
}

// Overrides: VideoTrackList | __getter__()
JSValue wisp_videotracklist___getter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

// Overrides: WebSocket | constructor
extern JSValue qjs_new_websocket(JSContext *ctx, void *node, bool is_dom_node);

JSValue wisp_websocket_constructor_impl(JSContext *ctx, const char * url, JSValue protocols) {
    WispWebSocketPrivate *wsp = calloc(1, sizeof(WispWebSocketPrivate));
    if (wsp) {
        if (url) wsp->url = strdup(url);
        wsp->binaryType = strdup("blob");
        wsp->readyState = 0; // CONNECTING
    }
    return qjs_new_websocket(ctx, wsp, false);
}

// Overrides: Window | __getter__()
JSValue wisp_window___getter___0_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

// Overrides: Window | __getter__()
JSValue wisp_window___getter___1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) {
    return JS_UNDEFINED;
}

// Overrides: Window | alert()
JSValue wisp_window_alert_0_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: Window | alert()
JSValue wisp_window_alert_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message) {
    return JS_UNDEFINED;
}

// Overrides: Window | clearInterval()
JSValue wisp_window_clearInterval_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return JS_UNDEFINED;
}

// Overrides: Window | clearTimeout()
JSValue wisp_window_clearTimeout_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return JS_UNDEFINED;
}

// Overrides: Window | name (getter)
JSValue wisp_window_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: Window | name (setter)
JSValue wisp_window_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: Window | onabort (getter)
JSValue wisp_window_onabort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onabort");
}

// Overrides: Window | onabort (setter)
JSValue wisp_window_onabort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onabort", "abort", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onafterprint (getter)
JSValue wisp_window_onafterprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onafterprint");
}

// Overrides: Window | onafterprint (setter)
JSValue wisp_window_onafterprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onafterprint", "afterprint", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onautocomplete (getter)
JSValue wisp_window_onautocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocomplete");
}

// Overrides: Window | onautocomplete (setter)
JSValue wisp_window_onautocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocomplete", "autocomplete", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onautocompleteerror (getter)
JSValue wisp_window_onautocompleteerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onautocompleteerror");
}

// Overrides: Window | onautocompleteerror (setter)
JSValue wisp_window_onautocompleteerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onautocompleteerror", "autocompleteerror", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onbeforeprint (getter)
JSValue wisp_window_onbeforeprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeprint");
}

// Overrides: Window | onbeforeprint (setter)
JSValue wisp_window_onbeforeprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeprint", "beforeprint", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onbeforeunload (getter)
JSValue wisp_window_onbeforeunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onbeforeunload");
}

// Overrides: Window | onbeforeunload (setter)
JSValue wisp_window_onbeforeunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onbeforeunload", "beforeunload", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onblur (getter)
JSValue wisp_window_onblur_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onblur");
}

// Overrides: Window | onblur (setter)
JSValue wisp_window_onblur_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onblur", "blur", value);
    return JS_UNDEFINED;
}

// Overrides: Window | oncancel (getter)
JSValue wisp_window_oncancel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncancel");
}

// Overrides: Window | oncancel (setter)
JSValue wisp_window_oncancel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncancel", "cancel", value);
    return JS_UNDEFINED;
}

// Overrides: Window | oncanplay (getter)
JSValue wisp_window_oncanplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplay");
}

// Overrides: Window | oncanplay (setter)
JSValue wisp_window_oncanplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplay", "canplay", value);
    return JS_UNDEFINED;
}

// Overrides: Window | oncanplaythrough (getter)
JSValue wisp_window_oncanplaythrough_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncanplaythrough");
}

// Overrides: Window | oncanplaythrough (setter)
JSValue wisp_window_oncanplaythrough_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncanplaythrough", "canplaythrough", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onchange (getter)
JSValue wisp_window_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onchange");
}

// Overrides: Window | onchange (setter)
JSValue wisp_window_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onchange", "change", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onclick (getter)
JSValue wisp_window_onclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclick");
}

// Overrides: Window | onclick (setter)
JSValue wisp_window_onclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclick", "click", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onclose (getter)
JSValue wisp_window_onclose_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclose");
}

// Overrides: Window | onclose (setter)
JSValue wisp_window_onclose_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclose", "close", value);
    return JS_UNDEFINED;
}

// Overrides: Window | oncontextmenu (getter)
JSValue wisp_window_oncontextmenu_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncontextmenu");
}

// Overrides: Window | oncontextmenu (setter)
JSValue wisp_window_oncontextmenu_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncontextmenu", "contextmenu", value);
    return JS_UNDEFINED;
}

// Overrides: Window | oncuechange (getter)
JSValue wisp_window_oncuechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oncuechange");
}

// Overrides: Window | oncuechange (setter)
JSValue wisp_window_oncuechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oncuechange", "cuechange", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondblclick (getter)
JSValue wisp_window_ondblclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondblclick");
}

// Overrides: Window | ondblclick (setter)
JSValue wisp_window_ondblclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondblclick", "dblclick", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondrag (getter)
JSValue wisp_window_ondrag_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrag");
}

// Overrides: Window | ondrag (setter)
JSValue wisp_window_ondrag_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrag", "drag", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondragend (getter)
JSValue wisp_window_ondragend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragend");
}

// Overrides: Window | ondragend (setter)
JSValue wisp_window_ondragend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragend", "dragend", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondragenter (getter)
JSValue wisp_window_ondragenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragenter");
}

// Overrides: Window | ondragenter (setter)
JSValue wisp_window_ondragenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragenter", "dragenter", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondragexit (getter)
JSValue wisp_window_ondragexit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragexit");
}

// Overrides: Window | ondragexit (setter)
JSValue wisp_window_ondragexit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragexit", "dragexit", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondragleave (getter)
JSValue wisp_window_ondragleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragleave");
}

// Overrides: Window | ondragleave (setter)
JSValue wisp_window_ondragleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragleave", "dragleave", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondragover (getter)
JSValue wisp_window_ondragover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragover");
}

// Overrides: Window | ondragover (setter)
JSValue wisp_window_ondragover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragover", "dragover", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondragstart (getter)
JSValue wisp_window_ondragstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondragstart");
}

// Overrides: Window | ondragstart (setter)
JSValue wisp_window_ondragstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondragstart", "dragstart", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondrop (getter)
JSValue wisp_window_ondrop_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondrop");
}

// Overrides: Window | ondrop (setter)
JSValue wisp_window_ondrop_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondrop", "drop", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ondurationchange (getter)
JSValue wisp_window_ondurationchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ondurationchange");
}

// Overrides: Window | ondurationchange (setter)
JSValue wisp_window_ondurationchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ondurationchange", "durationchange", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onemptied (getter)
JSValue wisp_window_onemptied_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onemptied");
}

// Overrides: Window | onemptied (setter)
JSValue wisp_window_onemptied_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onemptied", "emptied", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onended (getter)
JSValue wisp_window_onended_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onended");
}

// Overrides: Window | onended (setter)
JSValue wisp_window_onended_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onended", "ended", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onerror (getter)
JSValue wisp_window_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onerror");
}

// Overrides: Window | onerror (setter)
JSValue wisp_window_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onerror", "error", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onfocus (getter)
JSValue wisp_window_onfocus_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onfocus");
}

// Overrides: Window | onfocus (setter)
JSValue wisp_window_onfocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onfocus", "focus", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onhashchange (getter)
JSValue wisp_window_onhashchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onhashchange");
}

// Overrides: Window | onhashchange (setter)
JSValue wisp_window_onhashchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onhashchange", "hashchange", value);
    return JS_UNDEFINED;
}

// Overrides: Window | oninput (getter)
JSValue wisp_window_oninput_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninput");
}

// Overrides: Window | oninput (setter)
JSValue wisp_window_oninput_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninput", "input", value);
    return JS_UNDEFINED;
}

// Overrides: Window | oninvalid (getter)
JSValue wisp_window_oninvalid_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "oninvalid");
}

// Overrides: Window | oninvalid (setter)
JSValue wisp_window_oninvalid_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "oninvalid", "invalid", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onkeydown (getter)
JSValue wisp_window_onkeydown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeydown");
}

// Overrides: Window | onkeydown (setter)
JSValue wisp_window_onkeydown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeydown", "keydown", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onkeypress (getter)
JSValue wisp_window_onkeypress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeypress");
}

// Overrides: Window | onkeypress (setter)
JSValue wisp_window_onkeypress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeypress", "keypress", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onkeyup (getter)
JSValue wisp_window_onkeyup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onkeyup");
}

// Overrides: Window | onkeyup (setter)
JSValue wisp_window_onkeyup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onkeyup", "keyup", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onlanguagechange (getter)
JSValue wisp_window_onlanguagechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onlanguagechange");
}

// Overrides: Window | onlanguagechange (setter)
JSValue wisp_window_onlanguagechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onlanguagechange", "languagechange", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onload (getter)
JSValue wisp_window_onload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onload");
}

// Overrides: Window | onload (setter)
JSValue wisp_window_onload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onload", "load", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onloadeddata (getter)
JSValue wisp_window_onloadeddata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadeddata");
}

// Overrides: Window | onloadeddata (setter)
JSValue wisp_window_onloadeddata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadeddata", "loadeddata", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onloadedmetadata (getter)
JSValue wisp_window_onloadedmetadata_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadedmetadata");
}

// Overrides: Window | onloadedmetadata (setter)
JSValue wisp_window_onloadedmetadata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadedmetadata", "loadedmetadata", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onloadstart (getter)
JSValue wisp_window_onloadstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onloadstart");
}

// Overrides: Window | onloadstart (setter)
JSValue wisp_window_onloadstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onloadstart", "loadstart", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmessage (getter)
JSValue wisp_window_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmessage");
}

// Overrides: Window | onmessage (setter)
JSValue wisp_window_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmessage", "message", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmousedown (getter)
JSValue wisp_window_onmousedown_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousedown");
}

// Overrides: Window | onmousedown (setter)
JSValue wisp_window_onmousedown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousedown", "mousedown", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmouseenter (getter)
JSValue wisp_window_onmouseenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseenter");
}

// Overrides: Window | onmouseenter (setter)
JSValue wisp_window_onmouseenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseenter", "mouseenter", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmouseleave (getter)
JSValue wisp_window_onmouseleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseleave");
}

// Overrides: Window | onmouseleave (setter)
JSValue wisp_window_onmouseleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseleave", "mouseleave", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmousemove (getter)
JSValue wisp_window_onmousemove_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmousemove");
}

// Overrides: Window | onmousemove (setter)
JSValue wisp_window_onmousemove_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmousemove", "mousemove", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmouseout (getter)
JSValue wisp_window_onmouseout_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseout");
}

// Overrides: Window | onmouseout (setter)
JSValue wisp_window_onmouseout_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseout", "mouseout", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmouseover (getter)
JSValue wisp_window_onmouseover_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseover");
}

// Overrides: Window | onmouseover (setter)
JSValue wisp_window_onmouseover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseover", "mouseover", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onmouseup (getter)
JSValue wisp_window_onmouseup_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onmouseup");
}

// Overrides: Window | onmouseup (setter)
JSValue wisp_window_onmouseup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onmouseup", "mouseup", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onoffline (getter)
JSValue wisp_window_onoffline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onoffline");
}

// Overrides: Window | onoffline (setter)
JSValue wisp_window_onoffline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onoffline", "offline", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ononline (getter)
JSValue wisp_window_ononline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ononline");
}

// Overrides: Window | ononline (setter)
JSValue wisp_window_ononline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ononline", "online", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onpagehide (getter)
JSValue wisp_window_onpagehide_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpagehide");
}

// Overrides: Window | onpagehide (setter)
JSValue wisp_window_onpagehide_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpagehide", "pagehide", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onpageshow (getter)
JSValue wisp_window_onpageshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpageshow");
}

// Overrides: Window | onpageshow (setter)
JSValue wisp_window_onpageshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpageshow", "pageshow", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onpause (getter)
JSValue wisp_window_onpause_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpause");
}

// Overrides: Window | onpause (setter)
JSValue wisp_window_onpause_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpause", "pause", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onplay (getter)
JSValue wisp_window_onplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplay");
}

// Overrides: Window | onplay (setter)
JSValue wisp_window_onplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplay", "play", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onplaying (getter)
JSValue wisp_window_onplaying_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onplaying");
}

// Overrides: Window | onplaying (setter)
JSValue wisp_window_onplaying_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onplaying", "playing", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onpopstate (getter)
JSValue wisp_window_onpopstate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onpopstate");
}

// Overrides: Window | onpopstate (setter)
JSValue wisp_window_onpopstate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onpopstate", "popstate", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onprogress (getter)
JSValue wisp_window_onprogress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onprogress");
}

// Overrides: Window | onprogress (setter)
JSValue wisp_window_onprogress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onprogress", "progress", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onratechange (getter)
JSValue wisp_window_onratechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onratechange");
}

// Overrides: Window | onratechange (setter)
JSValue wisp_window_onratechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onratechange", "ratechange", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onreset (getter)
JSValue wisp_window_onreset_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onreset");
}

// Overrides: Window | onreset (setter)
JSValue wisp_window_onreset_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onreset", "reset", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onresize (getter)
JSValue wisp_window_onresize_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onresize");
}

// Overrides: Window | onresize (setter)
JSValue wisp_window_onresize_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onresize", "resize", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onscroll (getter)
JSValue wisp_window_onscroll_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onscroll");
}

// Overrides: Window | onscroll (setter)
JSValue wisp_window_onscroll_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onscroll", "scroll", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onseeked (getter)
JSValue wisp_window_onseeked_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeked");
}

// Overrides: Window | onseeked (setter)
JSValue wisp_window_onseeked_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeked", "seeked", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onseeking (getter)
JSValue wisp_window_onseeking_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onseeking");
}

// Overrides: Window | onseeking (setter)
JSValue wisp_window_onseeking_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onseeking", "seeking", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onselect (getter)
JSValue wisp_window_onselect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onselect");
}

// Overrides: Window | onselect (setter)
JSValue wisp_window_onselect_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onselect", "select", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onshow (getter)
JSValue wisp_window_onshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onshow");
}

// Overrides: Window | onshow (setter)
JSValue wisp_window_onshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onshow", "show", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onsort (getter)
JSValue wisp_window_onsort_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsort");
}

// Overrides: Window | onsort (setter)
JSValue wisp_window_onsort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsort", "sort", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onstalled (getter)
JSValue wisp_window_onstalled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstalled");
}

// Overrides: Window | onstalled (setter)
JSValue wisp_window_onstalled_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstalled", "stalled", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onstorage (getter)
JSValue wisp_window_onstorage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onstorage");
}

// Overrides: Window | onstorage (setter)
JSValue wisp_window_onstorage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onstorage", "storage", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onsubmit (getter)
JSValue wisp_window_onsubmit_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsubmit");
}

// Overrides: Window | onsubmit (setter)
JSValue wisp_window_onsubmit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsubmit", "submit", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onsuspend (getter)
JSValue wisp_window_onsuspend_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onsuspend");
}

// Overrides: Window | onsuspend (setter)
JSValue wisp_window_onsuspend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onsuspend", "suspend", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ontimeupdate (getter)
JSValue wisp_window_ontimeupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontimeupdate");
}

// Overrides: Window | ontimeupdate (setter)
JSValue wisp_window_ontimeupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontimeupdate", "timeupdate", value);
    return JS_UNDEFINED;
}

// Overrides: Window | ontoggle (getter)
JSValue wisp_window_ontoggle_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "ontoggle");
}

// Overrides: Window | ontoggle (setter)
JSValue wisp_window_ontoggle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "ontoggle", "toggle", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onunload (getter)
JSValue wisp_window_onunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onunload");
}

// Overrides: Window | onunload (setter)
JSValue wisp_window_onunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onunload", "unload", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onvolumechange (getter)
JSValue wisp_window_onvolumechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onvolumechange");
}

// Overrides: Window | onvolumechange (setter)
JSValue wisp_window_onvolumechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onvolumechange", "volumechange", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onwaiting (getter)
JSValue wisp_window_onwaiting_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwaiting");
}

// Overrides: Window | onwaiting (setter)
JSValue wisp_window_onwaiting_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwaiting", "waiting", value);
    return JS_UNDEFINED;
}

// Overrides: Window | onwheel (getter)
JSValue wisp_window_onwheel_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onwheel");
}

// Overrides: Window | onwheel (setter)
JSValue wisp_window_onwheel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onwheel", "wheel", value);
    return JS_UNDEFINED;
}

// Overrides: Window | setInterval()
JSValue wisp_window_setInterval_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return wisp_timer_create(ctx, handler, timeout, arguments, true);
}

// Overrides: Window | setInterval()
JSValue wisp_window_setInterval_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    JSValue handler_val = JS_NewString(ctx, handler);
    JSValue ret = wisp_timer_create(ctx, handler_val, timeout, arguments, true);
    JS_FreeValue(ctx, handler_val);
    return ret;
}

// Overrides: Window | setTimeout()
JSValue wisp_window_setTimeout_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return wisp_timer_create(ctx, handler, timeout, arguments, false);
}

// Overrides: Window | setTimeout()
JSValue wisp_window_setTimeout_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    JSValue handler_val = JS_NewString(ctx, handler);
    JSValue ret = wisp_timer_create(ctx, handler_val, timeout, arguments, false);
    JS_FreeValue(ctx, handler_val);
    return ret;
}

// Overrides: WindowLocalStorage | localStorage (getter)
JSValue wisp_windowlocalstorage_localStorage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue val = JS_GetPropertyStr(ctx, global_obj, "__wisp_localStorage");
    JS_FreeValue(ctx, global_obj);
    return val;
}

// Overrides: WindowModal | dialogArguments (getter)
JSValue wisp_windowmodal_dialogArguments_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: WindowModal | returnValue (getter)
JSValue wisp_windowmodal_returnValue_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: WindowModal | returnValue (setter)
JSValue wisp_windowmodal_returnValue_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: WindowSessionStorage | sessionStorage (getter)
JSValue wisp_windowsessionstorage_sessionStorage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue val = JS_GetPropertyStr(ctx, global_obj, "__wisp_sessionStorage");
    JS_FreeValue(ctx, global_obj);
    return val;
}

// Overrides: WindowTimers | clearInterval()
JSValue wisp_windowtimers_clearInterval_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return wisp_timer_clear(ctx, handle);
}

// Overrides: WindowTimers | clearTimeout()
JSValue wisp_windowtimers_clearTimeout_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return wisp_timer_clear(ctx, handle);
}

// Overrides: WindowTimers | setInterval()
JSValue wisp_windowtimers_setInterval_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return wisp_timer_create(ctx, handler, timeout, arguments, true);
}

// Overrides: WindowTimers | setInterval()
JSValue wisp_windowtimers_setInterval_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    JSValue handler_val = JS_NewString(ctx, handler);
    JSValue ret = wisp_timer_create(ctx, handler_val, timeout, arguments, true);
    JS_FreeValue(ctx, handler_val);
    return ret;
}

// Overrides: WindowTimers | setTimeout()
JSValue wisp_windowtimers_setTimeout_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return wisp_timer_create(ctx, handler, timeout, arguments, false);
}

// Overrides: WindowTimers | setTimeout()
JSValue wisp_windowtimers_setTimeout_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    JSValue handler_val = JS_NewString(ctx, handler);
    JSValue ret = wisp_timer_create(ctx, handler_val, timeout, arguments, false);
    JS_FreeValue(ctx, handler_val);
    return ret;
}

// Overrides: WorkerNavigator | language (getter)
JSValue wisp_workernavigator_language_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return wisp_navigator_language_get_impl(ctx, priv);
}


JSValue wisp_htmlcanvaselement_onclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return helper_get_event_handler(ctx, priv, "onclick");
}

JSValue wisp_htmlcanvaselement_onclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    helper_set_event_handler(ctx, priv, "onclick", "click", value);
    return JS_UNDEFINED;
}
