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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_READER_THREADS 32

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    pthread_t writer_thread;
    bool has_writer;
    int write_count;

    pthread_t upgrade_thread;
    bool has_upgrade;
    int upgrade_count;

    int pending_writers;

    struct {
        pthread_t thread;
        bool active;
        int count;
    } readers[MAX_READER_THREADS];
} doc_rwlock_t;

static inline void doc_rwlock_init(doc_rwlock_t *lock) {
    pthread_mutex_init(&lock->mutex, NULL);
    pthread_cond_init(&lock->cond, NULL);
    lock->has_writer = false;
    lock->write_count = 0;
    lock->has_upgrade = false;
    lock->upgrade_count = 0;
    lock->pending_writers = 0;
    memset(lock->readers, 0, sizeof(lock->readers));
}

static inline void doc_rwlock_destroy(doc_rwlock_t *lock) {
    pthread_mutex_destroy(&lock->mutex);
    pthread_cond_destroy(&lock->cond);
}

static inline void doc_rwlock_rdlock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    /* 1. Symmetrical per-thread recursive read lock check */
    for (int i = 0; i < MAX_READER_THREADS; i++) {
        if (lock->readers[i].active && pthread_equal(lock->readers[i].thread, self)) {
            lock->readers[i].count++;
            pthread_mutex_unlock(&lock->mutex);
            return;
        }
    }

    /* 2. Unified wait loop to prevent writer races and handle full slot conditions */
    while (true) {
        /* Wait while there is an active writer (unless the writer is ourselves) */
        while (lock->has_writer && !pthread_equal(lock->writer_thread, self)) {
            pthread_cond_wait(&lock->cond, &lock->mutex);
        }

        /* Find a free reader slot and register */
        for (int i = 0; i < MAX_READER_THREADS; i++) {
            if (!lock->readers[i].active) {
                lock->readers[i].active = true;
                lock->readers[i].thread = self;
                lock->readers[i].count = 1;
                pthread_mutex_unlock(&lock->mutex);
                return;
            }
        }

        /* Array is full, wait on condition variable and retry the entire check */
        pthread_cond_wait(&lock->cond, &lock->mutex);
    }
}

static inline void doc_rwlock_rdunlock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    for (int i = 0; i < MAX_READER_THREADS; i++) {
        if (lock->readers[i].active && pthread_equal(lock->readers[i].thread, self)) {
            lock->readers[i].count--;
            if (lock->readers[i].count == 0) {
                lock->readers[i].active = false;
                pthread_cond_broadcast(&lock->cond);
            }
            pthread_mutex_unlock(&lock->mutex);
            return;
        }
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

    /* 2. Wait while there is an active writer, OR there are active readers,
     * EXCEPT if the only active reader is this thread itself. */
    while (true) {
        bool has_other_readers = false;
        for (int i = 0; i < MAX_READER_THREADS; i++) {
            if (lock->readers[i].active && !pthread_equal(lock->readers[i].thread, self)) {
                has_other_readers = true;
                break;
            }
        }
        bool has_other_writer = lock->has_writer && !pthread_equal(lock->writer_thread, self);
        bool has_other_upgrade = lock->has_upgrade && !pthread_equal(lock->upgrade_thread, self);

        if (!has_other_writer && !has_other_readers && !has_other_upgrade) {
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

static inline void doc_rwlock_uplock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    /* If this thread already holds the write lock, recursively acquire the lock as writer */
    if (lock->has_writer && pthread_equal(lock->writer_thread, self)) {
        lock->write_count++;
        pthread_mutex_unlock(&lock->mutex);
        return;
    }

    if (lock->has_upgrade && pthread_equal(lock->upgrade_thread, self)) {
        lock->upgrade_count++;
        for (int i = 0; i < MAX_READER_THREADS; i++) {
            if (lock->readers[i].active && pthread_equal(lock->readers[i].thread, self)) {
                lock->readers[i].count++;
                break;
            }
        }
        pthread_mutex_unlock(&lock->mutex);
        return;
    }

    while (true) {
        while (lock->has_writer || lock->has_upgrade) {
            pthread_cond_wait(&lock->cond, &lock->mutex);
        }

        for (int i = 0; i < MAX_READER_THREADS; i++) {
            if (!lock->readers[i].active) {
                lock->readers[i].active = true;
                lock->readers[i].thread = self;
                lock->readers[i].count = 1;

                lock->has_upgrade = true;
                lock->upgrade_thread = self;
                lock->upgrade_count = 1;

                pthread_mutex_unlock(&lock->mutex);
                return;
            }
        }
        pthread_cond_wait(&lock->cond, &lock->mutex);
    }
}

static inline void doc_rwlock_upunlock(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    if (lock->has_writer && pthread_equal(lock->writer_thread, self)) {
        lock->write_count--;
        if (lock->write_count == 0) {
            lock->has_writer = false;
            pthread_cond_broadcast(&lock->cond);
        }
        pthread_mutex_unlock(&lock->mutex);
        return;
    }

    if (lock->has_upgrade && pthread_equal(lock->upgrade_thread, self)) {
        lock->upgrade_count--;

        /* Symmetrically decrement reader slot count on every upunlock */
        for (int i = 0; i < MAX_READER_THREADS; i++) {
            if (lock->readers[i].active && pthread_equal(lock->readers[i].thread, self)) {
                lock->readers[i].count--;
                if (lock->readers[i].count == 0) {
                    lock->readers[i].active = false;
                }
                break;
            }
        }

        if (lock->upgrade_count == 0) {
            lock->has_upgrade = false;
            pthread_cond_broadcast(&lock->cond);
        }
    }

    pthread_mutex_unlock(&lock->mutex);
}

static inline void doc_rwlock_upgrade(doc_rwlock_t *lock) {
    pthread_mutex_lock(&lock->mutex);
    pthread_t self = pthread_self();

    /* If we already hold the write lock, upgrading is a no-op */
    if (lock->has_writer && pthread_equal(lock->writer_thread, self)) {
        pthread_mutex_unlock(&lock->mutex);
        return;
    }

    assert(lock->has_upgrade && pthread_equal(lock->upgrade_thread, self));

    while (true) {
        bool has_other_readers = false;
        for (int i = 0; i < MAX_READER_THREADS; i++) {
            if (lock->readers[i].active && !pthread_equal(lock->readers[i].thread, self)) {
                has_other_readers = true;
                break;
            }
        }
        if (!has_other_readers) {
            break;
        }
        pthread_cond_wait(&lock->cond, &lock->mutex);
    }

    lock->has_writer = true;
    lock->writer_thread = self;
    lock->write_count = 1;
    lock->has_upgrade = false;
    lock->upgrade_count = 0;

    /* Completely clear/deactivate our reader slot registered during uplock */
    for (int i = 0; i < MAX_READER_THREADS; i++) {
        if (lock->readers[i].active && pthread_equal(lock->readers[i].thread, self)) {
            lock->readers[i].count = 0;
            lock->readers[i].active = false;
            break;
        }
    }

    pthread_mutex_unlock(&lock->mutex);
}

struct hashset {
    uint64_t *keys;
    unsigned int capacity;
    unsigned int count;
};

static inline struct hashset *hashset_create(unsigned int initial_capacity)
{
    struct hashset *set = malloc(sizeof(struct hashset));
    if (!set) return NULL;
    set->capacity = initial_capacity > 0 ? initial_capacity : 16;
    set->count = 0;
    set->keys = calloc(set->capacity, sizeof(uint64_t));
    if (!set->keys) {
        free(set);
        return NULL;
    }
    return set;
}

static inline void hashset_destroy(struct hashset *set)
{
    if (!set) return;
    free(set->keys);
    free(set);
}

static inline void hashset_clear(struct hashset *set)
{
    if (!set) return;
    memset(set->keys, 0, set->capacity * sizeof(uint64_t));
    set->count = 0;
}

static inline uint32_t hashset_hash_uint64(uint64_t key)
{
    key ^= key >> 33;
    key *= 0xff51afd7ed558ccdULL;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53ULL;
    key ^= key >> 33;
    return (uint32_t)key;
}

static inline bool hashset_insert(struct hashset *set, uint64_t key)
{
    if (!set) return false;
    uint64_t stored_val = key + 1;

    /* Check load factor, resize if >= 70% */
    if (set->count * 10 >= set->capacity * 7) {
        unsigned int old_capacity = set->capacity;
        uint64_t *old_keys = set->keys;
        unsigned int new_capacity = old_capacity * 2;
        uint64_t *new_keys = calloc(new_capacity, sizeof(uint64_t));
        if (new_keys) {
            set->capacity = new_capacity;
            set->keys = new_keys;
            set->count = 0;
            for (unsigned int i = 0; i < old_capacity; i++) {
                if (old_keys[i] != 0) {
                    uint64_t old_key = old_keys[i] - 1;
                    hashset_insert(set, old_key);
                }
            }
            free(old_keys);
        }
    }

    uint32_t h = hashset_hash_uint64(key);
    unsigned int idx = h % set->capacity;
    while (set->keys[idx] != 0) {
        if (set->keys[idx] == stored_val) {
            return false; /* Already exists */
        }
        idx = (idx + 1) % set->capacity;
    }
    set->keys[idx] = stored_val;
    set->count++;
    return true;
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
    void *active_parse_tasks; /**< Linked list of active html_parse_task structures */
    /** Box tree, or NULL. */
    struct box *layout;

    /** Registry of active sticky elements in this document. */
    struct box *sticky_list;
    /** Document background colour. */
    colour background_colour;

    /** Timestamp when we first delayed box conversion for fonts (ms), 0 if not waiting */
    uint64_t font_wait_start_ms;

    /** Timestamp when data_complete was set to true (ms), 0 if not yet set */
    uint64_t data_complete_time_ms;

    /** Font callback table */
    const struct gui_layout_table *font_func;

    /** Number of entries in scripts */
    unsigned int scripts_count;
    /** Scripts */
    struct html_script *scripts;
    /** javascript thread in use */
    struct jsthread *jsthread;
    /** Whether DOMContentLoaded event has been dispatched */
    bool dom_content_loaded_fired;
    /** Whether load event has been dispatched */
    bool load_event_fired;

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

    /** Virtual Grid Bitmask for change tracking */
    struct hashset *dirty_grid;

    /** Style cache and its mutex for parallel styling */
    struct style_cache_node *style_cache;
    pthread_mutex_t style_cache_mutex;

} html_content;

struct style_cache_node {
    struct dom_node *node;
    struct css_select_results *styles;
    struct style_cache_node *next;
};

static inline void html_mark_grid_dirty(struct html_content *html, const struct rect *r)
{
    if (r->x0 <= -2000000000 || r->y0 <= -2000000000 ||
        r->x1 <= -2000000000 || r->y1 <= -2000000000 ||
        r->x1 < r->x0 || r->y1 < r->y0) {
        return; /* Skip completely invalid or unpositioned rectangles */
    }

    /* Clamp negative coordinates to 0, representing standard on-screen viewport space */
    int x0 = r->x0 < 0 ? 0 : r->x0;
    int x1 = r->x1 < 0 ? 0 : r->x1;
    int y0 = r->y0 < 0 ? 0 : r->y0;
    int y1 = r->y1 < 0 ? 0 : r->y1;

    /* Skip zero-width or zero-height rectangles */
    if (x1 <= x0 || y1 <= y0) {
        return;
    }

    /* Standard layout coordinate space maps to 256x256 tiles with exclusive end boundaries */
    int start_x = x0 / 256;
    int end_x = (x1 - 1) / 256;
    int start_y = y0 / 256;
    int end_y = (y1 - 1) / 256;

    for (int ty = start_y; ty <= end_y; ty++) {
        for (int tx = start_x; tx <= end_x; tx++) {
            uint64_t tile_key = ((uint64_t)tx << 32) | (uint32_t)ty;
            hashset_insert(html->dirty_grid, tile_key);
        }
    }
}

/**
 * Add a rectangle to the document's disjoint dirty list.
 */
static inline void html_add_dirty_rect(struct html_content *html, const struct rect *r)
{
    html_mark_grid_dirty(html, r);
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
