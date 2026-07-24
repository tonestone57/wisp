#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include "utils/libdom.h"
#include <wisp/wisp_dnd_bridge.h>
#include <wisp/browser_window.h>
#include <wisp/plot_style.h>
#include "desktop/browser_private.h"
#include <wisp/content/content_protected.h>
#include "content/content.h"
#include "content/hlcache.h"

#include "JSDragEvent.gen.h"
#include "JSDataTransfer.gen.h"
#include "JSDataTransferItem.gen.h"
#include "JSDataTransferItemList.gen.h"

/* Dummy implementations to satisfy compiler/linker for the generated bindings */

JSValue wisp_dragevent_constructor_impl(JSContext *ctx, const char * type, JSValue eventInitDict) {
    return JS_UNDEFINED;
}

JSValue wisp_dragevent_dataTransfer_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_datatransfer_clearData_impl(JSContext *ctx, QJSNodePrivate *priv, const char * format) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransfer_getData_impl(JSContext *ctx, QJSNodePrivate *priv, const char * format) {
    return JS_NewString(ctx, "");
}

JSValue wisp_datatransfer_setData_impl(JSContext *ctx, QJSNodePrivate *priv, const char * format, const char * data) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransfer_setDragImage_impl(JSContext *ctx, QJSNodePrivate *priv, void * image, int32_t x, int32_t y) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransfer_dropEffect_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "none");
}

JSValue wisp_datatransfer_dropEffect_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransfer_effectAllowed_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "all");
}

JSValue wisp_datatransfer_effectAllowed_set_impl(JSContext *ctx, QJSNodePrivate *priv, const char * value) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransfer_files_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_datatransfer_items_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_datatransfer_types_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewArray(ctx);
}

JSValue wisp_datatransferitemlist___getter___impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransferitemlist_add_0_impl(JSContext *ctx, QJSNodePrivate *priv, const char * data, const char * type) {
    return JS_NULL;
}

JSValue wisp_datatransferitemlist_add_1_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue data) {
    return JS_NULL;
}

JSValue wisp_datatransferitemlist_clear_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransferitemlist_remove_impl(JSContext *ctx, QJSNodePrivate *priv, uint32_t index) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransferitemlist_length_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewInt32(ctx, 0);
}

JSValue wisp_datatransferitem_getAsFile_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NULL;
}

JSValue wisp_datatransferitem_getAsString_impl(JSContext *ctx, QJSNodePrivate *priv, JSValue callback) {
    return JS_UNDEFINED;
}

JSValue wisp_datatransferitem_kind_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "string");
}

JSValue wisp_datatransferitem_type_get_impl(JSContext *ctx, QJSNodePrivate *priv) {
    return JS_NewString(ctx, "text/plain");
}

/* JavaScript Polyfill / Redefinitions */

static const char *drag_drop_polyfill_js =
    "(function() {\n"
    "    const _store = Symbol('dragDataStore');\n"
    "\n"
    "    // DataTransferItem Polyfill/Override\n"
    "    if (globalThis.DataTransferItem) {\n"
    "        const proto = globalThis.DataTransferItem.prototype;\n"
    "        Object.defineProperty(proto, 'kind', {\n"
    "            get() { return this[_store] ? this[_store].kind : 'string'; },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "        Object.defineProperty(proto, 'type', {\n"
    "            get() { return this[_store] ? this[_store].type : ''; },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "        proto.getAsString = function(callback) {\n"
    "            if (!this[_store] || this[_store].mode === 'protected') {\n"
    "                if (callback) callback('');\n"
    "                return;\n"
    "            }\n"
    "            if (this[_store].kind === 'string' && callback) {\n"
    "                callback(this[_store].data);\n"
    "            }\n"
    "        };\n"
    "        proto.getAsFile = function() {\n"
    "            if (!this[_store] || this[_store].mode === 'protected' || this[_store].kind !== 'file') {\n"
    "                return null;\n"
    "            }\n"
    "            return this[_store].file || null;\n"
    "        };\n"
    "    }\n"
    "\n"
    "    // DataTransferItemList Polyfill/Override\n"
    "    if (globalThis.DataTransferItemList) {\n"
    "        const proto = globalThis.DataTransferItemList.prototype;\n"
    "        Object.defineProperty(proto, 'length', {\n"
    "            get() { return this[_store] ? this[_store].items.length : 0; },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "        proto.add = function(data, type) {\n"
    "            if (!this[_store]) return null;\n"
    "            if (this[_store].mode !== 'readwrite') return null;\n"
    "\n"
    "            let item;\n"
    "            if (typeof data === 'string') {\n"
    "                item = {\n"
    "                    kind: 'string',\n"
    "                    type: type ? type.toLowerCase() : 'text/plain',\n"
    "                    data: data,\n"
    "                    mode: this[_store].mode\n"
    "                };\n"
    "            } else if (data && typeof data === 'object') {\n"
    "                item = {\n"
    "                    kind: 'file',\n"
    "                    type: data.type || 'application/octet-stream',\n"
    "                    file: data,\n"
    "                    mode: this[_store].mode\n"
    "                };\n"
    "            } else {\n"
    "                return null;\n"
    "            }\n"
    "\n"
    "            const itemObj = Object.create(globalThis.DataTransferItem.prototype);\n"
    "            itemObj[_store] = item;\n"
    "            this[_store].items.push(itemObj);\n"
    "\n"
    "            if (item.kind === 'string' && !this[_store].types.includes(item.type)) {\n"
    "                this[_store].types.push(item.type);\n"
    "            } else if (item.kind === 'file' && !this[_store].types.includes('Files')) {\n"
    "                this[_store].types.push('Files');\n"
    "            }\n"
    "\n"
    "            return itemObj;\n"
    "        };\n"
    "        proto.remove = function(index) {\n"
    "            if (!this[_store] || this[_store].mode !== 'readwrite') return;\n"
    "            if (index >= 0 && index < this[_store].items.length) {\n"
    "                this[_store].items.splice(index, 1);\n"
    "                this[_store].types = [];\n"
    "                for (const it of this[_store].items) {\n"
    "                    const s = it[_store];\n"
    "                    if (s.kind === 'string' && !this[_store].types.includes(s.type)) {\n"
    "                        this[_store].types.push(s.type);\n"
    "                    } else if (s.kind === 'file' && !this[_store].types.includes('Files')) {\n"
    "                        this[_store].types.push('Files');\n"
    "                    }\n"
    "                }\n"
    "            }\n"
    "        };\n"
    "        proto.clear = function() {\n"
    "            if (!this[_store] || this[_store].mode !== 'readwrite') return;\n"
    "            this[_store].items = [];\n"
    "            this[_store].types = [];\n"
    "        };\n"
    "    }\n"
    "\n"
    "    // DataTransfer Polyfill/Override\n"
    "    if (globalThis.DataTransfer) {\n"
    "        const proto = globalThis.DataTransfer.prototype;\n"
    "        \n"
    "        globalThis.__initDataTransferInstance = function(dt, mode) {\n"
    "            mode = mode || 'readwrite';\n"
    "            const storeObj = {\n"
    "                mode: mode,\n"
    "                items: [],\n"
    "                types: [],\n"
    "                dropEffect: 'none',\n"
    "                effectAllowed: 'all',\n"
    "                dragImage: null\n"
    "            };\n"
    "            dt[_store] = storeObj;\n"
    "\n"
    "            const itemsObj = Object.create(globalThis.DataTransferItemList.prototype);\n"
    "            itemsObj[_store] = storeObj;\n"
    "            \n"
    "            dt._itemsProxy = new Proxy(itemsObj, {\n"
    "                get(target, prop) {\n"
    "                    if (typeof prop === 'string' && /^\\d+$/.test(prop)) {\n"
    "                        const idx = parseInt(prop, 10);\n"
    "                        return storeObj.items[idx] || undefined;\n"
    "                    }\n"
    "                    return target[prop];\n"
    "                }\n"
    "            });\n"
    "        };\n"
    "\n"
    "        Object.defineProperty(proto, 'dropEffect', {\n"
    "            get() { return this[_store] ? this[_store].dropEffect : 'none'; },\n"
    "            set(v) { if (this[_store]) this[_store].dropEffect = v; },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "        Object.defineProperty(proto, 'effectAllowed', {\n"
    "            get() { return this[_store] ? this[_store].effectAllowed : 'all'; },\n"
    "            set(v) { if (this[_store]) this[_store].effectAllowed = v; },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "        Object.defineProperty(proto, 'items', {\n"
    "            get() {\n"
    "                if (!this[_store]) globalThis.__initDataTransferInstance(this);\n"
    "                return this._itemsProxy;\n"
    "            },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "        Object.defineProperty(proto, 'types', {\n"
    "            get() {\n"
    "                if (!this[_store]) globalThis.__initDataTransferInstance(this);\n"
    "                return this[_store].types;\n"
    "            },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "        Object.defineProperty(proto, 'files', {\n"
    "            get() {\n"
    "                if (!this[_store]) globalThis.__initDataTransferInstance(this);\n"
    "                if (this[_store].mode === 'protected') return [];\n"
    "                return this[_store].items\n"
    "                    .filter(it => it[_store].kind === 'file')\n"
    "                    .map(it => it[_store].file);\n"
    "            },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "\n"
    "        proto.setData = function(format, data) {\n"
    "            if (!this[_store]) globalThis.__initDataTransferInstance(this);\n"
    "            if (this[_store].mode !== 'readwrite') return;\n"
    "            format = format.toLowerCase();\n"
    "            const idx = this[_store].items.findIndex(it => it[_store].kind === 'string' && it[_store].type === format);\n"
    "            if (idx !== -1) {\n"
    "                this[_store].items.splice(idx, 1);\n"
    "            }\n"
    "            this.items.add(data, format);\n"
    "        };\n"
    "\n"
    "        proto.getData = function(format) {\n"
    "            if (!this[_store]) globalThis.__initDataTransferInstance(this);\n"
    "            if (this[_store].mode === 'protected') return '';\n"
    "            format = format.toLowerCase();\n"
    "            const item = this[_store].items.find(it => it[_store].kind === 'string' && it[_store].type === format);\n"
    "            return item ? item[_store].data : '';\n"
    "        };\n"
    "\n"
    "        proto.clearData = function(format) {\n"
    "            if (!this[_store]) globalThis.__initDataTransferInstance(this);\n"
    "            if (this[_store].mode !== 'readwrite') return;\n"
    "            if (format !== undefined) {\n"
    "                format = format.toLowerCase();\n"
    "                const idx = this[_store].items.findIndex(it => it[_store].kind === 'string' && it[_store].type === format);\n"
    "                if (idx !== -1) {\n"
    "                    this[_store].items.splice(idx, 1);\n"
    "                    const typeIdx = this[_store].types.indexOf(format);\n"
    "                    if (typeIdx !== -1) this[_store].types.splice(typeIdx, 1);\n"
    "                }\n"
    "            } else {\n"
    "                this[_store].items = this[_store].items.filter(it => it[_store].kind !== 'string');\n"
    "                this[_store].types = this[_store].types.filter(t => t === 'Files');\n"
    "            }\n"
    "        };\n"
    "\n"
    "        proto.setDragImage = function(image, x, y) {\n"
    "            if (!this[_store]) globalThis.__initDataTransferInstance(this);\n"
    "            this[_store].dragImage = { image, x, y };\n"
    "        };\n"
    "\n"
    "        const originalCtor = globalThis.DataTransfer;\n"
    "        const newCtor = function() {\n"
    "            const dt = Object.create(proto);\n"
    "            globalThis.__initDataTransferInstance(dt, 'readwrite');\n"
    "            return dt;\n"
    "        };\n"
    "        newCtor.prototype = proto;\n"
    "        proto.constructor = newCtor;\n"
    "        globalThis.DataTransfer = newCtor;\n"
    "    }\n"
    "\n"
    "    // DragEvent Polyfill/Override\n"
    "    if (globalThis.DragEvent) {\n"
    "        const proto = globalThis.DragEvent.prototype;\n"
    "        Object.defineProperty(proto, 'dataTransfer', {\n"
    "            get() {\n"
    "                if (!this._dataTransfer) {\n"
    "                    let mode = 'protected';\n"
    "                    const type = (this.type || '').toLowerCase();\n"
    "                    if (type === 'dragstart') {\n"
    "                        mode = 'readwrite';\n"
    "                    } else if (type === 'drop') {\n"
    "                        mode = 'readonly';\n"
    "                    }\n"
    "                    const dt = new globalThis.DataTransfer();\n"
    "                    globalThis.__initDataTransferInstance(dt, mode);\n"
    "                    this._dataTransfer = dt;\n"
    "                }\n"
    "                return this._dataTransfer;\n"
    "            },\n"
    "            configurable: true, enumerable: true\n"
    "        });\n"
    "    }\n"
    "})();\n";

static void run_drag_drop_polyfill(JSContext *ctx) {
    JSValue val = JS_Eval(ctx, drag_drop_polyfill_js, strlen(drag_drop_polyfill_js), "<drag_drop_polyfill>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val)) {
        JSValue exception = JS_GetException(ctx);
        const char *str = JS_ToCString(ctx, exception);
        NSLOG(wisp, ERROR, "Error in Drag & Drop Polyfill: %s", str);
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, exception);
    }
    JS_FreeValue(ctx, val);
}

int qjs_init_dragevent(JSContext *ctx) {
    qjs_init_dragevent_gen(ctx);
    run_drag_drop_polyfill(ctx);
    return 0;
}

void wisp_dnd_dispatch_native_event(
    void *thread_ptr,
    wisp_dnd_event_type_t type,
    wisp_dnd_payload_t *payload,
    int screen_x,
    int screen_y)
{
    struct jsthread *t = thread_ptr;
    if (!t || t->closed || !t->ctx) return;
    JSContext *ctx = t->ctx;

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue dispatch_fn = JS_GetPropertyStr(ctx, global, "__dispatchNativeDragEvent");
    if (!JS_IsFunction(ctx, dispatch_fn)) {
        JS_FreeValue(ctx, global);
        JS_FreeValue(ctx, dispatch_fn);
        return;
    }

    const char *type_str = "dragover";
    switch (type) {
        case WISP_DND_DRAGSTART: type_str = "dragstart"; break;
        case WISP_DND_DRAGENTER: type_str = "dragenter"; break;
        case WISP_DND_DRAGOVER:  type_str = "dragover";  break;
        case WISP_DND_DRAGLEAVE: type_str = "dragleave"; break;
        case WISP_DND_DROP:      type_str = "drop";      break;
        case WISP_DND_DRAGEND:   type_str = "dragend";   break;
    }

    JSValue js_type = JS_NewString(ctx, type_str);

    JSValue js_mimes = JS_NewArray(ctx);
    if (payload && payload->mime_types) {
        for (size_t i = 0; i < payload->type_count; i++) {
            JS_SetPropertyUint32(ctx, js_mimes, i, JS_NewString(ctx, payload->mime_types[i]));
        }
    }

    JSValue js_data = JS_NewString(ctx, "");
    if (payload && payload->raw_data && payload->data_len > 0) {
        js_data = JS_NewStringLen(ctx, (const char *)payload->raw_data, payload->data_len);
    }

    struct dom_document *doc = qjs_thread_get_document(t);
    JSValue js_target = JS_NULL;
    if (doc) {
        dom_element *doc_el = NULL;
        dom_document_get_document_element(doc, &doc_el);
        if (doc_el) {
            js_target = qjs_wrap_node(ctx, (dom_node *)doc_el);
            dom_node_unref((dom_node *)doc_el);
        } else {
            js_target = qjs_wrap_node(ctx, (dom_node *)doc);
        }
    }

    uint32_t allowed = payload ? payload->allowed_effects : 0;
    JSValue js_allowed = JS_NewInt32(ctx, allowed);

    JSValue args[5] = { js_target, js_type, js_mimes, js_data, js_allowed };
    JSValue ret = JS_Call(ctx, dispatch_fn, JS_UNDEFINED, 5, args);

    JS_FreeValue(ctx, ret);
    JS_FreeValue(ctx, js_target);
    JS_FreeValue(ctx, js_type);
    JS_FreeValue(ctx, js_mimes);
    JS_FreeValue(ctx, js_data);
    JS_FreeValue(ctx, js_allowed);
    JS_FreeValue(ctx, dispatch_fn);
    JS_FreeValue(ctx, global);
}

void wisp_dnd_dispatch_native_event_bw(
    struct browser_window *bw,
    wisp_dnd_event_type_t type,
    wisp_dnd_payload_t *payload,
    int screen_x,
    int screen_y)
{
    if (!bw || !bw->current_content) return;

    struct jsthread *thread = NULL;
    union content_msg_data msg_data;
    msg_data.jsthread = &thread;

    content_broadcast(hlcache_handle_get_content(bw->current_content), CONTENT_MSG_GETTHREAD, &msg_data);

    if (thread) {
        wisp_dnd_dispatch_native_event(thread, type, payload, screen_x, screen_y);
    }
}

int qjs_init_datatransfer(JSContext *ctx) {
    qjs_init_datatransfer_gen(ctx);
    run_drag_drop_polyfill(ctx);
    return 0;
}

int qjs_init_datatransferitem(JSContext *ctx) {
    qjs_init_datatransferitem_gen(ctx);
    run_drag_drop_polyfill(ctx);
    return 0;
}

int qjs_init_datatransferitemlist(JSContext *ctx) {
    qjs_init_datatransferitemlist_gen(ctx);
    run_drag_drop_polyfill(ctx);
    return 0;
}
