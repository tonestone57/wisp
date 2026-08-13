#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "quickjs.h"

struct dom_node;

bool wisp_dom_element_get_style_property(struct dom_node *node, const char *property, char *buf, size_t buf_size) {
    return false;
}

void wisp_dom_element_remove_style_property(struct dom_node *node, const char *property, char *removed_val, size_t max_len) {
}

bool wisp_dom_element_is_style_important(struct dom_node *node, const char *property) {
    return false;
}

void wisp_dom_element_set_style_property(struct dom_node *node, const char *property, const char *value, const char *priority) {
}

JSValue qjs_new_computed_style_declaration(JSContext *ctx, struct dom_node *node) {
    return JS_NULL;
}
