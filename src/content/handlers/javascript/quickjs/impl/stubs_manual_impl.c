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
            for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
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
                for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
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
            for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
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
                        for (uint32_t j = 0; j < wisp_shm_dom->node_count; j++) {
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
            for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
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
            for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
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
            for (uint32_t i = 0; i < wisp_shm_dom->node_count; i++) {
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
