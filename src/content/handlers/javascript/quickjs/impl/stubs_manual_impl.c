#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <libwapcaplet/libwapcaplet.h>

struct nsurl;
extern const char *nsurl_access(const struct nsurl *url);
extern struct nsurl *content_get_url(void *c);
extern nserror nsurl_create(const char *const url_s, struct nsurl **url);

extern bool wisp_is_js_process;

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
            if (buf) {
                memcpy(buf, data, len);
                buf[len] = ':';
                buf[len + 1] = '\0';
                JSValue res = JS_NewString(ctx, buf);
                free(buf);
                lwc_string_unref(scheme);
                nsurl_unref(url);
                return res;
            }
            lwc_string_unref(scheme);
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
        if (val_len > 0 && value[val_len - 1] == ':') {
            char *tmp = strdup(value);
            tmp[val_len - 1] = '\0';
            lwc_intern_string((const char *)tmp, val_len - 1, &scheme);
            free(tmp);
        } else {
            lwc_intern_string(value, val_len, &scheme);
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

        const char *colon = strchr(value, ':');
        if (colon) {
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
            size_t p_len = port ? lwc_string_length(port) : 0;
            const char *p_data = port ? lwc_string_data(port) : "";

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
// Global redirects for HTMLAnchorElement's Object.defineProperty properties
// -----------------------------------------------------------------------------

static JSValue js_htmlanchorelement_get_property_global(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 2) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, argv[0]);
    if (!priv || !priv->node) return JS_UNDEFINED;

    const char *prop = JS_ToCString(ctx, argv[1]);
    if (!prop) return JS_UNDEFINED;

    JSValue res = JS_NULL;
    if (strcmp(prop, "href") == 0) {
        res = wisp_htmlanchorelement_href_get_impl(ctx, priv);
    } else if (strcmp(prop, "protocol") == 0) {
        res = wisp_htmlanchorelement_protocol_get_impl(ctx, priv);
    } else if (strcmp(prop, "host") == 0) {
        res = wisp_htmlanchorelement_host_get_impl(ctx, priv);
    } else if (strcmp(prop, "hostname") == 0) {
        res = wisp_htmlanchorelement_hostname_get_impl(ctx, priv);
    } else if (strcmp(prop, "port") == 0) {
        res = wisp_htmlanchorelement_port_get_impl(ctx, priv);
    } else if (strcmp(prop, "pathname") == 0) {
        res = wisp_htmlanchorelement_pathname_get_impl(ctx, priv);
    } else if (strcmp(prop, "search") == 0) {
        res = wisp_htmlanchorelement_search_get_impl(ctx, priv);
    } else if (strcmp(prop, "hash") == 0) {
        res = wisp_htmlanchorelement_hash_get_impl(ctx, priv);
    } else if (strcmp(prop, "origin") == 0) {
        res = wisp_htmlanchorelement_origin_get_impl(ctx, priv);
    }

    JS_FreeCString(ctx, prop);
    return res;
}

static JSValue js_htmlanchorelement_set_property_global(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 3) return JS_UNDEFINED;
    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, argv[0]);
    if (!priv || !priv->node) return JS_UNDEFINED;

    const char *prop = JS_ToCString(ctx, argv[1]);
    const char *value = JS_ToCString(ctx, argv[2]);
    if (!prop || !value) {
        if (prop) JS_FreeCString(ctx, prop);
        if (value) JS_FreeCString(ctx, value);
        return JS_UNDEFINED;
    }

    if (strcmp(prop, "href") == 0) {
        wisp_htmlanchorelement_href_set_impl(ctx, priv, value);
    } else if (strcmp(prop, "protocol") == 0) {
        wisp_htmlanchorelement_protocol_set_impl(ctx, priv, value);
    } else if (strcmp(prop, "host") == 0) {
        wisp_htmlanchorelement_host_set_impl(ctx, priv, value);
    } else if (strcmp(prop, "hostname") == 0) {
        wisp_htmlanchorelement_hostname_set_impl(ctx, priv, value);
    } else if (strcmp(prop, "port") == 0) {
        wisp_htmlanchorelement_port_set_impl(ctx, priv, value);
    } else if (strcmp(prop, "pathname") == 0) {
        wisp_htmlanchorelement_pathname_set_impl(ctx, priv, value);
    } else if (strcmp(prop, "search") == 0) {
        wisp_htmlanchorelement_search_set_impl(ctx, priv, value);
    } else if (strcmp(prop, "hash") == 0) {
        wisp_htmlanchorelement_hash_set_impl(ctx, priv, value);
    }

    JS_FreeCString(ctx, prop);
    JS_FreeCString(ctx, value);
    return JS_UNDEFINED;
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
    JS_FreeValue(ctx, proto);

    /* Set up global C helper functions for property redirection */
    JSValue get_fn = JS_NewCFunction(ctx, js_htmlanchorelement_get_property_global, "__wisp_get_anchor_property", 2);
    JS_SetPropertyStr(ctx, global_obj, "__wisp_get_anchor_property", get_fn);

    JSValue set_fn = JS_NewCFunction(ctx, js_htmlanchorelement_set_property_global, "__wisp_set_anchor_property", 3);
    JS_SetPropertyStr(ctx, global_obj, "__wisp_set_anchor_property", set_fn);

    /* JS initialization to define anchor prototype getters/setters */
    const char *init_js =
        "if (typeof HTMLAnchorElement !== 'undefined' && HTMLAnchorElement.prototype) {\n"
        "    const props = ['href', 'protocol', 'host', 'hostname', 'port', 'pathname', 'search', 'hash', 'origin'];\n"
        "    props.forEach(prop => {\n"
        "        Object.defineProperty(HTMLAnchorElement.prototype, prop, {\n"
        "            get() {\n"
        "                return globalThis.__wisp_get_anchor_property(this, prop);\n"
        "            },\n"
        "            set(v) {\n"
        "                if (prop !== 'origin') {\n" // origin is read-only
        "                    globalThis.__wisp_set_anchor_property(this, prop, String(v));\n"
        "                }\n"
        "            },\n"
        "            configurable: true,\n"
        "            enumerable: true\n"
        "        });\n"
        "    });\n"
        "}\n";

    JSValue eval_res = JS_Eval(ctx, init_js, strlen(init_js), "<anchor_init>", JS_EVAL_TYPE_GLOBAL);
    JS_FreeValue(ctx, eval_res);

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
