#ifndef WISP_QUICKJS_DOM_BRIDGE_H
#define WISP_QUICKJS_DOM_BRIDGE_H

#include "quickjs.h"
#include "utils/libdom.h"
#include <stdbool.h>
#include "qjs_internal.h"

extern JSClassID qjs_eventtarget_class_id;
extern JSClassID qjs_node_class_id;
extern JSClassID qjs_element_class_id;
extern JSClassID qjs_document_class_id;
extern JSClassID qjs_text_class_id;
extern JSClassID qjs_attr_class_id;
extern JSClassID qjs_namednodemap_class_id;
extern JSClassID qjs_htmlcollection_class_id;
extern JSClassID qjs_window_class_id;
extern JSClassID qjs_mutationobserver_class_id;
extern JSClassID qjs_intersectionobserver_class_id;
extern JSClassID qjs_domrect_class_id;
extern JSClassID qjs_domrectreadonly_class_id;

int qjs_init_node(JSContext *ctx);
int qjs_init_element(JSContext *ctx);
int qjs_init_document(JSContext *ctx);
int qjs_init_text(JSContext *ctx);
int qjs_init_attr(JSContext *ctx);
int qjs_init_namednodemap(JSContext *ctx);
int qjs_init_htmlcollection(JSContext *ctx);

/* These are now generated in generated_bindings.h */
/* We include it here so all components see the consistent generated signatures */
#include "generated_bindings.h"

void qjs_bridge_remove_node(JSRuntime *rt, struct dom_node *node, JSContext *ctx);

/**
 * Wrap a libdom node into a QuickJS object.
 * This handles memoization to ensure that the same dom_node always
 * returns the same JSValue.
 */
JSValue qjs_wrap_node(JSContext *ctx, struct dom_node *node);

/**
 * Initialize the DOM bridge for a context.
 */
int qjs_init_dom_bridge(JSContext *ctx);

void qjs_bridge_remove_node(JSRuntime *rt, struct dom_node *node, JSContext *ctx);
void qjs_finalise_dom_bridge(JSContext *ctx);

/* Other component initializers */
int qjs_init_console(JSContext *ctx);
int qjs_init_window(JSContext *ctx);
int qjs_init_timers(JSContext *ctx);
int qjs_init_navigator(JSContext *ctx);
int qjs_init_location(JSContext *ctx);
int qjs_init_storage(JSContext *ctx);
int qjs_init_eventtarget(JSContext *ctx);
int qjs_init_xhr(JSContext *ctx);
int qjs_init_mutationobserver(JSContext *ctx);
int qjs_init_intersectionobserver(JSContext *ctx);
int qjs_init_domrect(JSContext *ctx);
int qjs_init_domrectreadonly(JSContext *ctx);
int qjs_init_unimplemented(JSContext *ctx);

#endif /* WISP_QUICKJS_DOM_BRIDGE_H */
