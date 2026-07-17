/*
 * Copyright 2004 James Bursa <bursa@users.sourceforge.net>
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
 * Private data for text/html content.
 */

#ifndef WISP_HTML_PRIVATE_H
#define WISP_HTML_PRIVATE_H

#include <dom/dom.h>
#include <libdom/bindings/hubbub/errors.h>
#include <libdom/bindings/hubbub/parser.h>

#include <wisp/content/content_protected.h>
#include <wisp/content/handlers/css/utils.h>
#include "wisp/types.h"


struct gui_layout_table;
struct scrollbar_msg_data;
struct content_redraw_data;
struct selection;
struct svgtiny_diagram;

/**
 * Pre-serialized inline SVG, stored in linked list during parsing.
 * The XML string is serialized during the parser callback to avoid
 * redundant DOM traversal during box construction.
 */
struct html_inline_svg {
    struct dom_node *node; /**< SVG DOM element (key for lookup) */
    char *svg_xml; /**< Pre-serialized XML string */
    size_t svg_xml_len; /**< Length of XML string */
    struct html_inline_svg *next; /**< Next in list */
};

typedef enum {
    HTML_DRAG_NONE, /** No drag */
    HTML_DRAG_SELECTION, /** Own; Text selection */
    HTML_DRAG_SCROLLBAR, /** Not own; drag in scrollbar widget */
    HTML_DRAG_TEXTAREA_SELECTION, /** Not own; drag in textarea widget */
    HTML_DRAG_TEXTAREA_SCROLLBAR, /** Not own; drag in textarea widget */
    HTML_DRAG_CONTENT_SELECTION, /** Not own; drag in child content */
    HTML_DRAG_CONTENT_SCROLL /** Not own; drag in child content */
} html_drag_type;

/**
 * For drags we don't own
 */
union html_drag_owner {
    bool no_owner;
    struct box *content;
    struct scrollbar *scrollbar;
    struct box *textarea;
};

typedef enum {
    HTML_SELECTION_NONE, /** No selection */
    HTML_SELECTION_TEXTAREA, /** Selection in one of our textareas */
    HTML_SELECTION_SELF, /** Selection in this html content */
    HTML_SELECTION_CONTENT /** Selection in child content */
} html_selection_type;

/**
 * For getting at selections in this content or things in this content
 */
union html_selection_owner {
    bool none;
    struct box *textarea;
    struct box *content;
};

typedef enum {
    HTML_FOCUS_SELF, /**< Focus is our own */
    HTML_FOCUS_CONTENT, /**< Focus belongs to child content */
    HTML_FOCUS_TEXTAREA /**< Focus belongs to textarea */
} html_focus_type;

/**
 * For directing input
 */
union html_focus_owner {
    bool self;
    struct box *textarea;
    struct box *content;
};


#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

typedef struct doc_reader_node {
    pthread_t thread;
    int count;
    struct doc_reader_node *next;
} doc_reader_node_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t writer_thread;
    bool has_writer;
    int write_count;
    int pending_writers;
    doc_reader_node_t *readers;
} doc_rwlock_t;

static inline void doc_rwlock_init(doc_rwlock_t *lock) {
    pthread_mutex_init(&lock->mutex, NULL);
    pthread_cond_init(&lock->cond, NULL);
    lock->has_writer = false;
    lock->write_count = 0;
    lock->pending_writers = 0;
    lock->readers = NULL;
}

static inline void doc_rwlock_destroy(doc_rwlock_t *lock) {
    pthread_mutex_destroy(&lock->mutex);
    pthread_cond_destroy(&lock->cond);

    doc_reader_node_t *curr = lock->readers;
    while (curr != NULL) {
        doc_reader_node_t *next = curr->next;
        free(curr);
        curr = next;
    }
    lock->readers = NULL;
}

static inline void doc_rwlock_rdlock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    /* 1. Check if we already hold a read lock (recursion) */
    doc_reader_node_t *curr = lock->readers;
    while (curr != NULL) {
        if (pthread_equal(curr->thread, self)) {
            curr->count++;
            pthread_mutex_unlock(&lock->mutex);
            return;
        }
        curr = curr->next;
    }

    /* 2. Wait until there is no active writer (unless the writer is ourselves) */
    while (lock->has_writer && !pthread_equal(lock->writer_thread, self)) {
        pthread_cond_wait(&lock->cond, &lock->mutex);
    }

    /* 3. Register as active reader */
    doc_reader_node_t *node = malloc(sizeof(doc_reader_node_t));
    if (node != NULL) {
        node->thread = self;
        node->count = 1;
        node->next = lock->readers;
        lock->readers = node;
    }

    pthread_mutex_unlock(&lock->mutex);
}

static inline void doc_rwlock_rdunlock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    doc_reader_node_t *curr = lock->readers;
    doc_reader_node_t *prev = NULL;

    while (curr != NULL) {
        if (pthread_equal(curr->thread, self)) {
            curr->count--;
            if (curr->count == 0) {
                /* Remove from list */
                if (prev == NULL) {
                    lock->readers = curr->next;
                } else {
                    prev->next = curr->next;
                }
                free(curr);
                pthread_cond_broadcast(&lock->cond);
            }
            pthread_mutex_unlock(&lock->mutex);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    pthread_mutex_unlock(&lock->mutex);
}

static inline void doc_rwlock_wrlock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    /* 1. If we are already the active writer, support recursive write lock. */
    if (lock->has_writer && pthread_equal(lock->writer_thread, self)) {
        lock->write_count++;
        pthread_mutex_unlock(&lock->mutex);
        return;
    }

    lock->pending_writers++;

    /* 2. Wait while there is an active writer, OR there are other active readers. */
    while (true) {
        bool has_other_readers = false;
        doc_reader_node_t *curr = lock->readers;
        while (curr != NULL) {
            if (!pthread_equal(curr->thread, self)) {
                has_other_readers = true;
                break;
            }
            curr = curr->next;
        }
        if (!lock->has_writer && !has_other_readers) {
            break;
        }
        pthread_cond_wait(&lock->cond, &lock->mutex);
    }

    lock->pending_writers--;
    lock->has_writer = true;
    lock->writer_thread = self;
    lock->write_count = 1;
    pthread_mutex_unlock(&lock->mutex);
}

static inline void doc_rwlock_wrunlock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    if (lock->has_writer && pthread_equal(lock->writer_thread, self)) {
        lock->write_count--;
        if (lock->write_count == 0) {
            lock->has_writer = false;
            pthread_cond_broadcast(&lock->cond);
        }
    }

    pthread_mutex_unlock(&lock->mutex);
}

struct csp;

/**
 * Data specific to CONTENT_HTML.
 */
typedef struct html_content {
    struct content base;

    doc_rwlock_t doc_mutex; /**< Protects dom_document mutation */
    struct csp *csp; /**< Content Security Policy */
    char *coop; /**< Cross-Origin-Opener-Policy header value */
    char *coep; /**< Cross-Origin-Embedder-Policy header value */
    dom_hubbub_parser *parser; /**< Parser object handle */
    bool parse_completed; /**< Whether the parse has been completed */
    bool conversion_begun; /**< Whether or not the conversion has begun */
    bool conversion_restart_pending; /**< Whether a restart is pending */
    unsigned int scripts_active; /**< Number of script fetches currently active */
    bool data_complete; /**< Whether HTML data download is complete */

    /** Document tree */
    dom_document *document;
    /** Quirkyness of document */
    dom_document_quirks_mode quirks;

    /** Encoding of source, NULL if unknown. */
    char *encoding;
    /** Source of encoding information. */
    dom_hubbub_encoding_source encoding_source;

    /** Base URL (may be a copy of content->url). */
    struct nsurl *base_url;
    /** Base target */
    char *base_target;

    /** Content has been aborted in the LOADING state */
    bool aborted;

    /** Whether a meta refresh has been handled */
    bool refresh;

    /** Whether a layout (reflow) is in progress */
    bool reflowing;

    /** Whether a reformat is pending (scheduled) */
    bool pending_reformat;

    /** Whether an initial layout has been done */
    bool had_initial_layout;

    /** Dimensions of the last successful layout */
    int last_layout_width;
    int last_layout_height;

    /** Whether scripts are enabled for this content */
    bool enable_scripting;

    /* Title element node */
    dom_node *title;

    /** A talloc context purely for the render box tree */
    struct arena *bctx;
    /** A context pointer for the box conversion, NULL if no conversion
     * is in progress.
     */
    void *box_conversion_context;
    /** Box tree, or NULL. */
    struct box *layout;

    /** Registry of active sticky elements in this document. */
    struct box *sticky_list;
    /** Document background colour. */
    colour background_colour;

    /** Timestamp when we first delayed box conversion for fonts (ms), 0 if not waiting */
    uint64_t font_wait_start_ms;

    /** Font callback table */
    const struct gui_layout_table *font_func;

    /** Number of entries in scripts */
    unsigned int scripts_count;
    /** Scripts */
    struct html_script *scripts;
    /** javascript thread in use */
    struct jsthread *jsthread;

    /** Number of entries in stylesheet_content. */
    unsigned int stylesheet_count;
    /** Stylesheets. Each may be NULL. */
    struct html_stylesheet *stylesheets;
    /**< Style selection context */
    css_select_ctx *select_ctx;
    /**< Style selection media specification */
    css_media media;
    /** CSS length conversion context for document. */
    css_unit_ctx unit_len_ctx;
    /**< Universal selector */
    lwc_string *universal;

    /** Number of entries in object_list. */
    unsigned int num_objects;
    /** List of objects. */
    struct content_html_object *object_list;
    /** Forms, in reverse order to document. */
    struct form *forms;
    /** Hash table of imagemaps. */
    struct imagemap **imagemaps;

    /** Browser window containing this document, or NULL if not open. */
    struct browser_window *bw;

    /** Frameset information */
    struct content_html_frames *frameset;

    /** Inline frame information */
    struct content_html_iframe *iframe;

    /** Content of type CONTENT_HTML containing this, or NULL if not an
     * object within a page. */
    struct html_content *page;

    /** Current drag type */
    html_drag_type drag_type;
    /** Widget capturing all mouse events */
    union html_drag_owner drag_owner;

    /** Current selection state */
    html_selection_type selection_type;
    /** Current selection owner */
    union html_selection_owner selection_owner;

    /** Current input focus target type */
    html_focus_type focus_type;
    /** Current input focus target */
    union html_focus_owner focus_owner;

    /** HTML content's own text selection object */
    struct selection *sel;

    /**
     * Open core-handled form SELECT menu, or NULL if none
     *  currently open.
     */
    struct form_control *visible_select_menu;

    /** SVG symbol registry for inline SVG <use> resolution, or NULL */
    struct svg_symbol_registry *svg_symbols;

    /** Linked list of pre-parsed inline SVG diagrams, or NULL */
    struct html_inline_svg *inline_svgs;

    /** Registry of boxes changed during this layout cycle. */
    struct box *dirty_list;

    /** List of disjoint dirty areas accumulated since last redraw */
    struct rect dirty_rects[16];
    /** Number of rectangles in dirty_rects */
    unsigned int dirty_rect_count;
    /** Whether we have fallen back to a single unioned bounding box */
    bool dirty_use_union;

} html_content;

/**
 * Add a rectangle to the document's disjoint dirty list.
 */
static inline void html_add_dirty_rect(struct html_content *html, const struct rect *r)
{
	if (html->dirty_use_union) {
		if (html->dirty_rect_count == 0) {
			html->dirty_rects[0] = *r;
			html->dirty_rect_count = 1;
		} else {
			ns_rect_union(&html->dirty_rects[0], r);
		}
	} else {
		bool merged = false;
		for (unsigned int i = 0; i < html->dirty_rect_count; i++) {
			struct rect *e = &html->dirty_rects[i];
			/* Check if new rect is entirely contained in existing one */
			if (r->x0 >= e->x0 && r->x1 <= e->x1 && r->y0 >= e->y0 && r->y1 <= e->y1) {
				return;
			}
			/* Check for overlap */
			if (!(r->x1 < e->x0 || r->x0 > e->x1 || r->y1 < e->y0 || r->y0 > e->y1)) {
				ns_rect_union(e, r);
				merged = true;
				break;
			}
		}
		if (!merged) {
			if (html->dirty_rect_count < 16) {
				html->dirty_rects[html->dirty_rect_count++] = *r;
			} else {
				/* Fallback to union if too many disjoint regions */
				struct rect union_rect = html->dirty_rects[0];
				for (unsigned int i = 1; i < html->dirty_rect_count; i++) {
					ns_rect_union(&union_rect, &html->dirty_rects[i]);
				}
				ns_rect_union(&union_rect, r);
				html->dirty_rects[0] = union_rect;
				html->dirty_rect_count = 1;
				html->dirty_use_union = true;
			}
		}
	}
}

/**
 * Set our drag status, and inform whatever owns the content
 *
 * \param html		HTML content
 * \param drag_type	Type of drag
 * \param drag_owner	What owns the drag
 * \param rect		Pointer movement bounds
 */
void html_set_drag_type(
    html_content *html, html_drag_type drag_type, union html_drag_owner drag_owner, const struct rect *rect);

/**
 * Set our selection status, and inform whatever owns the content
 *
 * \param html			HTML content
 * \param selection_type	Type of selection
 * \param selection_owner	What owns the selection
 * \param read_only		True iff selection is read only
 */
void html_set_selection(
    html_content *html, html_selection_type selection_type, union html_selection_owner selection_owner, bool read_only);

/**
 * Set our input focus, and inform whatever owns the content
 *
 * \param html			HTML content
 * \param focus_type		Type of input focus
 * \param focus_owner		What owns the focus
 * \param hide_caret		True iff caret to be hidden
 * \param x			Carret x-coord rel to owner
 * \param y			Carret y-coord rel to owner
 * \param height		Carret height
 * \param clip			Carret clip rect
 */
void html_set_focus(html_content *html, html_focus_type focus_type, union html_focus_owner focus_owner, bool hide_caret,
    int x, int y, int height, const struct rect *clip);

/**
 * Render padding and margin box outlines in html_redraw().
 */
extern bool html_redraw_debug;


/* in html/html.c */

/**
 * redraw a box
 *
 * \param htmlc HTML content
 * \param box The box to redraw.
 */
void html__redraw_a_box(html_content *htmlc, struct box *box);


/**
 * Complete conversion of an HTML document
 *
 * \param htmlc Content to convert
 */
void html_finish_conversion(html_content *htmlc);


/**
 * Test if an HTML content conversion can begin
 *
 * \param htmlc		html content to test
 * \return true iff the html content conversion can begin
 */
bool html_can_begin_conversion(html_content *htmlc);


/**
 * Begin conversion of an HTML document
 *
 * \param htmlc Content to convert
 */
bool html_begin_conversion(html_content *htmlc);

/**
 * Resume conversion of an html content
 *
 * \param p html content
 */
void html_resume_conversion_cb(void *p);


/**
 * execute some text as a script element
 */
bool html_exec(struct content *c, const char *src, size_t srclen);


/**
 * Attempt script execution for defer and async scripts
 *
 * execute scripts using algorithm found in:
 * http://www.whatwg.org/specs/web-apps/current-work/multipage/scripting-1.html#the-script-element
 *
 * \param htmlc html content.
 * \param allow_defer allow deferred execution, if not, only async scripts.
 * \return NSERROR_OK error code.
 */
nserror html_script_exec(html_content *htmlc, bool allow_defer);


/**
 * Free all script resources and references for a html content.
 *
 * \param htmlc html content.
 * \return NSERROR_OK or error code.
 */
nserror html_script_free(html_content *htmlc);


/**
 * Check if any of the scripts loaded were insecure
 */
bool html_saw_insecure_scripts(html_content *htmlc);


/**
 * Complete the HTML content state machine *iff* all scripts are finished
 */
nserror html_proceed_to_done(html_content *html);


/* in html/redraw.c */
bool html_redraw(
    struct content *c, struct content_redraw_data *data, const struct rect *clip, const struct redraw_context *ctx);


/* in html/redraw_border.c */
bool html_redraw_borders(struct box *box, int x_parent, int y_parent, int p_width, int p_height,
    const struct rect *clip, float scale, const struct redraw_context *ctx);


bool html_redraw_inline_borders(struct box *box, struct rect b, const struct rect *clip, float scale, bool first,
    bool last, const struct redraw_context *ctx);


/* in html/script.c */
dom_hubbub_error html_process_script(void *ctx, dom_node *node);

/* in html/html_svg.c */
dom_hubbub_error html_process_svg(void *ctx, dom_node *node);


/* in html/forms.c */
struct form *html_forms_get_forms(const char *docenc, dom_html_document *doc);
struct form_control *html_forms_get_control_for_node(struct form *forms, dom_node *node);


/* in html/css_fetcher.c */
/**
 * Register the fetcher for the pseudo x-ns-css scheme.
 *
 * \return NSERROR_OK on successful registration or error code on failure.
 */
nserror html_css_fetcher_register(void);
nserror html_css_fetcher_add_item(dom_string *data, struct nsurl *base_url, uint32_t *key);


/* Events */
/**
 * Construct an event and fire it at the DOM
 *
 */
bool fire_generic_dom_event(dom_string *type, dom_node *target, bool bubbles, bool cancelable);

/**
 * Construct a keyboard event and fire it at the DOM
 */
bool fire_dom_keyboard_event(dom_string *type, dom_node *target, bool bubbles, bool cancelable, uint32_t key);

/* Useful dom_string pointers */
struct dom_string;

extern struct dom_string *html_dom_string_map;
extern struct dom_string *html_dom_string_id;
extern struct dom_string *html_dom_string_name;
extern struct dom_string *html_dom_string_area;
extern struct dom_string *html_dom_string_a;
extern struct dom_string *html_dom_string_nohref;
extern struct dom_string *html_dom_string_href;
extern struct dom_string *html_dom_string_target;
extern struct dom_string *html_dom_string_shape;
extern struct dom_string *html_dom_string_default;
extern struct dom_string *html_dom_string_rect;
extern struct dom_string *html_dom_string_rectangle;
extern struct dom_string *html_dom_string_coords;
extern struct dom_string *html_dom_string_circle;
extern struct dom_string *html_dom_string_poly;
extern struct dom_string *html_dom_string_polygon;
extern struct dom_string *html_dom_string_text_javascript;
extern struct dom_string *html_dom_string_type;
extern struct dom_string *html_dom_string_src;

#endif
