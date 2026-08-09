#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/corestrings.h>
#include "utils/libdom.h"
#include "JSXMLHttpRequest.gen.h"
#include <wisp/content/fetch.h>
#include <dom/dom.h>
#include <libdom/bindings/hubbub/parser.h>
#include "JSEvent.gen.h"

JSClassID qjs_xmlhttprequest_class_id;

static void xhr_add_active(JSContext *ctx, WispXHR *xhr);
static void xhr_remove_active(JSContext *ctx, WispXHR *xhr);

static void xhr_mark(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_xmlhttprequest_class_id);
    if (priv && priv->node) {
        WispXHR *xhr = priv->node;
        JS_MarkValue(rt, xhr->self, mark_func);
    }
}

static void xhr_finalizer(JSRuntime *rt, JSValue val)
{
    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_xmlhttprequest_class_id);
    if (priv) {
        WispXHR *xhr = priv->node;
        if (xhr) {
            if (JS_ContextIsAlive(rt, xhr->ctx)) {
                struct jsthread *t = JS_GetContextOpaque(xhr->ctx);
                if (t) {
                    WispXHR **curr = &t->xmlhttprequests;
                    while (*curr) {
                        if (*curr == xhr) {
                            *curr = xhr->next;
                            break;
                        }
                        curr = &((*curr)->next);
                    }
                }
            }
            if (xhr->fetch_handle) {
                fetch_change_callback(xhr->fetch_handle, NULL, NULL);
                fetch_abort(xhr->fetch_handle);
                fetch_free(xhr->fetch_handle);
                xhr->fetch_handle = NULL;
                if (JS_ContextIsAlive(rt, xhr->ctx)) {
                    xhr_remove_active(xhr->ctx, xhr);
                }
            }
            free(xhr->statusText);
            free(xhr->method);
            if (xhr->url) nsurl_unref(xhr->url);
            free(xhr->response_buf);
            free(xhr->response_headers);
            fetch_multipart_data_destroy(xhr->out_headers);
            if (xhr->response_xml) dom_node_unref((dom_node *)xhr->response_xml);
            if (!JS_IsUndefined(xhr->self)) JS_FreeValueRT(rt, xhr->self);
            free(xhr);
        }
        free(priv);
    }
}

static JSClassDef wisp_xmlhttprequest_class = { "XMLHttpRequest", .finalizer = xhr_finalizer, .gc_mark = xhr_mark };

static void xhr_dispatch_event_helper(WispXHR *xhr, const char *type)
{
    JSContext *ctx = xhr->ctx;
    if (!ctx || JS_IsUndefined(xhr->self)) return;

    /* 1. Call on<event> if it exists (e.g., onload, onreadystatechange, onerror) */
    char on_name[64];
    snprintf(on_name, sizeof(on_name), "on%s", type);
    JSValue on_val = JS_GetPropertyStr(ctx, xhr->self, on_name);
    if (JS_IsFunction(ctx, on_val)) {
        JSValue ret = JS_Call(ctx, on_val, xhr->self, 0, NULL);
        JS_FreeValue(ctx, ret);
    }
    JS_FreeValue(ctx, on_val);

    /* 2. Dispatch a proper Event object using dispatchEvent */
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue event_ctor = JS_GetPropertyStr(ctx, global_obj, "Event");
    if (JS_IsFunction(ctx, event_ctor)) {
        JSValue type_val = JS_NewString(ctx, type);
        JSValue ev_obj = JS_CallConstructor(ctx, event_ctor, 1, &type_val);
        JS_FreeValue(ctx, type_val);

        if (!JS_IsException(ev_obj)) {
            JSValue dispatch_fn = JS_GetPropertyStr(ctx, xhr->self, "dispatchEvent");
            if (JS_IsFunction(ctx, dispatch_fn)) {
                JSValue ret = JS_Call(ctx, dispatch_fn, xhr->self, 1, &ev_obj);
                JS_FreeValue(ctx, ret);
            }
            JS_FreeValue(ctx, dispatch_fn);
            JS_FreeValue(ctx, ev_obj);
        }
    }
    JS_FreeValue(ctx, event_ctor);
    JS_FreeValue(ctx, global_obj);
}

static void xhr_set_ready_state(WispXHR *xhr, int state)
{
    if (xhr->readyState != state) {
        xhr->readyState = state;
        xhr_dispatch_event_helper(xhr, "readystatechange");
    }
}

static void xhr_parse_response_xml(WispXHR *xhr)
{
    if (xhr->response_xml) {
        dom_node_unref((dom_node *)xhr->response_xml);
        xhr->response_xml = NULL;
    }

    if (!xhr->response_buf || xhr->response_len == 0) return;

    dom_hubbub_parser_params parse_params;
    memset(&parse_params, 0, sizeof(parse_params));
    parse_params.enc = "UTF-8";
    parse_params.fix_enc = true;
    parse_params.idname = corestring_dom_id;

    dom_hubbub_parser *parser;
    dom_document *doc;
    dom_hubbub_error err = dom_hubbub_parser_create(&parse_params, &parser, &doc);
    if (err == DOM_HUBBUB_OK) {
        dom_hubbub_parser_parse_chunk(parser, xhr->response_buf, xhr->response_len);
        dom_hubbub_parser_completed(parser);
        dom_hubbub_parser_destroy(parser);
        xhr->response_xml = doc;
    }
}

static void xhr_add_active(JSContext *ctx, WispXHR *xhr)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue active_xhrs = JS_GetPropertyStr(ctx, global_obj, "__active_xhrs");
    if (JS_IsUndefined(active_xhrs) || JS_IsNull(active_xhrs)) {
        JS_FreeValue(ctx, active_xhrs);
        active_xhrs = JS_NewArray(ctx);
        JS_SetPropertyStr(ctx, global_obj, "__active_xhrs", JS_DupValue(ctx, active_xhrs));
    }

    JSValue len_val = JS_GetPropertyStr(ctx, active_xhrs, "length");
    uint32_t len = 0;
    JS_ToUint32(ctx, &len, len_val);
    JS_FreeValue(ctx, len_val);

    JS_SetPropertyUint32(ctx, active_xhrs, len, JS_DupValue(ctx, xhr->self));
    JS_FreeValue(ctx, active_xhrs);
    JS_FreeValue(ctx, global_obj);
}

static void xhr_remove_active(JSContext *ctx, WispXHR *xhr)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue active_xhrs = JS_GetPropertyStr(ctx, global_obj, "__active_xhrs");
    if (JS_IsObject(active_xhrs)) {
        JSValue len_val = JS_GetPropertyStr(ctx, active_xhrs, "length");
        uint32_t len = 0;
        JS_ToUint32(ctx, &len, len_val);
        JS_FreeValue(ctx, len_val);

        for (uint32_t i = 0; i < len; i++) {
            JSValue val = JS_GetPropertyUint32(ctx, active_xhrs, i);
            if (JS_IsSameValue(ctx, val, xhr->self)) {
                JS_SetPropertyUint32(ctx, active_xhrs, i, JS_UNDEFINED);
                JS_FreeValue(ctx, val);
                break;
            }
            JS_FreeValue(ctx, val);
        }
    }
    JS_FreeValue(ctx, active_xhrs);
    JS_FreeValue(ctx, global_obj);
}

static void xhr_callback(const struct fetch_msg *msg, void *p)
{
    WispXHR *xhr = p;
    if (!xhr) return;

    /* If the message type is >= FETCH_FINISHED, the fetch is completed/finished.
     * The XHR object is protected from GC during dispatch and will be safely cleaned up
     * by xhr_finalizer when eventually garbage-collected. */
    bool is_terminal = (msg->type >= FETCH_FINISHED);

    switch (msg->type) {
    case FETCH_HEADER: {
        size_t existing_len = xhr->response_headers ? strlen(xhr->response_headers) : 0;
        size_t new_len = existing_len + msg->data.header_or_data.len;
        char *new_headers = realloc(xhr->response_headers, new_len + 1);
        if (new_headers) {
            memcpy(new_headers + existing_len, msg->data.header_or_data.buf, msg->data.header_or_data.len);
            new_headers[new_len] = '\0';
            xhr->response_headers = new_headers;
        }

        /* Try to extract status code and text if we haven't yet */
        if (xhr->readyState < 2) {
            xhr->status = (int)fetch_http_code(xhr->fetch_handle);
            if (xhr->status != 0) {
                /* Try to find status text in headers */
                if (xhr->response_headers && !xhr->statusText) {
                    char *line_end = strchr(xhr->response_headers, '\r');
                    if (!line_end) line_end = strchr(xhr->response_headers, '\n');
                    if (line_end) {
                        char *status_line = strndup(xhr->response_headers, line_end - xhr->response_headers);
                        if (status_line) {
                            /* Format is usually "HTTP/1.1 200 OK" */
                            char *p = strchr(status_line, ' ');
                            if (p) {
                                p++;
                                p = strchr(p, ' ');
                                if (p) {
                                    p++;
                                    free(xhr->statusText);
                                    xhr->statusText = strdup(p);
                                }
                            }
                            free(status_line);
                        }
                    }
                }
                xhr_set_ready_state(xhr, 2); /* HEADERS_RECEIVED */
            }
        }
        break;
    }

    case FETCH_DATA: {
        if (msg->data.header_or_data.len > 0) {
            size_t required = xhr->response_len + msg->data.header_or_data.len;
            if (required > xhr->response_alloc) {
                size_t new_alloc = xhr->response_alloc ? xhr->response_alloc * 2 : 1024;
                while (new_alloc < required) new_alloc *= 2;
                void *new_buf = realloc(xhr->response_buf, new_alloc);
                if (new_buf) {
                    xhr->response_buf = new_buf;
                    xhr->response_alloc = new_alloc;
                } else {
                    return;
                }
            }
            memcpy(xhr->response_buf + xhr->response_len, msg->data.header_or_data.buf, msg->data.header_or_data.len);
            xhr->response_len += msg->data.header_or_data.len;
            xhr_set_ready_state(xhr, 3); /* LOADING */
        }
        break;
    }

    case FETCH_FINISHED:
        if (xhr->fetch_handle) {
            xhr->status = (int)fetch_http_code(xhr->fetch_handle);
        }
        break;

    default:
        break;
    }

    if (is_terminal) {
        xhr_set_ready_state(xhr, 4); /* DONE - protected from GC during dispatch */
        if (msg->type == FETCH_FINISHED) {
            xhr_dispatch_event_helper(xhr, "load");
        } else if (msg->type == FETCH_TIMEDOUT) {
            xhr_dispatch_event_helper(xhr, "timeout");
        } else {
            xhr_dispatch_event_helper(xhr, "error");
        }
        xhr_dispatch_event_helper(xhr, "loadend");
        xhr_remove_active(xhr->ctx, xhr); /* Unprotect now that callbacks are complete */
    }
}

static JSValue js_xhr_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    WispXHR *xhr = calloc(1, sizeof(WispXHR));
    if (!xhr) return JS_ThrowOutOfMemory(ctx);
    xhr->ctx = ctx;
    xhr->async = true;
    xhr->self = JS_UNDEFINED;

    JSValue obj = qjs_new_xmlhttprequest(ctx, xhr, false);
    if (JS_IsException(obj)) {
        free(xhr);
        return obj;
    }
    xhr->self = JS_DupValue(ctx, obj);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t) {
        xhr->next = t->xmlhttprequests;
        t->xmlhttprequests = xhr;
    }

    return obj;
}

JSValue wisp_xmlhttprequest_readyState_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    return JS_NewInt32(ctx, xhr ? xhr->readyState : 0);
}

JSValue wisp_xmlhttprequest_status_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    return JS_NewInt32(ctx, xhr ? xhr->status : 0);
}

JSValue wisp_xmlhttprequest_statusText_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    return JS_NewString(ctx, (xhr && xhr->statusText) ? xhr->statusText : "");
}

JSValue wisp_xmlhttprequest_responseText_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr || !xhr->response_buf) return JS_NewString(ctx, "");
    return JS_NewStringLen(ctx, (const char *)xhr->response_buf, xhr->response_len);
}

JSValue wisp_xmlhttprequest_open_impl(JSContext *ctx, QJSNodePrivate *priv, const char * method, const char * url)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr) return JS_UNDEFINED;

    char *new_method = strdup(method);
    if (!new_method) return JS_ThrowOutOfMemory(ctx);

    nsurl *new_url = NULL;
    nserror err = nsurl_create(url, &new_url);
    if (err != NSERROR_OK) {
        free(new_method);
        return JS_ThrowInternalError(ctx, "Invalid URL: %s", url);
    }

    if (xhr->method) free(xhr->method);
    xhr->method = new_method;

    if (xhr->url) nsurl_unref(xhr->url);
    xhr->url = new_url;

    xhr_set_ready_state(xhr, 1); /* OPENED */
    return JS_UNDEFINED;
}

JSValue wisp_xmlhttprequest_send_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr || !xhr->url) return JS_UNDEFINED;

    struct fetch_postdata post;
    memset(&post, 0, sizeof(post));

    JSValue body_val = JS_GetPropertyStr(ctx, xhr->self, "__body");
    if (JS_IsString(body_val) || JS_IsObject(body_val)) {
        const char *body_str = JS_ToCString(ctx, body_val);
        if (body_str) {
            post.type = FETCH_POSTDATA_URLENC;
            post.data.urlenc = (char *)body_str;
        } else {
            post.type = FETCH_POSTDATA_NONE;
        }
    } else if (xhr->out_headers) {
        post.type = FETCH_POSTDATA_MULTIPART;
        post.data.multipart = xhr->out_headers;
    } else {
        post.type = FETCH_POSTDATA_NONE;
    }

    /* Convert out_headers linked list to the format fetch_start expects (array of strings) */
    int header_count = 0;
    struct fetch_multipart_data *h = xhr->out_headers;
    while (h) {
        header_count++;
        h = h->next;
    }

    const char **headers = calloc((header_count * 2) + 1, sizeof(char *));
    if (headers) {
        h = xhr->out_headers;
        for (int i = 0; i < header_count; i++) {
            headers[i * 2] = h->name;
            headers[i * 2 + 1] = h->value;
            h = h->next;
        }
        headers[header_count * 2] = NULL;
    }

    nserror err = fetch_start(xhr->url, NULL, xhr_callback, xhr, false, &post, false, false, headers, &xhr->fetch_handle);
    if (err != NSERROR_OK) {
        NSLOG(wisp, ERROR, "XHR fetch failed to start: %s", messages_get_errorcode(err));
    } else {
        xhr_add_active(ctx, xhr);
    }

    if (post.type == FETCH_POSTDATA_URLENC && post.data.urlenc) {
        JS_FreeCString(ctx, post.data.urlenc);
    }
    JS_FreeValue(ctx, body_val);

    free(headers);
    return JS_UNDEFINED;
}

JSValue wisp_xmlhttprequest_setRequestHeader_impl(JSContext *ctx, QJSNodePrivate *priv, const char * header, const char * value)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr || xhr->readyState != 1) return JS_UNDEFINED;
    fetch_multipart_data_new_kv(&xhr->out_headers, header, value);
    return JS_UNDEFINED;
}

JSValue wisp_xmlhttprequest_getResponseHeader_impl(JSContext *ctx, QJSNodePrivate *priv, const char * header)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr || !xhr->response_headers) return JS_NULL;
    char *found = strcasestr(xhr->response_headers, header);
    if (found) {
        char *end = strchr(found, '\r');
        if (!end) end = strchr(found, '\n');
        char *colon = strchr(found, ':');
        if (colon && (end == NULL || colon < end)) {
            colon++;
            while (*colon == ' ') colon++;
            int len = end ? (int)(end - colon) : (int)strlen(colon);
            return JS_NewStringLen(ctx, colon, len);
        }
    }
    return JS_NULL;
}

JSValue wisp_xmlhttprequest_getAllResponseHeaders_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    return JS_NewString(ctx, (xhr && xhr->response_headers) ? xhr->response_headers : "");
}

JSValue wisp_xmlhttprequest_abort_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispXHR *xhr = priv ? priv->node : NULL;
    if (xhr && xhr->fetch_handle) {
        fetch_change_callback(xhr->fetch_handle, NULL, NULL);
        fetch_abort(xhr->fetch_handle);
        fetch_free(xhr->fetch_handle);
        xhr->fetch_handle = NULL;
        xhr_remove_active(ctx, xhr);
    }
    return JS_UNDEFINED;
}
JSValue wisp_xmlhttprequest_overrideMimeType_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mime) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_responseType_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_responseType_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }

JSValue wisp_xmlhttprequest_responseXML_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr || xhr->readyState != 4 || !xhr->response_buf) return JS_NULL;

    if (!xhr->response_xml) {
        xhr_parse_response_xml(xhr);
    }

    return xhr->response_xml ? qjs_wrap_node(ctx, (dom_node *)xhr->response_xml) : JS_NULL;
}
JSValue wisp_xmlhttprequest_response_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return wisp_xmlhttprequest_responseText_get_impl(ctx, priv); }
JSValue wisp_xmlhttprequest_timeout_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewInt32(ctx, 0); }
JSValue wisp_xmlhttprequest_timeout_set_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t value) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_withCredentials_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_FALSE; }
JSValue wisp_xmlhttprequest_withCredentials_set_impl(JSContext *ctx, QJSNodePrivate *priv, bool value) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_upload_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
JSValue wisp_xmlhttprequest_responseURL_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr || !xhr->url) return JS_NewString(ctx, "");
    return JS_NewString(ctx, nsurl_access(xhr->url));
}

int qjs_init_xmlhttprequest(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_xhr_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JSRuntime *rt = JS_GetRuntime(ctx);
    if (qjs_xmlhttprequest_class_id == 0) JS_NewClassID(rt, &qjs_xmlhttprequest_class_id);
    static JSClassDef wisp_xmlhttprequest_class_manual = { "XMLHttpRequest", .finalizer = xhr_finalizer, .gc_mark = xhr_mark };
    if (!JS_IsRegisteredClass(rt, qjs_xmlhttprequest_class_id)) JS_NewClass(rt, qjs_xmlhttprequest_class_id, &wisp_xmlhttprequest_class_manual);

    qjs_init_xmlhttprequest_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_xmlhttprequest_class_id);
    if (!JS_IsObject(proto)) {
        JS_FreeValue(ctx, proto);
        proto = JS_NewObject(ctx);
        JS_SetClassProto(ctx, qjs_xmlhttprequest_class_id, JS_DupValue(ctx, proto));
    }
    JSValue ctor = JS_NewCFunction2(ctx, js_xhr_constructor, "XMLHttpRequest", 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetPropertyStr(ctx, global_obj, "XMLHttpRequest", ctor);
    JS_FreeValue(ctx, proto);

    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_xhr_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
