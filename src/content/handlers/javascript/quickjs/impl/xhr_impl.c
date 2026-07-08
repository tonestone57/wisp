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
#include "utils/libdom.h"
#include "JSXMLHttpRequest.gen.h"
#include <wisp/content/fetch.h>

JSClassID qjs_xmlhttprequest_class_id;

typedef struct WispXHR {
    JSContext *ctx;
    JSValue self;
    int readyState;
    int status;
    char *statusText;
    char *method;
    nsurl *url;
    bool async;
    struct fetch *fetch_handle;
    uint8_t *response_buf;
    size_t response_len;
    char *response_headers;
    struct fetch_multipart_data *out_headers;
} WispXHR;

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
            if (xhr->fetch_handle) fetch_abort(xhr->fetch_handle);
            free(xhr->statusText);
            free(xhr->method);
            if (xhr->url) nsurl_unref(xhr->url);
            free(xhr->response_buf);
            free(xhr->response_headers);
            fetch_multipart_data_destroy(xhr->out_headers);
            JS_FreeValueRT(rt, xhr->self);
            free(xhr);
        }
        free(priv);
    }
}

static JSClassDef wisp_xmlhttprequest_class = { "XMLHttpRequest", .finalizer = xhr_finalizer, .gc_mark = xhr_mark };

static void xhr_trigger_event(WispXHR *xhr, const char *name)
{
    JSValue on_val = JS_GetPropertyStr(xhr->ctx, xhr->self, name);
    if (JS_IsFunction(xhr->ctx, on_val)) {
        JSValue ret = JS_Call(xhr->ctx, on_val, xhr->self, 0, NULL);
        JS_FreeValue(xhr->ctx, ret);
    }
    JS_FreeValue(xhr->ctx, on_val);
}

static void xhr_set_ready_state(WispXHR *xhr, int state)
{
    xhr->readyState = state;
    xhr_trigger_event(xhr, "onreadystatechange");
}

static void xhr_pipeline_cb(const struct fetch_response *res, void *p)
{
    WispXHR *xhr = p;
    if (!xhr) return;

    xhr->status = (int)res->http_code;
    if (res->header_buf) {
        free(xhr->response_headers);
        xhr->response_headers = malloc(res->header_len + 1);
        if (xhr->response_headers) {
            memcpy(xhr->response_headers, res->header_buf, res->header_len);
            xhr->response_headers[res->header_len] = '\0';
        }
    }

    if (res->data_buf && res->data_len > 0) {
        void *new_buf = realloc(xhr->response_buf, xhr->response_len + res->data_len);
        if (new_buf) {
            xhr->response_buf = new_buf;
            memcpy(xhr->response_buf + xhr->response_len, res->data_buf, res->data_len);
            xhr->response_len += res->data_len;
            xhr_set_ready_state(xhr, 3); /* LOADING */
        }
    } else {
        xhr_set_ready_state(xhr, 4); /* DONE */
    }
}

static JSValue js_xhr_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)
{
    WispXHR *xhr = calloc(1, sizeof(WispXHR));
    if (!xhr) return JS_ThrowOutOfMemory(ctx);
    xhr->ctx = ctx;
    xhr->async = true;

    JSValue obj = qjs_new_xmlhttprequest(ctx, xhr, false);
    if (JS_IsException(obj)) {
        free(xhr);
        return obj;
    }
    xhr->self = JS_DupValue(ctx, obj);
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

    free(xhr->method);
    xhr->method = strdup(method);
    if (xhr->url) nsurl_unref(xhr->url);
    nsurl_create(url, &xhr->url);

    xhr_set_ready_state(xhr, 1); /* OPENED */
    return JS_UNDEFINED;
}

JSValue wisp_xmlhttprequest_send_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    WispXHR *xhr = priv ? priv->node : NULL;
    if (!xhr || !xhr->url) return JS_UNDEFINED;

    struct fetch_request req;
    memset(&req, 0, sizeof(req));
    req.url = xhr->url;
    req.method = xhr->method;

    nserror err = fetch_pipeline_start(&req, xhr_pipeline_cb, xhr, &xhr->fetch_handle);
    if (err != NSERROR_OK) {
        NSLOG(wisp, ERROR, "XHR fetch failed to start: %s", messages_get_errorcode(err));
    }

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
        fetch_abort(xhr->fetch_handle);
        xhr->fetch_handle = NULL;
    }
    return JS_UNDEFINED;
}
JSValue wisp_xmlhttprequest_overrideMimeType_impl(JSContext *ctx, QJSNodePrivate *priv, const char * mime) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_responseType_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, ""); }
JSValue wisp_xmlhttprequest_responseType_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_xmlhttprequest_responseXML_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NULL; }
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

int qjs_init_xhr(JSContext *ctx)
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
    if (!JS_IsRegisteredClass(rt, qjs_xmlhttprequest_class_id)) JS_NewClass(rt, qjs_xmlhttprequest_class_id, &wisp_xmlhttprequest_class);

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
