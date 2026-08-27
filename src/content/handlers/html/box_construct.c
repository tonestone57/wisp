/*
 * Copyright 2005 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2003 Phil Mellor <monkeyson@users.sourceforge.net>
 * Copyright 2005 John M Bell <jmb202@ecs.soton.ac.uk>
 * Copyright 2006 Richard Wilson <info@tinct.net>
 * Copyright 2008 Michael Drake <tlsa@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * Implementation of conversion from DOM tree to box tree.
 */

#include <dom/dom.h>
#include <string.h>

#include <wisp/desktop/gui_internal.h>
#include <wisp/misc.h>
#include <wisp/utils/ascii.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/log.h>
#include <wisp/utils/nsoption.h>
#include <wisp/utils/nsurl.h>
#include <wisp/utils/string.h>
#include <wisp/utils/utf8.h>
#include <nsutils/time.h>
#include "utils/arena.h"
#include "utils/talloc.h"
#include "utils/utils.h"
#include "content/handlers/css/select.h"
#include <wctype.h>

#include <wisp/content/fetch.h>
#include <wisp/content/handlers/html/box.h>
#include <wisp/content/handlers/html/form_internal.h>
#include <wisp/content/handlers/html/private.h>
#include "content/handlers/html/box_construct.h"
#include "content/handlers/html/box_manipulate.h"
#include "content/handlers/html/box_special.h"
#include "content/handlers/html/object.h"

/**
 * Context for box tree construction
 */
struct box_construct_ctx {
	html_content *content; /**< Content we're constructing for */

	dom_node *n; /**< Current node to process */

	struct box *root_box; /**< Root box in the tree */

	box_construct_complete_cb cb; /**< Callback to invoke on completion */

	struct arena *bctx; /**< talloc context */

	int quote_nesting_level;
};

/**
 * Transient properties for construction of current node
 */
struct box_construct_props {
	/** Style from which to inherit, or NULL if none */
	const css_computed_style *parent_style;
	/** Current link target, or NULL if none */
	struct nsurl *href;
	/** Current frame target, or NULL if none */
	const char *target;
	/** Current title attribute, or NULL if none */
	const char *title;
	/** Identity of the current block-level container */
	struct box *containing_block;
	/** Current container for inlines, or NULL if none
	 * \note If non-NULL, will be the last child of containing_block */
	struct box *inline_container;
	/** Whether the current node is the root of the DOM tree */
	bool node_is_root;
};

static const content_type image_types = CONTENT_IMAGE;

/* Mapping from CSS display to box type.
 * Uses designated initializers so order doesn't matter. */
static const box_type box_map[CSS_DISPLAY_CONTENTS + 1] = {
	[CSS_DISPLAY_INHERIT] = BOX_BLOCK,
	[CSS_DISPLAY_INLINE] = BOX_INLINE,
	[CSS_DISPLAY_BLOCK] = BOX_BLOCK,
	[CSS_DISPLAY_LIST_ITEM] = BOX_BLOCK,
	[CSS_DISPLAY_RUN_IN] = BOX_INLINE,
	[CSS_DISPLAY_INLINE_BLOCK] = BOX_INLINE_BLOCK,
	[CSS_DISPLAY_TABLE] = BOX_TABLE,
	[CSS_DISPLAY_INLINE_TABLE] = BOX_TABLE,
	[CSS_DISPLAY_TABLE_ROW_GROUP] = BOX_TABLE_ROW_GROUP,
	[CSS_DISPLAY_TABLE_HEADER_GROUP] = BOX_TABLE_ROW_GROUP,
	[CSS_DISPLAY_TABLE_FOOTER_GROUP] = BOX_TABLE_ROW_GROUP,
	[CSS_DISPLAY_TABLE_ROW] = BOX_TABLE_ROW,
	[CSS_DISPLAY_TABLE_COLUMN_GROUP] = BOX_NONE,
	[CSS_DISPLAY_TABLE_COLUMN] = BOX_NONE,
	[CSS_DISPLAY_TABLE_CELL] = BOX_TABLE_CELL,
	[CSS_DISPLAY_TABLE_CAPTION] = BOX_INLINE,
	[CSS_DISPLAY_NONE] = BOX_NONE,
	[CSS_DISPLAY_FLEX] = BOX_FLEX,
	[CSS_DISPLAY_INLINE_FLEX] = BOX_INLINE_FLEX,
	[CSS_DISPLAY_GRID] = BOX_GRID,
	[CSS_DISPLAY_INLINE_GRID] = BOX_INLINE_GRID,
	[CSS_DISPLAY_CONTENTS] = BOX_NONE,
};
_Static_assert(CSS_DISPLAY_CONTENTS == 0x15, "css_display_e has new values — update box_map");


/**
 * determine if a box is the root node
 *
 * \param n node to check
 * \return true if node is root else false.
 */
static inline bool box_is_root(dom_node *n)
{
	dom_node *parent;
	dom_node_type type;
	dom_exception err;

	err = dom_node_get_parent_node(n, &parent);
	if (err != DOM_NO_ERR)
		return false;

	if (parent != NULL) {
		err = dom_node_get_node_type(parent, &type);

		dom_node_unref(parent);

		if (err != DOM_NO_ERR)
			return false;

		if (type != DOM_DOCUMENT_NODE)
			return false;
	}

	return true;
}

/**
 * Extract transient construction properties
 *
 * \param n      Current DOM node to convert
 * \param props  Property object to populate
 */
static void box_extract_properties(dom_node *n, struct box_construct_props *props)
{
	memset(props, 0, sizeof(*props));

	props->node_is_root = box_is_root(n);

	/* Extract properties from containing DOM node */
	if (props->node_is_root == false) {
		dom_node *current_node = n;
		dom_node *parent_node = NULL;
		struct box *parent_box;
		dom_exception err;

		/* Find ancestor node containing parent box */
		while (true) {
			err = dom_node_get_parent_node(current_node, &parent_node);
			if (err != DOM_NO_ERR || parent_node == NULL)
				break;

			parent_box = box_for_node(parent_node);

			if (parent_box != NULL) {
				props->parent_style = parent_box->style;
				props->href = parent_box->href;
				props->target = parent_box->target;
				props->title = parent_box->title;

				dom_node_unref(parent_node);
				break;
			} else {
				if (current_node != n)
					dom_node_unref(current_node);
				current_node = parent_node;
				parent_node = NULL;
			}
		}

		/* Reset node pointers to traverse again for containing block */
		current_node = n;
		parent_node = NULL;

		/* Find containing block (may be parent) */
		while (true) {
			struct box *b;

			err = dom_node_get_parent_node(current_node, &parent_node);
			if (err != DOM_NO_ERR || parent_node == NULL) {
				if (current_node != n)
					dom_node_unref(current_node);
				break;
			}

			if (current_node != n)
				dom_node_unref(current_node);

			b = box_for_node(parent_node);

			/* Children of nodes that created an inline box
			 * will generate boxes which are attached as
			 * _siblings_ of the box generated for their
			 * parent node. Note, however, that we'll still
			 * use the parent node's styling as the parent
			 * style, above. */
			if (b != NULL && b->type != BOX_INLINE && b->type != BOX_BR && b->type != BOX_NONE) {
				props->containing_block = b;

				dom_node_unref(parent_node);
				break;
			} else {
				current_node = parent_node;
				parent_node = NULL;
			}
		}
	}

	/* Compute current inline container, if any */
	if (props->containing_block != NULL && props->containing_block->last != NULL &&
		props->containing_block->last->type == BOX_INLINE_CONTAINER)
		props->inline_container = props->containing_block->last;
}


/**
 * Get the style for an element.
 *
 * \param  c               content of type CONTENT_HTML that is being processed
 * \param  parent_style    style at this point in xml tree, or NULL for root
 * \param  root_style      root node's style, or NULL for root
 * \param  n               node in xml tree
 * \return  the new style, or NULL on memory exhaustion
 */
#include <unistd.h>
#include "content/handlers/javascript/quickjs/wisp_subsystem.h"

extern bool wisp_dispatch_style(const char *script, void (*func)(void*), void *arg, float priority);

static pthread_mutex_t dom_lock = PTHREAD_MUTEX_INITIALIZER;

struct wisp_wait_group {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    volatile int count;
};

static inline void wisp_wait_group_init(struct wisp_wait_group *wg, int count) {
    pthread_mutex_init(&wg->mutex, NULL);
    pthread_cond_init(&wg->cond, NULL);
    wg->count = count;
}

static inline void wisp_wait_group_done(struct wisp_wait_group *wg) {
    pthread_mutex_lock(&wg->mutex);
    wg->count--;
    if (wg->count <= 0) {
        pthread_cond_broadcast(&wg->cond);
    }
    pthread_mutex_unlock(&wg->mutex);
}

static inline void wisp_wait_group_wait(struct wisp_wait_group *wg) {
    pthread_mutex_lock(&wg->mutex);
    while (wg->count > 0) {
        pthread_cond_wait(&wg->cond, &wg->mutex);
    }
    pthread_mutex_unlock(&wg->mutex);
}

static inline void wisp_wait_group_wait_and_pump(struct wisp_wait_group *wg, WispPool *style_pool) {
    while (__atomic_load_n(&wg->count, __ATOMIC_RELAXED) > 0) {
        /* Main thread pops and executes pending style/layout tasks directly */
        js_task_t *task = wisp_pool_pop_task(style_pool);
        if (task) {
            if (task->function) {
                task->function(task->arg);
            }
            if (task->script) {
                free(task->script);
            }
            free(task);
        } else {
            /* Fall back to short micro-sleep if queue is empty */
            usleep(100);
        }
    }
}

static inline void wisp_wait_group_destroy(struct wisp_wait_group *wg) {
    pthread_mutex_destroy(&wg->mutex);
    pthread_cond_destroy(&wg->cond);
}

static bool node_is_independent_subtree_root(dom_node *node) {
    if (node == NULL) return false;
    dom_node_type type;
    if (dom_node_get_node_type(node, &type) != DOM_NO_ERR || type != DOM_ELEMENT_NODE) {
        return false;
    }

    /* Check if it's a CSS Grid item, i.e. its parent is a grid container */
    dom_node *parent = NULL;
    if (dom_node_get_parent_node(node, &parent) == DOM_NO_ERR && parent != NULL) {
        dom_string *parent_class_attr = NULL;
        if (dom_element_get_attribute(parent, corestring_dom_class, &parent_class_attr) == DOM_NO_ERR && parent_class_attr != NULL) {
            const char *pcls = dom_string_data(parent_class_attr);
            if (pcls != NULL && (strstr(pcls, "grid") != NULL || strstr(pcls, "flex") != NULL)) {
                dom_string_unref(parent_class_attr);
                dom_node_unref(parent);
                return true;
            }
            dom_string_unref(parent_class_attr);
        }
        dom_node_unref(parent);
    }

    /* Check if it has classes like contain-layout or contain */
    dom_string *class_attr = NULL;
    if (dom_element_get_attribute(node, corestring_dom_class, &class_attr) == DOM_NO_ERR && class_attr != NULL) {
        const char *cls = dom_string_data(class_attr);
        if (cls != NULL && (strstr(cls, "contain-layout") != NULL || strstr(cls, "contain") != NULL || strstr(cls, "grid") != NULL || strstr(cls, "flex") != NULL)) {
            dom_string_unref(class_attr);
            return true;
        }
        dom_string_unref(class_attr);
    }

    /* Check style attribute */
    dom_string *style_attr = NULL;
    if (dom_element_get_attribute(node, corestring_dom_style, &style_attr) == DOM_NO_ERR && style_attr != NULL) {
        const char *style = dom_string_data(style_attr);
        if (style != NULL && (strstr(style, "display: grid") != NULL || strstr(style, "display: flex") != NULL || strstr(style, "contain: layout") != NULL || strstr(style, "contain") != NULL)) {
            dom_string_unref(style_attr);
            return true;
        }
        dom_string_unref(style_attr);
    }
    return false;
}

#include <wisp/plot_style.h>

__attribute__((weak)) extern lwc_string *corestring_lwc_a;
__attribute__((weak)) extern dom_string *corestring_dom_href;

__attribute__((weak)) extern css_error node_is_visited(void *pw, void *node, bool *match);
__attribute__((weak)) extern css_error node_presentational_hint(void *pw, void *node, uint32_t *nhints, css_hint **hints);
__attribute__((weak)) extern css_error get_libcss_node_data(void *pw, void *node, void **libcss_node_data);
__attribute__((weak)) extern css_error set_libcss_node_data(void *pw, void *node, void *libcss_node_data);

typedef struct {
    lwc_string *name_lwc;
    lwc_string *value_lwc;
} snapshot_attr_t;

typedef struct style_snapshot_s style_snapshot_t;

struct style_snapshot_s {
    dom_node *node;                  /* Weak pointer back to original dom_node */
    lwc_string *name_lwc;            /* Tag name */
    lwc_string *id;                  /* Element ID */
    lwc_string **classes;            /* Class names array */
    uint32_t n_classes;              /* Number of classes */
    dom_html_element_type tag_type;   /* Tag type for presentational hints */
    css_stylesheet *inline_style;    /* Inline style stylesheet */
    void *libcss_node_data;          /* libcss private node data */

    snapshot_attr_t *attrs;          /* Array of attributes */
    uint32_t n_attrs;                /* Number of attributes */

    /* Pre-calculated selection states */
    bool is_link;
    bool is_visited;
    bool is_empty;

    /* Pre-calculated presentational hints */
    uint32_t nhints;
    css_hint *hints;

    /* Snapshot structural relationships */
    style_snapshot_t *parent;
    style_snapshot_t *prev_sibling;
    style_snapshot_t *next_sibling;
    style_snapshot_t *first_child;
    style_snapshot_t *last_child;
};

static css_error snap_node_name(void *pw, void *node, css_qname *qname) {
    style_snapshot_t *snap = node;
    qname->ns = NULL;
    qname->name = lwc_string_ref(snap->name_lwc);
    return CSS_OK;
}

static css_error snap_node_classes(void *pw, void *node, lwc_string ***classes, uint32_t *n_classes) {
    style_snapshot_t *snap = node;
    *classes = snap->classes;
    *n_classes = snap->n_classes;
    /* Note: LibCSS unconditionally calls lwc_string_unref on the classes returned
     * from this node_classes selection callback, so we must increment their reference
     * counts here to balance LibCSS's cleanup decrement. */
    for (uint32_t i = 0; i < snap->n_classes; i++) {
        lwc_string_ref(snap->classes[i]);
    }
    return CSS_OK;
}

static css_error snap_node_id(void *pw, void *node, lwc_string **id) {
    style_snapshot_t *snap = node;
    if (snap->id != NULL) {
        *id = lwc_string_ref(snap->id);
    } else {
        *id = NULL;
    }
    return CSS_OK;
}

static css_error snap_named_ancestor_node(void *pw, void *node, const css_qname *qname, void **ancestor) {
    style_snapshot_t *curr = ((style_snapshot_t *)node)->parent;
    *ancestor = NULL;
    while (curr != NULL) {
        bool match = false;
        if (lwc_string_caseless_isequal(curr->name_lwc, qname->name, &match) == lwc_error_ok && match) {
            *ancestor = curr;
            break;
        }
        curr = curr->parent;
    }
    return CSS_OK;
}

static css_error snap_named_parent_node(void *pw, void *node, const css_qname *qname, void **parent) {
    style_snapshot_t *curr = ((style_snapshot_t *)node)->parent;
    *parent = NULL;
    if (curr != NULL) {
        bool match = false;
        if (lwc_string_caseless_isequal(curr->name_lwc, qname->name, &match) == lwc_error_ok && match) {
            *parent = curr;
        }
    }
    return CSS_OK;
}

static css_error snap_named_sibling_node(void *pw, void *node, const css_qname *qname, void **sibling) {
    style_snapshot_t *curr = ((style_snapshot_t *)node)->prev_sibling;
    *sibling = NULL;
    while (curr != NULL) {
        bool match = false;
        if (lwc_string_caseless_isequal(curr->name_lwc, qname->name, &match) == lwc_error_ok && match) {
            *sibling = curr;
            break;
        }
        curr = curr->prev_sibling;
    }
    return CSS_OK;
}

static css_error snap_named_generic_sibling_node(void *pw, void *node, const css_qname *qname, void **sibling) {
    style_snapshot_t *curr = ((style_snapshot_t *)node)->prev_sibling;
    *sibling = NULL;
    while (curr != NULL) {
        bool match = false;
        if (lwc_string_caseless_isequal(curr->name_lwc, qname->name, &match) == lwc_error_ok && match) {
            *sibling = curr;
            break;
        }
        curr = curr->prev_sibling;
    }
    return CSS_OK;
}

static css_error snap_parent_node(void *pw, void *node, void **parent) {
    *parent = ((style_snapshot_t *)node)->parent;
    return CSS_OK;
}

static css_error snap_sibling_node(void *pw, void *node, void **sibling) {
    *sibling = ((style_snapshot_t *)node)->prev_sibling;
    return CSS_OK;
}

static css_error snap_node_has_name(void *pw, void *node, const css_qname *qname, bool *match) {
    style_snapshot_t *snap = node;
    nscss_select_ctx *ctx = pw;
    if (lwc_string_isequal(qname->name, ctx->universal, match) == lwc_error_ok && *match == false) {
        lwc_string_caseless_isequal(snap->name_lwc, qname->name, match);
    }
    return CSS_OK;
}

static css_error snap_node_has_class(void *pw, void *node, lwc_string *name, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    for (uint32_t i = 0; i < snap->n_classes; i++) {
        if (lwc_string_caseless_isequal(snap->classes[i], name, match) == lwc_error_ok && *match) {
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_has_id(void *pw, void *node, lwc_string *name, bool *match) {
    style_snapshot_t *snap = node;
    if (snap->id != NULL) {
        lwc_string_isequal(snap->id, name, match);
    } else {
        *match = false;
    }
    return CSS_OK;
}

static css_error snap_node_has_attribute(void *pw, void *node, const css_qname *qname, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    for (uint32_t i = 0; i < snap->n_attrs; i++) {
        bool name_match = false;
        if (lwc_string_caseless_isequal(snap->attrs[i].name_lwc, qname->name, &name_match) == lwc_error_ok && name_match) {
            *match = true;
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_has_attribute_equal(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    for (uint32_t i = 0; i < snap->n_attrs; i++) {
        bool name_match = false;
        if (lwc_string_caseless_isequal(snap->attrs[i].name_lwc, qname->name, &name_match) == lwc_error_ok && name_match) {
            lwc_string_caseless_isequal(snap->attrs[i].value_lwc, value, match);
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_has_attribute_dashmatch(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    size_t vlen = lwc_string_length(value);
    if (vlen == 0) return CSS_OK;

    for (uint32_t i = 0; i < snap->n_attrs; i++) {
        bool name_match = false;
        if (lwc_string_caseless_isequal(snap->attrs[i].name_lwc, qname->name, &name_match) == lwc_error_ok && name_match) {
            lwc_string_caseless_isequal(snap->attrs[i].value_lwc, value, match);
            if (*match == false) {
                const char *vdata = lwc_string_data(value);
                const char *data = lwc_string_data(snap->attrs[i].value_lwc);
                size_t len = lwc_string_length(snap->attrs[i].value_lwc);
                if (len > vlen && data[vlen] == '-' && strncasecmp(data, vdata, vlen) == 0) {
                    *match = true;
                }
            }
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_has_attribute_includes(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    size_t vlen = lwc_string_length(value);
    if (vlen == 0) return CSS_OK;

    for (uint32_t i = 0; i < snap->n_attrs; i++) {
        bool name_match = false;
        if (lwc_string_caseless_isequal(snap->attrs[i].name_lwc, qname->name, &name_match) == lwc_error_ok && name_match) {
            const char *start = lwc_string_data(snap->attrs[i].value_lwc);
            const char *end = start + lwc_string_length(snap->attrs[i].value_lwc);
            const char *p;
            for (p = start; p <= end; p++) {
                if (*p == ' ' || *p == '\0') {
                    if ((size_t)(p - start) == vlen && strncasecmp(start, lwc_string_data(value), vlen) == 0) {
                        *match = true;
                        break;
                    }
                    start = p + 1;
                }
            }
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_has_attribute_prefix(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    size_t vlen = lwc_string_length(value);
    if (vlen == 0) return CSS_OK;

    for (uint32_t i = 0; i < snap->n_attrs; i++) {
        bool name_match = false;
        if (lwc_string_caseless_isequal(snap->attrs[i].name_lwc, qname->name, &name_match) == lwc_error_ok && name_match) {
            lwc_string_caseless_isequal(snap->attrs[i].value_lwc, value, match);
            if (*match == false) {
                const char *data = lwc_string_data(snap->attrs[i].value_lwc);
                size_t len = lwc_string_length(snap->attrs[i].value_lwc);
                if (len >= vlen && strncasecmp(data, lwc_string_data(value), vlen) == 0) {
                    *match = true;
                }
            }
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_has_attribute_suffix(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    size_t vlen = lwc_string_length(value);
    if (vlen == 0) return CSS_OK;

    for (uint32_t i = 0; i < snap->n_attrs; i++) {
        bool name_match = false;
        if (lwc_string_caseless_isequal(snap->attrs[i].name_lwc, qname->name, &name_match) == lwc_error_ok && name_match) {
            lwc_string_caseless_isequal(snap->attrs[i].value_lwc, value, match);
            if (*match == false) {
                const char *data = lwc_string_data(snap->attrs[i].value_lwc);
                size_t len = lwc_string_length(snap->attrs[i].value_lwc);
                if (len >= vlen) {
                    const char *start = data + len - vlen;
                    if (strncasecmp(start, lwc_string_data(value), vlen) == 0) {
                        *match = true;
                    }
                }
            }
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_has_attribute_substring(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match) {
    style_snapshot_t *snap = node;
    *match = false;
    size_t vlen = lwc_string_length(value);
    if (vlen == 0) return CSS_OK;

    for (uint32_t i = 0; i < snap->n_attrs; i++) {
        bool name_match = false;
        if (lwc_string_caseless_isequal(snap->attrs[i].name_lwc, qname->name, &name_match) == lwc_error_ok && name_match) {
            lwc_string_caseless_isequal(snap->attrs[i].value_lwc, value, match);
            if (*match == false) {
                const char *vdata = lwc_string_data(value);
                const char *start = lwc_string_data(snap->attrs[i].value_lwc);
                size_t len = lwc_string_length(snap->attrs[i].value_lwc);
                if (len >= vlen) {
                    const char *last_start = start + len - vlen;
                    while (start <= last_start) {
                        if (strncasecmp(start, vdata, vlen) == 0) {
                            *match = true;
                            break;
                        }
                        start++;
                    }
                }
            }
            break;
        }
    }
    return CSS_OK;
}

static css_error snap_node_is_root(void *pw, void *node, bool *match) {
    *match = (((style_snapshot_t *)node)->parent == NULL);
    return CSS_OK;
}

static css_error snap_node_count_siblings(void *pw, void *n, bool same_name, bool after, int32_t *count) {
    style_snapshot_t *snap = n;
    int32_t cnt = 0;
    if (after) {
        style_snapshot_t *curr = snap->next_sibling;
        while (curr != NULL) {
            if (same_name) {
                bool match = false;
                if (lwc_string_caseless_isequal(curr->name_lwc, snap->name_lwc, &match) == lwc_error_ok && match) {
                    cnt++;
                }
            } else {
                cnt++;
            }
            curr = curr->next_sibling;
        }
    } else {
        style_snapshot_t *curr = snap->prev_sibling;
        while (curr != NULL) {
            if (same_name) {
                bool match = false;
                if (lwc_string_caseless_isequal(curr->name_lwc, snap->name_lwc, &match) == lwc_error_ok && match) {
                    cnt++;
                }
            } else {
                cnt++;
            }
            curr = curr->prev_sibling;
        }
    }
    *count = cnt;
    return CSS_OK;
}

static css_error snap_node_is_empty(void *pw, void *node, bool *match) {
    *match = ((style_snapshot_t *)node)->is_empty;
    return CSS_OK;
}

static css_error snap_node_is_link(void *pw, void *node, bool *match) {
    *match = ((style_snapshot_t *)node)->is_link;
    return CSS_OK;
}

static css_error snap_node_is_visited(void *pw, void *node, bool *match) {
    *match = ((style_snapshot_t *)node)->is_visited;
    return CSS_OK;
}

static css_error snap_node_is_hover(void *pw, void *node, bool *match) {
    *match = false;
    return CSS_OK;
}

static css_error snap_node_is_active(void *pw, void *node, bool *match) {
    *match = false;
    return CSS_OK;
}

static css_error snap_node_is_focus(void *pw, void *node, bool *match) {
    nscss_select_ctx *ctx = pw;

    if (ctx == NULL || ctx->c == NULL) {
        *match = false;
        return CSS_OK;
    }

    dom_node *n = ((style_snapshot_t *)node)->node;
    *match = false;

    if (ctx->c->focus_type == HTML_FOCUS_CONTENT) {
        if (ctx->c->focus_owner.content && ctx->c->focus_owner.content->node == n) {
            *match = true;
        }
    } else if (ctx->c->focus_type == HTML_FOCUS_TEXTAREA) {
        if (ctx->c->focus_owner.textarea && ctx->c->focus_owner.textarea->node == n) {
            *match = true;
        }
    }

    return CSS_OK;
}

static css_error snap_node_is_enabled(void *pw, void *node, bool *match) {
    *match = false;
    return CSS_OK;
}

static css_error snap_node_is_disabled(void *pw, void *node, bool *match) {
    *match = false;
    return CSS_OK;
}

static css_error snap_node_is_checked(void *pw, void *node, bool *match) {
    *match = false;
    return CSS_OK;
}

static css_error snap_node_is_target(void *pw, void *node, bool *match) {
    style_snapshot_t *snap = node;
    return node_is_target(pw, snap->node, match);
}

static css_error snap_node_is_lang(void *pw, void *node, lwc_string *lang, bool *match) {
    *match = false;
    return CSS_OK;
}

static css_error snap_node_presentational_hint(void *pw, void *node, uint32_t *nhints, css_hint **hints) {
    style_snapshot_t *snap = node;
    *nhints = snap->nhints;
    if (snap->nhints > 0) {
        *hints = snap->hints;
    } else {
        *hints = NULL;
    }
    return CSS_OK;
}

static css_error snap_set_libcss_node_data(void *pw, void *node, void *libcss_node_data) {
    style_snapshot_t *snap = node;
    snap->libcss_node_data = libcss_node_data;
    return CSS_OK;
}

static css_error snap_get_libcss_node_data(void *pw, void *node, void **libcss_node_data) {
    style_snapshot_t *snap = node;
    *libcss_node_data = snap->libcss_node_data;
    return CSS_OK;
}

static css_error snap_ua_default_for_property(void *pw, uint32_t property, css_hint *hint)
{
    if (property == CSS_PROP_COLOR) {
        hint->data.color = 0xff000000;
        hint->status = CSS_COLOR_COLOR;
    } else if (property == CSS_PROP_FONT_FAMILY) {
        hint->data.strings = NULL;
        switch (nsoption_int(font_default)) {
        case PLOT_FONT_FAMILY_SANS_SERIF:
            hint->status = CSS_FONT_FAMILY_SANS_SERIF;
            break;
        case PLOT_FONT_FAMILY_SERIF:
            hint->status = CSS_FONT_FAMILY_SERIF;
            break;
        case PLOT_FONT_FAMILY_MONOSPACE:
            hint->status = CSS_FONT_FAMILY_MONOSPACE;
            break;
        case PLOT_FONT_FAMILY_CURSIVE:
            hint->status = CSS_FONT_FAMILY_CURSIVE;
            break;
        case PLOT_FONT_FAMILY_FANTASY:
            hint->status = CSS_FONT_FAMILY_FANTASY;
            break;
        }
    } else if (property == CSS_PROP_QUOTES) {
        extern void *wisp_get_default_quotes_ptr(void);
        hint->data.strings = wisp_get_default_quotes_ptr();
        hint->status = CSS_QUOTES_STRING;
    } else if (property == CSS_PROP_VOICE_FAMILY) {
        hint->data.strings = NULL;
        hint->status = 0;
    } else {
        return CSS_INVALID;
    }

    return CSS_OK;
}

static css_select_handler snapshot_selection_handler = {
    CSS_SELECT_HANDLER_VERSION_1,

    snap_node_name,
    snap_node_classes,
    snap_node_id,
    snap_named_ancestor_node,
    snap_named_parent_node,
    snap_named_sibling_node,
    snap_named_generic_sibling_node,
    snap_parent_node,
    snap_sibling_node,
    snap_node_has_name,
    snap_node_has_class,
    snap_node_has_id,
    snap_node_has_attribute,
    snap_node_has_attribute_equal,
    snap_node_has_attribute_dashmatch,
    snap_node_has_attribute_includes,
    snap_node_has_attribute_prefix,
    snap_node_has_attribute_suffix,
    snap_node_has_attribute_substring,
    snap_node_is_root,
    snap_node_count_siblings,
    snap_node_is_empty,
    snap_node_is_link,
    snap_node_is_visited,
    snap_node_is_hover,
    snap_node_is_active,
    snap_node_is_focus,
    snap_node_is_enabled,
    snap_node_is_disabled,
    snap_node_is_checked,
    snap_node_is_target,
    snap_node_is_lang,
    snap_node_presentational_hint,
    snap_ua_default_for_property,
    snap_set_libcss_node_data,
    snap_get_libcss_node_data,
};

static void html_style_cache_add(html_content *c, dom_node *node, css_select_results *styles) {
	pthread_mutex_lock(&c->style_cache_mutex);
	struct style_cache_node *n = malloc(sizeof(struct style_cache_node));
	if (n) {
		n->node = dom_node_ref(node);
		n->styles = styles;
		n->next = c->style_cache;
		c->style_cache = n;
	}
	pthread_mutex_unlock(&c->style_cache_mutex);
}

static void check_is_link(dom_node *node, bool *match) {
    *match = false;
    if (&corestring_lwc_a == NULL || corestring_lwc_a == NULL ||
        &corestring_dom_href == NULL || corestring_dom_href == NULL) {
        return;
    }
    dom_string *node_name = NULL;
    if (dom_node_get_node_name(node, &node_name) == DOM_NO_ERR && node_name != NULL) {
        if (dom_string_caseless_lwc_isequal(node_name, corestring_lwc_a)) {
            bool has_href = false;
            if (dom_element_has_attribute(node, corestring_dom_href, &has_href) == DOM_NO_ERR && has_href) {
                *match = true;
            }
        }
        dom_string_unref(node_name);
    }
}

static void check_is_empty(dom_node *node, bool *match) {
    *match = true;
    dom_node *child = NULL;
    if (dom_node_get_first_child(node, &child) == DOM_NO_ERR && child != NULL) {
        while (child != NULL) {
            dom_node_type ntype;
            if (dom_node_get_node_type(child, &ntype) == DOM_NO_ERR) {
                if (ntype == DOM_ELEMENT_NODE || ntype == DOM_TEXT_NODE) {
                    *match = false;
                    dom_node_unref(child);
                    break;
                }
            }
            dom_node *next = NULL;
            if (dom_node_get_next_sibling(child, &next) != DOM_NO_ERR) {
                dom_node_unref(child);
                break;
            }
            dom_node_unref(child);
            child = next;
        }
    }
}

static style_snapshot_t *create_style_snapshot(html_content *c, dom_node *node, style_snapshot_t *parent, nscss_select_ctx *select_ctx) {
    if (node == NULL) return NULL;

    dom_node_type type;
    if (dom_node_get_node_type(node, &type) != DOM_NO_ERR || type != DOM_ELEMENT_NODE) {
        return NULL;
    }

    style_snapshot_t *snap = calloc(1, sizeof(style_snapshot_t));
    if (snap == NULL) return NULL;

    snap->node = dom_node_ref(node);
    snap->parent = parent;

    /* 1. Tag name */
    dom_string *name_dom = NULL;
    if (dom_node_get_node_name(node, &name_dom) == DOM_NO_ERR && name_dom != NULL) {
        dom_string_intern(name_dom, &snap->name_lwc);
        dom_string_unref(name_dom);
    }

    /* 2. Element ID */
    dom_string *id_dom = NULL;
    if (dom_element_get_attribute(node, corestring_dom_id, &id_dom) == DOM_NO_ERR && id_dom != NULL) {
        dom_string_intern(id_dom, &snap->id);
        dom_string_unref(id_dom);
    }

    /* 3. Class list */
    lwc_string **classes = NULL;
    uint32_t n_classes = 0;
    /* dom_element_get_classes retrieves a weak pointer to the element's internal class array
     * (element->classes), so we must NOT free the 'classes' pointer itself. It also increments the
     * refcount of each individual class string inside the array by 1. */
    dom_element_get_classes(node, &classes, &n_classes);
    snap->n_classes = n_classes;
    if (n_classes > 0 && classes != NULL) {
        /* Allocate a private copy of the classes array pointers to prevent corrupting/freeing
         * the persistent element's classes array. */
        snap->classes = malloc(sizeof(lwc_string *) * n_classes);
        if (snap->classes != NULL) {
            for (uint32_t i = 0; i < n_classes; i++) {
                /* Hand-off ownership of the string reference increments done by dom_element_get_classes */
                snap->classes[i] = classes[i];
            }
        } else {
            /* On malloc failure, decrement string reference counts to prevent memory leaks,
             * safely leaving the DOM element's own reference counts intact. */
            for (uint32_t i = 0; i < n_classes; i++) {
                lwc_string_unref(classes[i]);
            }
        }
    } else {
        snap->classes = NULL;
    }

    /* 4. Tag type */
    dom_html_element_get_tag_type(node, &snap->tag_type);

    /* 5. Attributes */
    dom_namednodemap *attrs = NULL;
    if (dom_node_get_attributes(node, &attrs) == DOM_NO_ERR && attrs != NULL) {
        uint32_t num_attrs = 0;
        if (dom_namednodemap_get_length(attrs, &num_attrs) == DOM_NO_ERR && num_attrs > 0) {
            snap->attrs = calloc(num_attrs, sizeof(snapshot_attr_t));
            if (snap->attrs != NULL) {
                uint32_t active_attrs = 0;
                for (uint32_t idx = 0; idx < num_attrs; idx++) {
                    dom_attr *attr = NULL;
                    if (dom_namednodemap_item(attrs, idx, (void *)&attr) == DOM_NO_ERR && attr != NULL) {
                        dom_string *name = NULL, *value = NULL;
                        if (dom_attr_get_name(attr, &name) == DOM_NO_ERR && name != NULL) {
                            if (dom_attr_get_value(attr, &value) == DOM_NO_ERR && value != NULL) {
                                dom_string_intern(name, &snap->attrs[active_attrs].name_lwc);
                                dom_string_intern(value, &snap->attrs[active_attrs].value_lwc);
                                active_attrs++;
                                dom_string_unref(value);
                            }
                            dom_string_unref(name);
                        }
                        dom_node_unref(attr);
                    }
                }
                snap->n_attrs = active_attrs;
            }
        }
        dom_namednodemap_unref(attrs);
    }

    /* 6. Pre-calculate states */
    check_is_link(node, &snap->is_link);
    if (node_is_visited != NULL) {
        node_is_visited(select_ctx, node, &snap->is_visited);
    } else {
        snap->is_visited = false;
    }
    check_is_empty(node, &snap->is_empty);

    /* 7. Pre-fetch presentational hints */
    if (node_presentational_hint != NULL) {
        uint32_t nhints = 0;
        css_hint *hints = NULL;
        node_presentational_hint(select_ctx, node, &nhints, &hints);
        if (nhints > 0) {
            snap->nhints = nhints;
            snap->hints = malloc(sizeof(css_hint) * nhints);
            if (snap->hints != NULL) {
                memcpy(snap->hints, hints, sizeof(css_hint) * nhints);
            }
        }
    }

    /* 8. Pre-create/parse inline style stylesheet on the main thread */
    dom_string *s = NULL;
    if (nsoption_bool(author_level_css)) {
        dom_element_get_attribute(node, corestring_dom_style, &s);
    }
    if (s != NULL) {
        snap->inline_style = nscss_create_inline_style((const uint8_t *)dom_string_data(s), dom_string_byte_length(s),
            c->encoding, nsurl_access(c->base_url), c->quirks != DOM_DOCUMENT_QUIRKS_MODE_NONE);
        dom_string_unref(s);
    }

    /* 9. Read the existing libcss_node_data */
    if (get_libcss_node_data != NULL) {
        get_libcss_node_data(select_ctx, node, &snap->libcss_node_data);
    }

    /* 10. Recursively traverse and build child snapshots */
    dom_node *child = NULL;
    if (dom_node_get_first_child(node, &child) == DOM_NO_ERR && child != NULL) {
        style_snapshot_t *prev_child_snap = NULL;
        while (child != NULL) {
            style_snapshot_t *child_snap = create_style_snapshot(c, child, snap, select_ctx);
            if (child_snap != NULL) {
                if (snap->first_child == NULL) {
                    snap->first_child = child_snap;
                }
                if (prev_child_snap != NULL) {
                    prev_child_snap->next_sibling = child_snap;
                    child_snap->prev_sibling = prev_child_snap;
                }
                snap->last_child = child_snap;
                prev_child_snap = child_snap;
            }
            dom_node *next = NULL;
            if (dom_node_get_next_sibling(child, &next) != DOM_NO_ERR) {
                dom_node_unref(child);
                break;
            }
            dom_node_unref(child);
            child = next;
        }
    }

    return snap;
}

static void flatten_snapshot_tree(style_snapshot_t *snap, style_snapshot_t **array, int *count, int max_capacity) {
    if (snap == NULL || *count >= max_capacity) return;
    array[*count] = snap;
    (*count)++;

    style_snapshot_t *child = snap->first_child;
    while (child != NULL) {
        flatten_snapshot_tree(child, array, count, max_capacity);
        child = child->next_sibling;
    }
}

static void free_style_snapshot(style_snapshot_t *snap) {
    if (snap == NULL) return;

    style_snapshot_t *child = snap->first_child;
    while (child != NULL) {
        style_snapshot_t *next = child->next_sibling;
        free_style_snapshot(child);
        child = next;
    }

    if (snap->name_lwc != NULL) {
        lwc_string_unref(snap->name_lwc);
    }
    if (snap->id != NULL) {
        lwc_string_unref(snap->id);
    }

    if (snap->classes != NULL) {
        for (uint32_t i = 0; i < snap->n_classes; i++) {
            lwc_string_unref(snap->classes[i]);
        }
        free(snap->classes);
    }

    if (snap->attrs != NULL) {
        for (uint32_t i = 0; i < snap->n_attrs; i++) {
            if (snap->attrs[i].name_lwc != NULL) {
                lwc_string_unref(snap->attrs[i].name_lwc);
            }
            if (snap->attrs[i].value_lwc != NULL) {
                lwc_string_unref(snap->attrs[i].value_lwc);
            }
        }
        free(snap->attrs);
    }

    if (snap->hints != NULL) {
        free(snap->hints);
    }

    if (snap->inline_style != NULL) {
        css_stylesheet_destroy(snap->inline_style);
    }

    if (snap->libcss_node_data != NULL) {
        css_libcss_node_data_handler(&snapshot_selection_handler, CSS_NODE_DELETED, NULL, snap, NULL, snap->libcss_node_data);
        snap->libcss_node_data = NULL;
    }

    if (snap->node != NULL) {
        dom_node_unref(snap->node);
    }

    free(snap);
}

struct parallel_style_task_data {
    html_content *content;
    style_snapshot_t *snap;
    css_select_results **out_results;
    int index;
    struct wisp_wait_group *wg;
};

static void parallel_style_worker_cb(void *arg) {
    struct parallel_style_task_data *task = arg;
    html_content *c = task->content;
    style_snapshot_t *snap = task->snap;

    nscss_select_ctx select_ctx = {0};
    select_ctx.ctx = c->select_ctx;
    select_ctx.quirks = (c->quirks == DOM_DOCUMENT_QUIRKS_MODE_FULL);
    select_ctx.base_url = c->base_url;
    select_ctx.universal = c->universal;
    select_ctx.root_style = NULL;
    select_ctx.parent_style = NULL;
    select_ctx.c = c;

    css_select_results *styles = NULL;
    pthread_mutex_lock(&dom_lock);
    css_error error = css_select_style(c->select_ctx, snap, &c->unit_len_ctx, &c->media, snap->inline_style, &snapshot_selection_handler, &select_ctx, &styles);
    pthread_mutex_unlock(&dom_lock);

    if (error == CSS_OK) {
        task->out_results[task->index] = styles;
    } else {
        task->out_results[task->index] = NULL;
    }

    wisp_wait_group_done(task->wg);
    free(task);
}

static void html_parallel_style_selection(html_content *c, dom_node *root) {
    if (c == NULL || root == NULL || c->select_ctx == NULL) return;

    nscss_select_ctx select_ctx = {0};
    select_ctx.ctx = c->select_ctx;
    select_ctx.quirks = (c->quirks == DOM_DOCUMENT_QUIRKS_MODE_FULL);
    select_ctx.base_url = c->base_url;
    select_ctx.universal = c->universal;
    select_ctx.root_style = NULL;
    select_ctx.parent_style = NULL;
    select_ctx.c = c;

    /* Perform a fast, sequential pre-pass down the DOM tree to construct a lightweight Element Property Snapshot */
    style_snapshot_t *snap_root = create_style_snapshot(c, root, NULL, &select_ctx);
    if (snap_root == NULL) return;

    #define MAX_ELEMENTS 512
    style_snapshot_t *snap_elements[MAX_ELEMENTS];
    int count = 0;
    flatten_snapshot_tree(snap_root, snap_elements, &count, MAX_ELEMENTS);

    if (count == 0) {
        free_style_snapshot(snap_root);
        return;
    }

    /* Flat concurrent-write results array */
    css_select_results **out_styles = calloc(count, sizeof(css_select_results *));
    if (out_styles == NULL) {
        free_style_snapshot(snap_root);
        return;
    }

    struct wisp_wait_group wg;
    wisp_wait_group_init(&wg, count);

    /* Dispatch styling tasks in parallel to workers */
    for (int i = 0; i < count; i++) {
        struct parallel_style_task_data *task = malloc(sizeof(struct parallel_style_task_data));
        if (task != NULL) {
            task->content = c;
            task->snap = snap_elements[i];
            task->out_results = out_styles;
            task->index = i;
            task->wg = &wg;

            if (!wisp_dispatch_style(NULL, parallel_style_worker_cb, task, 0.5f)) {
                parallel_style_worker_cb(task);
            }
        } else {
            out_styles[i] = NULL;
            wisp_wait_group_done(&wg);
        }
    }

    /* Join phase: wait for all worker tasks to finish while pumping pending tasks */
    wisp_wait_group_wait_and_pump(&wg, wisp_style_pool);
    wisp_wait_group_destroy(&wg);

    /* Top-down snapshot composition on the main thread */
    for (int i = 0; i < count; i++) {
        style_snapshot_t *snap = snap_elements[i];
        dom_node *n = snap->node;
        css_select_results *styles = out_styles[i];
        if (styles == NULL) {
            continue;
        }

        /* Determine parent style */
        const css_computed_style *parent_style = NULL;
        dom_node *parent = NULL;
        if (dom_node_get_parent_node(n, &parent) == DOM_NO_ERR && parent != NULL) {
            /* Check if the parent style is in our cache */
            css_select_results *parent_cached = NULL;
            dom_node_get_user_data(parent, corestring_dom___ns_key_style_cache_data, (void *)&parent_cached);
            if (parent_cached != NULL) {
                parent_style = parent_cached->styles[CSS_PSEUDO_ELEMENT_NONE];
            } else {
                /* Try to find parent's box style */
                struct box *parent_box = box_for_node(parent);
                if (parent_box != NULL) {
                    parent_style = parent_box->style;
                }
            }
            dom_node_unref(parent);
        }

        /* Compose style top-down with parent style snapshot */
        if (styles->styles[CSS_PSEUDO_ELEMENT_NONE] == NULL) {
            nscss_select_ctx select_ctx = {0};
            select_ctx.ctx = c->select_ctx;
            select_ctx.quirks = (c->quirks == DOM_DOCUMENT_QUIRKS_MODE_FULL);
            select_ctx.base_url = c->base_url;
            select_ctx.universal = c->universal;
            select_ctx.root_style = NULL;
            select_ctx.parent_style = parent_style;
            select_ctx.c = c;
            styles->styles[CSS_PSEUDO_ELEMENT_NONE] = nscss_get_blank_style(&select_ctx, &c->unit_len_ctx, parent_style);
        } else if (parent_style != NULL) {
            css_computed_style *composed = NULL;
            css_error error = CSS_OK;
            error = css_computed_style_compose(
                parent_style, styles->styles[CSS_PSEUDO_ELEMENT_NONE], &c->unit_len_ctx, &composed);
            if (error == CSS_OK) {
                css_computed_style_destroy(styles->styles[CSS_PSEUDO_ELEMENT_NONE]);
                styles->styles[CSS_PSEUDO_ELEMENT_NONE] = composed;
            }
        }

        if (parent_style != NULL) {
            css_error error = CSS_OK;
            css_computed_style *composed = NULL;

            /* Compose pseudo elements as well */
            for (int pseudo = CSS_PSEUDO_ELEMENT_NONE + 1; pseudo < CSS_PSEUDO_ELEMENT_COUNT; pseudo++) {
                if (styles->styles[pseudo] == NULL) continue;

                css_computed_style *base_style = styles->styles[CSS_PSEUDO_ELEMENT_NONE];
                css_computed_style *temp_base_style = NULL;
                if (base_style == NULL) {
                    nscss_select_ctx select_ctx = {0};
                    select_ctx.ctx = c->select_ctx;
                    select_ctx.quirks = (c->quirks == DOM_DOCUMENT_QUIRKS_MODE_FULL);
                    select_ctx.base_url = c->base_url;
                    select_ctx.universal = c->universal;
                    select_ctx.root_style = NULL;
                    select_ctx.parent_style = parent_style;
                    select_ctx.c = c;
                    temp_base_style = nscss_get_blank_style(&select_ctx, &c->unit_len_ctx, parent_style);
                    base_style = temp_base_style;
                }

                if (base_style != NULL) {
                    error = css_computed_style_compose(
                        base_style, styles->styles[pseudo], &c->unit_len_ctx, &composed);
                    if (error == CSS_OK) {
                        css_computed_style_destroy(styles->styles[pseudo]);
                        styles->styles[pseudo] = composed;
                    }
                }

                if (temp_base_style != NULL) {
                    css_computed_style_destroy(temp_base_style);
                }
            }
        }

        /* Save the updated libcss_node_data back to the original dom_node's user data */
        if (set_libcss_node_data != NULL) {
            set_libcss_node_data(&select_ctx, n, snap->libcss_node_data);
            snap->libcss_node_data = NULL;
        }

        /* Cache pre-computed style results on the DOM node */
        dom_node_set_user_data(n, corestring_dom___ns_key_style_cache_data, styles, NULL, NULL);

        /* Also add to c->style_cache linked list for centralized cleanup */
        html_style_cache_add(c, n, styles);
    }

    free(out_styles);
    free_style_snapshot(snap_root);
}

__attribute__((weak)) bool wisp_dispatch_style(const char *script, void (*func)(void*), void *arg, float priority) {
    if (func) func(arg);
    return true;
}

static css_select_results *box_get_style(
	html_content *c, const css_computed_style *parent_style, const css_computed_style *root_style, dom_node *n)
{
	css_select_results *cached = NULL;
	dom_node_get_user_data(n, corestring_dom___ns_key_style_cache_data, (void *)&cached);
	if (cached != NULL) {
		return cached;
	}

	dom_string *s = NULL;
	css_stylesheet *inline_style = NULL;
	css_select_results *styles;
	nscss_select_ctx ctx = {0};

	/* Firstly, construct inline stylesheet, if any */
	if (nsoption_bool(author_level_css)) {
		dom_exception err;
		err = dom_element_get_attribute(n, corestring_dom_style, &s);
		if (err != DOM_NO_ERR) {
			return NULL;
		}
	}

	if (s != NULL) {
		inline_style = nscss_create_inline_style((const uint8_t *)dom_string_data(s), dom_string_byte_length(s),
			c->encoding, nsurl_access(c->base_url), c->quirks != DOM_DOCUMENT_QUIRKS_MODE_NONE);

		dom_string_unref(s);

		if (inline_style == NULL)
			return NULL;
	}

	/* Populate selection context */
	ctx.ctx = c->select_ctx;
	ctx.quirks = (c->quirks == DOM_DOCUMENT_QUIRKS_MODE_FULL);
	ctx.base_url = c->base_url;
	ctx.universal = c->universal;
	ctx.root_style = root_style;
	ctx.parent_style = parent_style;
	ctx.c = c;

	/* Select style for element */
	styles = nscss_get_style(&ctx, n, &c->media, &c->unit_len_ctx, inline_style);

	/* No longer need inline style */
	if (inline_style != NULL)
		css_stylesheet_destroy(inline_style);

	if (styles != NULL && c != NULL && c->select_ctx != NULL) {
		dom_node_set_user_data(n, corestring_dom___ns_key_style_cache_data, styles, NULL, NULL);
		html_style_cache_add(c, n, styles);
	}

	return styles;
}


/**
 * Create a box from a CSS content item.
 *
 * This handles all content types defined in CSS 2.1 and CSS Generated Content:
 * - STRING: text content
 * - URI: images or font icons
 * - COUNTER/COUNTERS: counter values
 * - ATTR: attribute values
 * - quotes: open/close quote characters
 *
 * \param item      Content item to create box from
 * \param style     Computed style for the pseudo-element
 * \param content   HTML content for memory allocation
 * \param node      DOM node for ATTR lookups (may be NULL)
 * \return          Box, or NULL on failure or unsupported type
 */
static struct box *create_content_box(
	const css_computed_content_item *item, const css_computed_style *style, struct box_construct_ctx *ctx, dom_node *node)
{
	struct box *box = NULL;

	switch (item->type) {
	case CSS_COMPUTED_CONTENT_STRING: {
		/* Text content - most common case */
		const char *text_data = lwc_string_data(item->data.string);
		size_t text_len = lwc_string_length(item->data.string);

		if (text_len == 0)
			return NULL;

		box = box_create(ctx->content, NULL, style, false, NULL, NULL, NULL, NULL, ctx->bctx);
		if (box == NULL)
			return NULL;

		box->type = BOX_TEXT;
		box->text = arena_strndup(ctx->bctx, text_data, text_len);
		if (box->text == NULL) {
			/* Can't free box here - relies on talloc cleanup */
			return NULL;
		}
		box->length = text_len;

		NSLOG(wisp, DEEPDEBUG, "create_content_box: STRING '%.*s'", (int)(text_len > 50 ? 50 : text_len), text_data);
		break;
	}

	case CSS_COMPUTED_CONTENT_URI: {
		/* URI content - fetch image and create object box.
		 * Similar pattern to list-style-image handling. */
		nsurl *url;
		nserror error;

		error = nsurl_create(lwc_string_data(item->data.uri), &url);
		if (error != NSERROR_OK) {
			NSLOG(wisp, WARNING, "create_content_box: URI nsurl_create failed");
			break;
		}

		/* Create box to hold the image object */
		box = box_create(ctx->content, NULL, style, false, NULL, NULL, NULL, NULL, ctx->bctx);
		if (box == NULL) {
			nsurl_unref(url);
			break;
		}

		/* Mark as replaced (image) and set type for inline context */
		box->type = BOX_INLINE;
		box->flags |= IS_REPLACED;

		/* Start async fetch - box->object will be set when done */
		if (html_fetch_object(ctx->content, url, box, CONTENT_IMAGE, false) == false) {
			NSLOG(wisp, WARNING, "create_content_box: URI html_fetch_object failed");
			nsurl_unref(url);
			/* Box allocation will be cleaned up by talloc */
			box = NULL;
			break;
		}

		nsurl_unref(url);
		NSLOG(wisp, DEEPDEBUG, "create_content_box: URI started fetch for %s", lwc_string_data(item->data.uri));
		break;
	}

	case CSS_COMPUTED_CONTENT_COUNTER: {
		lwc_string *name = item->data.counter.name;
		int32_t value = 0;

		/* Find counter in ancestor boxes */
		struct box *cbox = box_for_node(node);
		if (cbox == NULL && box != NULL) cbox = box;
		while (cbox != NULL) {
			bool found = false;
			for (size_t i = 0; i < cbox->n_counters; i++) {
				bool match = false;
				if (lwc_string_isequal(cbox->counters[i].name, name, &match) == lwc_error_ok && match) {
					value = cbox->counters[i].value;
					found = true;
					break;
				}
			}
			if (found) break;
			cbox = cbox->parent;
		}

		char buf[32];
		size_t len = 0;
		css_error err = css_computed_format_list_style(style, value, buf, sizeof(buf), &len);

		if (err == CSS_OK && len > 0 && len < sizeof(buf)) {
			box = box_create(ctx->content, NULL, style, false, NULL, NULL, NULL, NULL, ctx->bctx);
			if (box != NULL) {
				box->type = BOX_TEXT;
				box->text = talloc_strndup(ctx->bctx, buf, len);
				box->length = len;
			}
		} else {
			box = NULL;
		}
		break;
	}

	case CSS_COMPUTED_CONTENT_COUNTERS: {
		lwc_string *name = item->data.counters.name;
		lwc_string *sep = item->data.counters.sep;

		/* Collect all counter values up the tree */
		int32_t values[32];
		size_t n_values = 0;

		struct box *cbox = box_for_node(node);
		if (cbox == NULL && box != NULL) cbox = box;
		while (cbox != NULL && n_values < 32) {
			for (size_t i = 0; i < cbox->n_counters; i++) {
				bool match = false;
				if (lwc_string_isequal(cbox->counters[i].name, name, &match) == lwc_error_ok && match) {
					values[n_values++] = cbox->counters[i].value;
					break;
				}
			}
			cbox = cbox->parent;
		}

		if (n_values == 0) {
			box = NULL;
			break;
		}

		char buf[256];
		size_t len = 0;
		css_error err = CSS_OK;

		/* Format them in reverse order (top-down) */
		for (size_t i = 0; i < n_values; i++) {
			size_t v_idx = n_values - 1 - i;
			size_t seg_len = 0;
			err = css_computed_format_list_style(style, values[v_idx], buf + len, sizeof(buf) - len, &seg_len);
			if (err != CSS_OK) break;
			len += seg_len;

			if (i < n_values - 1 && sep != NULL) {
				const char *sep_str = lwc_string_data(sep);
				size_t sep_len = lwc_string_length(sep);
				if (len + sep_len < sizeof(buf)) {
					memcpy(buf + len, sep_str, sep_len);
					len += sep_len;
				}
			}
		}

		if (err == CSS_OK && len > 0 && len < sizeof(buf)) {
			box = box_create(ctx->content, NULL, style, false, NULL, NULL, NULL, NULL, ctx->bctx);
			if (box != NULL) {
				box->type = BOX_TEXT;
				box->text = talloc_strndup(ctx->bctx, buf, len);
				box->length = len;
			}
		} else {
			box = NULL;
		}
		break;
	}

	case CSS_COMPUTED_CONTENT_ATTR: {
		/* Attribute value - get from DOM node */
		if (node != NULL && item->data.attr != NULL) {
			dom_string *attr_value = NULL;
			dom_string *attr_name = NULL;
			dom_exception err;

			err = dom_string_create_interned(
				(const uint8_t *)lwc_string_data(item->data.attr), lwc_string_length(item->data.attr), &attr_name);

			if (err == DOM_NO_ERR && attr_name != NULL) {
				err = dom_element_get_attribute(node, attr_name, &attr_value);
				dom_string_unref(attr_name);

				if (err == DOM_NO_ERR && attr_value != NULL) {
					const char *text_data = dom_string_data(attr_value);
					size_t text_len = dom_string_length(attr_value);

					if (text_len > 0) {
						box = box_create(ctx->content,
							NULL, style, false, NULL, NULL, NULL, NULL, ctx->bctx);
						if (box != NULL) {
							box->type = BOX_TEXT;
							box->text = arena_strndup(ctx->bctx, text_data, text_len);
							box->length = text_len;
							NSLOG(wisp, DEEPDEBUG, "create_content_box: ATTR '%.*s'",
								(int)(text_len > 50 ? 50 : text_len), text_data);
						}
					}
					dom_string_unref(attr_value);
				}
			}
		}
		break;
	}

	case CSS_COMPUTED_CONTENT_OPEN_QUOTE:
	case CSS_COMPUTED_CONTENT_CLOSE_QUOTE:
	case CSS_COMPUTED_CONTENT_NO_OPEN_QUOTE:
	case CSS_COMPUTED_CONTENT_NO_CLOSE_QUOTE: {
		bool is_open = (item->type == CSS_COMPUTED_CONTENT_OPEN_QUOTE || item->type == CSS_COMPUTED_CONTENT_NO_OPEN_QUOTE);
		bool is_insert = (item->type == CSS_COMPUTED_CONTENT_OPEN_QUOTE || item->type == CSS_COMPUTED_CONTENT_CLOSE_QUOTE);

		/* Decrease level for close quotes before getting quote */
		if (!is_open && ctx->quote_nesting_level > 0) {
			ctx->quote_nesting_level--;
		}

		const char *quote = "";
		if (is_open) quote = "\"";
		else quote = "\"";

		if (is_insert) {
			size_t quote_len = strlen(quote);
			box = box_create(ctx->content, NULL, style, false, NULL, NULL, NULL, NULL, ctx->bctx);
			if (box != NULL) {
				box->type = BOX_TEXT;
				box->text = talloc_strndup(ctx->bctx, quote, quote_len);
				box->length = quote_len;

				/* Handle text transformation or encoding here if needed.
				 * Typically done in a generic way, but basic quote should be okay. */
			}
		} else {
			box = NULL;
		}

		/* Increase level for open quotes after getting quote */
		if (is_open) {
			ctx->quote_nesting_level++;
		}
		break;
	}

	default:
		NSLOG(wisp, WARNING, "create_content_box: unknown type %d", item->type);
		box = NULL;
		break;
	}

	return box;
}


/**
 * Ensure an inline container exists for inline-level content.
 *
 * This helper creates or reuses an inline container. It checks if the
 * containing block already has an inline container as its last child
 * (e.g., from a ::before pseudo-element) and reuses it if so.
 *
 * \param containing_block  Parent block to contain the inline container
 * \param inline_container_ptr  Pointer to inline container (may be updated)
 * \param bctx              Box context for memory allocation
 * \return true on success, false on memory allocation failure
 */
static bool box_ensure_inline_container(struct html_content *content, struct box *containing_block, struct box **inline_container_ptr, struct arena *bctx)
{
	if (*inline_container_ptr != NULL) {
		return true; /* Already have one */
	}

	/* Check if containing block's last child is an inline container */
	if (containing_block->last != NULL && containing_block->last->type == BOX_INLINE_CONTAINER) {
		*inline_container_ptr = containing_block->last;
		return true;
	}

	/* Create new inline container */
	struct box *ic = box_create(content, NULL, NULL, false, NULL, NULL, NULL, NULL, bctx);
	if (ic == NULL) {
		return false;
	}
	ic->type = BOX_INLINE_CONTAINER;
	box_add_child(containing_block, ic);
	*inline_container_ptr = ic;
	return true;
}


/**
 * Add box to parent with optional float wrapping.
 *
 * If the box has float:left or float:right (and is not a flex child),
 * wraps it in a BOX_FLOAT_LEFT/RIGHT box before adding to the inline container.
 * Otherwise, adds directly to the parent.
 *
 * \param box              Box to add
 * \param parent           Parent to add to (inline_container or containing_block)
 * \param bctx             Box context for memory allocation
 * \param is_flex_child    True if parent is flex/grid (floats don't apply)
 * \return true on success, false on memory allocation failure
 */
static bool box_add_with_float_wrap(struct html_content *content, struct box *box, struct box *parent, struct arena *bctx, bool is_flex_child)
{
	if (box->style == NULL) {
		box_add_child(parent, box);
		return true;
	}

	uint8_t float_val = css_computed_float(box->style);
	bool is_floated = !is_flex_child && (float_val == CSS_FLOAT_LEFT || float_val == CSS_FLOAT_RIGHT);

	if (is_floated) {
		struct box *flt = box_create(content, NULL, NULL, false, NULL, NULL, NULL, NULL, bctx);
		if (flt == NULL) {
			return false;
		}
		flt->type = (float_val == CSS_FLOAT_LEFT) ? BOX_FLOAT_LEFT : BOX_FLOAT_RIGHT;
		box_add_child(parent, flt);
		box_add_child(flt, box);
	} else {
		box_add_child(parent, box);
	}

	return true;
}


/**
 * Fetch background image for a box if specified in its style.
 *
 * \param box      Box to fetch background for (must have style)
 * \param content  HTML content for resource fetching
 * \return true on success, false on fetch failure
 */
static bool box_fetch_background(struct box *box, html_content *content)
{
	lwc_string *bgimage_uri;

	if (box->style == NULL) {
		return true;
	}

	if (css_computed_background_image(box->style, &bgimage_uri) == CSS_BACKGROUND_IMAGE_IMAGE && bgimage_uri != NULL &&
		nsoption_bool(background_images) == true) {
		nsurl *url;
		nserror error;

		error = nsurl_create(lwc_string_data(bgimage_uri), &url);
		if (error == NSERROR_OK) {
			if (html_fetch_object(content, url, box, image_types, true) == false) {
				NSLOG(wisp, WARNING, "box_fetch_background: Failed to fetch background image");
				nsurl_unref(url);
				return false;
			}
			nsurl_unref(url);
		}
	}

	return true;
}


/**
 * Check if a box type needs an inline container.
 *
 * \param box_type      The box type to check
 * \param is_floated    Whether the box has float:left or float:right
 * \return true if box needs inline container, false otherwise
 */
static inline bool box_needs_inline_container(box_type type, bool is_floated)
{
	return type == BOX_INLINE || type == BOX_BR || type == BOX_INLINE_BLOCK || type == BOX_INLINE_FLEX ||
		type == BOX_INLINE_GRID || is_floated;
}


/**
 * Construct the box required for a generated element.
 *
 * \param n        XML node of type XML_ELEMENT_NODE
 * \param content  Content of type CONTENT_HTML that is being processed
 * \param box      Box which may have generated content
 * \param style    Complete computed style for pseudo element, or NULL
 *
 * This function handles ::before and ::after pseudo-elements by:
 * 1. Creating a box for the pseudo-element itself
 * 2. Processing the 'content' property to create child text boxes
 */
static void box_construct_generate(dom_node *n, struct box_construct_ctx *ctx, struct box *box, const css_computed_style *style)
{
	struct box *gen = NULL;
	struct box *inline_container = NULL;
	enum css_display_e computed_display;
	const css_computed_content_item *c_item;
	uint8_t content_type;

	/* Generated content can be added to container box types that can have children.
	 * Block-level and inline-level containers that establish formatting contexts
	 * can have ::before/::after pseudo-elements per CSS spec.
	 *
	 * Note: BOX_INLINE is NOT supported here because inline boxes in this
	 * codebase have a different structure (text stored directly on box, not
	 * as children). Inline elements are handled separately in box_construct_element. */
	switch (box->type) {
	case BOX_BLOCK:
	case BOX_INLINE_BLOCK:
	case BOX_FLEX:
	case BOX_INLINE_FLEX:
	case BOX_GRID:
	case BOX_INLINE_GRID:
		/* These can have generated content children */
		break;
	default:
		/* Other box types (BOX_INLINE, TABLE_*, FLOAT_*, etc.) cannot directly
		 * have generated content in the current implementation */
		return;
	}

	/* To determine if an element has a pseudo element, we select
	 * for it and test to see if the returned style's content
	 * property is set to normal. */
	if (style == NULL)
		return;

	content_type = css_computed_content(style, &c_item);
	if (content_type == CSS_CONTENT_NORMAL || content_type == CSS_CONTENT_NONE)
		return;

	/* create box for this element */
	computed_display = ns_computed_display(style, box_is_root(n));

	gen = box_create(ctx->content, NULL, style, false, NULL, NULL, NULL, NULL, ctx->bctx);
	if (gen == NULL) {
		return;
	}

	/* set box type from computed display */
	gen->type = box_map[computed_display];

	/* Skip BOX_NONE - display:none pseudo-elements should not be added */
	if (gen->type == BOX_NONE) {
		return;
	}

	/* Fetch background image for pseudo-element */
	if (!box_fetch_background(gen, ctx->content)) {
		return;
	}

	/* Check if we need an inline container */
	uint8_t float_val = css_computed_float(style);
	bool is_floated = (float_val == CSS_FLOAT_LEFT || float_val == CSS_FLOAT_RIGHT);

	if (box_needs_inline_container(gen->type, is_floated)) {
		/* Ensure inline container exists */
		if (!box_ensure_inline_container(ctx->content, box, &inline_container, ctx->bctx)) {
			return;
		}
		/* Add with float wrapping if needed */
		if (!box_add_with_float_wrap(ctx->content, gen, inline_container, ctx->bctx, false)) {
			return;
		}
	} else {
		/* Block-level: add directly to parent */
		box_add_child(box, gen);
	}

	/* Now process the content property items */
	if (c_item != NULL) {
		while (c_item->type != CSS_COMPUTED_CONTENT_NONE) {
			struct box *content_box = create_content_box(c_item, style, ctx, n);
			if (content_box != NULL) {
				if (gen->type == BOX_INLINE) {
					/* For inline boxes, text goes directly on the box.
					 * Note: this is a simplification and may not handle all cases
					 * correctly if there are multiple content items. */
					if (content_box->type == BOX_TEXT) {
						gen->text = arena_strndup(ctx->bctx, content_box->text, content_box->length);
						gen->length = content_box->length;
					}
				} else {
					struct box *text_container = NULL;
					if (content_box->type == BOX_TEXT) {
						text_container = box_create(ctx->content, NULL, NULL, false, NULL, NULL, NULL, NULL, ctx->bctx);
						if (text_container != NULL) {
							text_container->type = BOX_INLINE_CONTAINER;
							box_add_child(gen, text_container);
							box_add_child(text_container, content_box);
						}
					} else {
						box_add_child(gen, content_box);
					}
				}
			}
			c_item++;
		}
	}
}


/**
 * Construct a list marker box
 *
 * \param box      Box to attach marker to
 * \param title    Current title attribute
 * \param ctx      Box construction context
 * \param parent   Current block-level container
 * \return true on success, false on memory exhaustion
 */
static bool box_construct_marker(struct box *box, const char *title, struct box_construct_ctx *ctx, struct box *parent)
{
	lwc_string *image_uri;
	struct box *marker;
	enum css_list_style_type_e list_style_type;

	marker = box_create(ctx->content, NULL, box->style, false, NULL, NULL, title, NULL, ctx->bctx);
	if (marker == false)
		return false;

	marker->type = BOX_BLOCK;

	list_style_type = css_computed_list_style_type(box->style);

	/* Set marker text based on list-style-type */
	switch (list_style_type) {
	case CSS_LIST_STYLE_TYPE_DISC:
		/* 2022 BULLET */
		marker->text = (char *)"\342\200\242";
		marker->length = 3;
		break;

	case CSS_LIST_STYLE_TYPE_CIRCLE:
		/* 25CB WHITE CIRCLE */
		marker->text = (char *)"\342\227\213";
		marker->length = 3;
		break;

	case CSS_LIST_STYLE_TYPE_SQUARE:
		/* 25AA BLACK SMALL SQUARE */
		marker->text = (char *)"\342\226\252";
		marker->length = 3;
		break;

	case CSS_LIST_STYLE_TYPE_DECIMAL:
	case CSS_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
	case CSS_LIST_STYLE_TYPE_LOWER_ROMAN:
	case CSS_LIST_STYLE_TYPE_UPPER_ROMAN:
	case CSS_LIST_STYLE_TYPE_LOWER_ALPHA:
	case CSS_LIST_STYLE_TYPE_UPPER_ALPHA:
		/* These are handled via counters in layout */
		marker->text = NULL;
		marker->length = 0;
		break;

	default:
		/* Numerical list counters get handled in layout. */
		/* Fall through. */
	case CSS_LIST_STYLE_TYPE_NONE:
		marker->text = NULL;
		marker->length = 0;
		break;
	}

	if (css_computed_list_style_image(box->style, &image_uri) == CSS_LIST_STYLE_IMAGE_URI && (image_uri != NULL) &&
		(nsoption_bool(foreground_images) == true)) {
		nsurl *url;
		nserror error;

		/* Fetch the marker image URI */
		error = nsurl_create(lwc_string_data(image_uri), &url);
		if (error != NSERROR_OK)
			return false;

		if (html_fetch_object(ctx->content, url, marker, image_types, false) == false) {
			nsurl_unref(url);
			return false;
		}
		nsurl_unref(url);
	}

	box->list_marker = marker;
	marker->parent = box;

	return true;
}

static inline bool box__style_is_float(const struct box *box)
{
	return css_computed_float(box->style) == CSS_FLOAT_LEFT || css_computed_float(box->style) == CSS_FLOAT_RIGHT;
}

static inline bool box__is_flex(const struct box *box)
{
	return box->type == BOX_FLEX || box->type == BOX_INLINE_FLEX;
}

static inline bool box__containing_block_is_flex(const struct box_construct_props *props)
{
	return props->containing_block != NULL && box__is_flex(props->containing_block);
}

/**
 * Construct the box tree for an XML element.
 *
 * \param ctx               Tree construction context
 * \param convert_children  Whether to convert children
 * \return  true on success, false on memory exhaustion
 */
static bool box_construct_element(struct box_construct_ctx *ctx, bool *convert_children)
{
	dom_string *title0, *s;
	lwc_string *id = NULL;
	enum css_display_e css_display;
	struct box *box = NULL, *old_box;
	css_select_results *styles = NULL;
	lwc_string *bgimage_uri;
	dom_exception err;
	struct box_construct_props props;
	const css_computed_style *root_style = NULL;

	assert(ctx->n != NULL);

	box_extract_properties(ctx->n, &props);

	if (props.containing_block != NULL) {
		/* In case the containing block is a pre block, we clear
		 * the PRE_STRIP flag since it is not used if we follow
		 * the pre with a tag */
		props.containing_block->flags &= ~PRE_STRIP;
	}

	if (props.node_is_root == false) {
		root_style = ctx->root_box->style;
	}

	if (node_is_independent_subtree_root(ctx->n)) {
		css_select_results *cached = NULL;
		dom_node_get_user_data(ctx->n, corestring_dom___ns_key_style_cache_data, (void *)&cached);
		if (cached == NULL) {
			html_parallel_style_selection(ctx->content, ctx->n);
		}
	}

	styles = box_get_style(ctx->content, props.parent_style, root_style, ctx->n);
	if (styles == NULL)
		return false;

	/* Extract title attribute, if present */
	err = dom_element_get_attribute(ctx->n, corestring_dom_title, &title0);
	if (err != DOM_NO_ERR)
		return false;

	if (title0 != NULL) {
		char *t = squash_whitespace(dom_string_data(title0));

		dom_string_unref(title0);

		if (t == NULL)
			return false;

		props.title = arena_strdup(ctx->bctx, t);

		free(t);

		if (props.title == NULL)
			return false;
	}

	/* Extract id attribute, if present */
	err = dom_element_get_attribute(ctx->n, corestring_dom_id, &s);
	if (err != DOM_NO_ERR)
		return false;

	if (s != NULL) {
		err = dom_string_intern(s, &id);
		if (err != DOM_NO_ERR)
			id = NULL;

		dom_string_unref(s);
	}

	box = box_create(ctx->content,
		styles, styles->styles[CSS_PSEUDO_ELEMENT_NONE], false, props.href, props.target, props.title, id, ctx->bctx);
	if (box == NULL)
		return false;

	/* If this is the root box, add it to the context */
	if (props.node_is_root)
		ctx->root_box = box;

	/* Deal with colspan/rowspan */
	err = dom_element_get_attribute(ctx->n, corestring_dom_colspan, &s);
	if (err != DOM_NO_ERR) {
		NSLOG(wisp, WARNING, "Failed to get colspan attribute");
		goto error;
	}

	if (s != NULL) {
		const char *val = dom_string_data(s);

		/* Convert to a number, clamping to [1,1000] according to 4.9.11
		 */
		if ('0' <= val[0] && val[0] <= '9')
			box->columns = clamp(strtol(val, NULL, 10), 1, 1000);

		dom_string_unref(s);
	}

	err = dom_element_get_attribute(ctx->n, corestring_dom_rowspan, &s);
	if (err != DOM_NO_ERR) {
		NSLOG(wisp, WARNING, "Failed to get rowspan attribute");
		goto error;
	}

	if (s != NULL) {
		const char *val = dom_string_data(s);

		/* Convert to a number, clamping to [0,65534] according
		 * to 4.9.11 */
		if ('0' <= val[0] && val[0] <= '9')
			box->rows = clamp(strtol(val, NULL, 10), 0, 65534);

		dom_string_unref(s);
	}

	css_display = ns_computed_display_static(box->style);

	/* Set box type from computed display */
	if ((css_computed_position(box->style) == CSS_POSITION_ABSOLUTE ||
			css_computed_position(box->style) == CSS_POSITION_FIXED) &&
		(css_display == CSS_DISPLAY_INLINE || css_display == CSS_DISPLAY_INLINE_BLOCK ||
			css_display == CSS_DISPLAY_INLINE_TABLE || css_display == CSS_DISPLAY_INLINE_FLEX)) {
		/* Special case for absolute positioning: make absolute inlines
		 * into inline block so that the boxes are constructed in an
		 * inline container as if they were not absolutely positioned.
		 * Layout expects and handles this. */
		box->type = box_map[CSS_DISPLAY_INLINE_BLOCK];
	} else if (props.node_is_root) {
		/* Special case for root element: force it to BLOCK, or the
		 * rest of the layout will break. */
		box->type = BOX_BLOCK;
	} else {
		/* Normal mapping */
		box->type = box_map[ns_computed_display(box->style, props.node_is_root)];

		NSLOG(wisp, INFO, "box_construct: display %d map_type %d mapped from %d",
			ns_computed_display(box->style, props.node_is_root), box->type,
			ns_computed_display(box->style, props.node_is_root));

		if (props.containing_block->type == BOX_FLEX || props.containing_block->type == BOX_INLINE_FLEX ||
			props.containing_block->type == BOX_GRID || props.containing_block->type == BOX_INLINE_GRID) {
			/* Blockification per CSS Flexbox spec §4, CSS Grid spec, and CSS Display 3 §2.7:
			 * In-flow children of flex/grid containers are blockified.
			 * This means display:inline becomes display:block, etc.
			 * Layout-internal boxes (table-cell, table-row, etc.) also become block.
			 * This must happen BEFORE anonymous box creation. */
			switch (box->type) {
			case BOX_INLINE_FLEX:
				box->type = BOX_FLEX;
				break;
			case BOX_INLINE_GRID:
				box->type = BOX_GRID;
				break;
			case BOX_INLINE_BLOCK:
			case BOX_INLINE:
			case BOX_TABLE_CELL:
			case BOX_TABLE_ROW:
			case BOX_TABLE_ROW_GROUP:
				/* Layout-internal boxes blockified to block per CSS Display 3 §2.7 */
				box->type = BOX_BLOCK;
				break;
			default:
				break;
			}
		}
	}

	if (convert_special_elements(ctx->n, ctx->content, box, convert_children) == false) {
		NSLOG(wisp, WARNING, "Failed to convert special elements");
		goto error;
	}

	if (box->type != BOX_NONE && ns_computed_display(box->style, props.node_is_root) != CSS_DISPLAY_NONE) {
		const css_computed_counter *reset = NULL, *inc = NULL, *set = NULL;
		uint32_t total_counters = 0;
		if (css_computed_counter_reset(box->style, &reset) == CSS_COUNTER_RESET_NAMED && reset != NULL) {
			for (size_t i = 0; reset[i].name != NULL; i++) total_counters++;
		}
		if (css_computed_counter_set(box->style, &set) == CSS_COUNTER_SET_NAMED && set != NULL) {
			for (size_t i = 0; set[i].name != NULL; i++) total_counters++;
		}
		if (css_computed_counter_increment(box->style, &inc) == CSS_COUNTER_INCREMENT_NAMED && inc != NULL) {
			for (size_t i = 0; inc[i].name != NULL; i++) total_counters++;
		}
		if (total_counters > 0) {
			box->counters = talloc_zero_array(ctx->bctx, struct css_computed_counter, total_counters);
			if (box->counters != NULL) {
				uint32_t idx = 0;
				if (reset != NULL) {
					for (size_t i = 0; reset[i].name != NULL; i++) {
						box->counters[idx].name = lwc_string_ref(reset[i].name);
						box->counters[idx].value = reset[i].value; idx++;
					}
				}
				if (set != NULL) {
					for (size_t i = 0; set[i].name != NULL; i++) {
						bool found = false;
						/* Check if it was just reset/set on this box first (innermost scope) */
						for (int32_t j = (int32_t)idx - 1; j >= 0; j--) {
							bool match = false;
							if (lwc_string_isequal(box->counters[j].name, set[i].name, &match) == lwc_error_ok && match) {
								box->counters[j].value = set[i].value; found = true; break;
							}
						}
						/* Search for existing counter of same name in ancestors (innermost first) */
						struct box *cbox = box->parent;
						while (cbox != NULL && !found) {
							for (int32_t j = (int32_t)cbox->n_counters - 1; j >= 0; j--) {
								bool match = false;
								if (lwc_string_isequal(cbox->counters[j].name, set[i].name, &match) == lwc_error_ok && match) {
									cbox->counters[j].value = set[i].value; found = true; break;
								}
							}
							cbox = cbox->parent;
						}
						/* If still not found, instantiate it on this box */
						if (!found) {
							box->counters[idx].name = lwc_string_ref(set[i].name);
							box->counters[idx].value = set[i].value; idx++;
						}
					}
				}
				if (inc != NULL) {
					for (size_t i = 0; inc[i].name != NULL; i++) {
						bool found = false;
						/* Check if it was just reset/set on this box first (innermost scope) */
						for (int32_t j = (int32_t)idx - 1; j >= 0; j--) {
							bool match = false;
							if (lwc_string_isequal(box->counters[j].name, inc[i].name, &match) == lwc_error_ok && match) {
								box->counters[j].value += inc[i].value; found = true; break;
							}
						}
						/* Search for existing counter of same name in ancestors (innermost first) */
						struct box *cbox = box->parent;
						while (cbox != NULL && !found) {
							for (int32_t j = (int32_t)cbox->n_counters - 1; j >= 0; j--) {
								bool match = false;
								if (lwc_string_isequal(cbox->counters[j].name, inc[i].name, &match) == lwc_error_ok && match) {
									cbox->counters[j].value += inc[i].value; found = true; break;
								}
							}
							cbox = cbox->parent;
						}
						/* If still not found, instantiate it on this box */
						if (!found) {
							box->counters[idx].name = lwc_string_ref(inc[i].name);
							box->counters[idx].value = inc[i].value; idx++;
						}
					}
				}
				box->n_counters = idx;
			}
		}
	}
	/* Handle the :before pseudo element */
	if (!(box->flags & IS_REPLACED)) {
		box_construct_generate(ctx->n, ctx, box, box->styles->styles[CSS_PSEUDO_ELEMENT_BEFORE]);
	}


	if (box->type == BOX_NONE ||
		(ns_computed_display(box->style, props.node_is_root) == CSS_DISPLAY_NONE && props.node_is_root == false)) {

		bool is_contents = (ns_computed_display(box->style, props.node_is_root) == CSS_DISPLAY_CONTENTS);

		if (!is_contents) {
			if (ctx->content == NULL || ctx->content->select_ctx == NULL) {
				css_select_results_destroy(styles);
			}
			box->styles = NULL;
			box->style = NULL;
		}

		/* Free associated gadget, if any. This handles both formless controls
		 * and controls in a form's list. form_free_control sets box->gadget
		 * to NULL via control->box->gadget = NULL. */
		if (box->gadget != NULL) {
			form_free_control(box->gadget);
			box->gadget = NULL;
		}

		/* Can't do this, because the lifetimes of boxes and gadgets
		 * are inextricably linked. Fortunately, talloc will save us
		 * (for now) */
		/* box_free_box(box); */

		if (!is_contents) {
			*convert_children = false;
			return true;
		} else {
			/* For display: contents, attach the node mapping so the tree
			 * traversal algorithms can resolve parent mappings, but immediately
			 * return to skip tree insertion and layout generation.
			 */
			err = dom_node_set_user_data(ctx->n, corestring_dom___ns_key_box_node_data, box, NULL, (void *)&old_box);
			if (err != DOM_NO_ERR)
				return false;

			box->node = dom_node_ref(ctx->n);

			if (box->styles != NULL) {
				html_style_cache_add(ctx->content, ctx->n, box->styles);
				box->styles = NULL; /* Transfer ownership to cache so they are freed */
			}

			/* Children conversion continues natively and finds the original layout-generating ancestors. */
			return true;
		}
	}

	/* Attach DOM node to box */
	err = dom_node_set_user_data(ctx->n, corestring_dom___ns_key_box_node_data, box, NULL, (void *)&old_box);
	if (err != DOM_NO_ERR)
		return false;

	/* Attach box to DOM node */
	box->node = dom_node_ref(ctx->n);

	if (props.inline_container == NULL &&
		(box->type == BOX_INLINE || box->type == BOX_BR || box->type == BOX_INLINE_BLOCK ||
			box->type == BOX_INLINE_FLEX || box->type == BOX_INLINE_GRID ||
			(box__style_is_float(box) && !box__containing_block_is_flex(&props))) &&
		props.node_is_root == false) {
		/* Found an inline child of a block without a current container
		 * (i.e. this box is the first child of its parent, or was
		 * preceded by block-level siblings) */
		assert(props.containing_block != NULL && "Box must have containing block.");

		/* Use helper to ensure inline container exists (may reuse from ::before) */
		if (!box_ensure_inline_container(ctx->content, props.containing_block, &props.inline_container, ctx->bctx)) {
			NSLOG(wisp, WARNING, "Failed to create inline container box");
			goto error;
		}
	}

	/* Kick off fetch for any background image */
	if (!box_fetch_background(box, ctx->content)) {
		goto error;
	}

	if (*convert_children)
		box->flags |= CONVERT_CHILDREN;

	if (box->type == BOX_INLINE || box->type == BOX_BR || box->type == BOX_INLINE_FLEX ||
		box->type == BOX_INLINE_BLOCK || box->type == BOX_INLINE_GRID) {
		/* Inline container must exist, as we'll have
		 * created it above if it didn't */
		assert(props.inline_container != NULL);

		box_add_child(props.inline_container, box);
	} else {
		if (ns_computed_display(box->style, props.node_is_root) == CSS_DISPLAY_LIST_ITEM) {
			/* List item: compute marker */
			if (box_construct_marker(box, props.title, ctx, props.containing_block) == false) {
				NSLOG(wisp, WARNING, "Failed to construct list marker");
				goto error;
			}
		}

		if (props.node_is_root == false && box__containing_block_is_flex(&props) == false &&
			(css_computed_float(box->style) == CSS_FLOAT_LEFT || css_computed_float(box->style) == CSS_FLOAT_RIGHT)) {
			/* Float: insert a float between the parent and box. */
			struct box *flt = box_create(ctx->content, NULL, NULL, false, props.href, props.target, props.title, NULL, ctx->bctx);
			if (flt == NULL) {
				NSLOG(wisp, WARNING, "Failed to create float box");
				goto error;
			}

			if (css_computed_float(box->style) == CSS_FLOAT_LEFT)
				flt->type = BOX_FLOAT_LEFT;
			else
				flt->type = BOX_FLOAT_RIGHT;

			box_add_child(props.inline_container, flt);
			box_add_child(flt, box);
		} else {
			/* Non-floated block-level box: add to containing block
			 * if there is one. If we're the root box, then there
			 * won't be. */
			if (props.containing_block != NULL)
				box_add_child(props.containing_block, box);
		}
	}

	return true;

error:
	if (box != NULL) {
		if (ctx->root_box == box)
			ctx->root_box = NULL;
		box_free(box);
	}
	return false;
}


/**
 * Find the first text box descendant of a box.
 */
static struct box *find_first_text_box(struct box *b)
{
	if (b == NULL) return NULL;
	if (b->type == BOX_TEXT && b->text != NULL && b->length > 0) return b;

	for (struct box *c = b->children; c != NULL; c = c->next) {
		struct box *res = find_first_text_box(c);
		if (res != NULL) return res;
	}
	return NULL;
}

/**
 * Handle ::first-letter pseudo-element for block boxes.
 */
static void box__handle_first_letter(struct box *block, struct box_construct_ctx *ctx)
{
	if (block->styles == NULL || block->styles->styles[CSS_PSEUDO_ELEMENT_FIRST_LETTER] == NULL) {
		return;
	}

	struct box *text_box = find_first_text_box(block);
	if (text_box == NULL) return;

	size_t split_pos = 0;
	const char *s = text_box->text;
	size_t len = text_box->length;

	/* Consume leading punctuation and first alphanumeric */
	while (split_pos < len) {
		size_t char_len = utf8_next((char *)s, len, split_pos) - split_pos;
		uint32_t c = utf8_to_ucs4(s + split_pos, char_len);

		split_pos += char_len;

		if (iswalpha(c) || iswdigit(c)) {
			break;
		}
	}

	if (split_pos == 0) return;

	/* Create wrapper inline box for the first letter */
	const css_computed_style *fl_style = block->styles->styles[CSS_PSEUDO_ELEMENT_FIRST_LETTER];
	struct box *fl_inline = box_create(ctx->content, NULL, fl_style, false, text_box->href, text_box->target, text_box->title, NULL, ctx->bctx);
	if (fl_inline == NULL) return;
	fl_inline->type = BOX_INLINE;

	/* Create new text box for the first letter */
	struct box *fl_text = box_create(ctx->content, NULL, fl_style, false, text_box->href, text_box->target, text_box->title, NULL, ctx->bctx);
	if (fl_text == NULL) return;
	fl_text->type = BOX_TEXT;
	fl_text->text = talloc_strndup(ctx->bctx, text_box->text, split_pos);
	fl_text->length = split_pos;

	box_add_child(fl_inline, fl_text);

	/* Modify original text box */
	if (split_pos < len) {
		size_t new_len = len - split_pos;
		char *new_text = talloc_strndup(ctx->bctx, text_box->text + split_pos, new_len);
		if (new_text == NULL) return;
		talloc_free(text_box->text);
		text_box->text = new_text;
		text_box->length = new_len;

		/* Insert new first-letter inline box before original text box */
		fl_inline->parent = text_box->parent;
		fl_inline->prev = text_box->prev;
		fl_inline->next = text_box;
		if (text_box->prev != NULL) {
			text_box->prev->next = fl_inline;
		} else {
			text_box->parent->children = fl_inline;
		}
		text_box->prev = fl_inline;
	} else {
		/* Replace original text box entirely (was just one char) */
		fl_inline->parent = text_box->parent;
		fl_inline->prev = text_box->prev;
		fl_inline->next = text_box->next;

		if (text_box->prev != NULL) {
			text_box->prev->next = fl_inline;
		} else {
			text_box->parent->children = fl_inline;
		}

		if (text_box->next != NULL) {
			text_box->next->prev = fl_inline;
		} else {
			text_box->parent->last = fl_inline;
		}

		text_box->parent = NULL;
		box_free(text_box);
	}
}

/**
 * Complete construction of the box tree for an element.
 *
 * \param n        DOM node to construct for
 * \param content  Containing document
 *
 * This will be called after all children of an element have been processed
 */
static void box_construct_element_after(dom_node *n, struct box_construct_ctx *ctx)
{
	struct box_construct_props props;
	struct box *box = box_for_node(n);

	assert(box != NULL);

	/* Handle ::first-letter for block-level elements */
	if (box->type == BOX_BLOCK || box->type == BOX_INLINE_BLOCK || box->type == BOX_TABLE_CELL) {
		box__handle_first_letter(box, ctx);
	}

	box_extract_properties(n, &props);

	if (box->type == BOX_INLINE && !(box->flags & IS_REPLACED) && box->styles != NULL &&
		box->styles->styles[CSS_PSEUDO_ELEMENT_BEFORE] != NULL) {
		const css_computed_style *before_style = box->styles->styles[CSS_PSEUDO_ELEMENT_BEFORE];
		const css_computed_content_item *c_item;
		uint8_t content_type = css_computed_content(before_style, &c_item);

		if (content_type != CSS_CONTENT_NORMAL && content_type != CSS_CONTENT_NONE && c_item != NULL) {
			/* Create BOX_INLINE wrapper - this gets margins/padding from the style */
			struct box *pseudo_box = box_create(ctx->content,
				NULL, before_style, false, NULL, NULL, NULL, NULL, ctx->bctx);

			if (pseudo_box != NULL) {
				pseudo_box->type = BOX_INLINE;
				bool has_content = false;

				/* Create content boxes as children of the pseudo-element */
				while (c_item->type != CSS_COMPUTED_CONTENT_NONE) {
					struct box *content_box = create_content_box(c_item, before_style, ctx, n);
					if (content_box != NULL) {
						box_add_child(pseudo_box, content_box);
						has_content = true;
					}
					c_item++;
				}

				/* Only insert if we created content */
				if (has_content) {
					/* Insert as FIRST child of parent inline box.
					 * After flattening in normalization:
					 *   INLINE_CONTAINER
					 *     ├─ INLINE(parent)
					 *     ├─ INLINE(::before)  <- pseudo_box
					 *     ├─ content children  <- flattened
					 *     ├─ original content
					 *     └─ INLINE_END(parent)
					 */
					pseudo_box->parent = box;
					pseudo_box->next = box->children;
					pseudo_box->prev = NULL;
					if (box->children != NULL) {
						box->children->prev = pseudo_box;
					}
					box->children = pseudo_box;
					if (box->last == NULL) {
						box->last = pseudo_box;
					}

					NSLOG(wisp, DEEPDEBUG, "inline_before: created BOX_INLINE %p for ::before with %d children",
						(void *)pseudo_box, pseudo_box->children ? 1 : 0);
				}
			}
		}
	}

	if (box->type == BOX_INLINE && !(box->flags & IS_REPLACED) && box->styles != NULL &&
		box->styles->styles[CSS_PSEUDO_ELEMENT_AFTER] != NULL) {
		const css_computed_style *after_style = box->styles->styles[CSS_PSEUDO_ELEMENT_AFTER];
		const css_computed_content_item *c_item;
		uint8_t content_type = css_computed_content(after_style, &c_item);

		if (content_type != CSS_CONTENT_NORMAL && content_type != CSS_CONTENT_NONE && c_item != NULL) {
			/* Create BOX_INLINE wrapper - this gets margins/padding from the style */
			struct box *pseudo_box = box_create(ctx->content,
				NULL, after_style, false, NULL, NULL, NULL, NULL, ctx->bctx);

			if (pseudo_box != NULL) {
				pseudo_box->type = BOX_INLINE;
				bool has_content = false;

				/* Create content boxes as children of the pseudo-element */
				while (c_item->type != CSS_COMPUTED_CONTENT_NONE) {
					struct box *content_box = create_content_box(c_item, after_style, ctx, n);
					if (content_box != NULL) {
						box_add_child(pseudo_box, content_box);
						has_content = true;
					}
					c_item++;
				}

				/* Only insert if we created content */
				if (has_content) {
					pseudo_box->parent = box;
					pseudo_box->next = NULL;
					pseudo_box->prev = box->last;
					if (box->last != NULL) {
						box->last->next = pseudo_box;
					} else {
						box->children = pseudo_box;
					}
					box->last = pseudo_box;

					NSLOG(wisp, DEEPDEBUG, "inline_after: created BOX_INLINE %p for ::after with %d children",
						(void *)pseudo_box, pseudo_box->children ? 1 : 0);
				}
			}
		}
	}

	if (box->type == BOX_INLINE || box->type == BOX_BR) {
		/* Insert INLINE_END into containing block */
		struct box *inline_end;
		bool has_children;
		dom_exception err;

		err = dom_node_has_child_nodes(n, &has_children);
		if (err != DOM_NO_ERR)
			return;

		if (has_children == false || (box->flags & CONVERT_CHILDREN) == 0) {
			/* No children, or didn't want children converted */
			return;
		}

		if (props.inline_container == NULL) {
			/* Create inline container if we don't have one */
			props.inline_container = box_create(ctx->content, NULL, NULL, false, NULL, NULL, NULL, NULL, ctx->bctx);
			if (props.inline_container == NULL)
				return;

			props.inline_container->type = BOX_INLINE_CONTAINER;

			box_add_child(props.containing_block, props.inline_container);
		}

		inline_end = box_create(ctx->content, NULL, box->style, false, box->href, box->target, box->title,
			box->id == NULL ? NULL : lwc_string_ref(box->id), ctx->bctx);
		if (inline_end != NULL) {
			inline_end->type = BOX_INLINE_END;

			assert(props.inline_container != NULL);

			box_add_child(props.inline_container, inline_end);

			box->inline_end = inline_end;
			inline_end->inline_end = box;
		}
	} else if (!(box->flags & IS_REPLACED)) {
		/* Handle the :after pseudo element */
		box_construct_generate(n, ctx, box, box->styles->styles[CSS_PSEUDO_ELEMENT_AFTER]);
	}
}


/**
 * Find the next node in the DOM tree, completing element construction
 * where appropriate.
 *
 * \param n                 Current node
 * \param content           Containing content
 * \param convert_children  Whether to consider children of \a n
 * \return Next node to process, or NULL if complete
 *
 * \note \a n will be unreferenced
 */
static dom_node *next_node(dom_node *n, struct box_construct_ctx *ctx, bool convert_children)
{
	dom_node *next = NULL;
	bool has_children;
	dom_exception err;

	err = dom_node_has_child_nodes(n, &has_children);
	if (err != DOM_NO_ERR) {
		dom_node_unref(n);
		return NULL;
	}

	if (convert_children && has_children) {
		err = dom_node_get_first_child(n, &next);
		if (err != DOM_NO_ERR) {
			dom_node_unref(n);
			return NULL;
		}
		dom_node_unref(n);
	} else {
		err = dom_node_get_next_sibling(n, &next);
		if (err != DOM_NO_ERR) {
			dom_node_unref(n);
			return NULL;
		}

		if (next != NULL) {
			if (box_for_node(n) != NULL)
				box_construct_element_after(n, ctx);
			dom_node_unref(n);
		} else {
			if (box_for_node(n) != NULL)
				box_construct_element_after(n, ctx);

			while (box_is_root(n) == false) {
				dom_node *parent = NULL;
				dom_node *parent_next = NULL;

				err = dom_node_get_parent_node(n, &parent);
				if (err != DOM_NO_ERR) {
					dom_node_unref(n);
					return NULL;
				}

				assert(parent != NULL);

				err = dom_node_get_next_sibling(parent, &parent_next);
				if (err != DOM_NO_ERR) {
					dom_node_unref(parent);
					dom_node_unref(n);
					return NULL;
				}

				if (parent_next != NULL) {
					dom_node_unref(parent_next);
					dom_node_unref(parent);
					break;
				}

				dom_node_unref(n);
				n = parent;
				parent = NULL;

				if (box_for_node(n) != NULL) {
					box_construct_element_after(n, ctx);
				}
			}

			if (box_is_root(n) == false) {
				dom_node *parent = NULL;

				err = dom_node_get_parent_node(n, &parent);
				if (err != DOM_NO_ERR) {
					dom_node_unref(n);
					return NULL;
				}

				assert(parent != NULL);

				err = dom_node_get_next_sibling(parent, &next);
				if (err != DOM_NO_ERR) {
					dom_node_unref(parent);
					dom_node_unref(n);
					return NULL;
				}

				if (box_for_node(parent) != NULL) {
					box_construct_element_after(parent, ctx);
				}

				dom_node_unref(parent);
			}

			dom_node_unref(n);
		}
	}

	return next;
}


/**
 * Apply the CSS text-transform property to given text (Unicode-aware).
 *
 * \param  s    string to transform (UTF-8, will be modified in-place)
 * \param  len  length of s in bytes
 * \param  tt   transform type
 *
 * Note: This function handles multi-byte UTF-8 characters correctly.
 * For case transformations where the result has the same byte length
 * (which covers most Latin characters including Romanian diacritics),
 * the transformation is done in-place.
 */
static void box_text_transform(char *s, unsigned int len, enum css_text_transform_e tt)
{
	size_t off = 0;
	bool prev_was_space = true; /* For capitalize: treat start as after space */

	if (len == 0)
		return;

	while (off < len) {
		size_t next_off = utf8_next(s, len, off);
		size_t char_len = next_off - off;
		uint32_t c = utf8_to_ucs4(s + off, char_len);
		uint32_t transformed = c;

		switch (tt) {
		case CSS_TEXT_TRANSFORM_UPPERCASE:
			transformed = towupper(c);
			break;
		case CSS_TEXT_TRANSFORM_LOWERCASE:
			transformed = towlower(c);
			break;
		case CSS_TEXT_TRANSFORM_CAPITALIZE:
			if (prev_was_space) {
				transformed = towupper(c);
			}
			/* Track if current char is whitespace for next iteration */
			prev_was_space = (c == ' ' || c == '\t' || c == '\n' || c == '\r');
			break;
		default:
			break;
		}

		/* Only modify if transformation changed the character */
		if (transformed != c) {
			char new_char[6];
			size_t new_len = utf8_from_ucs4(transformed, new_char);

			/* In-place replacement only works if byte length matches.
			 * For most European languages (including Romanian), upper/lower
			 * case variants have the same UTF-8 byte length. */
			if (new_len == char_len) {
				memcpy(s + off, new_char, new_len);
			}
			/* If lengths differ, skip this character (rare case) */
		}

		off = next_off;
	}
}


/**
 * Construct the box tree for an XML text node.
 *
 * \param  ctx  Tree construction context
 * \return  true on success, false on memory exhaustion
 */
static bool box_construct_text(struct box_construct_ctx *ctx)
{
	struct box_construct_props props;
	struct box *box = NULL;
	dom_string *content;
	dom_exception err;

	assert(ctx->n != NULL);

	box_extract_properties(ctx->n, &props);

	assert(props.containing_block != NULL);

	err = dom_characterdata_get_data(ctx->n, &content);
	if (err != DOM_NO_ERR || content == NULL)
		return false;

	if (css_computed_white_space(props.parent_style) == CSS_WHITE_SPACE_NORMAL ||
		css_computed_white_space(props.parent_style) == CSS_WHITE_SPACE_NOWRAP) {
		char *text;

		text = squash_whitespace(dom_string_data(content));

		dom_string_unref(content);

		if (text == NULL)
			return false;

		/* if the text is just a space, combine it with the preceding
		 * text node, if any */
		if (text[0] == ' ' && text[1] == 0) {
			if (props.inline_container != NULL) {
				assert(props.inline_container->last != NULL);

				props.inline_container->last->space = UNKNOWN_WIDTH;
			}

			free(text);

			return true;
		}

		if (props.inline_container == NULL) {
			/* Child of a block without a current container
			 * (i.e. this box is the first child of its parent, or
			 * was preceded by block-level siblings) */

			/* DEBUG: Log when containing block doesn't have inline container */
			if (props.containing_block != NULL) {
				const char *tag = "";
				const char *cls = "";
				dom_string *name = NULL;
				dom_string *class_attr = NULL;
				if (props.containing_block->node != NULL) {
					if (dom_node_get_node_name(props.containing_block->node, &name) == DOM_NO_ERR && name != NULL) {
						tag = dom_string_data(name);
					}
					if (dom_element_get_attribute(props.containing_block->node, corestring_dom_class, &class_attr) ==
							DOM_NO_ERR &&
						class_attr != NULL) {
						cls = dom_string_data(class_attr);
					}
				}
				NSLOG(wisp, INFO, "TEXT_BOX: creating inline_container for text, parent: tag=%s class='%s' type=%d",
					tag, cls, props.containing_block->type);
				if (name)
					dom_string_unref(name);
				if (class_attr)
					dom_string_unref(class_attr);
			}

			props.inline_container = box_create(ctx->content, NULL, NULL, false, NULL, NULL, NULL, NULL, ctx->bctx);
			if (props.inline_container == NULL) {
				free(text);
				return false;
			}

			props.inline_container->type = BOX_INLINE_CONTAINER;

			box_add_child(props.containing_block, props.inline_container);
		}

		box = box_create(ctx->content, NULL, props.parent_style, false, props.href, props.target, props.title,
			NULL, ctx->bctx);
		if (box == NULL) {
			free(text);
			return false;
		}

		box->type = BOX_TEXT;

		box->text = arena_strdup(ctx->bctx, text);
		free(text);
		if (box->text == NULL)
			return false;

		box->length = strlen(box->text);

		/* strip ending space char off */
		if (box->length > 1 && box->text[box->length - 1] == ' ') {
			box->space = UNKNOWN_WIDTH;
			box->length--;
		}

		if (css_computed_text_transform(props.parent_style) != CSS_TEXT_TRANSFORM_NONE)
			box_text_transform(box->text, box->length, css_computed_text_transform(props.parent_style));

		box_add_child(props.inline_container, box);

		if (box->text[0] == ' ') {
			box->length--;

			memmove(box->text, &box->text[1], box->length);

			if (box->prev != NULL)
				box->prev->space = UNKNOWN_WIDTH;
		}
	} else {
		/* white-space: pre */
		char *text;
		size_t text_len = dom_string_byte_length(content);
		size_t i;
		char *current;
		enum css_white_space_e white_space = css_computed_white_space(props.parent_style);

		/* note: pre-wrap/pre-line are unimplemented */
		assert(white_space == CSS_WHITE_SPACE_PRE || white_space == CSS_WHITE_SPACE_PRE_LINE ||
			white_space == CSS_WHITE_SPACE_PRE_WRAP);

		text = malloc(text_len + 1);
		dom_string_unref(content);

		if (text == NULL)
			return false;

		memcpy(text, dom_string_data(content), text_len);
		text[text_len] = '\0';

		if (css_computed_text_transform(props.parent_style) != CSS_TEXT_TRANSFORM_NONE)
			box_text_transform(text, strlen(text), css_computed_text_transform(props.parent_style));

		current = text;

		/* swallow a single leading new line */
		if (props.containing_block->flags & PRE_STRIP) {
			switch (*current) {
			case '\n':
				current++;
				break;
			case '\r':
				current++;
				if (*current == '\n')
					current++;
				break;
			}
			props.containing_block->flags &= ~PRE_STRIP;
		}

		do {
			size_t len = strcspn(current, "\r\n\t");

			char old = current[len];

			current[len] = 0;

			if (props.inline_container == NULL) {
				/* Child of a block without a current container
				 * (i.e. this box is the first child of its
				 * parent, or was preceded by block-level
				 * siblings) */
				props.inline_container = box_create(ctx->content, NULL, NULL, false, NULL, NULL, NULL, NULL, ctx->bctx);
				if (props.inline_container == NULL) {
					free(text);
					return false;
				}

				props.inline_container->type = BOX_INLINE_CONTAINER;

				box_add_child(props.containing_block, props.inline_container);
			}

			if (len > 0) {
				box = box_create(ctx->content, NULL, props.parent_style, false, props.href, props.target,
					props.title, NULL, ctx->bctx);
				if (box == NULL) {
					free(text);
					return false;
				}

				box->type = BOX_TEXT;

				box->text = arena_strdup(ctx->bctx, current);
				if (box->text == NULL) {
					free(text);
					return false;
				}

				box->length = strlen(box->text);

				box_add_child(props.inline_container, box);
			}

			current[len] = old;
			current += len;

			if (current[0] == '\t') {
				/* Create a box containing expanded tab spaces */
				int32_t tab_size = 8;
				css_computed_tab_size(props.parent_style, &tab_size);

				box = box_create(ctx->content, NULL, props.parent_style, false, props.href, props.target,
					props.title, NULL, ctx->bctx);
				if (box == NULL) {
					free(text);
					return false;
				}

				box->type = BOX_TEXT;
				box->text = arena_alloc(ctx->bctx, tab_size + 1);
				if (box->text == NULL) {
					free(text);
					return false;
				}
				memset(box->text, ' ', tab_size);
				box->text[tab_size] = '\0';
				box->length = tab_size;

				box_add_child(props.inline_container, box);
				current++;
			} else if (current[0] != '\0') {
				/* Linebreak: create new inline container */
				props.inline_container = box_create(ctx->content, NULL, NULL, false, NULL, NULL, NULL, NULL, ctx->bctx);
				if (props.inline_container == NULL) {
					free(text);
					return false;
				}

				props.inline_container->type = BOX_INLINE_CONTAINER;

				box_add_child(props.containing_block, props.inline_container);

				if (current[0] == '\r' && current[1] == '\n')
					current += 2;
				else
					current++;
			}
		} while (*current);

		free(text);
	}

	return true;
}


/**
 * Convert an ELEMENT node to a box tree fragment,
 * then schedule conversion of the next ELEMENT node
 */
static void convert_xml_to_box(void *p)
{
	struct box_construct_ctx *ctx = p;
	dom_node *next;
	bool convert_children;
	uint32_t num_processed = 0;
	uint64_t start_time, now_time;

	nsu_getmonotonic_ms(&start_time);
	NSLOG(wisp, DEBUG, "PROFILER: START Box construction slice %p", ctx);

	do {
		convert_children = true;

		assert(ctx->n != NULL);

		if (box_construct_element(ctx, &convert_children) == false) {
			NSLOG(wisp, WARNING, "box_construct_element failed");
			ctx->cb(ctx->content, false);
			dom_node_unref(ctx->n);
			if (ctx->root_box != NULL)
				box_free(ctx->root_box);
			free(ctx);
			NSLOG(wisp, DEBUG, "PROFILER: STOP Box construction slice %p", ctx);
			return;
		}

		/* Find next element to process, converting text nodes as we go
		 */
		next = next_node(ctx->n, ctx, convert_children);
		while (next != NULL) {
			dom_node_type type;
			dom_exception err;

			err = dom_node_get_node_type(next, &type);
			if (err != DOM_NO_ERR) {
				NSLOG(wisp, WARNING, "dom_node_get_node_type failed");
				ctx->cb(ctx->content, false);
				dom_node_unref(next);
				if (ctx->root_box != NULL)
					box_free(ctx->root_box);
				free(ctx);
				NSLOG(wisp, DEBUG, "PROFILER: STOP Box construction slice %p", ctx);
				return;
			}

			if (type == DOM_ELEMENT_NODE)
				break;

			if (type == DOM_TEXT_NODE) {
				ctx->n = next;
				if (box_construct_text(ctx) == false) {
					NSLOG(wisp, WARNING, "box_construct_text failed");
					ctx->cb(ctx->content, false);
					dom_node_unref(ctx->n);
					if (ctx->root_box != NULL)
						box_free(ctx->root_box);
					free(ctx);
					NSLOG(wisp, DEBUG, "PROFILER: STOP Box construction slice %p", ctx);
					return;
				}
			}

			next = next_node(next, ctx, true);
		}

		// dom_node_unref(ctx->n);
		ctx->n = next;

		if (next == NULL) {
			/* Conversion complete */
			ctx->content->layout = ctx->root_box;
			if (ctx->content->layout != NULL)
				ctx->content->layout->parent = NULL;

			ctx->cb(ctx->content, true);

			assert(ctx->n == NULL);

			free(ctx);
			NSLOG(wisp, DEBUG, "PROFILER: STOP Box construction slice %p", ctx);
			return;
		}

		/* Check for yield every 64 nodes */
		if ((++num_processed & 0x3F) == 0) {
			nsu_getmonotonic_ms(&now_time);
			/* Yield if we've been running for more than 50ms */
			if (now_time - start_time > 50) {
				break;
			}
		}
	} while (true);

	NSLOG(wisp, DEBUG, "PROFILER: STOP Box construction slice %p", ctx);
	/* More work to do: schedule a continuation */
	guit->misc->schedule(0, (void *)convert_xml_to_box, ctx);
}


/* exported function documented in html/box_construct.h */
nserror dom_to_box(dom_node *n, html_content *c, box_construct_complete_cb cb, void **box_conversion_context)
{
	struct box_construct_ctx *ctx;

	assert(box_conversion_context != NULL);

	if (c->bctx == NULL) {
		c->bctx = arena_create(64 * 1024);
		if (c->bctx == NULL) {
			return NSERROR_NOMEM;
		}
	}

	ctx = malloc(sizeof(*ctx));
	if (ctx == NULL) {
		return NSERROR_NOMEM;
	}

	ctx->content = c;
	ctx->n = dom_node_ref(n);
	ctx->root_box = NULL;
	ctx->cb = cb;
	ctx->bctx = c->bctx;
	ctx->quote_nesting_level = 0;

	*box_conversion_context = ctx;

	return guit->misc->schedule(0, (void *)convert_xml_to_box, ctx);
}


/* exported function documented in html/box_construct.h */
nserror cancel_dom_to_box(void *box_conversion_context)
{
	struct box_construct_ctx *ctx = box_conversion_context;
	nserror err;

	err = guit->misc->schedule(-1, (void *)convert_xml_to_box, ctx);
	if (err != NSERROR_OK) {
		return err;
	}

	dom_node_unref(ctx->n);
	free(ctx);

	return NSERROR_OK;
}


/* exported function documented in html/box_construct.h */
struct box *box_for_node(dom_node *n)
{
	struct box *box = NULL;
	dom_exception err;

	err = dom_node_get_user_data(n, corestring_dom___ns_key_box_node_data, (void *)&box);
	if (err != DOM_NO_ERR)
		return NULL;

	return box;
}

/* exported function documented in html/box_construct.h */
bool box_extract_link(const html_content *content, const dom_string *dsrel, nsurl *base, nsurl **result)
{
	char *s, *s1, *apos0 = 0, *apos1 = 0, *quot0 = 0, *quot1 = 0;
	unsigned int i, j, end;
	nserror error;
	const char *rel;

	if (dsrel == NULL)
		return false;

	rel = dom_string_data(dsrel);
	if (rel == NULL)
		return false;

	s1 = s = malloc(3 * strlen(rel) + 1);
	if (!s)
		return false;

	/* copy to s, removing white space and control characters */
	for (i = 0; rel[i] && ascii_is_space(rel[i]); i++)
		;
	for (end = strlen(rel); (end != i) && ascii_is_space(rel[end - 1]); end--)
		;
	for (j = 0; i != end; i++) {
		if ((unsigned char)rel[i] < 0x20) {
			; /* skip control characters */
		} else if (rel[i] == ' ') {
			s[j++] = '%';
			s[j++] = '2';
			s[j++] = '0';
		} else {
			s[j++] = rel[i];
		}
	}
	s[j] = 0;

	if (content->enable_scripting == false) {
		/* extract first quoted string out of "javascript:" link */
		if (strncmp(s, "javascript:", 11) == 0) {
			apos0 = strchr(s, '\'');
			if (apos0)
				apos1 = strchr(apos0 + 1, '\'');
			quot0 = strchr(s, '"');
			if (quot0)
				quot1 = strchr(quot0 + 1, '"');
			if (apos0 && apos1 && (!quot0 || !quot1 || apos0 < quot0)) {
				*apos1 = 0;
				s1 = apos0 + 1;
			} else if (quot0 && quot1) {
				*quot1 = 0;
				s1 = quot0 + 1;
			}
		}
	}

	/* construct absolute URL */
	error = nsurl_join(base, s1, result);
	free(s);
	if (error != NSERROR_OK) {
		*result = NULL;
		return false;
	}

	return true;
}

void *wisp_get_default_quotes_ptr(void) {
    static lwc_string *default_quotes[5];
    static bool init_quotes = false;
    if (!init_quotes) {
        default_quotes[0] = corestring_lwc_open_double_quote;
        default_quotes[1] = corestring_lwc_close_double_quote;
        default_quotes[2] = corestring_lwc_open_single_quote;
        default_quotes[3] = corestring_lwc_close_single_quote;
        default_quotes[4] = NULL;
        init_quotes = true;
    }
    return default_quotes;
}
