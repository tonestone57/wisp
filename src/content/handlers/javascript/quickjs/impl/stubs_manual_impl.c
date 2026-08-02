#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <libwapcaplet/libwapcaplet.h>
#include <wisp/utils/shm_dom.h>

struct nsurl;
extern const char *nsurl_access(const struct nsurl *url);
extern struct nsurl *content_get_url(void *c);
extern nserror nsurl_create(const char *const url_s, struct nsurl **url);

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
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t) return NULL;

    if (wisp_is_js_process) {
        if (!t->location_url && t->origin) {
            nsurl_create(t->origin, &t->location_url);
        }
        return t->location_url;
    }

    if (t->doc_priv) {
        return content_get_url((void *)t->doc_priv);
    }
    return NULL;
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
    struct nsurl *resolved_url = NULL;
    if (base_url) {
        nsurl_join(base_url, href_str, &resolved_url);
    } else {
        nsurl_create(href_str, &resolved_url);
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
    return val;
}

JSValue wisp_htmlinputelement_type_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "type", value);
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
    return get_element_str_attr(ctx, priv, "sandbox", "");
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
    return JS_UNDEFINED;
}

JSValue wisp_htmlformelement_submit_impl(JSContext *ctx, QJSNodePrivate *priv)
{
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

JSValue wisp_htmloptionelement_defaultSelected_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "selected");
}

JSValue wisp_htmloptionelement_defaultSelected_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "selected", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "selected");
    }
}

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

JSValue wisp_htmloptionelement_selected_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_element_hasAttribute_impl(ctx, priv, "selected");
}

JSValue wisp_htmloptionelement_selected_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value)
{
    if (value) {
        return wisp_element_setAttribute_impl(ctx, priv, "selected", "");
    } else {
        return wisp_element_removeAttribute_impl(ctx, priv, "selected");
    }
}

JSValue wisp_htmloptionelement_text_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return wisp_node_textContent_get_impl(ctx, priv);
}

JSValue wisp_htmloptionelement_text_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_node_textContent_set_impl(ctx, priv, value);
}

JSValue wisp_htmloptionelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue val = wisp_element_getAttribute_impl(ctx, priv, "value");
    if (JS_IsNull(val) || JS_IsUndefined(val)) {
        return wisp_htmloptionelement_text_get_impl(ctx, priv);
    }
    return val;
}

JSValue wisp_htmloptionelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value)
{
    return wisp_element_setAttribute_impl(ctx, priv, "value", value);
}

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
                    for (uint32_t j = 0; j < strings_arr[i].attr_count; j++) {
                        if (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].attrs[j].name, "selected")) {
                            is_sel = true;
                            break;
                        }
                    }
                    if (is_sel) {
                        for (uint32_t j = 0; j < strings_arr[i].attr_count; j++) {
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

JSValue wisp_htmlselectelement_length_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, 0);
    int count = 0;

    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t our_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].parent_id == our_id && nodes_arr[i].node_type == 1 &&
                    wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "option")) {
                    count++;
                }
            }
        }
        return JS_NewInt32(ctx, count);
    }

    dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    while (child) {
        dom_string *tag_name = NULL;
        dom_node_get_node_name(child, &tag_name);
        if (tag_name) {
            if (strcasecmp((const char *)dom_string_data(tag_name), "option") == 0) {
                count++;
            }
            dom_string_unref(tag_name);
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return JS_NewInt32(ctx, count);
}

JSValue wisp_htmlselectelement_length_set_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)
{
    return JS_UNDEFINED;
}

JSValue wisp_htmlselectelement_selectedIndex_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewInt32(ctx, -1);
    int idx = 0;

    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t our_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].parent_id == our_id && nodes_arr[i].node_type == 1 &&
                    wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "option")) {
                    bool is_sel = false;
                    for (uint32_t j = 0; j < strings_arr[i].attr_count; j++) {
                        if (wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].attrs[j].name, "selected")) {
                            is_sel = true;
                            break;
                        }
                    }
                    if (is_sel) {
                        return JS_NewInt32(ctx, idx);
                    }
                    idx++;
                }
            }
        }
        return JS_NewInt32(ctx, -1);
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
                    dom_node_unref(child);
                    return JS_NewInt32(ctx, idx);
                }
                idx++;
            } else {
                dom_string_unref(tag_name);
            }
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return JS_NewInt32(ctx, -1);
}

JSValue wisp_htmlselectelement_selectedIndex_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value)
{
    if (!priv || !priv->node) return JS_UNDEFINED;

    if (wisp_is_js_process) {
        return JS_UNDEFINED;
    }

    int idx = 0;
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
                if (idx == value) {
                    dom_element_set_attribute((dom_element *)child, attr_name, attr_name);
                } else {
                    dom_element_remove_attribute((dom_element *)child, attr_name);
                }
                dom_string_unref(attr_name);
                idx++;
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

JSValue wisp_htmlselectelement_options_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (!priv || !priv->node) return JS_NewArray(ctx);
    JSValue arr = JS_NewArray(ctx);
    uint32_t count = 0;

    if (wisp_is_js_process) {
        if (wisp_shm_dom) {
            uint32_t our_id = (uint32_t)(uintptr_t)priv->node;
            WispCompactNode *nodes_arr = shm_dom_get_nodes(wisp_shm_dom);
            WispNodeStrings *strings_arr = shm_dom_get_node_strings(wisp_shm_dom);
            for (uint32_t i = 1; i < wisp_shm_dom->node_count; i++) {
                if (nodes_arr[i].parent_id == our_id && nodes_arr[i].node_type == 1 &&
                    wisp_string_ref_caseeq(wisp_shm_dom, strings_arr[i].tag_name, "option")) {
                    JS_SetPropertyUint32(ctx, arr, count++, qjs_wrap_node(ctx, (struct dom_node *)(uintptr_t)i));
                }
            }
        }
        return arr;
    }

    dom_node *child = NULL;
    dom_node_get_first_child((dom_node *)priv->node, &child);
    while (child) {
        dom_string *tag_name = NULL;
        dom_node_get_node_name(child, &tag_name);
        if (tag_name) {
            if (strcasecmp((const char *)dom_string_data(tag_name), "option") == 0) {
                JS_SetPropertyUint32(ctx, arr, count++, qjs_wrap_node(ctx, child));
            }
            dom_string_unref(tag_name);
        }
        dom_node *next = NULL;
        dom_node_get_next_sibling(child, &next);
        dom_node_unref(child);
        child = next;
    }
    return arr;
}

JSValue wisp_htmlselectelement_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index)
{
    JSValue opts = wisp_htmlselectelement_options_get_impl(ctx, priv);
    JSValue item = JS_GetPropertyUint32(ctx, opts, index);
    JS_FreeValue(ctx, opts);
    return item;
}

JSValue wisp_htmlselectelement_namedItem_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name)
{
    if (!priv || !priv->node || !name) return JS_NULL;
    JSValue opts = wisp_htmlselectelement_options_get_impl(ctx, priv);
    uint32_t len = 0;
    JSValue len_val = JS_GetPropertyStr(ctx, opts, "length");
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    for (uint32_t i = 0; i < len; i++) {
        JSValue opt = JS_GetPropertyUint32(ctx, opts, i);
        QJSNodePrivate *opt_priv = qjs_get_dom_priv(ctx, opt);
        if (opt_priv) {
            JSValue id_val = wisp_element_getAttribute_impl(ctx, opt_priv, "id");
            JSValue name_val = wisp_element_getAttribute_impl(ctx, opt_priv, "name");
            bool match = false;
            if (JS_IsString(id_val)) {
                const char *id_str = JS_ToCString(ctx, id_val);
                if (id_str && strcmp(id_str, name) == 0) match = true;
                if (id_str) JS_FreeCString(ctx, id_str);
            }
            if (!match && JS_IsString(name_val)) {
                const char *name_str = JS_ToCString(ctx, name_val);
                if (name_str && strcmp(name_str, name) == 0) match = true;
                if (name_str) JS_FreeCString(ctx, name_str);
            }
            JS_FreeValue(ctx, id_val);
            JS_FreeValue(ctx, name_val);
            if (match) {
                JS_FreeValue(ctx, opts);
                return opt;
            }
        }
        JS_FreeValue(ctx, opt);
    }
    JS_FreeValue(ctx, opts);
    return JS_NULL;
}

extern JSValue wisp_node_appendChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node);
extern JSValue wisp_node_removeChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * child);

JSValue wisp_htmlselectelement_add_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue element, JSValue before)
{
    QJSNodePrivate *el_priv = qjs_get_dom_priv(ctx, element);
    if (el_priv && el_priv->node) {
        return wisp_node_appendChild_impl(ctx, priv, el_priv->node);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlselectelement_remove_0_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    if (wisp_is_js_process) return JS_UNDEFINED;
    if (!priv || !priv->node) return JS_UNDEFINED;
    dom_node *parent = NULL;
    dom_node_get_parent_node((dom_node *)priv->node, &parent);
    if (parent) {
        dom_node *removed = NULL;
        dom_node_remove_child(parent, (dom_node *)priv->node, &removed);
        if (removed) dom_node_unref(removed);
        dom_node_unref(parent);
    }
    return JS_UNDEFINED;
}

JSValue wisp_htmlselectelement_remove_1_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t index)
{
    JSValue item = wisp_htmlselectelement_item_impl(ctx, priv, index);
    if (!JS_IsNull(item) && !JS_IsUndefined(item)) {
        QJSNodePrivate *item_priv = qjs_get_dom_priv(ctx, item);
        if (item_priv && item_priv->node) {
            wisp_node_removeChild_impl(ctx, priv, item_priv->node);
        }
    }
    JS_FreeValue(ctx, item);
    return JS_UNDEFINED;
}

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
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlselectelement_validity_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlselectelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_htmlselectelement___setter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index, void * option) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlselectelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlselectelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlselectelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
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

JSValue wisp_location_reload_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_UNDEFINED;
}

JSValue wisp_location_assign_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url)
{
    return JS_UNDEFINED;
}

JSValue wisp_location_replace_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url)
{
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
    strcpy(priv->type_name, "children"); // Default is children
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
                for (uint32_t j = 0; j < strings[i].attr_count; j++) {
                    if (wisp_string_ref_caseeq(wisp_shm_dom, strings[i].attrs[j].name, "href")) {
                        match = true;
                        break;
                    }
                }
            } else if (strcmp(type, "anchors") == 0 && strcasecmp(tag, "a") == 0) {
                for (uint32_t j = 0; j < strings[i].attr_count; j++) {
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
            for (uint32_t j = 0; j < strings[node_id].attr_count; j++) {
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
    }
    JS_FreeValue(ctx, cells);
    return JS_UNDEFINED;
}

static int32_t get_cell_index_helper(JSContext *ctx, JSValue cell)
{
    JSValue parent = JS_GetPropertyStr(ctx, cell, "parentNode");
    if (JS_IsException(parent) || JS_IsNull(parent) || JS_IsUndefined(parent)) {
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

JSValue wisp_htmllabelelement_control_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
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

JSValue wisp_validitystate_customError_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
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

JSValue wisp_validitystate_valid_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_validitystate_valueMissing_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
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
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlfieldsetelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlfieldsetelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlfieldsetelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
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
    return JS_NewString(ctx, "");
}

JSValue wisp_htmloutputelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_htmloutputelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmloutputelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmloutputelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
    return JS_UNDEFINED;
}

// -----------------------------------------------------------------------------
// HTMLInputElement Additional WebIDL Implementations
// -----------------------------------------------------------------------------

JSValue wisp_htmlinputelement_stepUp_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t n) {
    double val = 0.0;
    JSValue num_val = wisp_htmlinputelement_valueAsNumber_get_impl(ctx, priv);
    JS_ToFloat64(ctx, &val, num_val);
    JS_FreeValue(ctx, num_val);
    val += n;
    wisp_htmlinputelement_valueAsNumber_set_impl(ctx, priv, val);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_stepDown_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t n) {
    double val = 0.0;
    JSValue num_val = wisp_htmlinputelement_valueAsNumber_get_impl(ctx, priv);
    JS_ToFloat64(ctx, &val, num_val);
    JS_FreeValue(ctx, num_val);
    val -= n;
    wisp_htmlinputelement_valueAsNumber_set_impl(ctx, priv, val);
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_select_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_setRangeText_impl(JSContext *ctx, QJSNodePrivate *priv, const char * replacement) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_setSelectionRange_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t start, uint32_t end, const char * direction) {
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
    return JS_NULL;
}

JSValue wisp_htmlinputelement_valueAsDate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlinputelement_valueAsNumber_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    extern JSValue wisp_htmlinputelement_value_get_impl(JSContext *ctx, QJSNodePrivate *priv);
    JSValue val = wisp_htmlinputelement_value_get_impl(ctx, priv);
    double d = 0.0;
    if (JS_IsString(val)) {
        const char *str = JS_ToCString(ctx, val);
        if (str && strlen(str) > 0) {
            d = atof(str);
        } else {
            JS_ToFloat64(ctx, &d, val);
        }
        if (str) JS_FreeCString(ctx, str);
    }
    JS_FreeValue(ctx, val);
    return JS_NewFloat64(ctx, d);
}

JSValue wisp_htmlinputelement_valueAsNumber_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    extern JSValue wisp_htmlinputelement_value_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value);
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
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlinputelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlinputelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlinputelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlinputelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
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
    return JS_NewString(ctx, "");
}

JSValue wisp_htmltextareaelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmltextareaelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmltextareaelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmltextareaelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
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
    return JS_NewString(ctx, "");
}

JSValue wisp_htmlbuttonelement_willValidate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlbuttonelement_checkValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlbuttonelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlbuttonelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
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
    return get_element_str_attr(ctx, priv, "designMode", "off");
}

JSValue wisp_document_designMode_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    if (!priv || !priv->node || !value) return JS_UNDEFINED;
    set_element_str_attr(ctx, priv, "designMode", value);
    return JS_UNDEFINED;
}

JSValue wisp_document_hasFocus_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
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
JSValue wisp_htmlmediaelement_addTextTrack_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue kind, const char * label, const char * language) {
    return JS_UNDEFINED;
}

JSValue wisp_htmlmediaelement_canPlayType_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type) {
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

JSValue wisp_htmlmediaelement_textTracks_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "texttracks", "");
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
    return JS_UNDEFINED;
}

JSValue wisp_htmlcanvaselement_toDataURL_impl(JSContext *ctx, QJSNodePrivate *priv, const char * type, JSValue arguments) {
    return JS_NewString(ctx, "data:image/png;base64,");
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
    return JS_NULL;
}

JSValue wisp_htmlmeterelement_high_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "high", 0.0));
}

JSValue wisp_htmlmeterelement_high_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "high", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmeterelement_labels_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_htmlmeterelement_low_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "low", 0.0));
}

JSValue wisp_htmlmeterelement_low_set_impl(JSContext *ctx, QJSNodePrivate *priv, double value) {
    set_element_double_attr(ctx, priv, "low", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmeterelement_max_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "max", 0.0));
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
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "optimum", 0.0));
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
    return JS_NULL;
}

JSValue wisp_htmlprogresselement_position_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewFloat64(ctx, get_element_double_attr(ctx, priv, "position", 0.0));
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

JSValue wisp_cssstyledeclaration_getPropertyPriority_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property) {
    return JS_NewString(ctx, "");
}

JSValue wisp_cssstyledeclaration_getPropertyValue_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property) {
    return JS_NewString(ctx, "");
}

JSValue wisp_cssstyledeclaration_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_NewString(ctx, "");
}

JSValue wisp_cssstyledeclaration_removeProperty_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property) {
    return JS_NewString(ctx, "");
}

JSValue wisp_cssstyledeclaration_setProperty_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property, const char * value, const char * priority) {
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_setPropertyPriority_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property, const char * priority) {
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_setPropertyValue_impl(JSContext *ctx, QJSNodePrivate *priv, const char * property, const char * value) {
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_cssFloat_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "none");
}

JSValue wisp_cssstyledeclaration_cssFloat_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    // Stub setter for cssstyledeclaration.cssFloat
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_cssText_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv) return JS_NewString(ctx, "");
    return wisp_element_getAttribute_impl(ctx, priv, "style");
}

JSValue wisp_cssstyledeclaration_cssText_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    // Stub setter for cssstyledeclaration.cssText
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_dashed_attribute_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_cssstyledeclaration_dashed_attribute_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    // Stub setter for cssstyledeclaration.dashed
    return JS_UNDEFINED;
}

JSValue wisp_cssstyledeclaration_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
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
    return get_element_str_attr(ctx, priv, "contenteditable", "");
}

JSValue wisp_htmlelement_contentEditable_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    set_element_str_attr(ctx, priv, "contenteditable", value);
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
    return get_element_str_attr(ctx, priv, "dataset", "");
}

JSValue wisp_htmlelement_draggable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_bool_attr(ctx, priv, "draggable");
}

JSValue wisp_htmlelement_draggable_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) {
    set_element_bool_attr(ctx, priv, "draggable", value);
    return JS_UNDEFINED;
}

JSValue wisp_htmlelement_dropzone_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "dropzone", "");
}

JSValue wisp_htmlelement_isContentEditable_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return get_element_str_attr(ctx, priv, "iscontenteditable", "");
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
    if (!priv || !priv->node) return JS_NULL;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue val = JS_GetPropertyStr(ctx, wrapper, "__onbounce_func");
    JS_FreeValue(ctx, wrapper);
    return val;
}

JSValue wisp_htmlmarqueeelement_onbounce_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JS_SetPropertyStr(ctx, wrapper, "__onbounce_func", JS_DupValue(ctx, value));
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_onfinish_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue val = JS_GetPropertyStr(ctx, wrapper, "__onfinish_func");
    JS_FreeValue(ctx, wrapper);
    return val;
}

JSValue wisp_htmlmarqueeelement_onfinish_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JS_SetPropertyStr(ctx, wrapper, "__onfinish_func", JS_DupValue(ctx, value));
    JS_FreeValue(ctx, wrapper);
    return JS_UNDEFINED;
}

JSValue wisp_htmlmarqueeelement_onstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    if (!priv || !priv->node) return JS_NULL;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JSValue val = JS_GetPropertyStr(ctx, wrapper, "__onstart_func");
    JS_FreeValue(ctx, wrapper);
    return val;
}

JSValue wisp_htmlmarqueeelement_onstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    if (!priv || !priv->node) return JS_UNDEFINED;
    JSValue wrapper = qjs_wrap_node(ctx, (dom_node *)priv->node);
    JS_SetPropertyStr(ctx, wrapper, "__onstart_func", JS_DupValue(ctx, value));
    JS_FreeValue(ctx, wrapper);
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

// -----------------------------------------------------------------------------
// NavigatorPlugins Implementation (3 stubs)
// -----------------------------------------------------------------------------

JSValue wisp_navigatorplugins_javaEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

JSValue wisp_navigatorplugins_mimeTypes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_navigatorplugins_plugins_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
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
    return JS_TRUE;
}

JSValue wisp_htmlobjectelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}

JSValue wisp_htmlobjectelement_setCustomValidity_impl(JSContext *ctx, QJSNodePrivate *priv, const char * error) {
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
    return JS_NewString(ctx, "");
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
    return JS_NULL;
}
JSValue wisp_messageport_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// 4. BroadcastChannel Implementation (5 stubs + constructor)
JSValue wisp_broadcastchannel_constructor_impl(JSContext *ctx, const char * name) {
    return JS_NewObject(ctx);
}
JSValue wisp_broadcastchannel_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}
JSValue wisp_broadcastchannel_postMessage_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue message) {
    return JS_UNDEFINED;
}
JSValue wisp_broadcastchannel_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_broadcastchannel_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_broadcastchannel_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
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
    return JS_TRUE;
}
JSValue wisp_htmlformelement_reportValidity_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_TRUE;
}
JSValue wisp_htmlformelement_requestAutocomplete_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
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
    return JS_NULL;
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
    return JS_NULL;
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
    return JS_NewObject(ctx);
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

// 31. CanvasGradient Implementation (1 stub)
JSValue wisp_canvasgradient_addColorStop_impl(JSContext *ctx, QJSNodePrivate *priv, double offset, const char * color) {
    return JS_UNDEFINED;
}

// 32. CanvasPattern Implementation (1 stub)
JSValue wisp_canvaspattern_setTransform_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue transform) {
    return JS_UNDEFINED;
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

// 38. CSSRule Implementation (5 stubs)
JSValue wisp_cssrule_cssText_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_cssrule_cssText_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_cssrule_parentRule_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_cssrule_parentStyleSheet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_cssrule_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// 39. CSSGroupingRule Implementation (3 stubs)
JSValue wisp_cssgroupingrule_deleteRule_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}
JSValue wisp_cssgroupingrule_insertRule_impl(JSContext *ctx, QJSNodePrivate *priv, const char * rule, uint32_t index) {
    return JS_NewInt32(ctx, index);
}
JSValue wisp_cssgroupingrule_cssRules_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 40. CSSPageRule Implementation (3 stubs)
JSValue wisp_csspagerule_selectorText_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_csspagerule_selectorText_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}
JSValue wisp_csspagerule_style_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 41. CSSMediaRule Implementation (1 stub)
JSValue wisp_cssmediarule_media_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// 42. CSSNamespaceRule Implementation (2 stubs)
JSValue wisp_cssnamespacerule_namespaceURI_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}
JSValue wisp_cssnamespacerule_prefix_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// 43. CSSStyleSheet Implementation (4 stubs)
JSValue wisp_cssstylesheet_deleteRule_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}
JSValue wisp_cssstylesheet_insertRule_impl(JSContext *ctx, QJSNodePrivate *priv, const char * rule, uint32_t index) {
    return JS_NewInt32(ctx, index);
}
JSValue wisp_cssstylesheet_cssRules_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}
JSValue wisp_cssstylesheet_ownerRule_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
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
JSValue wisp_cssmarginrule_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | CSSMarginRule::style(user);
JSValue wisp_cssmarginrule_style_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | CSSImportRule::href(string);
JSValue wisp_cssimportrule_href_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | CSSImportRule::media(user);
JSValue wisp_cssimportrule_media_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | CSSImportRule::styleSheet(user);
JSValue wisp_cssimportrule_styleSheet_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | CSSStyleRule::selectorText(string);
JSValue wisp_cssstylerule_selectorText_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: setter | CSSStyleRule::selectorText(string);
JSValue wisp_cssstylerule_selectorText_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

// Overrides: getter | CSSStyleRule::style(user);
JSValue wisp_cssstylerule_style_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: method | CSSRuleList::item();
JSValue wisp_cssrulelist_item_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

// Overrides: getter | CSSRuleList::length(unsigned long);
JSValue wisp_cssrulelist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: getter | StyleSheet::type(string);
JSValue wisp_stylesheet_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | StyleSheet::href(string);
JSValue wisp_stylesheet_href_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | StyleSheet::ownerNode(multiple);
JSValue wisp_stylesheet_ownerNode_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

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
JSValue wisp_element_insertAdjacentHTML_impl(JSContext *ctx, QJSNodePrivate *priv, const char * position, const char * text) {
    return JS_UNDEFINED;
}

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
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::appName(string);
JSValue wisp_workernavigator_appName_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::appVersion(string);
JSValue wisp_workernavigator_appVersion_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: getter | WorkerNavigator::platform(string);
JSValue wisp_workernavigator_platform_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::product(string);
JSValue wisp_workernavigator_product_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::productSub(string);
JSValue wisp_workernavigator_productSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::userAgent(string);
JSValue wisp_workernavigator_userAgent_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::vendor(string);
JSValue wisp_workernavigator_vendor_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::vendorSub(string);
JSValue wisp_workernavigator_vendorSub_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::languages(string);
JSValue wisp_workernavigator_languages_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WorkerNavigator::onLine(boolean);
JSValue wisp_workernavigator_onLine_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// Overrides: getter | SharedWorker::port(user);
JSValue wisp_sharedworker_port_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | SharedWorker::onerror(user);
JSValue wisp_sharedworker_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | SharedWorker::onerror(user);
JSValue wisp_sharedworker_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: method | WorkerGlobalScope::setTimeout();
JSValue wisp_workerglobalscope_setTimeout_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return JS_UNDEFINED;
}
JSValue wisp_workerglobalscope_setTimeout_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    return JS_UNDEFINED;
}

// Overrides: method | WorkerGlobalScope::clearTimeout();
JSValue wisp_workerglobalscope_clearTimeout_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return JS_UNDEFINED;
}

// Overrides: method | WorkerGlobalScope::setInterval();
JSValue wisp_workerglobalscope_setInterval_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) {
    return JS_UNDEFINED;
}
JSValue wisp_workerglobalscope_setInterval_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) {
    return JS_UNDEFINED;
}

// Overrides: method | WorkerGlobalScope::clearInterval();
JSValue wisp_workerglobalscope_clearInterval_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) {
    return JS_UNDEFINED;
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
    return JS_NULL;
}

// Overrides: setter | WorkerGlobalScope::onerror(user);
JSValue wisp_workerglobalscope_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerGlobalScope::onlanguagechange(user);
JSValue wisp_workerglobalscope_onlanguagechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | WorkerGlobalScope::onlanguagechange(user);
JSValue wisp_workerglobalscope_onlanguagechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerGlobalScope::onoffline(user);
JSValue wisp_workerglobalscope_onoffline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | WorkerGlobalScope::onoffline(user);
JSValue wisp_workerglobalscope_onoffline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | WorkerGlobalScope::ononline(user);
JSValue wisp_workerglobalscope_ononline_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | WorkerGlobalScope::ononline(user);
JSValue wisp_workerglobalscope_ononline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
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
    return JS_NULL;
}

// Overrides: setter | SharedWorkerGlobalScope::onconnect(user);
JSValue wisp_sharedworkerglobalscope_onconnect_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
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

// Overrides: getter | WebSocket::url(string);
JSValue wisp_websocket_url_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | WebSocket::readyState(unsigned short);
JSValue wisp_websocket_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: getter | WebSocket::bufferedAmount(unsigned long);
JSValue wisp_websocket_bufferedAmount_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: getter | WebSocket::onopen(user);
JSValue wisp_websocket_onopen_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | WebSocket::onopen(user);
JSValue wisp_websocket_onopen_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | WebSocket::onerror(user);
JSValue wisp_websocket_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | WebSocket::onerror(user);
JSValue wisp_websocket_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | WebSocket::onclose(user);
JSValue wisp_websocket_onclose_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | WebSocket::onclose(user);
JSValue wisp_websocket_onclose_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
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
    return JS_NULL;
}

// Overrides: setter | WebSocket::onmessage(user);
JSValue wisp_websocket_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | WebSocket::binaryType(user);
JSValue wisp_websocket_binaryType_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: setter | WebSocket::binaryType(user);
JSValue wisp_websocket_binaryType_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: method | EventSource::close();
JSValue wisp_eventsource_close_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

// Overrides: getter | EventSource::url(string);
JSValue wisp_eventsource_url_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | EventSource::withCredentials(boolean);
JSValue wisp_eventsource_withCredentials_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_FALSE;
}

// Overrides: getter | EventSource::readyState(unsigned short);
JSValue wisp_eventsource_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

// Overrides: getter | EventSource::onopen(user);
JSValue wisp_eventsource_onopen_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | EventSource::onopen(user);
JSValue wisp_eventsource_onopen_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | EventSource::onmessage(user);
JSValue wisp_eventsource_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | EventSource::onmessage(user);
JSValue wisp_eventsource_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | EventSource::onerror(user);
JSValue wisp_eventsource_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | EventSource::onerror(user);
JSValue wisp_eventsource_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
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
    return JS_FALSE;
}

// Overrides: getter | Navigator::plugins(user);
JSValue wisp_navigator_plugins_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
}

// Overrides: getter | Navigator::mimeTypes(user);
JSValue wisp_navigator_mimeTypes_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "");
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
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::onchecking(user);
JSValue wisp_applicationcache_onchecking_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onerror(user);
JSValue wisp_applicationcache_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::onerror(user);
JSValue wisp_applicationcache_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onnoupdate(user);
JSValue wisp_applicationcache_onnoupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::onnoupdate(user);
JSValue wisp_applicationcache_onnoupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::ondownloading(user);
JSValue wisp_applicationcache_ondownloading_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::ondownloading(user);
JSValue wisp_applicationcache_ondownloading_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onprogress(user);
JSValue wisp_applicationcache_onprogress_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::onprogress(user);
JSValue wisp_applicationcache_onprogress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onupdateready(user);
JSValue wisp_applicationcache_onupdateready_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::onupdateready(user);
JSValue wisp_applicationcache_onupdateready_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::oncached(user);
JSValue wisp_applicationcache_oncached_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::oncached(user);
JSValue wisp_applicationcache_oncached_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
    return JS_UNDEFINED;
}

// Overrides: getter | ApplicationCache::onobsolete(user);
JSValue wisp_applicationcache_onobsolete_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

// Overrides: setter | ApplicationCache::onobsolete(user);
JSValue wisp_applicationcache_onobsolete_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue value) {
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
