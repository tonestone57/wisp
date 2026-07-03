#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSWindow.gen.h"
#include "qjs_internal.h"

int qjs_init_window(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_window_init");
    bool already_init = JS_ToBool(ctx, check);
    JS_FreeValue(ctx, check);

    if (already_init) {
        JS_FreeValue(ctx, global_obj);
        return 0;
    }

    qjs_init_window_gen(ctx);

    JSValue proto = JS_GetClassProto(ctx, qjs_window_class_id);
    JSValue et_proto = JS_GetClassProto(ctx, qjs_eventtarget_class_id);
    if (JS_IsObject(proto) && JS_IsObject(et_proto)) {
        JS_SetPrototype(ctx, proto, et_proto);
    }
    JS_FreeValue(ctx, et_proto);
    JS_FreeValue(ctx, proto);

    JS_SetPropertyStr(ctx, global_obj, "__wisp_window_init", JS_TRUE);
    JS_FreeValue(ctx, global_obj);

    return 0;
}

JSValue wisp_window_window_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_GetGlobalObject(ctx);
}

JSValue wisp_window_self_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_GetGlobalObject(ctx);
}

JSValue wisp_window_document_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->doc_priv) {
        return qjs_wrap_node(ctx, (dom_node *)t->doc_priv);
    }
    return JS_NULL;
}

JSValue wisp_window_navigator_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue nav = JS_GetPropertyStr(ctx, global, "navigator");
    JS_FreeValue(ctx, global);
    return nav;
}

JSValue wisp_window_location_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue loc = JS_GetPropertyStr(ctx, global, "location");
    JS_FreeValue(ctx, global);
    return loc;
}

JSValue wisp_window_localStorage_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue store = JS_GetPropertyStr(ctx, global, "localStorage");
    JS_FreeValue(ctx, global);
    return store;
}

JSValue wisp_window_sessionStorage_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue store = JS_GetPropertyStr(ctx, global, "sessionStorage");
    JS_FreeValue(ctx, global);
    return store;
}

JSValue wisp_window_console_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue console = JS_GetPropertyStr(ctx, global, "console");
    JS_FreeValue(ctx, global);
    return console;
}

JSValue wisp_window_alert_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message)
{
    NSLOG(wisp, INFO, "Window.alert: %s", message ? message : "");
    return JS_UNDEFINED;
}

/* Stubs for other methods required by JSWindow.gen.c */
JSValue wisp_window_alert_0_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_alert_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message) { return JS_UNDEFINED; }
JSValue wisp_window___getter___0_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) { return JS_UNDEFINED; }
JSValue wisp_window___getter___1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * name) { return JS_UNDEFINED; }
JSValue wisp_window_atob_impl(JSContext *ctx, QJSNodePrivate *priv, const char * atob) { return JS_UNDEFINED; }
JSValue wisp_window_blur_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_btoa_impl(JSContext *ctx, QJSNodePrivate *priv, const char * btoa) { return JS_UNDEFINED; }
JSValue wisp_window_cancelAnimationFrame_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t handle) { return JS_UNDEFINED; }
JSValue wisp_window_captureEvents_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_clearInterval_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) { return JS_UNDEFINED; }
JSValue wisp_window_clearTimeout_impl(JSContext *ctx, QJSNodePrivate *priv, int32_t handle) { return JS_UNDEFINED; }
JSValue wisp_window_close_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_confirm_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message) { return JS_UNDEFINED; }
JSValue wisp_window_createImageBitmap_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image) { return JS_UNDEFINED; }
JSValue wisp_window_createImageBitmap_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue image, int32_t sx, int32_t sy, int32_t sw, int32_t sh) { return JS_UNDEFINED; }
JSValue wisp_window_focus_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_getComputedStyle_impl(JSContext *ctx, QJSNodePrivate *priv, void * elt, const char * pseudoElt) { return JS_UNDEFINED; }
JSValue wisp_window_open_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url, const char * target, const char * features, bool replace) { return JS_UNDEFINED; }
JSValue wisp_window_postMessage_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue message, const char * targetOrigin, JSValue transfer) { return JS_UNDEFINED; }
JSValue wisp_window_print_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_prompt_impl(JSContext *ctx, QJSNodePrivate *priv, const char * message, const char * default_val) { return JS_UNDEFINED; }
JSValue wisp_window_releaseEvents_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_requestAnimationFrame_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue callback) { return JS_UNDEFINED; }
JSValue wisp_window_setInterval_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) { return JS_UNDEFINED; }
JSValue wisp_window_setInterval_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) { return JS_UNDEFINED; }
JSValue wisp_window_setTimeout_0_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue handler, int32_t timeout, JSValue arguments) { return JS_UNDEFINED; }
JSValue wisp_window_setTimeout_1_impl(JSContext *ctx, QJSNodePrivate *priv, const char * handler, int32_t timeout, JSValue arguments) { return JS_UNDEFINED; }
JSValue wisp_window_showModalDialog_impl(JSContext *ctx, QJSNodePrivate *priv, const char * url, JSValue argument) { return JS_UNDEFINED; }
JSValue wisp_window_stop_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_applicationCache_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_closed_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_external_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_frameElement_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_frames_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_history_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_locationbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_menubar_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_name_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_name_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_window_onabort_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onabort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onafterprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onafterprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onautocomplete_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onautocomplete_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onautocompleteerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onautocompleteerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onbeforeprint_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onbeforeprint_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onbeforeunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onbeforeunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onblur_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onblur_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_oncancel_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_oncancel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_oncanplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_oncanplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_oncanplaythrough_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_oncanplaythrough_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onclose_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onclose_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_oncontextmenu_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_oncontextmenu_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_oncuechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_oncuechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondblclick_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondblclick_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondrag_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondrag_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondragend_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondragend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondragenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondragenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondragexit_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondragexit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondragleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondragleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondragover_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondragover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondragstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondragstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondrop_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondrop_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ondurationchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ondurationchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onemptied_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onemptied_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onended_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onended_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onerror_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onerror_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onfocus_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onfocus_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onhashchange_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onhashchange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_oninput_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_oninput_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_oninvalid_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_oninvalid_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onkeydown_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onkeydown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onkeypress_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onkeypress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onkeyup_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onkeyup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onlanguagechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onlanguagechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onload_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onloadeddata_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onloadeddata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onloadedmetadata_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onloadedmetadata_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onloadstart_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onloadstart_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmessage_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmessage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmousedown_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmousedown_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseenter_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseenter_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseleave_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseleave_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmousemove_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmousemove_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseout_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseout_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseover_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseover_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseup_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onmouseup_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onoffline_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onoffline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ononline_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ononline_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onpagehide_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onpagehide_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onpageshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onpageshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onpause_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onpause_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onplay_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onplay_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onplaying_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onplaying_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onpopstate_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onpopstate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onprogress_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onprogress_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onratechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onratechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onreset_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onreset_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onresize_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onresize_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onscroll_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onscroll_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onseeked_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onseeked_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onseeking_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onseeking_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onselect_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onselect_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onshow_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onshow_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onsort_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onsort_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onstalled_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onstalled_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onstorage_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onstorage_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onsubmit_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onsubmit_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onsuspend_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onsuspend_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ontimeupdate_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ontimeupdate_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_ontoggle_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_ontoggle_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onunload_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onunload_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onvolumechange_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onvolumechange_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onwaiting_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onwaiting_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_onwheel_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_onwheel_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_opener_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_opener_set_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue val) { return JS_UNDEFINED; }
JSValue wisp_window_parent_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_personalbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_scrollbars_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_status_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_status_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) { return JS_UNDEFINED; }
JSValue wisp_window_statusbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_toolbar_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
JSValue wisp_window_top_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_UNDEFINED; }
