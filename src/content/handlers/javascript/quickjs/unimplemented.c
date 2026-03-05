/*
 * Copyright 2025 Neosurf Contributors
 *
 * This file is part of NeoSurf, http://www.netsurf-browser.org/
 */

#include "unimplemented.h"
#include <wisp/utils/log.h>
#include "quickjs.h"

static JSValue js_urlsearchparams_append(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URLSearchParams.append() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_urlsearchparams_set(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URLSearchParams.set() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_urlsearchparams_has(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URLSearchParams.has() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_urlsearchparams_delete(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URLSearchParams.delete() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_urlsearchparams_get(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URLSearchParams.get() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_urlsearchparams_getAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URLSearchParams.getAll() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_urlsearchparams(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "append", JS_NewCFunction(ctx, js_urlsearchparams_append, "append", 0));
    JS_SetPropertyStr(ctx, obj, "set", JS_NewCFunction(ctx, js_urlsearchparams_set, "set", 0));
    JS_SetPropertyStr(ctx, obj, "has", JS_NewCFunction(ctx, js_urlsearchparams_has, "has", 0));
    JS_SetPropertyStr(ctx, obj, "delete", JS_NewCFunction(ctx, js_urlsearchparams_delete, "delete", 0));
    JS_SetPropertyStr(ctx, obj, "get", JS_NewCFunction(ctx, js_urlsearchparams_get, "get", 0));
    JS_SetPropertyStr(ctx, obj, "getAll", JS_NewCFunction(ctx, js_urlsearchparams_getAll, "getAll", 0));
    JS_SetPropertyStr(ctx, global_obj, "URLSearchParams", obj);
    return 0;
}

static JSValue js_url_origin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.origin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_pathname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.pathname getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_searchParams_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.searchParams getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_protocol_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.protocol getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_password_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.password getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_host_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.host getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_port_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.port getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_search_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.search getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_username_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.username getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_hash_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.hash getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_href_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.href getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_hostname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.hostname getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_pathname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.pathname setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_searchParams_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.searchParams setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_protocol_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.protocol setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_password_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.password setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_host_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.host setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_port_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.port setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_search_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.search setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_username_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.username setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_hash_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.hash setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_href_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.href setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_hostname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.hostname setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_domainToUnicode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.domainToUnicode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_url_domainToASCII(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "URL.domainToASCII() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_url(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "origin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_origin_getter, "origin", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "pathname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_pathname_getter, "pathname", 0),
            JS_NewCFunction(ctx, js_url_pathname_setter, "pathname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "searchParams");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_searchParams_getter, "searchParams", 0),
            JS_NewCFunction(ctx, js_url_searchParams_setter, "searchParams", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "protocol");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_protocol_getter, "protocol", 0),
            JS_NewCFunction(ctx, js_url_protocol_setter, "protocol", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "password");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_password_getter, "password", 0),
            JS_NewCFunction(ctx, js_url_password_setter, "password", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "host");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_host_getter, "host", 0),
            JS_NewCFunction(ctx, js_url_host_setter, "host", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "port");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_port_getter, "port", 0),
            JS_NewCFunction(ctx, js_url_port_setter, "port", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "search");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_search_getter, "search", 0),
            JS_NewCFunction(ctx, js_url_search_setter, "search", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "username");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_username_getter, "username", 0),
            JS_NewCFunction(ctx, js_url_username_setter, "username", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hash");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_hash_getter, "hash", 0),
            JS_NewCFunction(ctx, js_url_hash_setter, "hash", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "href");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_href_getter, "href", 0),
            JS_NewCFunction(ctx, js_url_href_setter, "href", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hostname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_url_hostname_getter, "hostname", 0),
            JS_NewCFunction(ctx, js_url_hostname_setter, "hostname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "domainToUnicode", JS_NewCFunction(ctx, js_url_domainToUnicode, "domainToUnicode", 0));
    JS_SetPropertyStr(ctx, obj, "domainToASCII", JS_NewCFunction(ctx, js_url_domainToASCII, "domainToASCII", 0));
    JS_SetPropertyStr(ctx, global_obj, "URL", obj);
    return 0;
}

static JSValue js_event_timeStamp_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Event.timeStamp getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_event(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "timeStamp");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_event_timeStamp_getter, "timeStamp", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "Event", obj);
    return 0;
}

static JSValue js_mutationevent_newValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationEvent.newValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationevent_prevValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationEvent.prevValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationevent_attrName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationEvent.attrName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationevent_attrChange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationEvent.attrChange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationevent_relatedNode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationEvent.relatedNode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationevent_initMutationEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationEvent.initMutationEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mutationevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "newValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationevent_newValue_getter, "newValue", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "prevValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationevent_prevValue_getter, "prevValue", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "attrName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationevent_attrName_getter, "attrName", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "attrChange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationevent_attrChange_getter, "attrChange", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "relatedNode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationevent_relatedNode_getter, "relatedNode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initMutationEvent", JS_NewCFunction(ctx, js_mutationevent_initMutationEvent, "initMutationEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "MutationEvent", obj);
    return 0;
}

static JSValue js_uievent_view_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "UIEvent.view getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_uievent_detail_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "UIEvent.detail getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_uievent_initUIEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "UIEvent.initUIEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_uievent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "view");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_uievent_view_getter, "view", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "detail");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_uievent_detail_getter, "detail", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initUIEvent", JS_NewCFunction(ctx, js_uievent_initUIEvent, "initUIEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "UIEvent", obj);
    return 0;
}

static JSValue js_compositionevent_data_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CompositionEvent.data getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_compositionevent_initCompositionEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CompositionEvent.initCompositionEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_compositionevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "data");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_compositionevent_data_getter, "data", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initCompositionEvent", JS_NewCFunction(ctx, js_compositionevent_initCompositionEvent, "initCompositionEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "CompositionEvent", obj);
    return 0;
}

static JSValue js_keyboardevent_keyCode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "KeyboardEvent.keyCode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_keyboardevent_charCode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "KeyboardEvent.charCode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_keyboardevent_isComposing_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "KeyboardEvent.isComposing getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_keyboardevent_repeat_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "KeyboardEvent.repeat getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_keyboardevent_which_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "KeyboardEvent.which getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_keyboardevent_initKeyboardEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "KeyboardEvent.initKeyboardEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_keyboardevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "keyCode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_keyboardevent_keyCode_getter, "keyCode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "charCode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_keyboardevent_charCode_getter, "charCode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "isComposing");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_keyboardevent_isComposing_getter, "isComposing", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "repeat");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_keyboardevent_repeat_getter, "repeat", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "which");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_keyboardevent_which_getter, "which", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initKeyboardEvent", JS_NewCFunction(ctx, js_keyboardevent_initKeyboardEvent, "initKeyboardEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "KeyboardEvent", obj);
    return 0;
}

static JSValue js_mouseevent_relatedTarget_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.relatedTarget getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_buttons_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.buttons getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_screenX_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.screenX getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_button_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.button getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_altKey_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.altKey getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_region_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.region getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_clientX_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.clientX getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_metaKey_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.metaKey getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_screenY_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.screenY getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_shiftKey_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.shiftKey getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_ctrlKey_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.ctrlKey getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_clientY_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.clientY getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_initMouseEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.initMouseEvent() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mouseevent_getModifierState(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MouseEvent.getModifierState() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mouseevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "relatedTarget");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_relatedTarget_getter, "relatedTarget", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "buttons");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_buttons_getter, "buttons", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "screenX");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_screenX_getter, "screenX", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "button");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_button_getter, "button", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "altKey");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_altKey_getter, "altKey", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "region");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_region_getter, "region", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "clientX");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_clientX_getter, "clientX", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "metaKey");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_metaKey_getter, "metaKey", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "screenY");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_screenY_getter, "screenY", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "shiftKey");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_shiftKey_getter, "shiftKey", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ctrlKey");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_ctrlKey_getter, "ctrlKey", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "clientY");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mouseevent_clientY_getter, "clientY", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initMouseEvent", JS_NewCFunction(ctx, js_mouseevent_initMouseEvent, "initMouseEvent", 0));
    JS_SetPropertyStr(ctx, obj, "getModifierState", JS_NewCFunction(ctx, js_mouseevent_getModifierState, "getModifierState", 0));
    JS_SetPropertyStr(ctx, global_obj, "MouseEvent", obj);
    return 0;
}

static JSValue js_wheelevent_deltaZ_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WheelEvent.deltaZ getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_wheelevent_deltaMode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WheelEvent.deltaMode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_wheelevent_deltaY_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WheelEvent.deltaY getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_wheelevent_deltaX_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WheelEvent.deltaX getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_wheelevent_initWheelEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WheelEvent.initWheelEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_wheelevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "deltaZ");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_wheelevent_deltaZ_getter, "deltaZ", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "deltaMode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_wheelevent_deltaMode_getter, "deltaMode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "deltaY");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_wheelevent_deltaY_getter, "deltaY", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "deltaX");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_wheelevent_deltaX_getter, "deltaX", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initWheelEvent", JS_NewCFunction(ctx, js_wheelevent_initWheelEvent, "initWheelEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "WheelEvent", obj);
    return 0;
}

static JSValue js_focusevent_relatedTarget_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "FocusEvent.relatedTarget getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_focusevent_initFocusEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "FocusEvent.initFocusEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_focusevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "relatedTarget");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_focusevent_relatedTarget_getter, "relatedTarget", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initFocusEvent", JS_NewCFunction(ctx, js_focusevent_initFocusEvent, "initFocusEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "FocusEvent", obj);
    return 0;
}

static JSValue js_css_escape(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSS.escape() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_css(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "escape", JS_NewCFunction(ctx, js_css_escape, "escape", 0));
    JS_SetPropertyStr(ctx, global_obj, "CSS", obj);
    return 0;
}

static JSValue js_pseudoelement_cascadedStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PseudoElement.cascadedStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_pseudoelement_usedStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PseudoElement.usedStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_pseudoelement_rawComputedStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PseudoElement.rawComputedStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_pseudoelement_defaultStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PseudoElement.defaultStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_pseudoelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "cascadedStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_pseudoelement_cascadedStyle_getter, "cascadedStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "usedStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_pseudoelement_usedStyle_getter, "usedStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "rawComputedStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_pseudoelement_rawComputedStyle_getter, "rawComputedStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "defaultStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_pseudoelement_defaultStyle_getter, "defaultStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "PseudoElement", obj);
    return 0;
}

static JSValue js_svgelement_style_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SVGElement.style getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_svgelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "style");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_svgelement_style_getter, "style", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "SVGElement", obj);
    return 0;
}

static JSValue js_cssstyledeclaration_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_dashed_attribute_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.dashed_attribute getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_parentRule_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.parentRule getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_cssFloat_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.cssFloat getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_cssText_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.cssText getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_cssText_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.cssText setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_dashed_attribute_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.dashed_attribute setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_cssFloat_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.cssFloat setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_removeProperty(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.removeProperty() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.item() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_getPropertyPriority(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.getPropertyPriority() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_setPropertyPriority(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.setPropertyPriority() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_setProperty(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.setProperty() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_getPropertyValue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.getPropertyValue() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstyledeclaration_setPropertyValue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleDeclaration.setPropertyValue() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssstyledeclaration(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstyledeclaration_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "dashed_attribute");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstyledeclaration_dashed_attribute_getter, "dashed_attribute", 0),
            JS_NewCFunction(ctx, js_cssstyledeclaration_dashed_attribute_setter, "dashed_attribute", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "parentRule");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstyledeclaration_parentRule_getter, "parentRule", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cssFloat");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstyledeclaration_cssFloat_getter, "cssFloat", 0),
            JS_NewCFunction(ctx, js_cssstyledeclaration_cssFloat_setter, "cssFloat", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cssText");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstyledeclaration_cssText_getter, "cssText", 0),
            JS_NewCFunction(ctx, js_cssstyledeclaration_cssText_setter, "cssText", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "removeProperty", JS_NewCFunction(ctx, js_cssstyledeclaration_removeProperty, "removeProperty", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_cssstyledeclaration_item, "item", 0));
    JS_SetPropertyStr(ctx, obj, "getPropertyPriority", JS_NewCFunction(ctx, js_cssstyledeclaration_getPropertyPriority, "getPropertyPriority", 0));
    JS_SetPropertyStr(ctx, obj, "setPropertyPriority", JS_NewCFunction(ctx, js_cssstyledeclaration_setPropertyPriority, "setPropertyPriority", 0));
    JS_SetPropertyStr(ctx, obj, "setProperty", JS_NewCFunction(ctx, js_cssstyledeclaration_setProperty, "setProperty", 0));
    JS_SetPropertyStr(ctx, obj, "getPropertyValue", JS_NewCFunction(ctx, js_cssstyledeclaration_getPropertyValue, "getPropertyValue", 0));
    JS_SetPropertyStr(ctx, obj, "setPropertyValue", JS_NewCFunction(ctx, js_cssstyledeclaration_setPropertyValue, "setPropertyValue", 0));
    JS_SetPropertyStr(ctx, global_obj, "CSSStyleDeclaration", obj);
    return 0;
}

static JSValue js_cssrule_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSRule.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssrule_cssText_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSRule.cssText getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssrule_parentStyleSheet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSRule.parentStyleSheet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssrule_parentRule_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSRule.parentRule getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssrule_cssText_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSRule.cssText setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssrule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssrule_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cssText");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssrule_cssText_getter, "cssText", 0),
            JS_NewCFunction(ctx, js_cssrule_cssText_setter, "cssText", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "parentStyleSheet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssrule_parentStyleSheet_getter, "parentStyleSheet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "parentRule");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssrule_parentRule_getter, "parentRule", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CSSRule", obj);
    return 0;
}

static JSValue js_cssnamespacerule_namespaceURI_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSNamespaceRule.namespaceURI getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssnamespacerule_prefix_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSNamespaceRule.prefix getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssnamespacerule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "namespaceURI");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssnamespacerule_namespaceURI_getter, "namespaceURI", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "prefix");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssnamespacerule_prefix_getter, "prefix", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CSSNamespaceRule", obj);
    return 0;
}

static JSValue js_cssmarginrule_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSMarginRule.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssmarginrule_style_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSMarginRule.style getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssmarginrule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssmarginrule_name_getter, "name", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "style");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssmarginrule_style_getter, "style", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CSSMarginRule", obj);
    return 0;
}

static JSValue js_cssgroupingrule_cssRules_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSGroupingRule.cssRules getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssgroupingrule_deleteRule(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSGroupingRule.deleteRule() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssgroupingrule_insertRule(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSGroupingRule.insertRule() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssgroupingrule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "cssRules");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssgroupingrule_cssRules_getter, "cssRules", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "deleteRule", JS_NewCFunction(ctx, js_cssgroupingrule_deleteRule, "deleteRule", 0));
    JS_SetPropertyStr(ctx, obj, "insertRule", JS_NewCFunction(ctx, js_cssgroupingrule_insertRule, "insertRule", 0));
    JS_SetPropertyStr(ctx, global_obj, "CSSGroupingRule", obj);
    return 0;
}

static JSValue js_csspagerule_style_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSPageRule.style getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_csspagerule_selectorText_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSPageRule.selectorText getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_csspagerule_selectorText_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSPageRule.selectorText setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_csspagerule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "style");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_csspagerule_style_getter, "style", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectorText");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_csspagerule_selectorText_getter, "selectorText", 0),
            JS_NewCFunction(ctx, js_csspagerule_selectorText_setter, "selectorText", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CSSPageRule", obj);
    return 0;
}

static JSValue js_cssmediarule_media_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSMediaRule.media getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssmediarule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "media");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssmediarule_media_getter, "media", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CSSMediaRule", obj);
    return 0;
}

static JSValue js_cssimportrule_media_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSImportRule.media getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssimportrule_styleSheet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSImportRule.styleSheet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssimportrule_href_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSImportRule.href getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssimportrule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "media");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssimportrule_media_getter, "media", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "styleSheet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssimportrule_styleSheet_getter, "styleSheet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "href");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssimportrule_href_getter, "href", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CSSImportRule", obj);
    return 0;
}

static JSValue js_cssstylerule_style_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleRule.style getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstylerule_selectorText_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleRule.selectorText getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstylerule_selectorText_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleRule.selectorText setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssstylerule(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "style");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstylerule_style_getter, "style", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectorText");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstylerule_selectorText_getter, "selectorText", 0),
            JS_NewCFunction(ctx, js_cssstylerule_selectorText_setter, "selectorText", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CSSStyleRule", obj);
    return 0;
}

static JSValue js_cssrulelist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSRuleList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssrulelist_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSRuleList.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssrulelist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssrulelist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_cssrulelist_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "CSSRuleList", obj);
    return 0;
}

static JSValue js_stylesheetlist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheetList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheetlist_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheetList.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_stylesheetlist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheetlist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_stylesheetlist_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "StyleSheetList", obj);
    return 0;
}

static JSValue js_stylesheet_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheet_disabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.disabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheet_title_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.title getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheet_href_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.href getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheet_media_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.media getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheet_ownerNode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.ownerNode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheet_parentStyleSheet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.parentStyleSheet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_stylesheet_disabled_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StyleSheet.disabled setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_stylesheet(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheet_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "disabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheet_disabled_getter, "disabled", 0),
            JS_NewCFunction(ctx, js_stylesheet_disabled_setter, "disabled", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "title");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheet_title_getter, "title", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "href");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheet_href_getter, "href", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "media");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheet_media_getter, "media", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ownerNode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheet_ownerNode_getter, "ownerNode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "parentStyleSheet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_stylesheet_parentStyleSheet_getter, "parentStyleSheet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "StyleSheet", obj);
    return 0;
}

static JSValue js_cssstylesheet_ownerRule_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleSheet.ownerRule getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstylesheet_cssRules_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleSheet.cssRules getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstylesheet_deleteRule(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleSheet.deleteRule() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_cssstylesheet_insertRule(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CSSStyleSheet.insertRule() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_cssstylesheet(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "ownerRule");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstylesheet_ownerRule_getter, "ownerRule", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cssRules");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_cssstylesheet_cssRules_getter, "cssRules", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "deleteRule", JS_NewCFunction(ctx, js_cssstylesheet_deleteRule, "deleteRule", 0));
    JS_SetPropertyStr(ctx, obj, "insertRule", JS_NewCFunction(ctx, js_cssstylesheet_insertRule, "insertRule", 0));
    JS_SetPropertyStr(ctx, global_obj, "CSSStyleSheet", obj);
    return 0;
}

static JSValue js_medialist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_medialist_mediaText_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaList.mediaText getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_medialist_mediaText_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaList.mediaText setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_medialist_deleteMedium(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaList.deleteMedium() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_medialist_appendMedium(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaList.appendMedium() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_medialist_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaList.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_medialist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_medialist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "mediaText");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_medialist_mediaText_getter, "mediaText", 0),
            JS_NewCFunction(ctx, js_medialist_mediaText_setter, "mediaText", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "deleteMedium", JS_NewCFunction(ctx, js_medialist_deleteMedium, "deleteMedium", 0));
    JS_SetPropertyStr(ctx, obj, "appendMedium", JS_NewCFunction(ctx, js_medialist_appendMedium, "appendMedium", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_medialist_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "MediaList", obj);
    return 0;
}

static JSValue js_element_localName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.localName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_outerHTML_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.outerHTML getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_tagName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.tagName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_namespaceURI_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.namespaceURI getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_prefix_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.prefix getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_rawComputedStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.rawComputedStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_children_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.children getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_cascadedStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.cascadedStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_usedStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.usedStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_defaultStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.defaultStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_outerHTML_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.outerHTML setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_getElementsByTagNameNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getElementsByTagNameNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_getAttributeNodeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getAttributeNodeNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.remove() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_replaceWith(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.replaceWith() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_closest(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.closest() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_removeAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.removeAttributeNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_setAttributeNodeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.setAttributeNodeNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_prepend(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.prepend() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_query(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.query() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_setAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.setAttributeNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_setAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.setAttributeNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_querySelector(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.querySelector() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_after(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.after() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_append(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.append() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_getElementsByClassName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getElementsByClassName() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_hasAttributes(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.hasAttributes() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_insertAdjacentHTML(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.insertAdjacentHTML() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_queryAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.queryAll() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_getAttributeNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getAttributeNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_hasAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.hasAttributeNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_before(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.before() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_matches(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.matches() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_removeAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.removeAttributeNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_querySelectorAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.querySelectorAll() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_pseudo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.pseudo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_element_getAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Element.getAttributeNS() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_element(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "localName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_localName_getter, "localName", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "outerHTML");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_outerHTML_getter, "outerHTML", 0),
            JS_NewCFunction(ctx, js_element_outerHTML_setter, "outerHTML", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "tagName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_tagName_getter, "tagName", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "namespaceURI");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_namespaceURI_getter, "namespaceURI", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "prefix");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_prefix_getter, "prefix", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "rawComputedStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_rawComputedStyle_getter, "rawComputedStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "children");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_children_getter, "children", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cascadedStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_cascadedStyle_getter, "cascadedStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "usedStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_usedStyle_getter, "usedStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "defaultStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_element_defaultStyle_getter, "defaultStyle", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getElementsByTagNameNS", JS_NewCFunction(ctx, js_element_getElementsByTagNameNS, "getElementsByTagNameNS", 0));
    JS_SetPropertyStr(ctx, obj, "getAttributeNodeNS", JS_NewCFunction(ctx, js_element_getAttributeNodeNS, "getAttributeNodeNS", 0));
    JS_SetPropertyStr(ctx, obj, "remove", JS_NewCFunction(ctx, js_element_remove, "remove", 0));
    JS_SetPropertyStr(ctx, obj, "replaceWith", JS_NewCFunction(ctx, js_element_replaceWith, "replaceWith", 0));
    JS_SetPropertyStr(ctx, obj, "closest", JS_NewCFunction(ctx, js_element_closest, "closest", 0));
    JS_SetPropertyStr(ctx, obj, "removeAttributeNode", JS_NewCFunction(ctx, js_element_removeAttributeNode, "removeAttributeNode", 0));
    JS_SetPropertyStr(ctx, obj, "setAttributeNodeNS", JS_NewCFunction(ctx, js_element_setAttributeNodeNS, "setAttributeNodeNS", 0));
    JS_SetPropertyStr(ctx, obj, "prepend", JS_NewCFunction(ctx, js_element_prepend, "prepend", 0));
    JS_SetPropertyStr(ctx, obj, "query", JS_NewCFunction(ctx, js_element_query, "query", 0));
    JS_SetPropertyStr(ctx, obj, "setAttributeNode", JS_NewCFunction(ctx, js_element_setAttributeNode, "setAttributeNode", 0));
    JS_SetPropertyStr(ctx, obj, "setAttributeNS", JS_NewCFunction(ctx, js_element_setAttributeNS, "setAttributeNS", 0));
    JS_SetPropertyStr(ctx, obj, "querySelector", JS_NewCFunction(ctx, js_element_querySelector, "querySelector", 0));
    JS_SetPropertyStr(ctx, obj, "after", JS_NewCFunction(ctx, js_element_after, "after", 0));
    JS_SetPropertyStr(ctx, obj, "append", JS_NewCFunction(ctx, js_element_append, "append", 0));
    JS_SetPropertyStr(ctx, obj, "getElementsByClassName", JS_NewCFunction(ctx, js_element_getElementsByClassName, "getElementsByClassName", 0));
    JS_SetPropertyStr(ctx, obj, "hasAttributes", JS_NewCFunction(ctx, js_element_hasAttributes, "hasAttributes", 0));
    JS_SetPropertyStr(ctx, obj, "insertAdjacentHTML", JS_NewCFunction(ctx, js_element_insertAdjacentHTML, "insertAdjacentHTML", 0));
    JS_SetPropertyStr(ctx, obj, "queryAll", JS_NewCFunction(ctx, js_element_queryAll, "queryAll", 0));
    JS_SetPropertyStr(ctx, obj, "getAttributeNode", JS_NewCFunction(ctx, js_element_getAttributeNode, "getAttributeNode", 0));
    JS_SetPropertyStr(ctx, obj, "hasAttributeNS", JS_NewCFunction(ctx, js_element_hasAttributeNS, "hasAttributeNS", 0));
    JS_SetPropertyStr(ctx, obj, "before", JS_NewCFunction(ctx, js_element_before, "before", 0));
    JS_SetPropertyStr(ctx, obj, "matches", JS_NewCFunction(ctx, js_element_matches, "matches", 0));
    JS_SetPropertyStr(ctx, obj, "removeAttributeNS", JS_NewCFunction(ctx, js_element_removeAttributeNS, "removeAttributeNS", 0));
    JS_SetPropertyStr(ctx, obj, "querySelectorAll", JS_NewCFunction(ctx, js_element_querySelectorAll, "querySelectorAll", 0));
    JS_SetPropertyStr(ctx, obj, "pseudo", JS_NewCFunction(ctx, js_element_pseudo, "pseudo", 0));
    JS_SetPropertyStr(ctx, obj, "getAttributeNS", JS_NewCFunction(ctx, js_element_getAttributeNS, "getAttributeNS", 0));
    JS_SetPropertyStr(ctx, global_obj, "Element", obj);
    return 0;
}

static JSValue js_htmlelement_commandIcon_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.commandIcon getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_accessKey_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.accessKey getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_dropzone_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.dropzone getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_spellcheck_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.spellcheck getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_commandChecked_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.commandChecked getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_commandHidden_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.commandHidden getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_dataset_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.dataset getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_commandDisabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.commandDisabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_contextMenu_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.contextMenu getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_isContentEditable_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.isContentEditable getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_contentEditable_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.contentEditable getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_tabIndex_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.tabIndex getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_accessKeyLabel_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.accessKeyLabel getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_translate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.translate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_draggable_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.draggable getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_commandLabel_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.commandLabel getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_hidden_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.hidden getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_commandType_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.commandType getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_translate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.translate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_draggable_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.draggable setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_hidden_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.hidden setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_contextMenu_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.contextMenu setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_contentEditable_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.contentEditable setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_accessKey_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.accessKey setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_tabIndex_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.tabIndex setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_spellcheck_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.spellcheck setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_click(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.click() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_blur(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.blur() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_focus(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.focus() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlelement_forceSpellCheck(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLElement.forceSpellCheck() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "commandIcon");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_commandIcon_getter, "commandIcon", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "accessKey");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_accessKey_getter, "accessKey", 0),
            JS_NewCFunction(ctx, js_htmlelement_accessKey_setter, "accessKey", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "dropzone");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_dropzone_getter, "dropzone", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "spellcheck");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_spellcheck_getter, "spellcheck", 0),
            JS_NewCFunction(ctx, js_htmlelement_spellcheck_setter, "spellcheck", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "commandChecked");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_commandChecked_getter, "commandChecked", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "commandHidden");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_commandHidden_getter, "commandHidden", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "dataset");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_dataset_getter, "dataset", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "commandDisabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_commandDisabled_getter, "commandDisabled", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contextMenu");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_contextMenu_getter, "contextMenu", 0),
            JS_NewCFunction(ctx, js_htmlelement_contextMenu_setter, "contextMenu", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "isContentEditable");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_isContentEditable_getter, "isContentEditable", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contentEditable");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_contentEditable_getter, "contentEditable", 0),
            JS_NewCFunction(ctx, js_htmlelement_contentEditable_setter, "contentEditable", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "tabIndex");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_tabIndex_getter, "tabIndex", 0),
            JS_NewCFunction(ctx, js_htmlelement_tabIndex_setter, "tabIndex", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "accessKeyLabel");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_accessKeyLabel_getter, "accessKeyLabel", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_htmlelement_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "translate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_translate_getter, "translate", 0),
            JS_NewCFunction(ctx, js_htmlelement_translate_setter, "translate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "draggable");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_draggable_getter, "draggable", 0),
            JS_NewCFunction(ctx, js_htmlelement_draggable_setter, "draggable", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "commandLabel");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_commandLabel_getter, "commandLabel", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hidden");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_hidden_getter, "hidden", 0),
            JS_NewCFunction(ctx, js_htmlelement_hidden_setter, "hidden", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "commandType");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlelement_commandType_getter, "commandType", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "click", JS_NewCFunction(ctx, js_htmlelement_click, "click", 0));
    JS_SetPropertyStr(ctx, obj, "blur", JS_NewCFunction(ctx, js_htmlelement_blur, "blur", 0));
    JS_SetPropertyStr(ctx, obj, "focus", JS_NewCFunction(ctx, js_htmlelement_focus, "focus", 0));
    JS_SetPropertyStr(ctx, obj, "forceSpellCheck", JS_NewCFunction(ctx, js_htmlelement_forceSpellCheck, "forceSpellCheck", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLElement", obj);
    return 0;
}

static JSValue js_htmldirectoryelement_compact_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDirectoryElement.compact getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldirectoryelement_compact_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDirectoryElement.compact setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmldirectoryelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "compact");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmldirectoryelement_compact_getter, "compact", 0),
            JS_NewCFunction(ctx, js_htmldirectoryelement_compact_setter, "compact", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLDirectoryElement", obj);
    return 0;
}

static JSValue js_htmlframeelement_contentWindow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFrameElement.contentWindow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlframeelement_contentDocument_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFrameElement.contentDocument getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlframeelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "contentWindow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlframeelement_contentWindow_getter, "contentWindow", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contentDocument");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlframeelement_contentDocument_getter, "contentDocument", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLFrameElement", obj);
    return 0;
}

static JSValue js_htmlmarqueeelement_onbounce_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.onbounce getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_scrollAmount_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.scrollAmount getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_vspace_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.vspace getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_onfinish_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.onfinish getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_width_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.width getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_hspace_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.hspace getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_height_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.height getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_onstart_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.onstart getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_loop_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.loop getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_bgColor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.bgColor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_scrollDelay_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.scrollDelay getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_behavior_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.behavior getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_direction_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.direction getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_trueSpeed_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.trueSpeed getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_onbounce_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.onbounce setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_scrollAmount_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.scrollAmount setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_vspace_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.vspace setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_onfinish_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.onfinish setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_width_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.width setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_hspace_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.hspace setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_height_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.height setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_onstart_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.onstart setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_loop_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.loop setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_bgColor_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.bgColor setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_scrollDelay_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.scrollDelay setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_behavior_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.behavior setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_direction_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.direction setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_trueSpeed_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.trueSpeed setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.stop() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmarqueeelement_start(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMarqueeElement.start() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlmarqueeelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onbounce");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_onbounce_getter, "onbounce", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_onbounce_setter, "onbounce", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "scrollAmount");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_scrollAmount_getter, "scrollAmount", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_scrollAmount_setter, "scrollAmount", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "vspace");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_vspace_getter, "vspace", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_vspace_setter, "vspace", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onfinish");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_onfinish_getter, "onfinish", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_onfinish_setter, "onfinish", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "width");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_width_getter, "width", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_width_setter, "width", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hspace");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_hspace_getter, "hspace", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_hspace_setter, "hspace", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "height");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_height_getter, "height", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_height_setter, "height", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onstart");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_onstart_getter, "onstart", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_onstart_setter, "onstart", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "loop");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_loop_getter, "loop", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_loop_setter, "loop", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "bgColor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_bgColor_getter, "bgColor", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_bgColor_setter, "bgColor", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "scrollDelay");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_scrollDelay_getter, "scrollDelay", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_scrollDelay_setter, "scrollDelay", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "behavior");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_behavior_getter, "behavior", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_behavior_setter, "behavior", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "direction");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_direction_getter, "direction", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_direction_setter, "direction", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "trueSpeed");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmarqueeelement_trueSpeed_getter, "trueSpeed", 0),
            JS_NewCFunction(ctx, js_htmlmarqueeelement_trueSpeed_setter, "trueSpeed", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "stop", JS_NewCFunction(ctx, js_htmlmarqueeelement_stop, "stop", 0));
    JS_SetPropertyStr(ctx, obj, "start", JS_NewCFunction(ctx, js_htmlmarqueeelement_start, "start", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLMarqueeElement", obj);
    return 0;
}

static JSValue js_htmlappletelement_hspace_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAppletElement.hspace getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlappletelement_vspace_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAppletElement.vspace getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlappletelement_hspace_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAppletElement.hspace setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlappletelement_vspace_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAppletElement.vspace setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlappletelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "hspace");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlappletelement_hspace_getter, "hspace", 0),
            JS_NewCFunction(ctx, js_htmlappletelement_hspace_setter, "hspace", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "vspace");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlappletelement_vspace_getter, "vspace", 0),
            JS_NewCFunction(ctx, js_htmlappletelement_vspace_setter, "vspace", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLAppletElement", obj);
    return 0;
}

static JSValue js_storageevent_url_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StorageEvent.url getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storageevent_newValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StorageEvent.newValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storageevent_oldValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StorageEvent.oldValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storageevent_key_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StorageEvent.key getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storageevent_storageArea_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "StorageEvent.storageArea getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_storageevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "url");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_storageevent_url_getter, "url", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "newValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_storageevent_newValue_getter, "newValue", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "oldValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_storageevent_oldValue_getter, "oldValue", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "key");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_storageevent_key_getter, "key", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "storageArea");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_storageevent_storageArea_getter, "storageArea", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "StorageEvent", obj);
    return 0;
}

static JSValue js_storage_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Storage.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storage_clear(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Storage.clear() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storage_key(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Storage.key() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storage_setItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Storage.setItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storage_getItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Storage.getItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_storage_removeItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Storage.removeItem() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_storage(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_storage_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "clear", JS_NewCFunction(ctx, js_storage_clear, "clear", 0));
    JS_SetPropertyStr(ctx, obj, "key", JS_NewCFunction(ctx, js_storage_key, "key", 0));
    JS_SetPropertyStr(ctx, obj, "setItem", JS_NewCFunction(ctx, js_storage_setItem, "setItem", 0));
    JS_SetPropertyStr(ctx, obj, "getItem", JS_NewCFunction(ctx, js_storage_getItem, "getItem", 0));
    JS_SetPropertyStr(ctx, obj, "removeItem", JS_NewCFunction(ctx, js_storage_removeItem, "removeItem", 0));
    JS_SetPropertyStr(ctx, global_obj, "Storage", obj);
    return 0;
}

static JSValue js_workerlocation_origin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.origin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_pathname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.pathname getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_protocol_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.protocol getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_host_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.host getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_port_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.port getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_search_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.search getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_hash_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.hash getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_href_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.href getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerlocation_hostname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerLocation.hostname getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_workerlocation(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "origin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_origin_getter, "origin", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "pathname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_pathname_getter, "pathname", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "protocol");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_protocol_getter, "protocol", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "host");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_host_getter, "host", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "port");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_port_getter, "port", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "search");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_search_getter, "search", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hash");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_hash_getter, "hash", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "href");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_href_getter, "href", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hostname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerlocation_hostname_getter, "hostname", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "WorkerLocation", obj);
    return 0;
}

static JSValue js_workernavigator_appName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.appName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_product_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.product getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_languages_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.languages getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_productSub_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.productSub getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_onLine_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.onLine getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_appCodeName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.appCodeName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_language_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.language getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_userAgent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.userAgent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_vendorSub_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.vendorSub getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_vendor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.vendor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_platform_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.platform getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_appVersion_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.appVersion getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workernavigator_taintEnabled(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerNavigator.taintEnabled() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_workernavigator(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "appName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_appName_getter, "appName", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "product");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_product_getter, "product", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "languages");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_languages_getter, "languages", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "productSub");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_productSub_getter, "productSub", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onLine");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_onLine_getter, "onLine", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "appCodeName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_appCodeName_getter, "appCodeName", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "language");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_language_getter, "language", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "userAgent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_userAgent_getter, "userAgent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "vendorSub");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_vendorSub_getter, "vendorSub", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "vendor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_vendor_getter, "vendor", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "platform");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_platform_getter, "platform", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "appVersion");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workernavigator_appVersion_getter, "appVersion", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "taintEnabled", JS_NewCFunction(ctx, js_workernavigator_taintEnabled, "taintEnabled", 0));
    JS_SetPropertyStr(ctx, global_obj, "WorkerNavigator", obj);
    return 0;
}

static JSValue js_sharedworker_port_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SharedWorker.port getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_sharedworker_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SharedWorker.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_sharedworker_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SharedWorker.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_sharedworker(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "port");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_sharedworker_port_getter, "port", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_sharedworker_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_sharedworker_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "SharedWorker", obj);
    return 0;
}

static JSValue js_worker_onmessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Worker.onmessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_worker_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Worker.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_worker_onmessage_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Worker.onmessage setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_worker_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Worker.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_worker_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Worker.postMessage() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_worker_terminate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Worker.terminate() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_worker(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onmessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_worker_onmessage_getter, "onmessage", 0),
            JS_NewCFunction(ctx, js_worker_onmessage_setter, "onmessage", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_worker_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_worker_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "postMessage", JS_NewCFunction(ctx, js_worker_postMessage, "postMessage", 0));
    JS_SetPropertyStr(ctx, obj, "terminate", JS_NewCFunction(ctx, js_worker_terminate, "terminate", 0));
    JS_SetPropertyStr(ctx, global_obj, "Worker", obj);
    return 0;
}

static JSValue js_workerglobalscope_self_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.self getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_navigator_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.navigator getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_onlanguagechange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.onlanguagechange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_location_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.location getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_onoffline_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.onoffline getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_ononline_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.ononline getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_ononline_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.ononline setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_onoffline_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.onoffline setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_onlanguagechange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.onlanguagechange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_clearInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.clearInterval() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.close() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_createImageBitmap(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.createImageBitmap() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_atob(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.atob() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_importScripts(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.importScripts() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_setInterval(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.setInterval() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_clearTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.clearTimeout() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_btoa(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.btoa() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_workerglobalscope_setTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WorkerGlobalScope.setTimeout() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_workerglobalscope(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "self");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerglobalscope_self_getter, "self", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "navigator");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerglobalscope_navigator_getter, "navigator", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onlanguagechange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerglobalscope_onlanguagechange_getter, "onlanguagechange", 0),
            JS_NewCFunction(ctx, js_workerglobalscope_onlanguagechange_setter, "onlanguagechange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "location");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerglobalscope_location_getter, "location", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onoffline");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerglobalscope_onoffline_getter, "onoffline", 0),
            JS_NewCFunction(ctx, js_workerglobalscope_onoffline_setter, "onoffline", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ononline");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerglobalscope_ononline_getter, "ononline", 0),
            JS_NewCFunction(ctx, js_workerglobalscope_ononline_setter, "ononline", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_workerglobalscope_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_workerglobalscope_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "clearInterval", JS_NewCFunction(ctx, js_workerglobalscope_clearInterval, "clearInterval", 0));
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_workerglobalscope_close, "close", 0));
    JS_SetPropertyStr(ctx, obj, "createImageBitmap", JS_NewCFunction(ctx, js_workerglobalscope_createImageBitmap, "createImageBitmap", 0));
    JS_SetPropertyStr(ctx, obj, "atob", JS_NewCFunction(ctx, js_workerglobalscope_atob, "atob", 0));
    JS_SetPropertyStr(ctx, obj, "importScripts", JS_NewCFunction(ctx, js_workerglobalscope_importScripts, "importScripts", 0));
    JS_SetPropertyStr(ctx, obj, "setInterval", JS_NewCFunction(ctx, js_workerglobalscope_setInterval, "setInterval", 0));
    JS_SetPropertyStr(ctx, obj, "clearTimeout", JS_NewCFunction(ctx, js_workerglobalscope_clearTimeout, "clearTimeout", 0));
    JS_SetPropertyStr(ctx, obj, "btoa", JS_NewCFunction(ctx, js_workerglobalscope_btoa, "btoa", 0));
    JS_SetPropertyStr(ctx, obj, "setTimeout", JS_NewCFunction(ctx, js_workerglobalscope_setTimeout, "setTimeout", 0));
    JS_SetPropertyStr(ctx, global_obj, "WorkerGlobalScope", obj);
    return 0;
}

static JSValue js_sharedworkerglobalscope_onconnect_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SharedWorkerGlobalScope.onconnect getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_sharedworkerglobalscope_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SharedWorkerGlobalScope.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_sharedworkerglobalscope_applicationCache_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SharedWorkerGlobalScope.applicationCache getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_sharedworkerglobalscope_onconnect_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "SharedWorkerGlobalScope.onconnect setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_sharedworkerglobalscope(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onconnect");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_sharedworkerglobalscope_onconnect_getter, "onconnect", 0),
            JS_NewCFunction(ctx, js_sharedworkerglobalscope_onconnect_setter, "onconnect", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_sharedworkerglobalscope_name_getter, "name", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "applicationCache");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_sharedworkerglobalscope_applicationCache_getter, "applicationCache", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "SharedWorkerGlobalScope", obj);
    return 0;
}

static JSValue js_dedicatedworkerglobalscope_onmessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DedicatedWorkerGlobalScope.onmessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_dedicatedworkerglobalscope_onmessage_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DedicatedWorkerGlobalScope.onmessage setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_dedicatedworkerglobalscope_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DedicatedWorkerGlobalScope.postMessage() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_dedicatedworkerglobalscope(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onmessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_dedicatedworkerglobalscope_onmessage_getter, "onmessage", 0),
            JS_NewCFunction(ctx, js_dedicatedworkerglobalscope_onmessage_setter, "onmessage", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "postMessage", JS_NewCFunction(ctx, js_dedicatedworkerglobalscope_postMessage, "postMessage", 0));
    JS_SetPropertyStr(ctx, global_obj, "DedicatedWorkerGlobalScope", obj);
    return 0;
}

static JSValue js_broadcastchannel_onmessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BroadcastChannel.onmessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_broadcastchannel_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BroadcastChannel.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_broadcastchannel_onmessage_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BroadcastChannel.onmessage setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_broadcastchannel_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BroadcastChannel.close() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_broadcastchannel_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BroadcastChannel.postMessage() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_broadcastchannel(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onmessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_broadcastchannel_onmessage_getter, "onmessage", 0),
            JS_NewCFunction(ctx, js_broadcastchannel_onmessage_setter, "onmessage", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_broadcastchannel_name_getter, "name", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_broadcastchannel_close, "close", 0));
    JS_SetPropertyStr(ctx, obj, "postMessage", JS_NewCFunction(ctx, js_broadcastchannel_postMessage, "postMessage", 0));
    JS_SetPropertyStr(ctx, global_obj, "BroadcastChannel", obj);
    return 0;
}

static JSValue js_messageport_onmessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessagePort.onmessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageport_onmessage_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessagePort.onmessage setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageport_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessagePort.close() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageport_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessagePort.postMessage() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageport_start(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessagePort.start() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_messageport(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onmessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messageport_onmessage_getter, "onmessage", 0),
            JS_NewCFunction(ctx, js_messageport_onmessage_setter, "onmessage", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_messageport_close, "close", 0));
    JS_SetPropertyStr(ctx, obj, "postMessage", JS_NewCFunction(ctx, js_messageport_postMessage, "postMessage", 0));
    JS_SetPropertyStr(ctx, obj, "start", JS_NewCFunction(ctx, js_messageport_start, "start", 0));
    JS_SetPropertyStr(ctx, global_obj, "MessagePort", obj);
    return 0;
}

static JSValue js_messagechannel_port1_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageChannel.port1 getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messagechannel_port2_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageChannel.port2 getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_messagechannel(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "port1");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messagechannel_port1_getter, "port1", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "port2");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messagechannel_port2_getter, "port2", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "MessageChannel", obj);
    return 0;
}

static JSValue js_closeevent_wasClean_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CloseEvent.wasClean getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_closeevent_reason_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CloseEvent.reason getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_closeevent_code_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CloseEvent.code getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_closeevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "wasClean");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_closeevent_wasClean_getter, "wasClean", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "reason");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_closeevent_reason_getter, "reason", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "code");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_closeevent_code_getter, "code", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "CloseEvent", obj);
    return 0;
}

static JSValue js_websocket_extensions_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.extensions getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_url_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.url getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_bufferedAmount_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.bufferedAmount getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onclose_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onclose getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_protocol_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.protocol getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onopen_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onopen getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onmessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onmessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_readyState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.readyState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_binaryType_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.binaryType getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onclose_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onclose setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onopen_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onopen setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onmessage_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onmessage setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_binaryType_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.binaryType setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.close() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_websocket_send(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "WebSocket.send() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_websocket(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "extensions");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_extensions_getter, "extensions", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "url");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_url_getter, "url", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "bufferedAmount");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_bufferedAmount_getter, "bufferedAmount", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onclose");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_onclose_getter, "onclose", 0),
            JS_NewCFunction(ctx, js_websocket_onclose_setter, "onclose", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "protocol");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_protocol_getter, "protocol", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onopen");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_onopen_getter, "onopen", 0),
            JS_NewCFunction(ctx, js_websocket_onopen_setter, "onopen", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onmessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_onmessage_getter, "onmessage", 0),
            JS_NewCFunction(ctx, js_websocket_onmessage_setter, "onmessage", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "readyState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_readyState_getter, "readyState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "binaryType");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_binaryType_getter, "binaryType", 0),
            JS_NewCFunction(ctx, js_websocket_binaryType_setter, "binaryType", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_websocket_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_websocket_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_websocket_close, "close", 0));
    JS_SetPropertyStr(ctx, obj, "send", JS_NewCFunction(ctx, js_websocket_send, "send", 0));
    JS_SetPropertyStr(ctx, global_obj, "WebSocket", obj);
    return 0;
}

static JSValue js_eventsource_url_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.url getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_onopen_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.onopen getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_withCredentials_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.withCredentials getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_onmessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.onmessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_readyState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.readyState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_onmessage_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.onmessage setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_onopen_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.onopen setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_eventsource_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventSource.close() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_eventsource(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "url");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_eventsource_url_getter, "url", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onopen");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_eventsource_onopen_getter, "onopen", 0),
            JS_NewCFunction(ctx, js_eventsource_onopen_setter, "onopen", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "withCredentials");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_eventsource_withCredentials_getter, "withCredentials", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onmessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_eventsource_onmessage_getter, "onmessage", 0),
            JS_NewCFunction(ctx, js_eventsource_onmessage_setter, "onmessage", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "readyState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_eventsource_readyState_getter, "readyState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_eventsource_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_eventsource_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_eventsource_close, "close", 0));
    JS_SetPropertyStr(ctx, global_obj, "EventSource", obj);
    return 0;
}

static JSValue js_messageevent_source_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageEvent.source getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageevent_origin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageEvent.origin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageevent_lastEventId_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageEvent.lastEventId getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageevent_ports_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageEvent.ports getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageevent_data_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageEvent.data getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_messageevent_initMessageEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MessageEvent.initMessageEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_messageevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "source");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messageevent_source_getter, "source", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "origin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messageevent_origin_getter, "origin", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lastEventId");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messageevent_lastEventId_getter, "lastEventId", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ports");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messageevent_ports_getter, "ports", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "data");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_messageevent_data_getter, "data", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initMessageEvent", JS_NewCFunction(ctx, js_messageevent_initMessageEvent, "initMessageEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "MessageEvent", obj);
    return 0;
}

static JSValue js_imagebitmap_height_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ImageBitmap.height getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_imagebitmap_width_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ImageBitmap.width getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_imagebitmap(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "height");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_imagebitmap_height_getter, "height", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "width");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_imagebitmap_width_getter, "width", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "ImageBitmap", obj);
    return 0;
}

static JSValue js_external_AddSearchProvider(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "External.AddSearchProvider() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_external_IsSearchProviderInstalled(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "External.IsSearchProviderInstalled() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_external(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "AddSearchProvider", JS_NewCFunction(ctx, js_external_AddSearchProvider, "AddSearchProvider", 0));
    JS_SetPropertyStr(ctx, obj, "IsSearchProviderInstalled", JS_NewCFunction(ctx, js_external_IsSearchProviderInstalled, "IsSearchProviderInstalled", 0));
    JS_SetPropertyStr(ctx, global_obj, "External", obj);
    return 0;
}

static JSValue js_mimetype_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MimeType.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mimetype_description_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MimeType.description getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mimetype_suffixes_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MimeType.suffixes getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mimetype_enabledPlugin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MimeType.enabledPlugin getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mimetype(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mimetype_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "description");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mimetype_description_getter, "description", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "suffixes");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mimetype_suffixes_getter, "suffixes", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "enabledPlugin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mimetype_enabledPlugin_getter, "enabledPlugin", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "MimeType", obj);
    return 0;
}

static JSValue js_plugin_description_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Plugin.description getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_plugin_filename_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Plugin.filename getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_plugin_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Plugin.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_plugin_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Plugin.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_plugin_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Plugin.namedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_plugin_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Plugin.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_plugin(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "description");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_plugin_description_getter, "description", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "filename");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_plugin_filename_getter, "filename", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_plugin_name_getter, "name", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_plugin_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "namedItem", JS_NewCFunction(ctx, js_plugin_namedItem, "namedItem", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_plugin_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "Plugin", obj);
    return 0;
}

static JSValue js_mimetypearray_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MimeTypeArray.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mimetypearray_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MimeTypeArray.namedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mimetypearray_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MimeTypeArray.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mimetypearray(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mimetypearray_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "namedItem", JS_NewCFunction(ctx, js_mimetypearray_namedItem, "namedItem", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_mimetypearray_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "MimeTypeArray", obj);
    return 0;
}

static JSValue js_pluginarray_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PluginArray.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_pluginarray_refresh(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PluginArray.refresh() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_pluginarray_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PluginArray.namedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_pluginarray_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PluginArray.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_pluginarray(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_pluginarray_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "refresh", JS_NewCFunction(ctx, js_pluginarray_refresh, "refresh", 0));
    JS_SetPropertyStr(ctx, obj, "namedItem", JS_NewCFunction(ctx, js_pluginarray_namedItem, "namedItem", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_pluginarray_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "PluginArray", obj);
    return 0;
}

static JSValue js_navigator_plugins_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.plugins getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_languages_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.languages getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_onLine_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.onLine getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_mimeTypes_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.mimeTypes getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_language_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.language getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_registerProtocolHandler(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.registerProtocolHandler() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_isContentHandlerRegistered(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.isContentHandlerRegistered() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_isProtocolHandlerRegistered(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.isProtocolHandlerRegistered() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_yieldForStorageUpdates(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.yieldForStorageUpdates() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_unregisterContentHandler(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.unregisterContentHandler() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_unregisterProtocolHandler(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.unregisterProtocolHandler() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_navigator_registerContentHandler(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Navigator.registerContentHandler() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_navigator(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "plugins");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_navigator_plugins_getter, "plugins", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "languages");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_navigator_languages_getter, "languages", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onLine");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_navigator_onLine_getter, "onLine", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "mimeTypes");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_navigator_mimeTypes_getter, "mimeTypes", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "language");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_navigator_language_getter, "language", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "registerProtocolHandler", JS_NewCFunction(ctx, js_navigator_registerProtocolHandler, "registerProtocolHandler", 0));
    JS_SetPropertyStr(ctx, obj, "isContentHandlerRegistered", JS_NewCFunction(ctx, js_navigator_isContentHandlerRegistered, "isContentHandlerRegistered", 0));
    JS_SetPropertyStr(ctx, obj, "isProtocolHandlerRegistered", JS_NewCFunction(ctx, js_navigator_isProtocolHandlerRegistered, "isProtocolHandlerRegistered", 0));
    JS_SetPropertyStr(ctx, obj, "yieldForStorageUpdates", JS_NewCFunction(ctx, js_navigator_yieldForStorageUpdates, "yieldForStorageUpdates", 0));
    JS_SetPropertyStr(ctx, obj, "unregisterContentHandler", JS_NewCFunction(ctx, js_navigator_unregisterContentHandler, "unregisterContentHandler", 0));
    JS_SetPropertyStr(ctx, obj, "unregisterProtocolHandler", JS_NewCFunction(ctx, js_navigator_unregisterProtocolHandler, "unregisterProtocolHandler", 0));
    JS_SetPropertyStr(ctx, obj, "registerContentHandler", JS_NewCFunction(ctx, js_navigator_registerContentHandler, "registerContentHandler", 0));
    JS_SetPropertyStr(ctx, global_obj, "Navigator", obj);
    return 0;
}

static JSValue js_errorevent_filename_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ErrorEvent.filename getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_errorevent_colno_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ErrorEvent.colno getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_errorevent_message_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ErrorEvent.message getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_errorevent_lineno_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ErrorEvent.lineno getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_errorevent_error_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ErrorEvent.error getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_errorevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "filename");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_errorevent_filename_getter, "filename", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "colno");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_errorevent_colno_getter, "colno", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "message");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_errorevent_message_getter, "message", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lineno");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_errorevent_lineno_getter, "lineno", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "error");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_errorevent_error_getter, "error", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "ErrorEvent", obj);
    return 0;
}

static JSValue js_applicationcache_oncached_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.oncached getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onchecking_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onchecking getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onprogress_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onprogress getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onnoupdate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onnoupdate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_ondownloading_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.ondownloading getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onupdateready_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onupdateready getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onobsolete_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onobsolete getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_status_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.status getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_oncached_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.oncached setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onchecking_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onchecking setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onprogress_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onprogress setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onnoupdate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onnoupdate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_ondownloading_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.ondownloading setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onupdateready_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onupdateready setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onobsolete_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onobsolete setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_swapCache(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.swapCache() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_abort(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.abort() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_applicationcache_update(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ApplicationCache.update() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_applicationcache(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "oncached");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_oncached_getter, "oncached", 0),
            JS_NewCFunction(ctx, js_applicationcache_oncached_setter, "oncached", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onchecking");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_onchecking_getter, "onchecking", 0),
            JS_NewCFunction(ctx, js_applicationcache_onchecking_setter, "onchecking", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onprogress");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_onprogress_getter, "onprogress", 0),
            JS_NewCFunction(ctx, js_applicationcache_onprogress_setter, "onprogress", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onnoupdate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_onnoupdate_getter, "onnoupdate", 0),
            JS_NewCFunction(ctx, js_applicationcache_onnoupdate_setter, "onnoupdate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ondownloading");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_ondownloading_getter, "ondownloading", 0),
            JS_NewCFunction(ctx, js_applicationcache_ondownloading_setter, "ondownloading", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onupdateready");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_onupdateready_getter, "onupdateready", 0),
            JS_NewCFunction(ctx, js_applicationcache_onupdateready_setter, "onupdateready", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onobsolete");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_onobsolete_getter, "onobsolete", 0),
            JS_NewCFunction(ctx, js_applicationcache_onobsolete_setter, "onobsolete", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "status");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_status_getter, "status", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_applicationcache_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_applicationcache_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "swapCache", JS_NewCFunction(ctx, js_applicationcache_swapCache, "swapCache", 0));
    JS_SetPropertyStr(ctx, obj, "abort", JS_NewCFunction(ctx, js_applicationcache_abort, "abort", 0));
    JS_SetPropertyStr(ctx, obj, "update", JS_NewCFunction(ctx, js_applicationcache_update, "update", 0));
    JS_SetPropertyStr(ctx, global_obj, "ApplicationCache", obj);
    return 0;
}

static JSValue js_beforeunloadevent_returnValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BeforeUnloadEvent.returnValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_beforeunloadevent_returnValue_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BeforeUnloadEvent.returnValue setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_beforeunloadevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "returnValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_beforeunloadevent_returnValue_getter, "returnValue", 0),
            JS_NewCFunction(ctx, js_beforeunloadevent_returnValue_setter, "returnValue", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "BeforeUnloadEvent", obj);
    return 0;
}

static JSValue js_pagetransitionevent_persisted_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PageTransitionEvent.persisted getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_pagetransitionevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "persisted");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_pagetransitionevent_persisted_getter, "persisted", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "PageTransitionEvent", obj);
    return 0;
}

static JSValue js_hashchangeevent_oldURL_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HashChangeEvent.oldURL getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_hashchangeevent_newURL_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HashChangeEvent.newURL getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_hashchangeevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "oldURL");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_hashchangeevent_oldURL_getter, "oldURL", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "newURL");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_hashchangeevent_newURL_getter, "newURL", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HashChangeEvent", obj);
    return 0;
}

static JSValue js_popstateevent_state_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "PopStateEvent.state getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_popstateevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "state");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_popstateevent_state_getter, "state", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "PopStateEvent", obj);
    return 0;
}

static JSValue js_location_ancestorOrigins_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.ancestorOrigins getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_pathname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.pathname setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_protocol_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.protocol setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_password_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.password setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_host_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.host setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_port_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.port setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_search_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.search setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_username_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.username setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_hash_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.hash setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_location_hostname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Location.hostname setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_location(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "ancestorOrigins");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_location_ancestorOrigins_getter, "ancestorOrigins", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "pathname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_pathname_setter, "pathname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "protocol");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_protocol_setter, "protocol", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "password");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_password_setter, "password", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "host");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_host_setter, "host", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "port");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_port_setter, "port", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "search");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_search_setter, "search", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "username");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_username_setter, "username", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hash");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_hash_setter, "hash", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hostname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_location_hostname_setter, "hostname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "Location", obj);
    return 0;
}

static JSValue js_history_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "History.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_history_state_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "History.state getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_history_back(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "History.back() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_history_replaceState(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "History.replaceState() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_history_forward(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "History.forward() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_history_pushState(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "History.pushState() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_history_go(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "History.go() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_history(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_history_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "state");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_history_state_getter, "state", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "back", JS_NewCFunction(ctx, js_history_back, "back", 0));
    JS_SetPropertyStr(ctx, obj, "replaceState", JS_NewCFunction(ctx, js_history_replaceState, "replaceState", 0));
    JS_SetPropertyStr(ctx, obj, "forward", JS_NewCFunction(ctx, js_history_forward, "forward", 0));
    JS_SetPropertyStr(ctx, obj, "pushState", JS_NewCFunction(ctx, js_history_pushState, "pushState", 0));
    JS_SetPropertyStr(ctx, obj, "go", JS_NewCFunction(ctx, js_history_go, "go", 0));
    JS_SetPropertyStr(ctx, global_obj, "History", obj);
    return 0;
}

static JSValue js_barprop_visible_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "BarProp.visible getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_barprop(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "visible");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_barprop_visible_getter, "visible", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "BarProp", obj);
    return 0;
}

static JSValue js_window_closed_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.closed getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_history_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.history getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_locationbar_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.locationbar getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_applicationCache_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.applicationCache getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_self_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.self getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_frames_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.frames getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_top_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.top getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_opener_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.opener getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_parent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.parent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_localStorage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.localStorage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_sessionStorage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.sessionStorage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_status_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.status getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_frameElement_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.frameElement getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_external_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.external getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_menubar_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.menubar getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_scrollbars_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.scrollbars getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_toolbar_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.toolbar getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_personalbar_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.personalbar getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_statusbar_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.statusbar getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_status_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.status setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_opener_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.opener setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_print(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.print() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_cancelAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.cancelAnimationFrame() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_showModalDialog(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.showModalDialog() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.close() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_focus(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.focus() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_createImageBitmap(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.createImageBitmap() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_blur(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.blur() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_atob(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.atob() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_releaseEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.releaseEvents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_requestAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.requestAnimationFrame() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_stop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.stop() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_postMessage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.postMessage() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_prompt(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.prompt() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_confirm(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.confirm() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.open() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_btoa(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.btoa() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_captureEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.captureEvents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_window_getComputedStyle(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Window.getComputedStyle() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_window(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "closed");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_closed_getter, "closed", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "history");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_history_getter, "history", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "locationbar");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_locationbar_getter, "locationbar", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "applicationCache");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_applicationCache_getter, "applicationCache", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "self");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_self_getter, "self", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "frames");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_frames_getter, "frames", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "top");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_top_getter, "top", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "opener");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_opener_getter, "opener", 0),
            JS_NewCFunction(ctx, js_window_opener_setter, "opener", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "parent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_parent_getter, "parent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "localStorage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_localStorage_getter, "localStorage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sessionStorage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_sessionStorage_getter, "sessionStorage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "status");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_status_getter, "status", 0),
            JS_NewCFunction(ctx, js_window_status_setter, "status", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "frameElement");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_frameElement_getter, "frameElement", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "external");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_external_getter, "external", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "menubar");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_menubar_getter, "menubar", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "scrollbars");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_scrollbars_getter, "scrollbars", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "toolbar");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_toolbar_getter, "toolbar", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "personalbar");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_personalbar_getter, "personalbar", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "statusbar");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_window_statusbar_getter, "statusbar", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "print", JS_NewCFunction(ctx, js_window_print, "print", 0));
    JS_SetPropertyStr(ctx, obj, "cancelAnimationFrame", JS_NewCFunction(ctx, js_window_cancelAnimationFrame, "cancelAnimationFrame", 0));
    JS_SetPropertyStr(ctx, obj, "showModalDialog", JS_NewCFunction(ctx, js_window_showModalDialog, "showModalDialog", 0));
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_window_close, "close", 0));
    JS_SetPropertyStr(ctx, obj, "focus", JS_NewCFunction(ctx, js_window_focus, "focus", 0));
    JS_SetPropertyStr(ctx, obj, "createImageBitmap", JS_NewCFunction(ctx, js_window_createImageBitmap, "createImageBitmap", 0));
    JS_SetPropertyStr(ctx, obj, "blur", JS_NewCFunction(ctx, js_window_blur, "blur", 0));
    JS_SetPropertyStr(ctx, obj, "atob", JS_NewCFunction(ctx, js_window_atob, "atob", 0));
    JS_SetPropertyStr(ctx, obj, "releaseEvents", JS_NewCFunction(ctx, js_window_releaseEvents, "releaseEvents", 0));
    JS_SetPropertyStr(ctx, obj, "requestAnimationFrame", JS_NewCFunction(ctx, js_window_requestAnimationFrame, "requestAnimationFrame", 0));
    JS_SetPropertyStr(ctx, obj, "stop", JS_NewCFunction(ctx, js_window_stop, "stop", 0));
    JS_SetPropertyStr(ctx, obj, "postMessage", JS_NewCFunction(ctx, js_window_postMessage, "postMessage", 0));
    JS_SetPropertyStr(ctx, obj, "prompt", JS_NewCFunction(ctx, js_window_prompt, "prompt", 0));
    JS_SetPropertyStr(ctx, obj, "confirm", JS_NewCFunction(ctx, js_window_confirm, "confirm", 0));
    JS_SetPropertyStr(ctx, obj, "open", JS_NewCFunction(ctx, js_window_open, "open", 0));
    JS_SetPropertyStr(ctx, obj, "btoa", JS_NewCFunction(ctx, js_window_btoa, "btoa", 0));
    JS_SetPropertyStr(ctx, obj, "captureEvents", JS_NewCFunction(ctx, js_window_captureEvents, "captureEvents", 0));
    JS_SetPropertyStr(ctx, obj, "getComputedStyle", JS_NewCFunction(ctx, js_window_getComputedStyle, "getComputedStyle", 0));
    JS_SetPropertyStr(ctx, global_obj, "Window", obj);
    return 0;
}

static JSValue js_dragevent_dataTransfer_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DragEvent.dataTransfer getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_dragevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "dataTransfer");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_dragevent_dataTransfer_getter, "dataTransfer", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "DragEvent", obj);
    return 0;
}

static JSValue js_datatransferitem_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItem.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransferitem_kind_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItem.kind getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransferitem_getAsString(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItem.getAsString() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransferitem_getAsFile(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItem.getAsFile() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_datatransferitem(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransferitem_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "kind");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransferitem_kind_getter, "kind", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getAsString", JS_NewCFunction(ctx, js_datatransferitem_getAsString, "getAsString", 0));
    JS_SetPropertyStr(ctx, obj, "getAsFile", JS_NewCFunction(ctx, js_datatransferitem_getAsFile, "getAsFile", 0));
    JS_SetPropertyStr(ctx, global_obj, "DataTransferItem", obj);
    return 0;
}

static JSValue js_datatransferitemlist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItemList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransferitemlist_clear(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItemList.clear() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransferitemlist_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItemList.remove() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransferitemlist_add(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransferItemList.add() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_datatransferitemlist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransferitemlist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "clear", JS_NewCFunction(ctx, js_datatransferitemlist_clear, "clear", 0));
    JS_SetPropertyStr(ctx, obj, "remove", JS_NewCFunction(ctx, js_datatransferitemlist_remove, "remove", 0));
    JS_SetPropertyStr(ctx, obj, "add", JS_NewCFunction(ctx, js_datatransferitemlist_add, "add", 0));
    JS_SetPropertyStr(ctx, global_obj, "DataTransferItemList", obj);
    return 0;
}

static JSValue js_datatransfer_items_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.items getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_types_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.types getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_files_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.files getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_dropEffect_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.dropEffect getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_effectAllowed_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.effectAllowed getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_effectAllowed_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.effectAllowed setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_dropEffect_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.dropEffect setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_getData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.getData() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_setDragImage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.setDragImage() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_setData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.setData() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_datatransfer_clearData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DataTransfer.clearData() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_datatransfer(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "items");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransfer_items_getter, "items", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "types");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransfer_types_getter, "types", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "files");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransfer_files_getter, "files", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "dropEffect");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransfer_dropEffect_getter, "dropEffect", 0),
            JS_NewCFunction(ctx, js_datatransfer_dropEffect_setter, "dropEffect", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "effectAllowed");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_datatransfer_effectAllowed_getter, "effectAllowed", 0),
            JS_NewCFunction(ctx, js_datatransfer_effectAllowed_setter, "effectAllowed", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getData", JS_NewCFunction(ctx, js_datatransfer_getData, "getData", 0));
    JS_SetPropertyStr(ctx, obj, "setDragImage", JS_NewCFunction(ctx, js_datatransfer_setDragImage, "setDragImage", 0));
    JS_SetPropertyStr(ctx, obj, "setData", JS_NewCFunction(ctx, js_datatransfer_setData, "setData", 0));
    JS_SetPropertyStr(ctx, obj, "clearData", JS_NewCFunction(ctx, js_datatransfer_clearData, "clearData", 0));
    JS_SetPropertyStr(ctx, global_obj, "DataTransfer", obj);
    return 0;
}

static JSValue js_touch_region_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Touch.region getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_touch(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "region");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_touch_region_getter, "region", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "Touch", obj);
    return 0;
}

static JSValue js_path2d_arcTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.arcTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_moveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.moveTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_addPathByStrokingPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.addPathByStrokingPath() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_addText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.addText() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_bezierCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.bezierCurveTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_rect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.rect() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_quadraticCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.quadraticCurveTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_arc(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.arc() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_lineTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.lineTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_addPathByStrokingText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.addPathByStrokingText() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_ellipse(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.ellipse() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_closePath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.closePath() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_path2d_addPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Path2D.addPath() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_path2d(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "arcTo", JS_NewCFunction(ctx, js_path2d_arcTo, "arcTo", 0));
    JS_SetPropertyStr(ctx, obj, "moveTo", JS_NewCFunction(ctx, js_path2d_moveTo, "moveTo", 0));
    JS_SetPropertyStr(ctx, obj, "addPathByStrokingPath", JS_NewCFunction(ctx, js_path2d_addPathByStrokingPath, "addPathByStrokingPath", 0));
    JS_SetPropertyStr(ctx, obj, "addText", JS_NewCFunction(ctx, js_path2d_addText, "addText", 0));
    JS_SetPropertyStr(ctx, obj, "bezierCurveTo", JS_NewCFunction(ctx, js_path2d_bezierCurveTo, "bezierCurveTo", 0));
    JS_SetPropertyStr(ctx, obj, "rect", JS_NewCFunction(ctx, js_path2d_rect, "rect", 0));
    JS_SetPropertyStr(ctx, obj, "quadraticCurveTo", JS_NewCFunction(ctx, js_path2d_quadraticCurveTo, "quadraticCurveTo", 0));
    JS_SetPropertyStr(ctx, obj, "arc", JS_NewCFunction(ctx, js_path2d_arc, "arc", 0));
    JS_SetPropertyStr(ctx, obj, "lineTo", JS_NewCFunction(ctx, js_path2d_lineTo, "lineTo", 0));
    JS_SetPropertyStr(ctx, obj, "addPathByStrokingText", JS_NewCFunction(ctx, js_path2d_addPathByStrokingText, "addPathByStrokingText", 0));
    JS_SetPropertyStr(ctx, obj, "ellipse", JS_NewCFunction(ctx, js_path2d_ellipse, "ellipse", 0));
    JS_SetPropertyStr(ctx, obj, "closePath", JS_NewCFunction(ctx, js_path2d_closePath, "closePath", 0));
    JS_SetPropertyStr(ctx, obj, "addPath", JS_NewCFunction(ctx, js_path2d_addPath, "addPath", 0));
    JS_SetPropertyStr(ctx, global_obj, "Path2D", obj);
    return 0;
}

static JSValue js_drawingstyle_lineCap_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineCap getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_textBaseline_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.textBaseline getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_font_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.font getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_textAlign_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.textAlign getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_miterLimit_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.miterLimit getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_lineJoin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineJoin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_lineDashOffset_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineDashOffset getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_lineWidth_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineWidth getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_direction_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.direction getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_lineCap_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineCap setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_textBaseline_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.textBaseline setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_font_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.font setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_textAlign_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.textAlign setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_miterLimit_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.miterLimit setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_lineJoin_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineJoin setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_lineDashOffset_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineDashOffset setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_lineWidth_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.lineWidth setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_direction_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.direction setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_getLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.getLineDash() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_drawingstyle_setLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DrawingStyle.setLineDash() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_drawingstyle(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "lineCap");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_lineCap_getter, "lineCap", 0),
            JS_NewCFunction(ctx, js_drawingstyle_lineCap_setter, "lineCap", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "textBaseline");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_textBaseline_getter, "textBaseline", 0),
            JS_NewCFunction(ctx, js_drawingstyle_textBaseline_setter, "textBaseline", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "font");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_font_getter, "font", 0),
            JS_NewCFunction(ctx, js_drawingstyle_font_setter, "font", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "textAlign");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_textAlign_getter, "textAlign", 0),
            JS_NewCFunction(ctx, js_drawingstyle_textAlign_setter, "textAlign", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "miterLimit");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_miterLimit_getter, "miterLimit", 0),
            JS_NewCFunction(ctx, js_drawingstyle_miterLimit_setter, "miterLimit", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lineJoin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_lineJoin_getter, "lineJoin", 0),
            JS_NewCFunction(ctx, js_drawingstyle_lineJoin_setter, "lineJoin", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lineDashOffset");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_lineDashOffset_getter, "lineDashOffset", 0),
            JS_NewCFunction(ctx, js_drawingstyle_lineDashOffset_setter, "lineDashOffset", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lineWidth");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_lineWidth_getter, "lineWidth", 0),
            JS_NewCFunction(ctx, js_drawingstyle_lineWidth_setter, "lineWidth", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "direction");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_drawingstyle_direction_getter, "direction", 0),
            JS_NewCFunction(ctx, js_drawingstyle_direction_setter, "direction", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getLineDash", JS_NewCFunction(ctx, js_drawingstyle_getLineDash, "getLineDash", 0));
    JS_SetPropertyStr(ctx, obj, "setLineDash", JS_NewCFunction(ctx, js_drawingstyle_setLineDash, "setLineDash", 0));
    JS_SetPropertyStr(ctx, global_obj, "DrawingStyle", obj);
    return 0;
}

static JSValue js_textmetrics_fontBoundingBoxAscent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.fontBoundingBoxAscent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_ideographicBaseline_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.ideographicBaseline getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_actualBoundingBoxAscent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.actualBoundingBoxAscent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_alphabeticBaseline_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.alphabeticBaseline getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_width_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.width getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_actualBoundingBoxDescent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.actualBoundingBoxDescent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_emHeightDescent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.emHeightDescent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_actualBoundingBoxRight_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.actualBoundingBoxRight getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_actualBoundingBoxLeft_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.actualBoundingBoxLeft getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_emHeightAscent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.emHeightAscent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_hangingBaseline_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.hangingBaseline getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_textmetrics_fontBoundingBoxDescent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextMetrics.fontBoundingBoxDescent getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_textmetrics(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "fontBoundingBoxAscent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_fontBoundingBoxAscent_getter, "fontBoundingBoxAscent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ideographicBaseline");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_ideographicBaseline_getter, "ideographicBaseline", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "actualBoundingBoxAscent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_actualBoundingBoxAscent_getter, "actualBoundingBoxAscent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "alphabeticBaseline");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_alphabeticBaseline_getter, "alphabeticBaseline", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "width");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_width_getter, "width", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "actualBoundingBoxDescent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_actualBoundingBoxDescent_getter, "actualBoundingBoxDescent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "emHeightDescent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_emHeightDescent_getter, "emHeightDescent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "actualBoundingBoxRight");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_actualBoundingBoxRight_getter, "actualBoundingBoxRight", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "actualBoundingBoxLeft");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_actualBoundingBoxLeft_getter, "actualBoundingBoxLeft", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "emHeightAscent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_emHeightAscent_getter, "emHeightAscent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hangingBaseline");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_hangingBaseline_getter, "hangingBaseline", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "fontBoundingBoxDescent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_textmetrics_fontBoundingBoxDescent_getter, "fontBoundingBoxDescent", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "TextMetrics", obj);
    return 0;
}

static JSValue js_canvaspattern_setTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasPattern.setTransform() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_canvaspattern(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "setTransform", JS_NewCFunction(ctx, js_canvaspattern_setTransform, "setTransform", 0));
    JS_SetPropertyStr(ctx, global_obj, "CanvasPattern", obj);
    return 0;
}

static JSValue js_canvasgradient_addColorStop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasGradient.addColorStop() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_canvasgradient(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "addColorStop", JS_NewCFunction(ctx, js_canvasgradient_addColorStop, "addColorStop", 0));
    JS_SetPropertyStr(ctx, global_obj, "CanvasGradient", obj);
    return 0;
}

static JSValue js_canvasrenderingcontext2d_lineCap_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineCap getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowColor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowColor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineDashOffset_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineDashOffset getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_imageSmoothingEnabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.imageSmoothingEnabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineWidth_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineWidth getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_textBaseline_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.textBaseline getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowOffsetX_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowOffsetX getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineJoin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineJoin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowBlur_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowBlur getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_strokeStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.strokeStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_miterLimit_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.miterLimit getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_currentTransform_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.currentTransform getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowOffsetY_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowOffsetY getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_globalCompositeOperation_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.globalCompositeOperation getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_fillStyle_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.fillStyle getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_font_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.font getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_globalAlpha_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.globalAlpha getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_textAlign_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.textAlign getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_imageSmoothingQuality_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.imageSmoothingQuality getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_direction_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.direction getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineCap_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineCap setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowColor_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowColor setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineDashOffset_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineDashOffset setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_imageSmoothingEnabled_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.imageSmoothingEnabled setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineWidth_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineWidth setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_textBaseline_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.textBaseline setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowOffsetX_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowOffsetX setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineJoin_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineJoin setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowBlur_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowBlur setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_strokeStyle_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.strokeStyle setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_miterLimit_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.miterLimit setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_currentTransform_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.currentTransform setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_shadowOffsetY_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.shadowOffsetY setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_globalCompositeOperation_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.globalCompositeOperation setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_fillStyle_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.fillStyle setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_font_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.font setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_globalAlpha_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.globalAlpha setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_textAlign_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.textAlign setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_imageSmoothingQuality_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.imageSmoothingQuality setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_direction_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.direction setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_arcTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.arcTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_moveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.moveTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_beginPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.beginPath() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_rect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.rect() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_measureText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.measureText() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_strokeText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.strokeText() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_removeHitRegion(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.removeHitRegion() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_setTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.setTransform() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_bezierCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.bezierCurveTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_isPointInPath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.isPointInPath() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_arc(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.arc() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_clearHitRegions(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.clearHitRegions() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_addHitRegion(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.addHitRegion() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_restore(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.restore() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_fill(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.fill() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_closePath(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.closePath() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_quadraticCurveTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.quadraticCurveTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_resetClip(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.resetClip() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_save(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.save() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_isPointInStroke(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.isPointInStroke() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_commit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.commit() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_drawImage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.drawImage() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_rotate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.rotate() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_fillText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.fillText() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_scale(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.scale() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_scrollPathIntoView(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.scrollPathIntoView() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_getLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.getLineDash() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_translate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.translate() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_clearRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.clearRect() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_strokeRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.strokeRect() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_createPattern(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.createPattern() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_drawFocusIfNeeded(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.drawFocusIfNeeded() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_createRadialGradient(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.createRadialGradient() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_clip(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.clip() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_stroke(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.stroke() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_transform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.transform() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_resetTransform(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.resetTransform() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_lineTo(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.lineTo() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_fillRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.fillRect() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_createLinearGradient(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.createLinearGradient() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_ellipse(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.ellipse() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_canvasrenderingcontext2d_setLineDash(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasRenderingContext2D.setLineDash() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_canvasrenderingcontext2d(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "lineCap");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineCap_getter, "lineCap", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineCap_setter, "lineCap", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "shadowColor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowColor_getter, "shadowColor", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowColor_setter, "shadowColor", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lineDashOffset");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineDashOffset_getter, "lineDashOffset", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineDashOffset_setter, "lineDashOffset", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "imageSmoothingEnabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_imageSmoothingEnabled_getter, "imageSmoothingEnabled", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_imageSmoothingEnabled_setter, "imageSmoothingEnabled", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lineWidth");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineWidth_getter, "lineWidth", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineWidth_setter, "lineWidth", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "textBaseline");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_textBaseline_getter, "textBaseline", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_textBaseline_setter, "textBaseline", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "shadowOffsetX");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowOffsetX_getter, "shadowOffsetX", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowOffsetX_setter, "shadowOffsetX", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lineJoin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineJoin_getter, "lineJoin", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineJoin_setter, "lineJoin", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "shadowBlur");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowBlur_getter, "shadowBlur", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowBlur_setter, "shadowBlur", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "strokeStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_strokeStyle_getter, "strokeStyle", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_strokeStyle_setter, "strokeStyle", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "miterLimit");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_miterLimit_getter, "miterLimit", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_miterLimit_setter, "miterLimit", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "currentTransform");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_currentTransform_getter, "currentTransform", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_currentTransform_setter, "currentTransform", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "shadowOffsetY");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowOffsetY_getter, "shadowOffsetY", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_shadowOffsetY_setter, "shadowOffsetY", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "globalCompositeOperation");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_globalCompositeOperation_getter, "globalCompositeOperation", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_globalCompositeOperation_setter, "globalCompositeOperation", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "fillStyle");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_fillStyle_getter, "fillStyle", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_fillStyle_setter, "fillStyle", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "font");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_font_getter, "font", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_font_setter, "font", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "globalAlpha");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_globalAlpha_getter, "globalAlpha", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_globalAlpha_setter, "globalAlpha", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "textAlign");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_textAlign_getter, "textAlign", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_textAlign_setter, "textAlign", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "imageSmoothingQuality");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_imageSmoothingQuality_getter, "imageSmoothingQuality", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_imageSmoothingQuality_setter, "imageSmoothingQuality", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "direction");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_direction_getter, "direction", 0),
            JS_NewCFunction(ctx, js_canvasrenderingcontext2d_direction_setter, "direction", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "arcTo", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_arcTo, "arcTo", 0));
    JS_SetPropertyStr(ctx, obj, "moveTo", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_moveTo, "moveTo", 0));
    JS_SetPropertyStr(ctx, obj, "beginPath", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_beginPath, "beginPath", 0));
    JS_SetPropertyStr(ctx, obj, "rect", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_rect, "rect", 0));
    JS_SetPropertyStr(ctx, obj, "measureText", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_measureText, "measureText", 0));
    JS_SetPropertyStr(ctx, obj, "strokeText", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_strokeText, "strokeText", 0));
    JS_SetPropertyStr(ctx, obj, "removeHitRegion", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_removeHitRegion, "removeHitRegion", 0));
    JS_SetPropertyStr(ctx, obj, "setTransform", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_setTransform, "setTransform", 0));
    JS_SetPropertyStr(ctx, obj, "bezierCurveTo", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_bezierCurveTo, "bezierCurveTo", 0));
    JS_SetPropertyStr(ctx, obj, "isPointInPath", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_isPointInPath, "isPointInPath", 0));
    JS_SetPropertyStr(ctx, obj, "arc", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_arc, "arc", 0));
    JS_SetPropertyStr(ctx, obj, "clearHitRegions", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_clearHitRegions, "clearHitRegions", 0));
    JS_SetPropertyStr(ctx, obj, "addHitRegion", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_addHitRegion, "addHitRegion", 0));
    JS_SetPropertyStr(ctx, obj, "restore", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_restore, "restore", 0));
    JS_SetPropertyStr(ctx, obj, "fill", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_fill, "fill", 0));
    JS_SetPropertyStr(ctx, obj, "closePath", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_closePath, "closePath", 0));
    JS_SetPropertyStr(ctx, obj, "quadraticCurveTo", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_quadraticCurveTo, "quadraticCurveTo", 0));
    JS_SetPropertyStr(ctx, obj, "resetClip", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_resetClip, "resetClip", 0));
    JS_SetPropertyStr(ctx, obj, "save", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_save, "save", 0));
    JS_SetPropertyStr(ctx, obj, "isPointInStroke", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_isPointInStroke, "isPointInStroke", 0));
    JS_SetPropertyStr(ctx, obj, "commit", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_commit, "commit", 0));
    JS_SetPropertyStr(ctx, obj, "drawImage", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_drawImage, "drawImage", 0));
    JS_SetPropertyStr(ctx, obj, "rotate", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_rotate, "rotate", 0));
    JS_SetPropertyStr(ctx, obj, "fillText", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_fillText, "fillText", 0));
    JS_SetPropertyStr(ctx, obj, "scale", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_scale, "scale", 0));
    JS_SetPropertyStr(ctx, obj, "scrollPathIntoView", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_scrollPathIntoView, "scrollPathIntoView", 0));
    JS_SetPropertyStr(ctx, obj, "getLineDash", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_getLineDash, "getLineDash", 0));
    JS_SetPropertyStr(ctx, obj, "translate", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_translate, "translate", 0));
    JS_SetPropertyStr(ctx, obj, "clearRect", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_clearRect, "clearRect", 0));
    JS_SetPropertyStr(ctx, obj, "strokeRect", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_strokeRect, "strokeRect", 0));
    JS_SetPropertyStr(ctx, obj, "createPattern", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_createPattern, "createPattern", 0));
    JS_SetPropertyStr(ctx, obj, "drawFocusIfNeeded", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_drawFocusIfNeeded, "drawFocusIfNeeded", 0));
    JS_SetPropertyStr(ctx, obj, "createRadialGradient", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_createRadialGradient, "createRadialGradient", 0));
    JS_SetPropertyStr(ctx, obj, "clip", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_clip, "clip", 0));
    JS_SetPropertyStr(ctx, obj, "stroke", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_stroke, "stroke", 0));
    JS_SetPropertyStr(ctx, obj, "transform", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_transform, "transform", 0));
    JS_SetPropertyStr(ctx, obj, "resetTransform", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_resetTransform, "resetTransform", 0));
    JS_SetPropertyStr(ctx, obj, "lineTo", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_lineTo, "lineTo", 0));
    JS_SetPropertyStr(ctx, obj, "fillRect", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_fillRect, "fillRect", 0));
    JS_SetPropertyStr(ctx, obj, "createLinearGradient", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_createLinearGradient, "createLinearGradient", 0));
    JS_SetPropertyStr(ctx, obj, "ellipse", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_ellipse, "ellipse", 0));
    JS_SetPropertyStr(ctx, obj, "setLineDash", JS_NewCFunction(ctx, js_canvasrenderingcontext2d_setLineDash, "setLineDash", 0));
    JS_SetPropertyStr(ctx, global_obj, "CanvasRenderingContext2D", obj);
    return 0;
}

static JSValue js_canvasproxy_setContext(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CanvasProxy.setContext() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_canvasproxy(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "setContext", JS_NewCFunction(ctx, js_canvasproxy_setContext, "setContext", 0));
    JS_SetPropertyStr(ctx, global_obj, "CanvasProxy", obj);
    return 0;
}

static JSValue js_htmlcanvaselement_probablySupportsContext(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCanvasElement.probablySupportsContext() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlcanvaselement_transferControlToProxy(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCanvasElement.transferControlToProxy() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlcanvaselement_setContext(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCanvasElement.setContext() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlcanvaselement_toBlob(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCanvasElement.toBlob() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlcanvaselement_toDataURL(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCanvasElement.toDataURL() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlcanvaselement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "probablySupportsContext", JS_NewCFunction(ctx, js_htmlcanvaselement_probablySupportsContext, "probablySupportsContext", 0));
    JS_SetPropertyStr(ctx, obj, "transferControlToProxy", JS_NewCFunction(ctx, js_htmlcanvaselement_transferControlToProxy, "transferControlToProxy", 0));
    JS_SetPropertyStr(ctx, obj, "setContext", JS_NewCFunction(ctx, js_htmlcanvaselement_setContext, "setContext", 0));
    JS_SetPropertyStr(ctx, obj, "toBlob", JS_NewCFunction(ctx, js_htmlcanvaselement_toBlob, "toBlob", 0));
    JS_SetPropertyStr(ctx, obj, "toDataURL", JS_NewCFunction(ctx, js_htmlcanvaselement_toDataURL, "toDataURL", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLCanvasElement", obj);
    return 0;
}

static JSValue js_htmltemplateelement_content_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTemplateElement.content getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltemplateelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "content");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltemplateelement_content_getter, "content", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLTemplateElement", obj);
    return 0;
}

static JSValue js_htmlscriptelement_nonce_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLScriptElement.nonce getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlscriptelement_crossOrigin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLScriptElement.crossOrigin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlscriptelement_async_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLScriptElement.async getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlscriptelement_nonce_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLScriptElement.nonce setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlscriptelement_crossOrigin_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLScriptElement.crossOrigin setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlscriptelement_async_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLScriptElement.async setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlscriptelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "nonce");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlscriptelement_nonce_getter, "nonce", 0),
            JS_NewCFunction(ctx, js_htmlscriptelement_nonce_setter, "nonce", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "crossOrigin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlscriptelement_crossOrigin_getter, "crossOrigin", 0),
            JS_NewCFunction(ctx, js_htmlscriptelement_crossOrigin_setter, "crossOrigin", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "async");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlscriptelement_async_getter, "async", 0),
            JS_NewCFunction(ctx, js_htmlscriptelement_async_setter, "async", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLScriptElement", obj);
    return 0;
}

static JSValue js_htmldialogelement_open_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDialogElement.open getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldialogelement_returnValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDialogElement.returnValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldialogelement_open_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDialogElement.open setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldialogelement_returnValue_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDialogElement.returnValue setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldialogelement_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDialogElement.close() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldialogelement_showModal(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDialogElement.showModal() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldialogelement_show(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDialogElement.show() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmldialogelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "open");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmldialogelement_open_getter, "open", 0),
            JS_NewCFunction(ctx, js_htmldialogelement_open_setter, "open", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "returnValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmldialogelement_returnValue_getter, "returnValue", 0),
            JS_NewCFunction(ctx, js_htmldialogelement_returnValue_setter, "returnValue", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_htmldialogelement_close, "close", 0));
    JS_SetPropertyStr(ctx, obj, "showModal", JS_NewCFunction(ctx, js_htmldialogelement_showModal, "showModal", 0));
    JS_SetPropertyStr(ctx, obj, "show", JS_NewCFunction(ctx, js_htmldialogelement_show, "show", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLDialogElement", obj);
    return 0;
}

static JSValue js_relatedevent_relatedTarget_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "RelatedEvent.relatedTarget getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_relatedevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "relatedTarget");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_relatedevent_relatedTarget_getter, "relatedTarget", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "RelatedEvent", obj);
    return 0;
}

static JSValue js_htmlmenuitemelement_checked_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.checked getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_command_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.command getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_label_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.label getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_disabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.disabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_default_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.default getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_icon_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.icon getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_radiogroup_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.radiogroup getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_checked_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.checked setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_label_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.label setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_disabled_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.disabled setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_default_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.default setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_icon_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.icon setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuitemelement_radiogroup_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuItemElement.radiogroup setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlmenuitemelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "checked");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_checked_getter, "checked", 0),
            JS_NewCFunction(ctx, js_htmlmenuitemelement_checked_setter, "checked", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "command");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_command_getter, "command", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "label");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_label_getter, "label", 0),
            JS_NewCFunction(ctx, js_htmlmenuitemelement_label_setter, "label", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlmenuitemelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "disabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_disabled_getter, "disabled", 0),
            JS_NewCFunction(ctx, js_htmlmenuitemelement_disabled_setter, "disabled", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "default");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_default_getter, "default", 0),
            JS_NewCFunction(ctx, js_htmlmenuitemelement_default_setter, "default", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "icon");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_icon_getter, "icon", 0),
            JS_NewCFunction(ctx, js_htmlmenuitemelement_icon_setter, "icon", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "radiogroup");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuitemelement_radiogroup_getter, "radiogroup", 0),
            JS_NewCFunction(ctx, js_htmlmenuitemelement_radiogroup_setter, "radiogroup", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLMenuItemElement", obj);
    return 0;
}

static JSValue js_htmlmenuelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuelement_label_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuElement.label getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmenuelement_label_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMenuElement.label setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlmenuelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlmenuelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "label");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmenuelement_label_getter, "label", 0),
            JS_NewCFunction(ctx, js_htmlmenuelement_label_setter, "label", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLMenuElement", obj);
    return 0;
}

static JSValue js_htmldetailselement_open_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDetailsElement.open getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldetailselement_open_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDetailsElement.open setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmldetailselement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "open");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmldetailselement_open_getter, "open", 0),
            JS_NewCFunction(ctx, js_htmldetailselement_open_setter, "open", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLDetailsElement", obj);
    return 0;
}

static JSValue js_validitystate_tooShort_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.tooShort getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_patternMismatch_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.patternMismatch getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_customError_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.customError getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_stepMismatch_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.stepMismatch getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_rangeUnderflow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.rangeUnderflow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_typeMismatch_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.typeMismatch getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_tooLong_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.tooLong getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_rangeOverflow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.rangeOverflow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_badInput_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.badInput getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_valueMissing_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.valueMissing getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_validitystate_valid_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ValidityState.valid getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_validitystate(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "tooShort");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_tooShort_getter, "tooShort", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "patternMismatch");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_patternMismatch_getter, "patternMismatch", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "customError");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_customError_getter, "customError", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "stepMismatch");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_stepMismatch_getter, "stepMismatch", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "rangeUnderflow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_rangeUnderflow_getter, "rangeUnderflow", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "typeMismatch");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_typeMismatch_getter, "typeMismatch", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "tooLong");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_tooLong_getter, "tooLong", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "rangeOverflow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_rangeOverflow_getter, "rangeOverflow", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "badInput");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_badInput_getter, "badInput", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "valueMissing");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_valueMissing_getter, "valueMissing", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "valid");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_validitystate_valid_getter, "valid", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "ValidityState", obj);
    return 0;
}

static JSValue js_autocompleteerrorevent_reason_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AutocompleteErrorEvent.reason getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_autocompleteerrorevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "reason");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_autocompleteerrorevent_reason_getter, "reason", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "AutocompleteErrorEvent", obj);
    return 0;
}

static JSValue js_htmllegendelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLegendElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmllegendelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmllegendelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLLegendElement", obj);
    return 0;
}

static JSValue js_htmlfieldsetelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_disabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.disabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_elements_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.elements getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_disabled_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.disabled setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_name_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.name setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlfieldsetelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFieldSetElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlfieldsetelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "disabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_disabled_getter, "disabled", 0),
            JS_NewCFunction(ctx, js_htmlfieldsetelement_disabled_setter, "disabled", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_name_getter, "name", 0),
            JS_NewCFunction(ctx, js_htmlfieldsetelement_name_setter, "name", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "elements");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_elements_getter, "elements", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlfieldsetelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmlfieldsetelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmlfieldsetelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmlfieldsetelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLFieldSetElement", obj);
    return 0;
}

static JSValue js_htmlmeterelement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_max_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.max getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_high_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.high getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_optimum_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.optimum getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_low_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.low getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_value_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.value getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_min_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.min getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_max_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.max setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_high_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.high setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_optimum_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.optimum setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_low_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.low setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_value_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.value setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmeterelement_min_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMeterElement.min setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlmeterelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmeterelement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "max");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmeterelement_max_getter, "max", 0),
            JS_NewCFunction(ctx, js_htmlmeterelement_max_setter, "max", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "high");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmeterelement_high_getter, "high", 0),
            JS_NewCFunction(ctx, js_htmlmeterelement_high_setter, "high", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "optimum");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmeterelement_optimum_getter, "optimum", 0),
            JS_NewCFunction(ctx, js_htmlmeterelement_optimum_setter, "optimum", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "low");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmeterelement_low_getter, "low", 0),
            JS_NewCFunction(ctx, js_htmlmeterelement_low_setter, "low", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "value");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmeterelement_value_getter, "value", 0),
            JS_NewCFunction(ctx, js_htmlmeterelement_value_setter, "value", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "min");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmeterelement_min_getter, "min", 0),
            JS_NewCFunction(ctx, js_htmlmeterelement_min_setter, "min", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLMeterElement", obj);
    return 0;
}

static JSValue js_htmlprogresselement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLProgressElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlprogresselement_max_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLProgressElement.max getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlprogresselement_position_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLProgressElement.position getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlprogresselement_value_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLProgressElement.value getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlprogresselement_max_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLProgressElement.max setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlprogresselement_value_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLProgressElement.value setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlprogresselement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlprogresselement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "max");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlprogresselement_max_getter, "max", 0),
            JS_NewCFunction(ctx, js_htmlprogresselement_max_setter, "max", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "position");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlprogresselement_position_getter, "position", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "value");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlprogresselement_value_getter, "value", 0),
            JS_NewCFunction(ctx, js_htmlprogresselement_value_setter, "value", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLProgressElement", obj);
    return 0;
}

static JSValue js_htmloutputelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_htmlFor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.htmlFor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_value_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.value getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_defaultValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.defaultValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_defaultValue_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.defaultValue setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_name_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.name setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_value_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.value setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloutputelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOutputElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmloutputelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_name_getter, "name", 0),
            JS_NewCFunction(ctx, js_htmloutputelement_name_setter, "name", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "htmlFor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_htmlFor_getter, "htmlFor", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "value");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_value_getter, "value", 0),
            JS_NewCFunction(ctx, js_htmloutputelement_value_setter, "value", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "defaultValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_defaultValue_getter, "defaultValue", 0),
            JS_NewCFunction(ctx, js_htmloutputelement_defaultValue_setter, "defaultValue", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloutputelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmloutputelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmloutputelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmloutputelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLOutputElement", obj);
    return 0;
}

static JSValue js_htmlkeygenelement_keytype_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.keytype getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_challenge_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.challenge getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_autofocus_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.autofocus getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_disabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.disabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_keytype_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.keytype setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_challenge_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.challenge setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_autofocus_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.autofocus setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_disabled_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.disabled setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_name_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.name setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlkeygenelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLKeygenElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlkeygenelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "keytype");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_keytype_getter, "keytype", 0),
            JS_NewCFunction(ctx, js_htmlkeygenelement_keytype_setter, "keytype", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "challenge");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_challenge_getter, "challenge", 0),
            JS_NewCFunction(ctx, js_htmlkeygenelement_challenge_setter, "challenge", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autofocus");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_autofocus_getter, "autofocus", 0),
            JS_NewCFunction(ctx, js_htmlkeygenelement_autofocus_setter, "autofocus", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "disabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_disabled_getter, "disabled", 0),
            JS_NewCFunction(ctx, js_htmlkeygenelement_disabled_setter, "disabled", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_name_getter, "name", 0),
            JS_NewCFunction(ctx, js_htmlkeygenelement_name_setter, "name", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlkeygenelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmlkeygenelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmlkeygenelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmlkeygenelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLKeygenElement", obj);
    return 0;
}

static JSValue js_htmltextareaelement_cols_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.cols getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_selectionEnd_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.selectionEnd getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_autofocus_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.autofocus getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_wrap_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.wrap getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_dirName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.dirName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_textLength_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.textLength getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_selectionDirection_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.selectionDirection getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_autocomplete_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.autocomplete getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_maxLength_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.maxLength getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_rows_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.rows getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_minLength_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.minLength getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_selectionStart_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.selectionStart getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_placeholder_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.placeholder getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_inputMode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.inputMode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_required_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.required getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_maxLength_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.maxLength setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_selectionStart_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.selectionStart setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_cols_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.cols setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_selectionEnd_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.selectionEnd setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_dirName_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.dirName setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_autofocus_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.autofocus setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_placeholder_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.placeholder setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_rows_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.rows setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_minLength_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.minLength setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_inputMode_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.inputMode setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_required_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.required setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_selectionDirection_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.selectionDirection setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_wrap_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.wrap setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_autocomplete_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.autocomplete setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_setSelectionRange(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.setSelectionRange() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_setRangeText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.setRangeText() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_select(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.select() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltextareaelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTextAreaElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltextareaelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "cols");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_cols_getter, "cols", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_cols_setter, "cols", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectionEnd");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_selectionEnd_getter, "selectionEnd", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_selectionEnd_setter, "selectionEnd", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autofocus");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_autofocus_getter, "autofocus", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_autofocus_setter, "autofocus", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "wrap");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_wrap_getter, "wrap", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_wrap_setter, "wrap", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "dirName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_dirName_getter, "dirName", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_dirName_setter, "dirName", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "textLength");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_textLength_getter, "textLength", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectionDirection");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_selectionDirection_getter, "selectionDirection", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_selectionDirection_setter, "selectionDirection", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autocomplete");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_autocomplete_getter, "autocomplete", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_autocomplete_setter, "autocomplete", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "maxLength");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_maxLength_getter, "maxLength", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_maxLength_setter, "maxLength", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "rows");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_rows_getter, "rows", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_rows_setter, "rows", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "minLength");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_minLength_getter, "minLength", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_minLength_setter, "minLength", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectionStart");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_selectionStart_getter, "selectionStart", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_selectionStart_setter, "selectionStart", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "placeholder");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_placeholder_getter, "placeholder", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_placeholder_setter, "placeholder", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "inputMode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_inputMode_getter, "inputMode", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_inputMode_setter, "inputMode", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "required");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltextareaelement_required_getter, "required", 0),
            JS_NewCFunction(ctx, js_htmltextareaelement_required_setter, "required", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "setSelectionRange", JS_NewCFunction(ctx, js_htmltextareaelement_setSelectionRange, "setSelectionRange", 0));
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmltextareaelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setRangeText", JS_NewCFunction(ctx, js_htmltextareaelement_setRangeText, "setRangeText", 0));
    JS_SetPropertyStr(ctx, obj, "select", JS_NewCFunction(ctx, js_htmltextareaelement_select, "select", 0));
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmltextareaelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmltextareaelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLTextAreaElement", obj);
    return 0;
}

static JSValue js_htmloptionelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptionelement_index_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionElement.index getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptionelement_text_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionElement.text setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmloptionelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloptionelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "index");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloptionelement_index_getter, "index", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "text");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_htmloptionelement_text_setter, "text", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLOptionElement", obj);
    return 0;
}

static JSValue js_htmloptgroupelement_disabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptGroupElement.disabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptgroupelement_label_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptGroupElement.label getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptgroupelement_disabled_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptGroupElement.disabled setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptgroupelement_label_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptGroupElement.label setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmloptgroupelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "disabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloptgroupelement_disabled_getter, "disabled", 0),
            JS_NewCFunction(ctx, js_htmloptgroupelement_disabled_setter, "disabled", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "label");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloptgroupelement_label_getter, "label", 0),
            JS_NewCFunction(ctx, js_htmloptgroupelement_label_setter, "label", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLOptGroupElement", obj);
    return 0;
}

static JSValue js_htmldatalistelement_options_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDataListElement.options getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmldatalistelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "options");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmldatalistelement_options_getter, "options", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLDataListElement", obj);
    return 0;
}

static JSValue js_htmlselectelement_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_autofocus_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.autofocus getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_size_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.size getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_options_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.options getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_selectedOptions_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.selectedOptions getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_required_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.required getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_selectedIndex_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.selectedIndex getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_autocomplete_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.autocomplete getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_length_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.length setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_autofocus_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.autofocus setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_size_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.size setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_required_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.required setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_selectedIndex_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.selectedIndex setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_autocomplete_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.autocomplete setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.item() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.namedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.remove() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlselectelement_add(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSelectElement.add() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlselectelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_length_getter, "length", 0),
            JS_NewCFunction(ctx, js_htmlselectelement_length_setter, "length", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autofocus");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_autofocus_getter, "autofocus", 0),
            JS_NewCFunction(ctx, js_htmlselectelement_autofocus_setter, "autofocus", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "size");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_size_getter, "size", 0),
            JS_NewCFunction(ctx, js_htmlselectelement_size_setter, "size", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "options");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_options_getter, "options", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectedOptions");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_selectedOptions_getter, "selectedOptions", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "required");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_required_getter, "required", 0),
            JS_NewCFunction(ctx, js_htmlselectelement_required_setter, "required", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectedIndex");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_selectedIndex_getter, "selectedIndex", 0),
            JS_NewCFunction(ctx, js_htmlselectelement_selectedIndex_setter, "selectedIndex", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autocomplete");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlselectelement_autocomplete_getter, "autocomplete", 0),
            JS_NewCFunction(ctx, js_htmlselectelement_autocomplete_setter, "autocomplete", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmlselectelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_htmlselectelement_item, "item", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmlselectelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmlselectelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "namedItem", JS_NewCFunction(ctx, js_htmlselectelement_namedItem, "namedItem", 0));
    JS_SetPropertyStr(ctx, obj, "remove", JS_NewCFunction(ctx, js_htmlselectelement_remove, "remove", 0));
    JS_SetPropertyStr(ctx, obj, "add", JS_NewCFunction(ctx, js_htmlselectelement_add, "add", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLSelectElement", obj);
    return 0;
}

static JSValue js_htmlbuttonelement_formAction_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formAction getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formTarget_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formTarget getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_autofocus_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.autofocus getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formMethod_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formMethod getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formNoValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formNoValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formEnctype_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formEnctype getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_menu_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.menu getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formAction_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formAction setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formTarget_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formTarget setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_autofocus_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.autofocus setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formMethod_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formMethod setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formNoValidate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formNoValidate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_formEnctype_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.formEnctype setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_menu_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.menu setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlbuttonelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLButtonElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlbuttonelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "formAction");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_formAction_getter, "formAction", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_formAction_setter, "formAction", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formTarget");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_formTarget_getter, "formTarget", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_formTarget_setter, "formTarget", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autofocus");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_autofocus_getter, "autofocus", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_autofocus_setter, "autofocus", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formMethod");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_formMethod_getter, "formMethod", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_formMethod_setter, "formMethod", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formNoValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_formNoValidate_getter, "formNoValidate", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_formNoValidate_setter, "formNoValidate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formEnctype");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_formEnctype_getter, "formEnctype", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_formEnctype_setter, "formEnctype", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "menu");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlbuttonelement_menu_getter, "menu", 0),
            JS_NewCFunction(ctx, js_htmlbuttonelement_menu_setter, "menu", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmlbuttonelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmlbuttonelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmlbuttonelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLButtonElement", obj);
    return 0;
}

static JSValue js_htmlinputelement_selectionEnd_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.selectionEnd getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_autofocus_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.autofocus getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_labels_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.labels getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueHigh_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueHigh getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_multiple_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.multiple getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_dirName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.dirName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formNoValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formNoValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_selectionDirection_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.selectionDirection getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_autocomplete_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.autocomplete getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueLow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueLow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formAction_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formAction getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formTarget_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formTarget getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_max_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.max getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_width_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.width getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_pattern_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.pattern getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_height_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.height getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_minLength_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.minLength getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_files_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.files getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_indeterminate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.indeterminate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_min_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.min getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueAsNumber_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueAsNumber getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_selectionStart_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.selectionStart getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_step_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.step getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_placeholder_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.placeholder getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formMethod_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formMethod getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_list_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.list getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formEnctype_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formEnctype getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_inputMode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.inputMode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_required_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.required getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueAsDate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueAsDate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_selectionEnd_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.selectionEnd setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_autofocus_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.autofocus setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueHigh_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueHigh setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_multiple_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.multiple setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_dirName_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.dirName setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formNoValidate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formNoValidate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_selectionDirection_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.selectionDirection setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_autocomplete_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.autocomplete setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueLow_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueLow setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formAction_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formAction setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formTarget_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formTarget setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_max_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.max setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_width_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.width setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_pattern_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.pattern setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_height_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.height setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_minLength_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.minLength setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_indeterminate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.indeterminate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_min_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.min setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueAsNumber_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueAsNumber setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_selectionStart_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.selectionStart setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_step_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.step setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_placeholder_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.placeholder setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formMethod_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formMethod setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_formEnctype_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.formEnctype setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_inputMode_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.inputMode setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_required_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.required setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_valueAsDate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.valueAsDate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_setSelectionRange(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.setSelectionRange() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_stepUp(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.stepUp() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_setRangeText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.setRangeText() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_select(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.select() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_stepDown(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.stepDown() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlinputelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLInputElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlinputelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "selectionEnd");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_selectionEnd_getter, "selectionEnd", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_selectionEnd_setter, "selectionEnd", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autofocus");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_autofocus_getter, "autofocus", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_autofocus_setter, "autofocus", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "labels");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_labels_getter, "labels", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "valueHigh");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_valueHigh_getter, "valueHigh", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_valueHigh_setter, "valueHigh", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "multiple");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_multiple_getter, "multiple", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_multiple_setter, "multiple", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "dirName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_dirName_getter, "dirName", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_dirName_setter, "dirName", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formNoValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_formNoValidate_getter, "formNoValidate", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_formNoValidate_setter, "formNoValidate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectionDirection");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_selectionDirection_getter, "selectionDirection", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_selectionDirection_setter, "selectionDirection", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autocomplete");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_autocomplete_getter, "autocomplete", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_autocomplete_setter, "autocomplete", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "valueLow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_valueLow_getter, "valueLow", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_valueLow_setter, "valueLow", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formAction");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_formAction_getter, "formAction", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_formAction_setter, "formAction", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formTarget");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_formTarget_getter, "formTarget", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_formTarget_setter, "formTarget", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "max");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_max_getter, "max", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_max_setter, "max", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "width");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_width_getter, "width", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_width_setter, "width", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "pattern");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_pattern_getter, "pattern", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_pattern_setter, "pattern", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "height");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_height_getter, "height", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_height_setter, "height", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "minLength");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_minLength_getter, "minLength", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_minLength_setter, "minLength", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "files");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_files_getter, "files", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "indeterminate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_indeterminate_getter, "indeterminate", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_indeterminate_setter, "indeterminate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "min");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_min_getter, "min", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_min_setter, "min", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "valueAsNumber");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_valueAsNumber_getter, "valueAsNumber", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_valueAsNumber_setter, "valueAsNumber", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectionStart");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_selectionStart_getter, "selectionStart", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_selectionStart_setter, "selectionStart", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "step");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_step_getter, "step", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_step_setter, "step", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "placeholder");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_placeholder_getter, "placeholder", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_placeholder_setter, "placeholder", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formMethod");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_formMethod_getter, "formMethod", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_formMethod_setter, "formMethod", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "list");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_list_getter, "list", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "formEnctype");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_formEnctype_getter, "formEnctype", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_formEnctype_setter, "formEnctype", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "inputMode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_inputMode_getter, "inputMode", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_inputMode_setter, "inputMode", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "required");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_required_getter, "required", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_required_setter, "required", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "valueAsDate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlinputelement_valueAsDate_getter, "valueAsDate", 0),
            JS_NewCFunction(ctx, js_htmlinputelement_valueAsDate_setter, "valueAsDate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_htmlinputelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "setSelectionRange", JS_NewCFunction(ctx, js_htmlinputelement_setSelectionRange, "setSelectionRange", 0));
    JS_SetPropertyStr(ctx, obj, "stepUp", JS_NewCFunction(ctx, js_htmlinputelement_stepUp, "stepUp", 0));
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmlinputelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setRangeText", JS_NewCFunction(ctx, js_htmlinputelement_setRangeText, "setRangeText", 0));
    JS_SetPropertyStr(ctx, obj, "select", JS_NewCFunction(ctx, js_htmlinputelement_select, "select", 0));
    JS_SetPropertyStr(ctx, obj, "stepDown", JS_NewCFunction(ctx, js_htmlinputelement_stepDown, "stepDown", 0));
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmlinputelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmlinputelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLInputElement", obj);
    return 0;
}

static JSValue js_htmllabelelement_control_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLabelElement.control getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmllabelelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLabelElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmllabelelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "control");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmllabelelement_control_getter, "control", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmllabelelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLLabelElement", obj);
    return 0;
}

static JSValue js_htmlformelement_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_elements_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.elements getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_noValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.noValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_encoding_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.encoding getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_autocomplete_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.autocomplete getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_noValidate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.noValidate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_encoding_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.encoding setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_name_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.name setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_autocomplete_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.autocomplete setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_submit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.submit() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_requestAutocomplete(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.requestAutocomplete() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_reset(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.reset() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlformelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlformelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlformelement_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlformelement_name_getter, "name", 0),
            JS_NewCFunction(ctx, js_htmlformelement_name_setter, "name", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "elements");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlformelement_elements_getter, "elements", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "noValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlformelement_noValidate_getter, "noValidate", 0),
            JS_NewCFunction(ctx, js_htmlformelement_noValidate_setter, "noValidate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "encoding");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlformelement_encoding_getter, "encoding", 0),
            JS_NewCFunction(ctx, js_htmlformelement_encoding_setter, "encoding", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autocomplete");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlformelement_autocomplete_getter, "autocomplete", 0),
            JS_NewCFunction(ctx, js_htmlformelement_autocomplete_setter, "autocomplete", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "submit", JS_NewCFunction(ctx, js_htmlformelement_submit, "submit", 0));
    JS_SetPropertyStr(ctx, obj, "requestAutocomplete", JS_NewCFunction(ctx, js_htmlformelement_requestAutocomplete, "requestAutocomplete", 0));
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmlformelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, obj, "reset", JS_NewCFunction(ctx, js_htmlformelement_reset, "reset", 0));
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmlformelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLFormElement", obj);
    return 0;
}

static JSValue js_htmltablecellelement_headers_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableCellElement.headers getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltablecellelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "headers");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltablecellelement_headers_getter, "headers", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLTableCellElement", obj);
    return 0;
}

static JSValue js_htmltableheadercellelement_scope_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableHeaderCellElement.scope getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableheadercellelement_abbr_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableHeaderCellElement.abbr getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableheadercellelement_sorted_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableHeaderCellElement.sorted getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableheadercellelement_scope_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableHeaderCellElement.scope setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableheadercellelement_abbr_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableHeaderCellElement.abbr setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableheadercellelement_sorted_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableHeaderCellElement.sorted setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableheadercellelement_sort(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableHeaderCellElement.sort() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltableheadercellelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "scope");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableheadercellelement_scope_getter, "scope", 0),
            JS_NewCFunction(ctx, js_htmltableheadercellelement_scope_setter, "scope", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "abbr");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableheadercellelement_abbr_getter, "abbr", 0),
            JS_NewCFunction(ctx, js_htmltableheadercellelement_abbr_setter, "abbr", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sorted");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableheadercellelement_sorted_getter, "sorted", 0),
            JS_NewCFunction(ctx, js_htmltableheadercellelement_sorted_setter, "sorted", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "sort", JS_NewCFunction(ctx, js_htmltableheadercellelement_sort, "sort", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLTableHeaderCellElement", obj);
    return 0;
}

static JSValue js_htmltabledatacellelement_abbr_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableDataCellElement.abbr getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltabledatacellelement_abbr_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableDataCellElement.abbr setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltabledatacellelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "abbr");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltabledatacellelement_abbr_getter, "abbr", 0),
            JS_NewCFunction(ctx, js_htmltabledatacellelement_abbr_setter, "abbr", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLTableDataCellElement", obj);
    return 0;
}

static JSValue js_htmltablerowelement_cells_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableRowElement.cells getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltablerowelement_deleteCell(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableRowElement.deleteCell() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltablerowelement_insertCell(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableRowElement.insertCell() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltablerowelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "cells");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltablerowelement_cells_getter, "cells", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "deleteCell", JS_NewCFunction(ctx, js_htmltablerowelement_deleteCell, "deleteCell", 0));
    JS_SetPropertyStr(ctx, obj, "insertCell", JS_NewCFunction(ctx, js_htmltablerowelement_insertCell, "insertCell", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLTableRowElement", obj);
    return 0;
}

static JSValue js_htmltablesectionelement_rows_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableSectionElement.rows getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltablesectionelement_deleteRow(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableSectionElement.deleteRow() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltablesectionelement_insertRow(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableSectionElement.insertRow() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltablesectionelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "rows");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltablesectionelement_rows_getter, "rows", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "deleteRow", JS_NewCFunction(ctx, js_htmltablesectionelement_deleteRow, "deleteRow", 0));
    JS_SetPropertyStr(ctx, obj, "insertRow", JS_NewCFunction(ctx, js_htmltablesectionelement_insertRow, "insertRow", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLTableSectionElement", obj);
    return 0;
}

static JSValue js_htmltablecolelement_span_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableColElement.span getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltablecolelement_span_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableColElement.span setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltablecolelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "span");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltablecolelement_span_getter, "span", 0),
            JS_NewCFunction(ctx, js_htmltablecolelement_span_setter, "span", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLTableColElement", obj);
    return 0;
}

static JSValue js_htmltableelement_caption_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.caption getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_sortable_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.sortable getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_rows_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.rows getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_tHead_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.tHead getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_tBodies_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.tBodies getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_tFoot_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.tFoot getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_tFoot_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.tFoot setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_caption_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.caption setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_sortable_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.sortable setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_tHead_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.tHead setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_createCaption(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.createCaption() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_stopSorting(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.stopSorting() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_deleteCaption(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.deleteCaption() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_createTBody(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.createTBody() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_createTHead(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.createTHead() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_deleteTFoot(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.deleteTFoot() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_createTFoot(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.createTFoot() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_deleteRow(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.deleteRow() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_insertRow(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.insertRow() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltableelement_deleteTHead(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTableElement.deleteTHead() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltableelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "caption");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableelement_caption_getter, "caption", 0),
            JS_NewCFunction(ctx, js_htmltableelement_caption_setter, "caption", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sortable");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableelement_sortable_getter, "sortable", 0),
            JS_NewCFunction(ctx, js_htmltableelement_sortable_setter, "sortable", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "rows");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableelement_rows_getter, "rows", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "tHead");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableelement_tHead_getter, "tHead", 0),
            JS_NewCFunction(ctx, js_htmltableelement_tHead_setter, "tHead", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "tBodies");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableelement_tBodies_getter, "tBodies", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "tFoot");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltableelement_tFoot_getter, "tFoot", 0),
            JS_NewCFunction(ctx, js_htmltableelement_tFoot_setter, "tFoot", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "createCaption", JS_NewCFunction(ctx, js_htmltableelement_createCaption, "createCaption", 0));
    JS_SetPropertyStr(ctx, obj, "stopSorting", JS_NewCFunction(ctx, js_htmltableelement_stopSorting, "stopSorting", 0));
    JS_SetPropertyStr(ctx, obj, "deleteCaption", JS_NewCFunction(ctx, js_htmltableelement_deleteCaption, "deleteCaption", 0));
    JS_SetPropertyStr(ctx, obj, "createTBody", JS_NewCFunction(ctx, js_htmltableelement_createTBody, "createTBody", 0));
    JS_SetPropertyStr(ctx, obj, "createTHead", JS_NewCFunction(ctx, js_htmltableelement_createTHead, "createTHead", 0));
    JS_SetPropertyStr(ctx, obj, "deleteTFoot", JS_NewCFunction(ctx, js_htmltableelement_deleteTFoot, "deleteTFoot", 0));
    JS_SetPropertyStr(ctx, obj, "createTFoot", JS_NewCFunction(ctx, js_htmltableelement_createTFoot, "createTFoot", 0));
    JS_SetPropertyStr(ctx, obj, "deleteRow", JS_NewCFunction(ctx, js_htmltableelement_deleteRow, "deleteRow", 0));
    JS_SetPropertyStr(ctx, obj, "insertRow", JS_NewCFunction(ctx, js_htmltableelement_insertRow, "insertRow", 0));
    JS_SetPropertyStr(ctx, obj, "deleteTHead", JS_NewCFunction(ctx, js_htmltableelement_deleteTHead, "deleteTHead", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLTableElement", obj);
    return 0;
}

static JSValue js_htmlareaelement_origin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.origin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_pathname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.pathname getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_rel_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.rel getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_protocol_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.protocol getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_password_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.password getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_host_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.host getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_port_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.port getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_search_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.search getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_username_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.username getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_ping_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.ping getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_href_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.href getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_hostname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.hostname getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_hash_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.hash getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_relList_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.relList getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_download_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.download getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_hreflang_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.hreflang getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_pathname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.pathname setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_rel_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.rel setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_protocol_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.protocol setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_password_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.password setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_host_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.host setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_port_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.port setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_search_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.search setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_username_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.username setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_hash_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.hash setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_href_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.href setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_hostname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.hostname setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_download_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.download setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlareaelement_hreflang_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAreaElement.hreflang setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlareaelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "origin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_origin_getter, "origin", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "pathname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_pathname_getter, "pathname", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_pathname_setter, "pathname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "rel");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_rel_getter, "rel", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_rel_setter, "rel", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "protocol");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_protocol_getter, "protocol", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_protocol_setter, "protocol", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "password");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_password_getter, "password", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_password_setter, "password", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "host");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_host_getter, "host", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_host_setter, "host", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "port");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_port_getter, "port", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_port_setter, "port", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "search");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_search_getter, "search", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_search_setter, "search", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "username");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_username_getter, "username", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_username_setter, "username", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ping");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_ping_getter, "ping", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "href");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_href_getter, "href", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_href_setter, "href", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hostname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_hostname_getter, "hostname", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_hostname_setter, "hostname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hash");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_hash_getter, "hash", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_hash_setter, "hash", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "relList");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_relList_getter, "relList", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "download");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_download_getter, "download", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_download_setter, "download", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hreflang");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlareaelement_hreflang_getter, "hreflang", 0),
            JS_NewCFunction(ctx, js_htmlareaelement_hreflang_setter, "hreflang", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLAreaElement", obj);
    return 0;
}

static JSValue js_htmlmapelement_areas_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMapElement.areas getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlmapelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "areas");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmapelement_areas_getter, "areas", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLMapElement", obj);
    return 0;
}

static JSValue js_trackevent_track_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TrackEvent.track getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_trackevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "track");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_trackevent_track_getter, "track", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "TrackEvent", obj);
    return 0;
}

static JSValue js_timeranges_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TimeRanges.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_timeranges_end(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TimeRanges.end() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_timeranges_start(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TimeRanges.start() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_timeranges(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_timeranges_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "end", JS_NewCFunction(ctx, js_timeranges_end, "end", 0));
    JS_SetPropertyStr(ctx, obj, "start", JS_NewCFunction(ctx, js_timeranges_start, "start", 0));
    JS_SetPropertyStr(ctx, global_obj, "TimeRanges", obj);
    return 0;
}

static JSValue js_texttrackcue_id_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.id getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_track_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.track getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_onenter_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.onenter getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_onexit_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.onexit getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_endTime_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.endTime getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_startTime_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.startTime getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_pauseOnExit_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.pauseOnExit getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_id_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.id setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_onenter_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.onenter setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_onexit_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.onexit setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_endTime_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.endTime setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_startTime_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.startTime setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcue_pauseOnExit_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCue.pauseOnExit setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_texttrackcue(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "id");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcue_id_getter, "id", 0),
            JS_NewCFunction(ctx, js_texttrackcue_id_setter, "id", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "track");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcue_track_getter, "track", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onenter");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcue_onenter_getter, "onenter", 0),
            JS_NewCFunction(ctx, js_texttrackcue_onenter_setter, "onenter", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onexit");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcue_onexit_getter, "onexit", 0),
            JS_NewCFunction(ctx, js_texttrackcue_onexit_setter, "onexit", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "endTime");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcue_endTime_getter, "endTime", 0),
            JS_NewCFunction(ctx, js_texttrackcue_endTime_setter, "endTime", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "startTime");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcue_startTime_getter, "startTime", 0),
            JS_NewCFunction(ctx, js_texttrackcue_startTime_setter, "startTime", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "pauseOnExit");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcue_pauseOnExit_getter, "pauseOnExit", 0),
            JS_NewCFunction(ctx, js_texttrackcue_pauseOnExit_setter, "pauseOnExit", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "TextTrackCue", obj);
    return 0;
}

static JSValue js_texttrackcuelist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCueList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrackcuelist_getCueById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackCueList.getCueById() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_texttrackcuelist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrackcuelist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getCueById", JS_NewCFunction(ctx, js_texttrackcuelist_getCueById, "getCueById", 0));
    JS_SetPropertyStr(ctx, global_obj, "TextTrackCueList", obj);
    return 0;
}

static JSValue js_texttrack_id_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.id getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_label_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.label getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_language_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.language getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_mode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.mode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_cues_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.cues getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_kind_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.kind getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_oncuechange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.oncuechange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_activeCues_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.activeCues getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_inBandMetadataTrackDispatchType_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.inBandMetadataTrackDispatchType getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_oncuechange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.oncuechange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_mode_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.mode setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_removeCue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.removeCue() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttrack_addCue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrack.addCue() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_texttrack(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "id");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_id_getter, "id", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "label");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_label_getter, "label", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "language");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_language_getter, "language", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "mode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_mode_getter, "mode", 0),
            JS_NewCFunction(ctx, js_texttrack_mode_setter, "mode", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cues");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_cues_getter, "cues", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "kind");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_kind_getter, "kind", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "oncuechange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_oncuechange_getter, "oncuechange", 0),
            JS_NewCFunction(ctx, js_texttrack_oncuechange_setter, "oncuechange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "activeCues");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_activeCues_getter, "activeCues", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "inBandMetadataTrackDispatchType");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttrack_inBandMetadataTrackDispatchType_getter, "inBandMetadataTrackDispatchType", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "removeCue", JS_NewCFunction(ctx, js_texttrack_removeCue, "removeCue", 0));
    JS_SetPropertyStr(ctx, obj, "addCue", JS_NewCFunction(ctx, js_texttrack_addCue, "addCue", 0));
    JS_SetPropertyStr(ctx, global_obj, "TextTrack", obj);
    return 0;
}

static JSValue js_texttracklist_onremovetrack_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.onremovetrack getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttracklist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttracklist_onchange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.onchange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttracklist_onaddtrack_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.onaddtrack getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttracklist_onremovetrack_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.onremovetrack setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttracklist_onaddtrack_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.onaddtrack setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttracklist_onchange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.onchange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_texttracklist_getTrackById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TextTrackList.getTrackById() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_texttracklist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onremovetrack");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttracklist_onremovetrack_getter, "onremovetrack", 0),
            JS_NewCFunction(ctx, js_texttracklist_onremovetrack_setter, "onremovetrack", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttracklist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onchange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttracklist_onchange_getter, "onchange", 0),
            JS_NewCFunction(ctx, js_texttracklist_onchange_setter, "onchange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onaddtrack");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_texttracklist_onaddtrack_getter, "onaddtrack", 0),
            JS_NewCFunction(ctx, js_texttracklist_onaddtrack_setter, "onaddtrack", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getTrackById", JS_NewCFunction(ctx, js_texttracklist_getTrackById, "getTrackById", 0));
    JS_SetPropertyStr(ctx, global_obj, "TextTrackList", obj);
    return 0;
}

static JSValue js_mediacontroller_seekable_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.seekable getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_duration_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.duration getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onemptied_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onemptied getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onplaying_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onplaying getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_playbackRate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.playbackRate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_defaultPlaybackRate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.defaultPlaybackRate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_playbackState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.playbackState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onpause_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onpause getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onloadeddata_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onloadeddata getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_ontimeupdate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.ontimeupdate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onplay_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onplay getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_readyState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.readyState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onwaiting_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onwaiting getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_currentTime_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.currentTime getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_played_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.played getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_buffered_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.buffered getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onratechange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onratechange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_volume_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.volume getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onvolumechange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onvolumechange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_ondurationchange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.ondurationchange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_oncanplaythrough_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.oncanplaythrough getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_muted_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.muted getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onloadedmetadata_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onloadedmetadata getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_paused_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.paused getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_oncanplay_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.oncanplay getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onended_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onended getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onemptied_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onemptied setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onplaying_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onplaying setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_playbackRate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.playbackRate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_defaultPlaybackRate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.defaultPlaybackRate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onpause_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onpause setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onloadeddata_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onloadeddata setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_ontimeupdate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.ontimeupdate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onplay_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onplay setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onwaiting_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onwaiting setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_currentTime_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.currentTime setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onratechange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onratechange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_volume_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.volume setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onvolumechange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onvolumechange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_ondurationchange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.ondurationchange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_oncanplaythrough_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.oncanplaythrough setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_muted_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.muted setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onloadedmetadata_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onloadedmetadata setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_oncanplay_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.oncanplay setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_onended_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.onended setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_play(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.play() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_unpause(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.unpause() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mediacontroller_pause(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaController.pause() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mediacontroller(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "seekable");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_seekable_getter, "seekable", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "duration");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_duration_getter, "duration", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onemptied");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onemptied_getter, "onemptied", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onemptied_setter, "onemptied", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onplaying");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onplaying_getter, "onplaying", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onplaying_setter, "onplaying", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "playbackRate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_playbackRate_getter, "playbackRate", 0),
            JS_NewCFunction(ctx, js_mediacontroller_playbackRate_setter, "playbackRate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "defaultPlaybackRate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_defaultPlaybackRate_getter, "defaultPlaybackRate", 0),
            JS_NewCFunction(ctx, js_mediacontroller_defaultPlaybackRate_setter, "defaultPlaybackRate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "playbackState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_playbackState_getter, "playbackState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onpause");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onpause_getter, "onpause", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onpause_setter, "onpause", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onloadeddata");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onloadeddata_getter, "onloadeddata", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onloadeddata_setter, "onloadeddata", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ontimeupdate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_ontimeupdate_getter, "ontimeupdate", 0),
            JS_NewCFunction(ctx, js_mediacontroller_ontimeupdate_setter, "ontimeupdate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onplay");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onplay_getter, "onplay", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onplay_setter, "onplay", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "readyState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_readyState_getter, "readyState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onwaiting");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onwaiting_getter, "onwaiting", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onwaiting_setter, "onwaiting", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "currentTime");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_currentTime_getter, "currentTime", 0),
            JS_NewCFunction(ctx, js_mediacontroller_currentTime_setter, "currentTime", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "played");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_played_getter, "played", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "buffered");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_buffered_getter, "buffered", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onratechange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onratechange_getter, "onratechange", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onratechange_setter, "onratechange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "volume");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_volume_getter, "volume", 0),
            JS_NewCFunction(ctx, js_mediacontroller_volume_setter, "volume", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onvolumechange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onvolumechange_getter, "onvolumechange", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onvolumechange_setter, "onvolumechange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ondurationchange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_ondurationchange_getter, "ondurationchange", 0),
            JS_NewCFunction(ctx, js_mediacontroller_ondurationchange_setter, "ondurationchange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "oncanplaythrough");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_oncanplaythrough_getter, "oncanplaythrough", 0),
            JS_NewCFunction(ctx, js_mediacontroller_oncanplaythrough_setter, "oncanplaythrough", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "muted");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_muted_getter, "muted", 0),
            JS_NewCFunction(ctx, js_mediacontroller_muted_setter, "muted", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onloadedmetadata");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onloadedmetadata_getter, "onloadedmetadata", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onloadedmetadata_setter, "onloadedmetadata", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "paused");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_paused_getter, "paused", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "oncanplay");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_oncanplay_getter, "oncanplay", 0),
            JS_NewCFunction(ctx, js_mediacontroller_oncanplay_setter, "oncanplay", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onended");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediacontroller_onended_getter, "onended", 0),
            JS_NewCFunction(ctx, js_mediacontroller_onended_setter, "onended", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "play", JS_NewCFunction(ctx, js_mediacontroller_play, "play", 0));
    JS_SetPropertyStr(ctx, obj, "unpause", JS_NewCFunction(ctx, js_mediacontroller_unpause, "unpause", 0));
    JS_SetPropertyStr(ctx, obj, "pause", JS_NewCFunction(ctx, js_mediacontroller_pause, "pause", 0));
    JS_SetPropertyStr(ctx, global_obj, "MediaController", obj);
    return 0;
}

static JSValue js_videotrack_label_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrack.label getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotrack_id_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrack.id getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotrack_language_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrack.language getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotrack_selected_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrack.selected getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotrack_kind_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrack.kind getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotrack_selected_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrack.selected setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_videotrack(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "label");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotrack_label_getter, "label", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "id");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotrack_id_getter, "id", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "language");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotrack_language_getter, "language", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selected");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotrack_selected_getter, "selected", 0),
            JS_NewCFunction(ctx, js_videotrack_selected_setter, "selected", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "kind");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotrack_kind_getter, "kind", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "VideoTrack", obj);
    return 0;
}

static JSValue js_videotracklist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_onchange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.onchange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_onaddtrack_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.onaddtrack getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_onremovetrack_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.onremovetrack getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_selectedIndex_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.selectedIndex getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_onremovetrack_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.onremovetrack setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_onaddtrack_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.onaddtrack setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_onchange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.onchange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_videotracklist_getTrackById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "VideoTrackList.getTrackById() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_videotracklist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotracklist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onchange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotracklist_onchange_getter, "onchange", 0),
            JS_NewCFunction(ctx, js_videotracklist_onchange_setter, "onchange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onaddtrack");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotracklist_onaddtrack_getter, "onaddtrack", 0),
            JS_NewCFunction(ctx, js_videotracklist_onaddtrack_setter, "onaddtrack", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onremovetrack");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotracklist_onremovetrack_getter, "onremovetrack", 0),
            JS_NewCFunction(ctx, js_videotracklist_onremovetrack_setter, "onremovetrack", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectedIndex");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_videotracklist_selectedIndex_getter, "selectedIndex", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getTrackById", JS_NewCFunction(ctx, js_videotracklist_getTrackById, "getTrackById", 0));
    JS_SetPropertyStr(ctx, global_obj, "VideoTrackList", obj);
    return 0;
}

static JSValue js_audiotrack_label_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrack.label getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotrack_id_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrack.id getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotrack_language_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrack.language getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotrack_kind_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrack.kind getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotrack_enabled_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrack.enabled getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotrack_enabled_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrack.enabled setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_audiotrack(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "label");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotrack_label_getter, "label", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "id");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotrack_id_getter, "id", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "language");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotrack_language_getter, "language", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "kind");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotrack_kind_getter, "kind", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "enabled");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotrack_enabled_getter, "enabled", 0),
            JS_NewCFunction(ctx, js_audiotrack_enabled_setter, "enabled", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "AudioTrack", obj);
    return 0;
}

static JSValue js_audiotracklist_onremovetrack_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.onremovetrack getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotracklist_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotracklist_onchange_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.onchange getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotracklist_onaddtrack_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.onaddtrack getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotracklist_onremovetrack_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.onremovetrack setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotracklist_onaddtrack_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.onaddtrack setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotracklist_onchange_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.onchange setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_audiotracklist_getTrackById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "AudioTrackList.getTrackById() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_audiotracklist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "onremovetrack");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotracklist_onremovetrack_getter, "onremovetrack", 0),
            JS_NewCFunction(ctx, js_audiotracklist_onremovetrack_setter, "onremovetrack", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotracklist_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onchange");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotracklist_onchange_getter, "onchange", 0),
            JS_NewCFunction(ctx, js_audiotracklist_onchange_setter, "onchange", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onaddtrack");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_audiotracklist_onaddtrack_getter, "onaddtrack", 0),
            JS_NewCFunction(ctx, js_audiotracklist_onaddtrack_setter, "onaddtrack", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getTrackById", JS_NewCFunction(ctx, js_audiotracklist_getTrackById, "getTrackById", 0));
    JS_SetPropertyStr(ctx, global_obj, "AudioTrackList", obj);
    return 0;
}

static JSValue js_mediaerror_code_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MediaError.code getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mediaerror(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "code");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mediaerror_code_getter, "code", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "MediaError", obj);
    return 0;
}

static JSValue js_htmlmediaelement_seekable_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.seekable getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_controls_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.controls getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_audioTracks_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.audioTracks getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_crossOrigin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.crossOrigin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_videoTracks_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.videoTracks getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_controller_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.controller getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_textTracks_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.textTracks getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_playbackRate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.playbackRate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_networkState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.networkState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_error_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.error getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_defaultPlaybackRate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.defaultPlaybackRate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_preload_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.preload getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_loop_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.loop getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_seeking_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.seeking getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_src_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.src getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_ended_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.ended getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_defaultMuted_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.defaultMuted getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_readyState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.readyState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_currentTime_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.currentTime getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_played_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.played getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_buffered_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.buffered getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_autoplay_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.autoplay getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_volume_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.volume getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_currentSrc_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.currentSrc getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_muted_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.muted getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_srcObject_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.srcObject getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_mediaGroup_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.mediaGroup getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_paused_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.paused getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_duration_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.duration getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_crossOrigin_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.crossOrigin setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_autoplay_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.autoplay setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_controller_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.controller setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_controls_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.controls setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_volume_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.volume setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_playbackRate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.playbackRate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_muted_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.muted setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_srcObject_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.srcObject setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_defaultPlaybackRate_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.defaultPlaybackRate setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_mediaGroup_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.mediaGroup setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_preload_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.preload setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_loop_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.loop setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_defaultMuted_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.defaultMuted setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_currentTime_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.currentTime setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_src_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.src setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_play(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.play() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_pause(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.pause() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.load() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_canPlayType(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.canPlayType() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_addTextTrack(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.addTextTrack() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_getStartDate(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.getStartDate() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmediaelement_fastSeek(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLMediaElement.fastSeek() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlmediaelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "seekable");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_seekable_getter, "seekable", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "controls");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_controls_getter, "controls", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_controls_setter, "controls", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "audioTracks");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_audioTracks_getter, "audioTracks", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "crossOrigin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_crossOrigin_getter, "crossOrigin", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_crossOrigin_setter, "crossOrigin", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "videoTracks");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_videoTracks_getter, "videoTracks", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "controller");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_controller_getter, "controller", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_controller_setter, "controller", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "textTracks");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_textTracks_getter, "textTracks", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "playbackRate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_playbackRate_getter, "playbackRate", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_playbackRate_setter, "playbackRate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "networkState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_networkState_getter, "networkState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "error");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_error_getter, "error", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "defaultPlaybackRate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_defaultPlaybackRate_getter, "defaultPlaybackRate", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_defaultPlaybackRate_setter, "defaultPlaybackRate", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "preload");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_preload_getter, "preload", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_preload_setter, "preload", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "loop");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_loop_getter, "loop", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_loop_setter, "loop", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "seeking");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_seeking_getter, "seeking", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "src");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_src_getter, "src", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_src_setter, "src", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ended");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_ended_getter, "ended", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "defaultMuted");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_defaultMuted_getter, "defaultMuted", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_defaultMuted_setter, "defaultMuted", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "readyState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_readyState_getter, "readyState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "currentTime");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_currentTime_getter, "currentTime", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_currentTime_setter, "currentTime", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "played");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_played_getter, "played", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "buffered");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_buffered_getter, "buffered", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "autoplay");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_autoplay_getter, "autoplay", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_autoplay_setter, "autoplay", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "volume");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_volume_getter, "volume", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_volume_setter, "volume", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "currentSrc");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_currentSrc_getter, "currentSrc", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "muted");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_muted_getter, "muted", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_muted_setter, "muted", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "srcObject");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_srcObject_getter, "srcObject", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_srcObject_setter, "srcObject", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "mediaGroup");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_mediaGroup_getter, "mediaGroup", 0),
            JS_NewCFunction(ctx, js_htmlmediaelement_mediaGroup_setter, "mediaGroup", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "paused");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_paused_getter, "paused", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "duration");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmediaelement_duration_getter, "duration", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "play", JS_NewCFunction(ctx, js_htmlmediaelement_play, "play", 0));
    JS_SetPropertyStr(ctx, obj, "pause", JS_NewCFunction(ctx, js_htmlmediaelement_pause, "pause", 0));
    JS_SetPropertyStr(ctx, obj, "load", JS_NewCFunction(ctx, js_htmlmediaelement_load, "load", 0));
    JS_SetPropertyStr(ctx, obj, "canPlayType", JS_NewCFunction(ctx, js_htmlmediaelement_canPlayType, "canPlayType", 0));
    JS_SetPropertyStr(ctx, obj, "addTextTrack", JS_NewCFunction(ctx, js_htmlmediaelement_addTextTrack, "addTextTrack", 0));
    JS_SetPropertyStr(ctx, obj, "getStartDate", JS_NewCFunction(ctx, js_htmlmediaelement_getStartDate, "getStartDate", 0));
    JS_SetPropertyStr(ctx, obj, "fastSeek", JS_NewCFunction(ctx, js_htmlmediaelement_fastSeek, "fastSeek", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLMediaElement", obj);
    return 0;
}

static JSValue js_htmltrackelement_label_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.label getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_default_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.default getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_track_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.track getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_srclang_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.srclang getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_kind_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.kind getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_readyState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.readyState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_src_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.src getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_label_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.label setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_default_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.default setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_srclang_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.srclang setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_kind_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.kind setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltrackelement_src_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTrackElement.src setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltrackelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "label");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltrackelement_label_getter, "label", 0),
            JS_NewCFunction(ctx, js_htmltrackelement_label_setter, "label", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "default");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltrackelement_default_getter, "default", 0),
            JS_NewCFunction(ctx, js_htmltrackelement_default_setter, "default", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "track");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltrackelement_track_getter, "track", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "srclang");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltrackelement_srclang_getter, "srclang", 0),
            JS_NewCFunction(ctx, js_htmltrackelement_srclang_setter, "srclang", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "kind");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltrackelement_kind_getter, "kind", 0),
            JS_NewCFunction(ctx, js_htmltrackelement_kind_setter, "kind", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "readyState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltrackelement_readyState_getter, "readyState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "src");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltrackelement_src_getter, "src", 0),
            JS_NewCFunction(ctx, js_htmltrackelement_src_setter, "src", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLTrackElement", obj);
    return 0;
}

static JSValue js_htmlvideoelement_videoWidth_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.videoWidth getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlvideoelement_videoHeight_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.videoHeight getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlvideoelement_width_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.width getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlvideoelement_poster_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.poster getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlvideoelement_height_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.height getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlvideoelement_height_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.height setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlvideoelement_width_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.width setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlvideoelement_poster_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLVideoElement.poster setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlvideoelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "videoWidth");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlvideoelement_videoWidth_getter, "videoWidth", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "videoHeight");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlvideoelement_videoHeight_getter, "videoHeight", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "width");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlvideoelement_width_getter, "width", 0),
            JS_NewCFunction(ctx, js_htmlvideoelement_width_setter, "width", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "poster");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlvideoelement_poster_getter, "poster", 0),
            JS_NewCFunction(ctx, js_htmlvideoelement_poster_setter, "poster", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "height");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlvideoelement_height_getter, "height", 0),
            JS_NewCFunction(ctx, js_htmlvideoelement_height_setter, "height", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLVideoElement", obj);
    return 0;
}

static JSValue js_htmlobjectelement_vspace_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.vspace getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_hspace_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.hspace getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_form_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.form getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_contentWindow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.contentWindow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_validity_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.validity getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_validationMessage_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.validationMessage getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_contentDocument_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.contentDocument getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_typeMustMatch_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.typeMustMatch getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_willValidate_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.willValidate getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_typeMustMatch_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.typeMustMatch setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_hspace_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.hspace setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_vspace_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.vspace setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_reportValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.reportValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_checkValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.checkValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_setCustomValidity(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.setCustomValidity() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlobjectelement_getSVGDocument(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLObjectElement.getSVGDocument() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlobjectelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "vspace");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_vspace_getter, "vspace", 0),
            JS_NewCFunction(ctx, js_htmlobjectelement_vspace_setter, "vspace", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hspace");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_hspace_getter, "hspace", 0),
            JS_NewCFunction(ctx, js_htmlobjectelement_hspace_setter, "hspace", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "form");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_form_getter, "form", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contentWindow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_contentWindow_getter, "contentWindow", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validity");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_validity_getter, "validity", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "validationMessage");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_validationMessage_getter, "validationMessage", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contentDocument");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_contentDocument_getter, "contentDocument", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "typeMustMatch");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_typeMustMatch_getter, "typeMustMatch", 0),
            JS_NewCFunction(ctx, js_htmlobjectelement_typeMustMatch_setter, "typeMustMatch", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "willValidate");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlobjectelement_willValidate_getter, "willValidate", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "reportValidity", JS_NewCFunction(ctx, js_htmlobjectelement_reportValidity, "reportValidity", 0));
    JS_SetPropertyStr(ctx, obj, "checkValidity", JS_NewCFunction(ctx, js_htmlobjectelement_checkValidity, "checkValidity", 0));
    JS_SetPropertyStr(ctx, obj, "setCustomValidity", JS_NewCFunction(ctx, js_htmlobjectelement_setCustomValidity, "setCustomValidity", 0));
    JS_SetPropertyStr(ctx, obj, "getSVGDocument", JS_NewCFunction(ctx, js_htmlobjectelement_getSVGDocument, "getSVGDocument", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLObjectElement", obj);
    return 0;
}

static JSValue js_htmlembedelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_width_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.width getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_height_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.height getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_align_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.align getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_src_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.src getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_width_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.width setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_name_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.name setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_height_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.height setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_align_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.align setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_src_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.src setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlembedelement_getSVGDocument(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLEmbedElement.getSVGDocument() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlembedelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlembedelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlembedelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "width");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlembedelement_width_getter, "width", 0),
            JS_NewCFunction(ctx, js_htmlembedelement_width_setter, "width", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlembedelement_name_getter, "name", 0),
            JS_NewCFunction(ctx, js_htmlembedelement_name_setter, "name", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "height");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlembedelement_height_getter, "height", 0),
            JS_NewCFunction(ctx, js_htmlembedelement_height_setter, "height", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "align");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlembedelement_align_getter, "align", 0),
            JS_NewCFunction(ctx, js_htmlembedelement_align_setter, "align", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "src");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlembedelement_src_getter, "src", 0),
            JS_NewCFunction(ctx, js_htmlembedelement_src_setter, "src", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getSVGDocument", JS_NewCFunction(ctx, js_htmlembedelement_getSVGDocument, "getSVGDocument", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLEmbedElement", obj);
    return 0;
}

static JSValue js_htmliframeelement_seamless_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.seamless getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_srcdoc_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.srcdoc getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_contentWindow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.contentWindow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_contentDocument_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.contentDocument getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_allowFullscreen_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.allowFullscreen getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_sandbox_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.sandbox getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_seamless_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.seamless setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_srcdoc_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.srcdoc setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_allowFullscreen_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.allowFullscreen setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmliframeelement_getSVGDocument(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLIFrameElement.getSVGDocument() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmliframeelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "seamless");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmliframeelement_seamless_getter, "seamless", 0),
            JS_NewCFunction(ctx, js_htmliframeelement_seamless_setter, "seamless", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "srcdoc");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmliframeelement_srcdoc_getter, "srcdoc", 0),
            JS_NewCFunction(ctx, js_htmliframeelement_srcdoc_setter, "srcdoc", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contentWindow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmliframeelement_contentWindow_getter, "contentWindow", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contentDocument");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmliframeelement_contentDocument_getter, "contentDocument", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "allowFullscreen");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmliframeelement_allowFullscreen_getter, "allowFullscreen", 0),
            JS_NewCFunction(ctx, js_htmliframeelement_allowFullscreen_setter, "allowFullscreen", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sandbox");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmliframeelement_sandbox_getter, "sandbox", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "getSVGDocument", JS_NewCFunction(ctx, js_htmliframeelement_getSVGDocument, "getSVGDocument", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLIFrameElement", obj);
    return 0;
}

static JSValue js_htmlimageelement_crossOrigin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.crossOrigin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_sizes_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.sizes getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_currentSrc_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.currentSrc getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_complete_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.complete getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_naturalHeight_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.naturalHeight getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_naturalWidth_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.naturalWidth getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_lowsrc_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.lowsrc getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_srcset_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.srcset getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_crossOrigin_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.crossOrigin setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_srcset_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.srcset setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_sizes_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.sizes setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlimageelement_lowsrc_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLImageElement.lowsrc setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlimageelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "crossOrigin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_crossOrigin_getter, "crossOrigin", 0),
            JS_NewCFunction(ctx, js_htmlimageelement_crossOrigin_setter, "crossOrigin", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sizes");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_sizes_getter, "sizes", 0),
            JS_NewCFunction(ctx, js_htmlimageelement_sizes_setter, "sizes", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "currentSrc");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_currentSrc_getter, "currentSrc", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "complete");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_complete_getter, "complete", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "naturalHeight");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_naturalHeight_getter, "naturalHeight", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "naturalWidth");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_naturalWidth_getter, "naturalWidth", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lowsrc");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_lowsrc_getter, "lowsrc", 0),
            JS_NewCFunction(ctx, js_htmlimageelement_lowsrc_setter, "lowsrc", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "srcset");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlimageelement_srcset_getter, "srcset", 0),
            JS_NewCFunction(ctx, js_htmlimageelement_srcset_setter, "srcset", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLImageElement", obj);
    return 0;
}

static JSValue js_htmlsourceelement_sizes_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.sizes getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_media_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.media getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_srcset_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.srcset getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_src_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.src getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_sizes_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.sizes setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_media_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.media setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_srcset_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.srcset setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlsourceelement_src_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLSourceElement.src setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlsourceelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "sizes");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlsourceelement_sizes_getter, "sizes", 0),
            JS_NewCFunction(ctx, js_htmlsourceelement_sizes_setter, "sizes", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlsourceelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlsourceelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "media");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlsourceelement_media_getter, "media", 0),
            JS_NewCFunction(ctx, js_htmlsourceelement_media_setter, "media", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "srcset");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlsourceelement_srcset_getter, "srcset", 0),
            JS_NewCFunction(ctx, js_htmlsourceelement_srcset_setter, "srcset", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "src");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlsourceelement_src_getter, "src", 0),
            JS_NewCFunction(ctx, js_htmlsourceelement_src_setter, "src", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLSourceElement", obj);
    return 0;
}

static JSValue js_htmlmodelement_dateTime_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLModElement.dateTime getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmodelement_cite_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLModElement.cite getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmodelement_dateTime_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLModElement.dateTime setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlmodelement_cite_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLModElement.cite setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlmodelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "dateTime");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmodelement_dateTime_getter, "dateTime", 0),
            JS_NewCFunction(ctx, js_htmlmodelement_dateTime_setter, "dateTime", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cite");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlmodelement_cite_getter, "cite", 0),
            JS_NewCFunction(ctx, js_htmlmodelement_cite_setter, "cite", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLModElement", obj);
    return 0;
}

static JSValue js_htmltimeelement_dateTime_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTimeElement.dateTime getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmltimeelement_dateTime_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLTimeElement.dateTime setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmltimeelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "dateTime");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmltimeelement_dateTime_getter, "dateTime", 0),
            JS_NewCFunction(ctx, js_htmltimeelement_dateTime_setter, "dateTime", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLTimeElement", obj);
    return 0;
}

static JSValue js_htmldataelement_value_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDataElement.value getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldataelement_value_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDataElement.value setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmldataelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "value");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmldataelement_value_getter, "value", 0),
            JS_NewCFunction(ctx, js_htmldataelement_value_setter, "value", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLDataElement", obj);
    return 0;
}

static JSValue js_htmlanchorelement_origin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.origin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_pathname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.pathname getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_text_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.text getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_protocol_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.protocol getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_password_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.password getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_host_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.host getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_port_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.port getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_search_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.search getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_username_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.username getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_ping_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.ping getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_href_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.href getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_hostname_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.hostname getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_hash_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.hash getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_relList_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.relList getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_download_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.download getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_pathname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.pathname setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_text_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.text setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_protocol_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.protocol setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_password_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.password setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_host_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.host setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_port_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.port setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_search_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.search setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_username_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.username setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_hash_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.hash setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_href_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.href setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_hostname_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.hostname setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlanchorelement_download_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAnchorElement.download setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlanchorelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "origin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_origin_getter, "origin", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "pathname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_pathname_getter, "pathname", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_pathname_setter, "pathname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "text");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_text_getter, "text", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_text_setter, "text", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "protocol");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_protocol_getter, "protocol", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_protocol_setter, "protocol", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "password");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_password_getter, "password", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_password_setter, "password", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "host");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_host_getter, "host", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_host_setter, "host", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "port");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_port_getter, "port", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_port_setter, "port", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "search");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_search_getter, "search", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_search_setter, "search", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "username");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_username_getter, "username", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_username_setter, "username", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ping");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_ping_getter, "ping", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "href");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_href_getter, "href", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_href_setter, "href", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hostname");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_hostname_getter, "hostname", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_hostname_setter, "hostname", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "hash");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_hash_getter, "hash", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_hash_setter, "hash", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "relList");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_relList_getter, "relList", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "download");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlanchorelement_download_getter, "download", 0),
            JS_NewCFunction(ctx, js_htmlanchorelement_download_setter, "download", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLAnchorElement", obj);
    return 0;
}

static JSValue js_htmldlistelement_compact_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDListElement.compact getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmldlistelement_compact_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLDListElement.compact setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmldlistelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "compact");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmldlistelement_compact_getter, "compact", 0),
            JS_NewCFunction(ctx, js_htmldlistelement_compact_setter, "compact", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLDListElement", obj);
    return 0;
}

static JSValue js_htmlulistelement_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLUListElement.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlulistelement_compact_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLUListElement.compact getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlulistelement_type_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLUListElement.type setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlulistelement_compact_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLUListElement.compact setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlulistelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlulistelement_type_getter, "type", 0),
            JS_NewCFunction(ctx, js_htmlulistelement_type_setter, "type", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "compact");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlulistelement_compact_getter, "compact", 0),
            JS_NewCFunction(ctx, js_htmlulistelement_compact_setter, "compact", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLUListElement", obj);
    return 0;
}

static JSValue js_htmlolistelement_reversed_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOListElement.reversed getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlolistelement_reversed_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOListElement.reversed setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlolistelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "reversed");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlolistelement_reversed_getter, "reversed", 0),
            JS_NewCFunction(ctx, js_htmlolistelement_reversed_setter, "reversed", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLOListElement", obj);
    return 0;
}

static JSValue js_htmlhrelement_color_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLHRElement.color getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlhrelement_color_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLHRElement.color setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlhrelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "color");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlhrelement_color_getter, "color", 0),
            JS_NewCFunction(ctx, js_htmlhrelement_color_setter, "color", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLHRElement", obj);
    return 0;
}

static JSValue js_htmlstyleelement_nonce_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLStyleElement.nonce getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlstyleelement_scoped_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLStyleElement.scoped getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlstyleelement_sheet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLStyleElement.sheet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlstyleelement_nonce_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLStyleElement.nonce setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlstyleelement_scoped_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLStyleElement.scoped setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlstyleelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "nonce");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlstyleelement_nonce_getter, "nonce", 0),
            JS_NewCFunction(ctx, js_htmlstyleelement_nonce_setter, "nonce", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "scoped");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlstyleelement_scoped_getter, "scoped", 0),
            JS_NewCFunction(ctx, js_htmlstyleelement_scoped_setter, "scoped", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sheet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlstyleelement_sheet_getter, "sheet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLStyleElement", obj);
    return 0;
}

static JSValue js_htmllinkelement_crossOrigin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLinkElement.crossOrigin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmllinkelement_sheet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLinkElement.sheet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmllinkelement_sizes_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLinkElement.sizes getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmllinkelement_relList_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLinkElement.relList getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmllinkelement_crossOrigin_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLLinkElement.crossOrigin setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmllinkelement(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "crossOrigin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmllinkelement_crossOrigin_getter, "crossOrigin", 0),
            JS_NewCFunction(ctx, js_htmllinkelement_crossOrigin_setter, "crossOrigin", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sheet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmllinkelement_sheet_getter, "sheet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "sizes");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmllinkelement_sizes_getter, "sizes", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "relList");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmllinkelement_relList_getter, "relList", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "HTMLLinkElement", obj);
    return 0;
}

static JSValue js_htmlcollection_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCollection.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlcollection_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCollection.namedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlcollection_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLCollection.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlcollection(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlcollection_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "namedItem", JS_NewCFunction(ctx, js_htmlcollection_namedItem, "namedItem", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_htmlcollection_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLCollection", obj);
    return 0;
}

static JSValue js_htmloptionscollection_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionsCollection.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptionscollection_selectedIndex_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionsCollection.selectedIndex getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptionscollection_length_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionsCollection.length setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptionscollection_selectedIndex_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionsCollection.selectedIndex setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptionscollection_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionsCollection.remove() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmloptionscollection_add(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLOptionsCollection.add() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmloptionscollection(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloptionscollection_length_getter, "length", 0),
            JS_NewCFunction(ctx, js_htmloptionscollection_length_setter, "length", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectedIndex");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmloptionscollection_selectedIndex_getter, "selectedIndex", 0),
            JS_NewCFunction(ctx, js_htmloptionscollection_selectedIndex_setter, "selectedIndex", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "remove", JS_NewCFunction(ctx, js_htmloptionscollection_remove, "remove", 0));
    JS_SetPropertyStr(ctx, obj, "add", JS_NewCFunction(ctx, js_htmloptionscollection_add, "add", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLOptionsCollection", obj);
    return 0;
}

static JSValue js_radionodelist_value_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "RadioNodeList.value getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_radionodelist_value_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "RadioNodeList.value setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_radionodelist(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "value");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_radionodelist_value_getter, "value", 0),
            JS_NewCFunction(ctx, js_radionodelist_value_setter, "value", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "RadioNodeList", obj);
    return 0;
}

static JSValue js_htmlformcontrolscollection_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLFormControlsCollection.namedItem() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlformcontrolscollection(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "namedItem", JS_NewCFunction(ctx, js_htmlformcontrolscollection_namedItem, "namedItem", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLFormControlsCollection", obj);
    return 0;
}

static JSValue js_htmlallcollection_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAllCollection.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlallcollection_namedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAllCollection.namedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_htmlallcollection_item(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "HTMLAllCollection.item() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_htmlallcollection(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_htmlallcollection_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "namedItem", JS_NewCFunction(ctx, js_htmlallcollection_namedItem, "namedItem", 0));
    JS_SetPropertyStr(ctx, obj, "item", JS_NewCFunction(ctx, js_htmlallcollection_item, "item", 0));
    JS_SetPropertyStr(ctx, global_obj, "HTMLAllCollection", obj);
    return 0;
}

static JSValue js_xmlserializer_serializeToString(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "XMLSerializer.serializeToString() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_xmlserializer(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "serializeToString", JS_NewCFunction(ctx, js_xmlserializer_serializeToString, "serializeToString", 0));
    JS_SetPropertyStr(ctx, global_obj, "XMLSerializer", obj);
    return 0;
}

static JSValue js_domparser_parseFromString(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DOMParser.parseFromString() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_domparser(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "parseFromString", JS_NewCFunction(ctx, js_domparser_parseFromString, "parseFromString", 0));
    JS_SetPropertyStr(ctx, global_obj, "DOMParser", obj);
    return 0;
}

static JSValue js_nodefilter_acceptNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeFilter.acceptNode() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_nodefilter(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "acceptNode", JS_NewCFunction(ctx, js_nodefilter_acceptNode, "acceptNode", 0));
    JS_SetPropertyStr(ctx, global_obj, "NodeFilter", obj);
    return 0;
}

static JSValue js_treewalker_filter_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.filter getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_currentNode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.currentNode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_whatToShow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.whatToShow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_root_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.root getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_currentNode_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.currentNode setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_parentNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.parentNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_firstChild(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.firstChild() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_lastChild(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.lastChild() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_nextNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.nextNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_nextSibling(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.nextSibling() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_previousNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.previousNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_treewalker_previousSibling(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "TreeWalker.previousSibling() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_treewalker(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "filter");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_treewalker_filter_getter, "filter", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "currentNode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_treewalker_currentNode_getter, "currentNode", 0),
            JS_NewCFunction(ctx, js_treewalker_currentNode_setter, "currentNode", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "whatToShow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_treewalker_whatToShow_getter, "whatToShow", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "root");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_treewalker_root_getter, "root", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "parentNode", JS_NewCFunction(ctx, js_treewalker_parentNode, "parentNode", 0));
    JS_SetPropertyStr(ctx, obj, "firstChild", JS_NewCFunction(ctx, js_treewalker_firstChild, "firstChild", 0));
    JS_SetPropertyStr(ctx, obj, "lastChild", JS_NewCFunction(ctx, js_treewalker_lastChild, "lastChild", 0));
    JS_SetPropertyStr(ctx, obj, "nextNode", JS_NewCFunction(ctx, js_treewalker_nextNode, "nextNode", 0));
    JS_SetPropertyStr(ctx, obj, "nextSibling", JS_NewCFunction(ctx, js_treewalker_nextSibling, "nextSibling", 0));
    JS_SetPropertyStr(ctx, obj, "previousNode", JS_NewCFunction(ctx, js_treewalker_previousNode, "previousNode", 0));
    JS_SetPropertyStr(ctx, obj, "previousSibling", JS_NewCFunction(ctx, js_treewalker_previousSibling, "previousSibling", 0));
    JS_SetPropertyStr(ctx, global_obj, "TreeWalker", obj);
    return 0;
}

static JSValue js_nodeiterator_pointerBeforeReferenceNode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.pointerBeforeReferenceNode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_nodeiterator_filter_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.filter getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_nodeiterator_whatToShow_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.whatToShow getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_nodeiterator_root_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.root getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_nodeiterator_referenceNode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.referenceNode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_nodeiterator_detach(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.detach() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_nodeiterator_nextNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.nextNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_nodeiterator_previousNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NodeIterator.previousNode() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_nodeiterator(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "pointerBeforeReferenceNode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_nodeiterator_pointerBeforeReferenceNode_getter, "pointerBeforeReferenceNode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "filter");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_nodeiterator_filter_getter, "filter", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "whatToShow");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_nodeiterator_whatToShow_getter, "whatToShow", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "root");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_nodeiterator_root_getter, "root", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "referenceNode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_nodeiterator_referenceNode_getter, "referenceNode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "detach", JS_NewCFunction(ctx, js_nodeiterator_detach, "detach", 0));
    JS_SetPropertyStr(ctx, obj, "nextNode", JS_NewCFunction(ctx, js_nodeiterator_nextNode, "nextNode", 0));
    JS_SetPropertyStr(ctx, obj, "previousNode", JS_NewCFunction(ctx, js_nodeiterator_previousNode, "previousNode", 0));
    JS_SetPropertyStr(ctx, global_obj, "NodeIterator", obj);
    return 0;
}

static JSValue js_range_endContainer_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.endContainer getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_startContainer_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.startContainer getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_collapsed_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.collapsed getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_endOffset_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.endOffset getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_startOffset_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.startOffset getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_commonAncestorContainer_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.commonAncestorContainer getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_insertNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.insertNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_setStartBefore(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.setStartBefore() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_cloneRange(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.cloneRange() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_selectNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.selectNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_deleteContents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.deleteContents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_extractContents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.extractContents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_createContextualFragment(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.createContextualFragment() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_intersectsNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.intersectsNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_selectNodeContents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.selectNodeContents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_setEndBefore(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.setEndBefore() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_compareBoundaryPoints(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.compareBoundaryPoints() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_detach(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.detach() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_collapse(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.collapse() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_setEndAfter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.setEndAfter() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_setStartAfter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.setStartAfter() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_cloneContents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.cloneContents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_setStart(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.setStart() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_setEnd(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.setEnd() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_isPointInRange(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.isPointInRange() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_surroundContents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.surroundContents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_range_comparePoint(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Range.comparePoint() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_range(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "endContainer");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_range_endContainer_getter, "endContainer", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "startContainer");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_range_startContainer_getter, "startContainer", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "collapsed");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_range_collapsed_getter, "collapsed", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "endOffset");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_range_endOffset_getter, "endOffset", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "startOffset");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_range_startOffset_getter, "startOffset", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "commonAncestorContainer");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_range_commonAncestorContainer_getter, "commonAncestorContainer", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "insertNode", JS_NewCFunction(ctx, js_range_insertNode, "insertNode", 0));
    JS_SetPropertyStr(ctx, obj, "setStartBefore", JS_NewCFunction(ctx, js_range_setStartBefore, "setStartBefore", 0));
    JS_SetPropertyStr(ctx, obj, "cloneRange", JS_NewCFunction(ctx, js_range_cloneRange, "cloneRange", 0));
    JS_SetPropertyStr(ctx, obj, "selectNode", JS_NewCFunction(ctx, js_range_selectNode, "selectNode", 0));
    JS_SetPropertyStr(ctx, obj, "deleteContents", JS_NewCFunction(ctx, js_range_deleteContents, "deleteContents", 0));
    JS_SetPropertyStr(ctx, obj, "extractContents", JS_NewCFunction(ctx, js_range_extractContents, "extractContents", 0));
    JS_SetPropertyStr(ctx, obj, "createContextualFragment", JS_NewCFunction(ctx, js_range_createContextualFragment, "createContextualFragment", 0));
    JS_SetPropertyStr(ctx, obj, "intersectsNode", JS_NewCFunction(ctx, js_range_intersectsNode, "intersectsNode", 0));
    JS_SetPropertyStr(ctx, obj, "selectNodeContents", JS_NewCFunction(ctx, js_range_selectNodeContents, "selectNodeContents", 0));
    JS_SetPropertyStr(ctx, obj, "setEndBefore", JS_NewCFunction(ctx, js_range_setEndBefore, "setEndBefore", 0));
    JS_SetPropertyStr(ctx, obj, "compareBoundaryPoints", JS_NewCFunction(ctx, js_range_compareBoundaryPoints, "compareBoundaryPoints", 0));
    JS_SetPropertyStr(ctx, obj, "detach", JS_NewCFunction(ctx, js_range_detach, "detach", 0));
    JS_SetPropertyStr(ctx, obj, "collapse", JS_NewCFunction(ctx, js_range_collapse, "collapse", 0));
    JS_SetPropertyStr(ctx, obj, "setEndAfter", JS_NewCFunction(ctx, js_range_setEndAfter, "setEndAfter", 0));
    JS_SetPropertyStr(ctx, obj, "setStartAfter", JS_NewCFunction(ctx, js_range_setStartAfter, "setStartAfter", 0));
    JS_SetPropertyStr(ctx, obj, "cloneContents", JS_NewCFunction(ctx, js_range_cloneContents, "cloneContents", 0));
    JS_SetPropertyStr(ctx, obj, "setStart", JS_NewCFunction(ctx, js_range_setStart, "setStart", 0));
    JS_SetPropertyStr(ctx, obj, "setEnd", JS_NewCFunction(ctx, js_range_setEnd, "setEnd", 0));
    JS_SetPropertyStr(ctx, obj, "isPointInRange", JS_NewCFunction(ctx, js_range_isPointInRange, "isPointInRange", 0));
    JS_SetPropertyStr(ctx, obj, "surroundContents", JS_NewCFunction(ctx, js_range_surroundContents, "surroundContents", 0));
    JS_SetPropertyStr(ctx, obj, "comparePoint", JS_NewCFunction(ctx, js_range_comparePoint, "comparePoint", 0));
    JS_SetPropertyStr(ctx, global_obj, "Range", obj);
    return 0;
}

static JSValue js_characterdata_length_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.length getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_nextElementSibling_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.nextElementSibling getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_data_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.data getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_previousElementSibling_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.previousElementSibling getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_data_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.data setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_after(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.after() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_substringData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.substringData() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_replaceWith(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.replaceWith() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_before(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.before() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_insertData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.insertData() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_replaceData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.replaceData() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_appendData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.appendData() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.remove() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_characterdata_deleteData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CharacterData.deleteData() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_characterdata(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "length");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_characterdata_length_getter, "length", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "nextElementSibling");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_characterdata_nextElementSibling_getter, "nextElementSibling", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "data");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_characterdata_data_getter, "data", 0),
            JS_NewCFunction(ctx, js_characterdata_data_setter, "data", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "previousElementSibling");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_characterdata_previousElementSibling_getter, "previousElementSibling", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "after", JS_NewCFunction(ctx, js_characterdata_after, "after", 0));
    JS_SetPropertyStr(ctx, obj, "substringData", JS_NewCFunction(ctx, js_characterdata_substringData, "substringData", 0));
    JS_SetPropertyStr(ctx, obj, "replaceWith", JS_NewCFunction(ctx, js_characterdata_replaceWith, "replaceWith", 0));
    JS_SetPropertyStr(ctx, obj, "before", JS_NewCFunction(ctx, js_characterdata_before, "before", 0));
    JS_SetPropertyStr(ctx, obj, "insertData", JS_NewCFunction(ctx, js_characterdata_insertData, "insertData", 0));
    JS_SetPropertyStr(ctx, obj, "replaceData", JS_NewCFunction(ctx, js_characterdata_replaceData, "replaceData", 0));
    JS_SetPropertyStr(ctx, obj, "appendData", JS_NewCFunction(ctx, js_characterdata_appendData, "appendData", 0));
    JS_SetPropertyStr(ctx, obj, "remove", JS_NewCFunction(ctx, js_characterdata_remove, "remove", 0));
    JS_SetPropertyStr(ctx, obj, "deleteData", JS_NewCFunction(ctx, js_characterdata_deleteData, "deleteData", 0));
    JS_SetPropertyStr(ctx, global_obj, "CharacterData", obj);
    return 0;
}

static JSValue js_processinginstruction_sheet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ProcessingInstruction.sheet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_processinginstruction_target_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "ProcessingInstruction.target getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_processinginstruction(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "sheet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_processinginstruction_sheet_getter, "sheet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "target");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_processinginstruction_target_getter, "target", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "ProcessingInstruction", obj);
    return 0;
}

static JSValue js_text_wholeText_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Text.wholeText getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_text_splitText(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Text.splitText() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_text(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "wholeText");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_text_wholeText_getter, "wholeText", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "splitText", JS_NewCFunction(ctx, js_text_splitText, "splitText", 0));
    JS_SetPropertyStr(ctx, global_obj, "Text", obj);
    return 0;
}

static JSValue js_attr_localName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.localName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_ownerElement_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.ownerElement getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_namespaceURI_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.namespaceURI getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_nodeValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.nodeValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_prefix_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.prefix getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_value_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.value getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_textContent_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.textContent getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_specified_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.specified getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_textContent_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.textContent setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_nodeValue_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.nodeValue setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_attr_value_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Attr.value setter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_attr(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "localName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_localName_getter, "localName", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "ownerElement");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_ownerElement_getter, "ownerElement", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_name_getter, "name", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "namespaceURI");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_namespaceURI_getter, "namespaceURI", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "nodeValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_nodeValue_getter, "nodeValue", 0),
            JS_NewCFunction(ctx, js_attr_nodeValue_setter, "nodeValue", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "prefix");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_prefix_getter, "prefix", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "value");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_value_getter, "value", 0),
            JS_NewCFunction(ctx, js_attr_value_setter, "value", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "textContent");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_textContent_getter, "textContent", 0),
            JS_NewCFunction(ctx, js_attr_textContent_setter, "textContent", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "specified");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_attr_specified_getter, "specified", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "Attr", obj);
    return 0;
}

static JSValue js_namednodemap_removeNamedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NamedNodeMap.removeNamedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_namednodemap_getNamedItemNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NamedNodeMap.getNamedItemNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_namednodemap_removeNamedItemNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NamedNodeMap.removeNamedItemNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_namednodemap_setNamedItem(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NamedNodeMap.setNamedItem() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_namednodemap_setNamedItemNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "NamedNodeMap.setNamedItemNS() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_namednodemap(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "removeNamedItem", JS_NewCFunction(ctx, js_namednodemap_removeNamedItem, "removeNamedItem", 0));
    JS_SetPropertyStr(ctx, obj, "getNamedItemNS", JS_NewCFunction(ctx, js_namednodemap_getNamedItemNS, "getNamedItemNS", 0));
    JS_SetPropertyStr(ctx, obj, "removeNamedItemNS", JS_NewCFunction(ctx, js_namednodemap_removeNamedItemNS, "removeNamedItemNS", 0));
    JS_SetPropertyStr(ctx, obj, "setNamedItem", JS_NewCFunction(ctx, js_namednodemap_setNamedItem, "setNamedItem", 0));
    JS_SetPropertyStr(ctx, obj, "setNamedItemNS", JS_NewCFunction(ctx, js_namednodemap_setNamedItemNS, "setNamedItemNS", 0));
    JS_SetPropertyStr(ctx, global_obj, "NamedNodeMap", obj);
    return 0;
}

static JSValue js_domimplementation_createDocument(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DOMImplementation.createDocument() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_domimplementation_hasFeature(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DOMImplementation.hasFeature() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_domimplementation_createDocumentType(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DOMImplementation.createDocumentType() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_domimplementation(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "createDocument", JS_NewCFunction(ctx, js_domimplementation_createDocument, "createDocument", 0));
    JS_SetPropertyStr(ctx, obj, "hasFeature", JS_NewCFunction(ctx, js_domimplementation_hasFeature, "hasFeature", 0));
    JS_SetPropertyStr(ctx, obj, "createDocumentType", JS_NewCFunction(ctx, js_domimplementation_createDocumentType, "createDocumentType", 0));
    JS_SetPropertyStr(ctx, global_obj, "DOMImplementation", obj);
    return 0;
}

static JSValue js_document_forms_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.forms getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_cssElementMap_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.cssElementMap getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_links_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.links getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_domain_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.domain getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_fgColor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.fgColor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_inputEncoding_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.inputEncoding getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_contentType_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.contentType getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_dir_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.dir getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_styleSheets_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.styleSheets getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_commands_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.commands getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_doctype_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.doctype getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_embeds_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.embeds getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_plugins_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.plugins getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_documentURI_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.documentURI getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_all_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.all getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_firstElementChild_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.firstElementChild getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_linkColor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.linkColor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_styleSheetSets_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.styleSheetSets getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_referrer_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.referrer getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_selectedStyleSheetSet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.selectedStyleSheetSet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_lastStyleSheetSet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.lastStyleSheetSet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_activeElement_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.activeElement getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_lastModified_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.lastModified getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_readyState_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.readyState getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_currentScript_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.currentScript getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_bgColor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.bgColor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_anchors_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.anchors getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_characterSet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.characterSet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_lastElementChild_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.lastElementChild getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_onerror_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.onerror getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_compatMode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.compatMode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_vlinkColor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.vlinkColor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_URL_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.URL getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_origin_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.origin getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_childElementCount_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.childElementCount getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_designMode_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.designMode getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_preferredStyleSheetSet_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.preferredStyleSheetSet getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_title_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.title getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_scripts_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.scripts getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_alinkColor_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.alinkColor getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_children_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.children getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_applets_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.applets getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_images_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.images getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_defaultView_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.defaultView getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_vlinkColor_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.vlinkColor setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_designMode_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.designMode setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_domain_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.domain setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_title_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.title setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_linkColor_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.linkColor setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_fgColor_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.fgColor setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_alinkColor_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.alinkColor setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_body_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.body setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_dir_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.dir setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_bgColor_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.bgColor setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_selectedStyleSheetSet_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.selectedStyleSheetSet setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_onerror_setter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.onerror setter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_createRange(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createRange() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_adoptNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.adoptNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_getElementsByTagNameNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByTagNameNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_queryCommandValue(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.queryCommandValue() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_createTreeWalker(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createTreeWalker() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_createAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createAttribute() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_createAttributeNS(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createAttributeNS() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_queryCommandIndeterm(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.queryCommandIndeterm() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_enableStyleSheetsForSet(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.enableStyleSheetsForSet() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_prepend(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.prepend() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_query(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.query() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_queryCommandSupported(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.queryCommandSupported() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_querySelector(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.querySelector() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_append(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.append() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_createProcessingInstruction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createProcessingInstruction() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_getElementsByClassName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByClassName() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.close() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_importNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.importNode() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_clear(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.clear() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_releaseEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.releaseEvents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.open() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_createNodeIterator(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createNodeIterator() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_queryCommandEnabled(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.queryCommandEnabled() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_getElementsByName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.getElementsByName() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_queryAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.queryAll() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_execCommand(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.execCommand() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_createComment(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.createComment() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_hasFocus(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.hasFocus() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_queryCommandState(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.queryCommandState() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_captureEvents(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.captureEvents() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_document_querySelectorAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "Document.querySelectorAll() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_document(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "forms");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_forms_getter, "forms", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "cssElementMap");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_cssElementMap_getter, "cssElementMap", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "links");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_links_getter, "links", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "domain");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_domain_getter, "domain", 0),
            JS_NewCFunction(ctx, js_document_domain_setter, "domain", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "fgColor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_fgColor_getter, "fgColor", 0),
            JS_NewCFunction(ctx, js_document_fgColor_setter, "fgColor", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "inputEncoding");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_inputEncoding_getter, "inputEncoding", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "contentType");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_contentType_getter, "contentType", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "dir");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_dir_getter, "dir", 0),
            JS_NewCFunction(ctx, js_document_dir_setter, "dir", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "styleSheets");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_styleSheets_getter, "styleSheets", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "commands");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_commands_getter, "commands", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "doctype");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_doctype_getter, "doctype", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "embeds");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_embeds_getter, "embeds", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "plugins");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_plugins_getter, "plugins", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "documentURI");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_documentURI_getter, "documentURI", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "all");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_all_getter, "all", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "firstElementChild");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_firstElementChild_getter, "firstElementChild", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "linkColor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_linkColor_getter, "linkColor", 0),
            JS_NewCFunction(ctx, js_document_linkColor_setter, "linkColor", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "styleSheetSets");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_styleSheetSets_getter, "styleSheetSets", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "referrer");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_referrer_getter, "referrer", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "selectedStyleSheetSet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_selectedStyleSheetSet_getter, "selectedStyleSheetSet", 0),
            JS_NewCFunction(ctx, js_document_selectedStyleSheetSet_setter, "selectedStyleSheetSet", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lastStyleSheetSet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_lastStyleSheetSet_getter, "lastStyleSheetSet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "activeElement");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_activeElement_getter, "activeElement", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lastModified");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_lastModified_getter, "lastModified", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "readyState");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_readyState_getter, "readyState", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "currentScript");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_currentScript_getter, "currentScript", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "bgColor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_bgColor_getter, "bgColor", 0),
            JS_NewCFunction(ctx, js_document_bgColor_setter, "bgColor", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "anchors");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_anchors_getter, "anchors", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "characterSet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_characterSet_getter, "characterSet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "lastElementChild");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_lastElementChild_getter, "lastElementChild", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "onerror");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_onerror_getter, "onerror", 0),
            JS_NewCFunction(ctx, js_document_onerror_setter, "onerror", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "compatMode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_compatMode_getter, "compatMode", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "vlinkColor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_vlinkColor_getter, "vlinkColor", 0),
            JS_NewCFunction(ctx, js_document_vlinkColor_setter, "vlinkColor", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "URL");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_URL_getter, "URL", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "origin");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_origin_getter, "origin", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "childElementCount");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_childElementCount_getter, "childElementCount", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "designMode");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_designMode_getter, "designMode", 0),
            JS_NewCFunction(ctx, js_document_designMode_setter, "designMode", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "preferredStyleSheetSet");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_preferredStyleSheetSet_getter, "preferredStyleSheetSet", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "title");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_title_getter, "title", 0),
            JS_NewCFunction(ctx, js_document_title_setter, "title", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "scripts");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_scripts_getter, "scripts", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "alinkColor");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_alinkColor_getter, "alinkColor", 0),
            JS_NewCFunction(ctx, js_document_alinkColor_setter, "alinkColor", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "children");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_children_getter, "children", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "applets");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_applets_getter, "applets", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "images");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_images_getter, "images", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "defaultView");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_document_defaultView_getter, "defaultView", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "body");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_UNDEFINED,
            JS_NewCFunction(ctx, js_document_body_setter, "body", 1),
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "createRange", JS_NewCFunction(ctx, js_document_createRange, "createRange", 0));
    JS_SetPropertyStr(ctx, obj, "adoptNode", JS_NewCFunction(ctx, js_document_adoptNode, "adoptNode", 0));
    JS_SetPropertyStr(ctx, obj, "getElementsByTagNameNS", JS_NewCFunction(ctx, js_document_getElementsByTagNameNS, "getElementsByTagNameNS", 0));
    JS_SetPropertyStr(ctx, obj, "queryCommandValue", JS_NewCFunction(ctx, js_document_queryCommandValue, "queryCommandValue", 0));
    JS_SetPropertyStr(ctx, obj, "createTreeWalker", JS_NewCFunction(ctx, js_document_createTreeWalker, "createTreeWalker", 0));
    JS_SetPropertyStr(ctx, obj, "createAttribute", JS_NewCFunction(ctx, js_document_createAttribute, "createAttribute", 0));
    JS_SetPropertyStr(ctx, obj, "createAttributeNS", JS_NewCFunction(ctx, js_document_createAttributeNS, "createAttributeNS", 0));
    JS_SetPropertyStr(ctx, obj, "queryCommandIndeterm", JS_NewCFunction(ctx, js_document_queryCommandIndeterm, "queryCommandIndeterm", 0));
    JS_SetPropertyStr(ctx, obj, "enableStyleSheetsForSet", JS_NewCFunction(ctx, js_document_enableStyleSheetsForSet, "enableStyleSheetsForSet", 0));
    JS_SetPropertyStr(ctx, obj, "prepend", JS_NewCFunction(ctx, js_document_prepend, "prepend", 0));
    JS_SetPropertyStr(ctx, obj, "query", JS_NewCFunction(ctx, js_document_query, "query", 0));
    JS_SetPropertyStr(ctx, obj, "queryCommandSupported", JS_NewCFunction(ctx, js_document_queryCommandSupported, "queryCommandSupported", 0));
    JS_SetPropertyStr(ctx, obj, "querySelector", JS_NewCFunction(ctx, js_document_querySelector, "querySelector", 0));
    JS_SetPropertyStr(ctx, obj, "append", JS_NewCFunction(ctx, js_document_append, "append", 0));
    JS_SetPropertyStr(ctx, obj, "createProcessingInstruction", JS_NewCFunction(ctx, js_document_createProcessingInstruction, "createProcessingInstruction", 0));
    JS_SetPropertyStr(ctx, obj, "getElementsByClassName", JS_NewCFunction(ctx, js_document_getElementsByClassName, "getElementsByClassName", 0));
    JS_SetPropertyStr(ctx, obj, "close", JS_NewCFunction(ctx, js_document_close, "close", 0));
    JS_SetPropertyStr(ctx, obj, "importNode", JS_NewCFunction(ctx, js_document_importNode, "importNode", 0));
    JS_SetPropertyStr(ctx, obj, "clear", JS_NewCFunction(ctx, js_document_clear, "clear", 0));
    JS_SetPropertyStr(ctx, obj, "releaseEvents", JS_NewCFunction(ctx, js_document_releaseEvents, "releaseEvents", 0));
    JS_SetPropertyStr(ctx, obj, "open", JS_NewCFunction(ctx, js_document_open, "open", 0));
    JS_SetPropertyStr(ctx, obj, "createNodeIterator", JS_NewCFunction(ctx, js_document_createNodeIterator, "createNodeIterator", 0));
    JS_SetPropertyStr(ctx, obj, "queryCommandEnabled", JS_NewCFunction(ctx, js_document_queryCommandEnabled, "queryCommandEnabled", 0));
    JS_SetPropertyStr(ctx, obj, "getElementsByName", JS_NewCFunction(ctx, js_document_getElementsByName, "getElementsByName", 0));
    JS_SetPropertyStr(ctx, obj, "queryAll", JS_NewCFunction(ctx, js_document_queryAll, "queryAll", 0));
    JS_SetPropertyStr(ctx, obj, "execCommand", JS_NewCFunction(ctx, js_document_execCommand, "execCommand", 0));
    JS_SetPropertyStr(ctx, obj, "createComment", JS_NewCFunction(ctx, js_document_createComment, "createComment", 0));
    JS_SetPropertyStr(ctx, obj, "hasFocus", JS_NewCFunction(ctx, js_document_hasFocus, "hasFocus", 0));
    JS_SetPropertyStr(ctx, obj, "queryCommandState", JS_NewCFunction(ctx, js_document_queryCommandState, "queryCommandState", 0));
    JS_SetPropertyStr(ctx, obj, "captureEvents", JS_NewCFunction(ctx, js_document_captureEvents, "captureEvents", 0));
    JS_SetPropertyStr(ctx, obj, "querySelectorAll", JS_NewCFunction(ctx, js_document_querySelectorAll, "querySelectorAll", 0));
    JS_SetPropertyStr(ctx, global_obj, "Document", obj);
    return 0;
}

static JSValue js_xmldocument_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "XMLDocument.load() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_xmldocument(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "load", JS_NewCFunction(ctx, js_xmldocument_load, "load", 0));
    JS_SetPropertyStr(ctx, global_obj, "XMLDocument", obj);
    return 0;
}

static JSValue js_mutationrecord_type_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.type getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_removedNodes_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.removedNodes getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_attributeNamespace_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.attributeNamespace getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_target_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.target getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_oldValue_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.oldValue getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_attributeName_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.attributeName getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_nextSibling_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.nextSibling getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_addedNodes_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.addedNodes getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationrecord_previousSibling_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationRecord.previousSibling getter is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mutationrecord(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "type");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_type_getter, "type", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "removedNodes");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_removedNodes_getter, "removedNodes", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "attributeNamespace");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_attributeNamespace_getter, "attributeNamespace", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "target");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_target_getter, "target", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "oldValue");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_oldValue_getter, "oldValue", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "attributeName");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_attributeName_getter, "attributeName", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "nextSibling");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_nextSibling_getter, "nextSibling", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "addedNodes");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_addedNodes_getter, "addedNodes", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "previousSibling");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_mutationrecord_previousSibling_getter, "previousSibling", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, global_obj, "MutationRecord", obj);
    return 0;
}

static JSValue js_mutationobserver_observe(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationObserver.observe() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationobserver_disconnect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationObserver.disconnect() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_mutationobserver_takeRecords(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "MutationObserver.takeRecords() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_mutationobserver(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "observe", JS_NewCFunction(ctx, js_mutationobserver_observe, "observe", 0));
    JS_SetPropertyStr(ctx, obj, "disconnect", JS_NewCFunction(ctx, js_mutationobserver_disconnect, "disconnect", 0));
    JS_SetPropertyStr(ctx, obj, "takeRecords", JS_NewCFunction(ctx, js_mutationobserver_takeRecords, "takeRecords", 0));
    JS_SetPropertyStr(ctx, global_obj, "MutationObserver", obj);
    return 0;
}

static JSValue js_documenttype_name_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentType.name getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documenttype_publicId_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentType.publicId getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documenttype_systemId_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentType.systemId getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documenttype_after(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentType.after() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documenttype_replaceWith(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentType.replaceWith() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documenttype_remove(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentType.remove() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documenttype_before(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentType.before() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_documenttype(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "name");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_documenttype_name_getter, "name", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "publicId");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_documenttype_publicId_getter, "publicId", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "systemId");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_documenttype_systemId_getter, "systemId", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "after", JS_NewCFunction(ctx, js_documenttype_after, "after", 0));
    JS_SetPropertyStr(ctx, obj, "replaceWith", JS_NewCFunction(ctx, js_documenttype_replaceWith, "replaceWith", 0));
    JS_SetPropertyStr(ctx, obj, "remove", JS_NewCFunction(ctx, js_documenttype_remove, "remove", 0));
    JS_SetPropertyStr(ctx, obj, "before", JS_NewCFunction(ctx, js_documenttype_before, "before", 0));
    JS_SetPropertyStr(ctx, global_obj, "DocumentType", obj);
    return 0;
}

static JSValue js_documentfragment_lastElementChild_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.lastElementChild getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_firstElementChild_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.firstElementChild getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_children_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.children getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_childElementCount_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.childElementCount getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_querySelector(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.querySelector() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_queryAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.queryAll() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_append(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.append() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_prepend(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.prepend() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_query(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.query() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.getElementById() is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_documentfragment_querySelectorAll(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "DocumentFragment.querySelectorAll() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_documentfragment(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "lastElementChild");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_documentfragment_lastElementChild_getter, "lastElementChild", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "firstElementChild");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_documentfragment_firstElementChild_getter, "firstElementChild", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "children");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_documentfragment_children_getter, "children", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    {
        JSAtom atom = JS_NewAtom(ctx, "childElementCount");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_documentfragment_childElementCount_getter, "childElementCount", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "querySelector", JS_NewCFunction(ctx, js_documentfragment_querySelector, "querySelector", 0));
    JS_SetPropertyStr(ctx, obj, "queryAll", JS_NewCFunction(ctx, js_documentfragment_queryAll, "queryAll", 0));
    JS_SetPropertyStr(ctx, obj, "append", JS_NewCFunction(ctx, js_documentfragment_append, "append", 0));
    JS_SetPropertyStr(ctx, obj, "prepend", JS_NewCFunction(ctx, js_documentfragment_prepend, "prepend", 0));
    JS_SetPropertyStr(ctx, obj, "query", JS_NewCFunction(ctx, js_documentfragment_query, "query", 0));
    JS_SetPropertyStr(ctx, obj, "getElementById", JS_NewCFunction(ctx, js_documentfragment_getElementById, "getElementById", 0));
    JS_SetPropertyStr(ctx, obj, "querySelectorAll", JS_NewCFunction(ctx, js_documentfragment_querySelectorAll, "querySelectorAll", 0));
    JS_SetPropertyStr(ctx, global_obj, "DocumentFragment", obj);
    return 0;
}

static JSValue js_eventlistener_handleEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "EventListener.handleEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_eventlistener(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "handleEvent", JS_NewCFunction(ctx, js_eventlistener_handleEvent, "handleEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "EventListener", obj);
    return 0;
}

static JSValue js_customevent_detail_getter(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CustomEvent.detail getter is unimplemented");
    return JS_UNDEFINED;
}
static JSValue js_customevent_initCustomEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    NSLOG(wisp, DEBUG, "CustomEvent.initCustomEvent() is unimplemented");
    return JS_UNDEFINED;
}
static int qjs_init_unimplemented_customevent(JSContext *ctx, JSValue global_obj)
{
    JSValue obj = JS_NewObject(ctx);
    {
        JSAtom atom = JS_NewAtom(ctx, "detail");
        JS_DefinePropertyGetSet(ctx, obj, atom,
            JS_NewCFunction(ctx, js_customevent_detail_getter, "detail", 0),
            JS_UNDEFINED,
            JS_PROP_ENUMERABLE | JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    }
    JS_SetPropertyStr(ctx, obj, "initCustomEvent", JS_NewCFunction(ctx, js_customevent_initCustomEvent, "initCustomEvent", 0));
    JS_SetPropertyStr(ctx, global_obj, "CustomEvent", obj);
    return 0;
}

int qjs_init_unimplemented(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);
    qjs_init_unimplemented_urlsearchparams(ctx, global_obj);
    qjs_init_unimplemented_url(ctx, global_obj);
    qjs_init_unimplemented_event(ctx, global_obj);
    qjs_init_unimplemented_mutationevent(ctx, global_obj);
    qjs_init_unimplemented_uievent(ctx, global_obj);
    qjs_init_unimplemented_compositionevent(ctx, global_obj);
    qjs_init_unimplemented_keyboardevent(ctx, global_obj);
    qjs_init_unimplemented_mouseevent(ctx, global_obj);
    qjs_init_unimplemented_wheelevent(ctx, global_obj);
    qjs_init_unimplemented_focusevent(ctx, global_obj);
    qjs_init_unimplemented_css(ctx, global_obj);
    qjs_init_unimplemented_pseudoelement(ctx, global_obj);
    qjs_init_unimplemented_svgelement(ctx, global_obj);
    qjs_init_unimplemented_cssstyledeclaration(ctx, global_obj);
    qjs_init_unimplemented_cssrule(ctx, global_obj);
    qjs_init_unimplemented_cssnamespacerule(ctx, global_obj);
    qjs_init_unimplemented_cssmarginrule(ctx, global_obj);
    qjs_init_unimplemented_cssgroupingrule(ctx, global_obj);
    qjs_init_unimplemented_csspagerule(ctx, global_obj);
    qjs_init_unimplemented_cssmediarule(ctx, global_obj);
    qjs_init_unimplemented_cssimportrule(ctx, global_obj);
    qjs_init_unimplemented_cssstylerule(ctx, global_obj);
    qjs_init_unimplemented_cssrulelist(ctx, global_obj);
    qjs_init_unimplemented_stylesheetlist(ctx, global_obj);
    qjs_init_unimplemented_stylesheet(ctx, global_obj);
    qjs_init_unimplemented_cssstylesheet(ctx, global_obj);
    qjs_init_unimplemented_medialist(ctx, global_obj);
    qjs_init_unimplemented_element(ctx, global_obj);
    qjs_init_unimplemented_htmlelement(ctx, global_obj);
    qjs_init_unimplemented_htmldirectoryelement(ctx, global_obj);
    qjs_init_unimplemented_htmlframeelement(ctx, global_obj);
    qjs_init_unimplemented_htmlmarqueeelement(ctx, global_obj);
    qjs_init_unimplemented_htmlappletelement(ctx, global_obj);
    qjs_init_unimplemented_storageevent(ctx, global_obj);
    qjs_init_unimplemented_storage(ctx, global_obj);
    qjs_init_unimplemented_workerlocation(ctx, global_obj);
    qjs_init_unimplemented_workernavigator(ctx, global_obj);
    qjs_init_unimplemented_sharedworker(ctx, global_obj);
    qjs_init_unimplemented_worker(ctx, global_obj);
    qjs_init_unimplemented_workerglobalscope(ctx, global_obj);
    qjs_init_unimplemented_sharedworkerglobalscope(ctx, global_obj);
    qjs_init_unimplemented_dedicatedworkerglobalscope(ctx, global_obj);
    qjs_init_unimplemented_broadcastchannel(ctx, global_obj);
    qjs_init_unimplemented_messageport(ctx, global_obj);
    qjs_init_unimplemented_messagechannel(ctx, global_obj);
    qjs_init_unimplemented_closeevent(ctx, global_obj);
    qjs_init_unimplemented_websocket(ctx, global_obj);
    qjs_init_unimplemented_eventsource(ctx, global_obj);
    qjs_init_unimplemented_messageevent(ctx, global_obj);
    qjs_init_unimplemented_imagebitmap(ctx, global_obj);
    qjs_init_unimplemented_external(ctx, global_obj);
    qjs_init_unimplemented_mimetype(ctx, global_obj);
    qjs_init_unimplemented_plugin(ctx, global_obj);
    qjs_init_unimplemented_mimetypearray(ctx, global_obj);
    qjs_init_unimplemented_pluginarray(ctx, global_obj);
    qjs_init_unimplemented_navigator(ctx, global_obj);
    qjs_init_unimplemented_errorevent(ctx, global_obj);
    qjs_init_unimplemented_applicationcache(ctx, global_obj);
    qjs_init_unimplemented_beforeunloadevent(ctx, global_obj);
    qjs_init_unimplemented_pagetransitionevent(ctx, global_obj);
    qjs_init_unimplemented_hashchangeevent(ctx, global_obj);
    qjs_init_unimplemented_popstateevent(ctx, global_obj);
    qjs_init_unimplemented_location(ctx, global_obj);
    qjs_init_unimplemented_history(ctx, global_obj);
    qjs_init_unimplemented_barprop(ctx, global_obj);
    qjs_init_unimplemented_window(ctx, global_obj);
    qjs_init_unimplemented_dragevent(ctx, global_obj);
    qjs_init_unimplemented_datatransferitem(ctx, global_obj);
    qjs_init_unimplemented_datatransferitemlist(ctx, global_obj);
    qjs_init_unimplemented_datatransfer(ctx, global_obj);
    qjs_init_unimplemented_touch(ctx, global_obj);
    qjs_init_unimplemented_path2d(ctx, global_obj);
    qjs_init_unimplemented_drawingstyle(ctx, global_obj);
    qjs_init_unimplemented_textmetrics(ctx, global_obj);
    qjs_init_unimplemented_canvaspattern(ctx, global_obj);
    qjs_init_unimplemented_canvasgradient(ctx, global_obj);
    qjs_init_unimplemented_canvasrenderingcontext2d(ctx, global_obj);
    qjs_init_unimplemented_canvasproxy(ctx, global_obj);
    qjs_init_unimplemented_htmlcanvaselement(ctx, global_obj);
    qjs_init_unimplemented_htmltemplateelement(ctx, global_obj);
    qjs_init_unimplemented_htmlscriptelement(ctx, global_obj);
    qjs_init_unimplemented_htmldialogelement(ctx, global_obj);
    qjs_init_unimplemented_relatedevent(ctx, global_obj);
    qjs_init_unimplemented_htmlmenuitemelement(ctx, global_obj);
    qjs_init_unimplemented_htmlmenuelement(ctx, global_obj);
    qjs_init_unimplemented_htmldetailselement(ctx, global_obj);
    qjs_init_unimplemented_validitystate(ctx, global_obj);
    qjs_init_unimplemented_autocompleteerrorevent(ctx, global_obj);
    qjs_init_unimplemented_htmllegendelement(ctx, global_obj);
    qjs_init_unimplemented_htmlfieldsetelement(ctx, global_obj);
    qjs_init_unimplemented_htmlmeterelement(ctx, global_obj);
    qjs_init_unimplemented_htmlprogresselement(ctx, global_obj);
    qjs_init_unimplemented_htmloutputelement(ctx, global_obj);
    qjs_init_unimplemented_htmlkeygenelement(ctx, global_obj);
    qjs_init_unimplemented_htmltextareaelement(ctx, global_obj);
    qjs_init_unimplemented_htmloptionelement(ctx, global_obj);
    qjs_init_unimplemented_htmloptgroupelement(ctx, global_obj);
    qjs_init_unimplemented_htmldatalistelement(ctx, global_obj);
    qjs_init_unimplemented_htmlselectelement(ctx, global_obj);
    qjs_init_unimplemented_htmlbuttonelement(ctx, global_obj);
    qjs_init_unimplemented_htmlinputelement(ctx, global_obj);
    qjs_init_unimplemented_htmllabelelement(ctx, global_obj);
    qjs_init_unimplemented_htmlformelement(ctx, global_obj);
    qjs_init_unimplemented_htmltablecellelement(ctx, global_obj);
    qjs_init_unimplemented_htmltableheadercellelement(ctx, global_obj);
    qjs_init_unimplemented_htmltabledatacellelement(ctx, global_obj);
    qjs_init_unimplemented_htmltablerowelement(ctx, global_obj);
    qjs_init_unimplemented_htmltablesectionelement(ctx, global_obj);
    qjs_init_unimplemented_htmltablecolelement(ctx, global_obj);
    qjs_init_unimplemented_htmltableelement(ctx, global_obj);
    qjs_init_unimplemented_htmlareaelement(ctx, global_obj);
    qjs_init_unimplemented_htmlmapelement(ctx, global_obj);
    qjs_init_unimplemented_trackevent(ctx, global_obj);
    qjs_init_unimplemented_timeranges(ctx, global_obj);
    qjs_init_unimplemented_texttrackcue(ctx, global_obj);
    qjs_init_unimplemented_texttrackcuelist(ctx, global_obj);
    qjs_init_unimplemented_texttrack(ctx, global_obj);
    qjs_init_unimplemented_texttracklist(ctx, global_obj);
    qjs_init_unimplemented_mediacontroller(ctx, global_obj);
    qjs_init_unimplemented_videotrack(ctx, global_obj);
    qjs_init_unimplemented_videotracklist(ctx, global_obj);
    qjs_init_unimplemented_audiotrack(ctx, global_obj);
    qjs_init_unimplemented_audiotracklist(ctx, global_obj);
    qjs_init_unimplemented_mediaerror(ctx, global_obj);
    qjs_init_unimplemented_htmlmediaelement(ctx, global_obj);
    qjs_init_unimplemented_htmltrackelement(ctx, global_obj);
    qjs_init_unimplemented_htmlvideoelement(ctx, global_obj);
    qjs_init_unimplemented_htmlobjectelement(ctx, global_obj);
    qjs_init_unimplemented_htmlembedelement(ctx, global_obj);
    qjs_init_unimplemented_htmliframeelement(ctx, global_obj);
    qjs_init_unimplemented_htmlimageelement(ctx, global_obj);
    qjs_init_unimplemented_htmlsourceelement(ctx, global_obj);
    qjs_init_unimplemented_htmlmodelement(ctx, global_obj);
    qjs_init_unimplemented_htmltimeelement(ctx, global_obj);
    qjs_init_unimplemented_htmldataelement(ctx, global_obj);
    qjs_init_unimplemented_htmlanchorelement(ctx, global_obj);
    qjs_init_unimplemented_htmldlistelement(ctx, global_obj);
    qjs_init_unimplemented_htmlulistelement(ctx, global_obj);
    qjs_init_unimplemented_htmlolistelement(ctx, global_obj);
    qjs_init_unimplemented_htmlhrelement(ctx, global_obj);
    qjs_init_unimplemented_htmlstyleelement(ctx, global_obj);
    qjs_init_unimplemented_htmllinkelement(ctx, global_obj);
    qjs_init_unimplemented_htmlcollection(ctx, global_obj);
    qjs_init_unimplemented_htmloptionscollection(ctx, global_obj);
    qjs_init_unimplemented_radionodelist(ctx, global_obj);
    qjs_init_unimplemented_htmlformcontrolscollection(ctx, global_obj);
    qjs_init_unimplemented_htmlallcollection(ctx, global_obj);
    qjs_init_unimplemented_xmlserializer(ctx, global_obj);
    qjs_init_unimplemented_domparser(ctx, global_obj);
    qjs_init_unimplemented_nodefilter(ctx, global_obj);
    qjs_init_unimplemented_treewalker(ctx, global_obj);
    qjs_init_unimplemented_nodeiterator(ctx, global_obj);
    qjs_init_unimplemented_range(ctx, global_obj);
    qjs_init_unimplemented_characterdata(ctx, global_obj);
    qjs_init_unimplemented_processinginstruction(ctx, global_obj);
    qjs_init_unimplemented_text(ctx, global_obj);
    qjs_init_unimplemented_attr(ctx, global_obj);
    qjs_init_unimplemented_namednodemap(ctx, global_obj);
    qjs_init_unimplemented_domimplementation(ctx, global_obj);
    qjs_init_unimplemented_document(ctx, global_obj);
    qjs_init_unimplemented_xmldocument(ctx, global_obj);
    qjs_init_unimplemented_mutationrecord(ctx, global_obj);
    qjs_init_unimplemented_mutationobserver(ctx, global_obj);
    qjs_init_unimplemented_documenttype(ctx, global_obj);
    qjs_init_unimplemented_documentfragment(ctx, global_obj);
    qjs_init_unimplemented_eventlistener(ctx, global_obj);
    qjs_init_unimplemented_customevent(ctx, global_obj);

    JS_FreeValue(ctx, global_obj);
    return 0;
}
