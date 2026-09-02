/*
 * Copyright 2009 John-Mark Bell <jmb@netsurf-browser.org>
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

#include <assert.h>
#include <string.h>
#include <strings.h>

#include <wisp/plot_style.h>
#include <wisp/url_db.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/log.h>
#include <wisp/utils/nsoption.h>
#include <wisp/utils/nsurl.h>
#include "desktop/system_colour.h"

#include "content/handlers/css/hints.h"
#include "content/handlers/css/internal.h"
#include "content/handlers/css/select.h"
#include "content/handlers/html/box.h"

#include <wisp/content/handlers/html/private.h>

static css_error node_name(void *pw, void *node, css_qname *qname);
static css_error node_classes(void *pw, void *node, lwc_string ***classes, uint32_t *n_classes);
static css_error node_id(void *pw, void *node, lwc_string **id);
static css_error named_parent_node(void *pw, void *node, const css_qname *qname, void **parent);
static css_error named_sibling_node(void *pw, void *node, const css_qname *qname, void **sibling);
static css_error named_generic_sibling_node(void *pw, void *node, const css_qname *qname, void **sibling);
static css_error parent_node(void *pw, void *node, void **parent);
static css_error sibling_node(void *pw, void *node, void **sibling);
static css_error node_has_name(void *pw, void *node, const css_qname *qname, bool *match);
static css_error node_has_class(void *pw, void *node, lwc_string *name, bool *match);
static css_error node_has_id(void *pw, void *node, lwc_string *name, bool *match);
static css_error node_has_attribute(void *pw, void *node, const css_qname *qname, bool *match);
static css_error node_has_attribute_equal(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match);
static css_error
node_has_attribute_dashmatch(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match);
static css_error
node_has_attribute_includes(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match);
static css_error
node_has_attribute_prefix(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match);
static css_error
node_has_attribute_suffix(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match);
static css_error
node_has_attribute_substring(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match);
static css_error node_is_root(void *pw, void *node, bool *match);
static css_error node_count_siblings(void *pw, void *node, bool same_name, bool after, int32_t *count);
static css_error node_is_empty(void *pw, void *node, bool *match);
static css_error node_is_link(void *pw, void *node, bool *match);
static css_error node_is_hover(void *pw, void *node, bool *match);
static css_error node_is_focus(void *pw, void *node, bool *match);
static css_error node_is_enabled(void *pw, void *node, bool *match);
static css_error node_is_disabled(void *pw, void *node, bool *match);
static css_error node_is_lang(void *pw, void *node, lwc_string *lang, bool *match);
static css_error ua_default_for_property(void *pw, uint32_t property, css_hint *hint);
static css_error set_libcss_node_data(void *pw, void *node, void *libcss_node_data);
static css_error get_libcss_node_data(void *pw, void *node, void **libcss_node_data);

/**
 * Selection callback table for libcss
 */
static css_select_handler selection_handler = {
    CSS_SELECT_HANDLER_VERSION_1,

    node_name,
    node_classes,
    node_id,
    named_ancestor_node,
    named_parent_node,
    named_sibling_node,
    named_generic_sibling_node,
    parent_node,
    sibling_node,
    node_has_name,
    node_has_class,
    node_has_id,
    node_has_attribute,
    node_has_attribute_equal,
    node_has_attribute_dashmatch,
    node_has_attribute_includes,
    node_has_attribute_prefix,
    node_has_attribute_suffix,
    node_has_attribute_substring,
    node_is_root,
    node_count_siblings,
    node_is_empty,
    node_is_link,
    node_is_visited,
    node_is_hover,
    node_is_active,
    node_is_focus,
    node_is_enabled,
    node_is_disabled,
    node_is_checked,
    node_is_target,
    node_is_lang,
    node_presentational_hint,
    ua_default_for_property,
    set_libcss_node_data,
    get_libcss_node_data,
};

static css_error nscss_error_handler(void *pw, css_stylesheet *sheet, css_error error, const char *msg)
{
    fprintf(stderr, "DEBUG: select.c nscss_error_handler called! Msg: %s, Code: %d\n", msg ? msg : "null", error);
    NSLOG(wisp, ERROR, "LibCSS Error: %s (Code: %d)", msg, error);
    return CSS_OK;
}

/**
 * Create an inline style
 *
 * \param data          Source data
 * \param len           Length of data in bytes
 * \param charset       Charset of data, or NULL if unknown
 * \param url           Base URL of document containing data
 * \param allow_quirks  True to permit CSS parsing quirks
 * \return Pointer to stylesheet, or NULL on failure.
 */
css_stylesheet *
nscss_create_inline_style(const uint8_t *data, size_t len, const char *charset, const char *url, bool allow_quirks)
{
    css_stylesheet_params params;
    css_stylesheet *sheet;
    css_error error;

    params.params_version = CSS_STYLESHEET_PARAMS_VERSION_2;
    params.level = CSS_LEVEL_DEFAULT;
    params.charset = charset;
    params.url = url;
    params.title = NULL;
    params.allow_quirks = allow_quirks;
    params.inline_style = true;
    params.resolve = nscss_resolve_url;
    params.resolve_pw = NULL;
    params.import = NULL;
    params.import_pw = NULL;
    params.color = ns_system_colour;
    params.color_pw = NULL;
    params.font = NULL;
    params.font_pw = NULL;
    params.error = nscss_error_handler;
    params.error_pw = NULL;

    error = css_stylesheet_create(&params, &sheet);
    if (error != CSS_OK) {
        NSLOG(wisp, ERROR, "Failed creating sheet: %d", error);
        return NULL;
    }

    error = css_stylesheet_append_data(sheet, data, len);
    if (error != CSS_OK && error != CSS_NEEDDATA) {
        NSLOG(wisp, ERROR, "failed appending data: %d", error);
        css_stylesheet_destroy(sheet);
        return NULL;
    }

    error = css_stylesheet_data_done(sheet);
    if (error != CSS_OK) {
        NSLOG(wisp, ERROR, "failed completing parse: %d", error);
        css_stylesheet_destroy(sheet);
        return NULL;
    }

    return sheet;
}

/* Handler for libcss_node_data, stored as libdom node user data */
static void nscss_dom_user_data_handler(
    dom_node_operation operation, dom_string *key, void *data, struct dom_node *src, struct dom_node *dst)
{
    css_error error;

    if (dom_string_isequal(corestring_dom___ns_key_libcss_node_data, key) == false || data == NULL) {
        return;
    }

    switch (operation) {
    case DOM_NODE_CLONED:
        error = css_libcss_node_data_handler(&selection_handler, CSS_NODE_CLONED, NULL, src, dst, data);
        if (error != CSS_OK)
            NSLOG(wisp, INFO, "Failed to clone libcss_node_data.");
        break;

    case DOM_NODE_RENAMED:
        error = css_libcss_node_data_handler(&selection_handler, CSS_NODE_MODIFIED, NULL, src, NULL, data);
        if (error != CSS_OK)
            NSLOG(wisp, INFO, "Failed to update libcss_node_data.");
        break;

    case DOM_NODE_IMPORTED:
    case DOM_NODE_ADOPTED:
    case DOM_NODE_DELETED:
        error = css_libcss_node_data_handler(&selection_handler, CSS_NODE_DELETED, NULL, src, NULL, data);
        if (error != CSS_OK)
            NSLOG(wisp, INFO, "Failed to delete libcss_node_data.");
        break;

    default:
        NSLOG(wisp, INFO, "User data operation not handled.");
        assert(0);
    }
}

/**
 * Get style selection results for an element
 *
 * \param ctx             CSS selection context
 * \param n               Element to select for
 * \param media           Permitted media types
 * \param unit_unit_len_ctx    Unit length conversion context
 * \param inline_style    Inline style associated with element, or NULL
 * \return Pointer to selection results (containing computed styles),
 *         or NULL on failure
 */
css_select_results *nscss_get_style(nscss_select_ctx *ctx, dom_node *n, const css_media *media,
    const css_unit_ctx *unit_len_ctx, const css_stylesheet *inline_style)
{
    css_computed_style *composed;
    css_select_results *styles;
    int pseudo_element;
    css_error error;

    /* Select style for node */
    error = css_select_style(ctx->ctx, n, unit_len_ctx, media, inline_style, &selection_handler, ctx, &styles);

    if (error != CSS_OK || styles == NULL) {
        /* Failed selecting partial style -- bail out */
        return NULL;
    }

    if (styles->styles[CSS_PSEUDO_ELEMENT_NONE] == NULL) {
        css_select_results_destroy(styles);
        return NULL;
    }

    /* If there's a parent style, compose with partial to obtain
     * complete computed style for element */
    if (styles->styles[CSS_PSEUDO_ELEMENT_NONE] == NULL) {
        styles->styles[CSS_PSEUDO_ELEMENT_NONE] = nscss_get_blank_style(ctx, unit_len_ctx, ctx->parent_style);
        if (styles->styles[CSS_PSEUDO_ELEMENT_NONE] == NULL) {
            css_select_results_destroy(styles);
            return NULL;
        }
    } else if (ctx->parent_style != NULL) {
        /* Complete the computed style, by composing with the parent
         * element's style */
        error = css_computed_style_compose(
            ctx->parent_style, styles->styles[CSS_PSEUDO_ELEMENT_NONE], unit_len_ctx, &composed);
        if (error != CSS_OK) {
            css_select_results_destroy(styles);
            return NULL;
        }

        /* Replace select_results style with composed style */
        css_computed_style_destroy(styles->styles[CSS_PSEUDO_ELEMENT_NONE]);
        styles->styles[CSS_PSEUDO_ELEMENT_NONE] = composed;
    }

    for (pseudo_element = CSS_PSEUDO_ELEMENT_NONE + 1; pseudo_element < CSS_PSEUDO_ELEMENT_COUNT; pseudo_element++) {

        if (styles->styles[pseudo_element] == NULL)
            /* There were no rules concerning this pseudo element */
            continue;

        css_computed_style *base_style = styles->styles[CSS_PSEUDO_ELEMENT_NONE];
        css_computed_style *temp_base_style = NULL;
        if (base_style == NULL) {
            temp_base_style = nscss_get_blank_style(ctx, unit_len_ctx, ctx->parent_style);
            base_style = temp_base_style;
        }

        if (base_style != NULL) {
            /* Complete the pseudo element's computed style, by composing
             * with the base element's style */
            error = css_computed_style_compose(
                base_style, styles->styles[pseudo_element], unit_len_ctx, &composed);
            if (error != CSS_OK) {
                NSLOG(wisp, WARNING, "Failed composing pseudo-element style: %d", error);
                css_computed_style_destroy(styles->styles[pseudo_element]);
                styles->styles[pseudo_element] = NULL;
            } else {
                /* Replace select_results style with composed style */
                css_computed_style_destroy(styles->styles[pseudo_element]);
                styles->styles[pseudo_element] = composed;
            }
        } else {
            css_computed_style_destroy(styles->styles[pseudo_element]);
            styles->styles[pseudo_element] = NULL;
        }

        if (temp_base_style != NULL) {
            css_computed_style_destroy(temp_base_style);
        }
    }

    return styles;
}

/**
 * Get a blank style
 *
 * \param ctx           CSS selection context
 * \param unit_unit_len_ctx  Unit length conversion context
 * \param parent        Parent style to cascade inherited properties from
 * \return Pointer to blank style, or NULL on failure
 */
css_computed_style *
nscss_get_blank_style(nscss_select_ctx *ctx, const css_unit_ctx *unit_len_ctx, const css_computed_style *parent)
{
    css_computed_style *partial, *composed;
    css_error error;

    error = css_select_default_style(ctx->ctx, &selection_handler, ctx, &partial);
    if (error != CSS_OK) {
        return NULL;
    }

    /* Compose defaults with parent style to cascade inherited properties */
    error = css_computed_style_compose(parent, partial, unit_len_ctx, &composed);
    css_computed_style_destroy(partial);
    if (error != CSS_OK) {
        css_computed_style_destroy(composed);
        return NULL;
    }

    return composed;
}

/******************************************************************************
 * Style selection callbacks                                                  *
 ******************************************************************************/

/**
 * Callback to retrieve a node's name.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Pointer to location to receive node name
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 */
css_error node_name(void *pw, void *node, css_qname *qname)
{
    dom_node *n = node;
    dom_string *name;
    dom_exception err;

    err = dom_node_get_node_name(n, &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    qname->ns = NULL;

    err = dom_string_intern(name, &qname->name);
    if (err != DOM_NO_ERR) {
        dom_string_unref(name);
        return CSS_NOMEM;
    }

    dom_string_unref(name);

    return CSS_OK;
}

/**
 * Callback to retrieve a node's classes.
 *
 * \param pw         HTML document
 * \param node       DOM node
 * \param classes    Pointer to location to receive class name array
 * \param n_classes  Pointer to location to receive length of class name array
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \note The returned array will be destroyed by libcss. Therefore, it must
 *       be allocated using the same allocator as used by libcss during style
 *       selection.
 */
css_error node_classes(void *pw, void *node, lwc_string ***classes, uint32_t *n_classes)
{
    dom_node *n = node;
    dom_exception err;

    *classes = NULL;
    *n_classes = 0;

    err = dom_element_get_classes(n, classes, n_classes);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    return CSS_OK;
}

/**
 * Callback to retrieve a node's ID.
 *
 * \param pw    HTML document
 * \param node  DOM node
 * \param id    Pointer to location to receive id value
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 */
css_error node_id(void *pw, void *node, lwc_string **id)
{
    dom_node *n = node;
    dom_string *attr;
    dom_exception err;

    *id = NULL;

    err = dom_element_get_attribute(n, corestring_dom_id, &attr);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    if (attr != NULL) {
        err = dom_string_intern(attr, id);
        if (err != DOM_NO_ERR) {
            dom_string_unref(attr);
            return CSS_NOMEM;
        }
        dom_string_unref(attr);
    }

    return CSS_OK;
}

/**
 * Callback to find a named ancestor node.
 *
 * \param pw        HTML document
 * \param node      DOM node
 * \param qname     Node name to search for
 * \param ancestor  Pointer to location to receive ancestor
 * \return CSS_OK.
 *
 * \post \a ancestor will contain the result, or NULL if there is no match
 */
css_error named_ancestor_node(void *pw, void *node, const css_qname *qname, void **ancestor)
{
    dom_element_named_ancestor_node(node, qname->name, (struct dom_element **)ancestor);
    dom_node_unref(*ancestor);

    return CSS_OK;
}

/**
 * Callback to find a named parent node
 *
 * \param pw      HTML document
 * \param node    DOM node
 * \param qname   Node name to search for
 * \param parent  Pointer to location to receive parent
 * \return CSS_OK.
 *
 * \post \a parent will contain the result, or NULL if there is no match
 */
css_error named_parent_node(void *pw, void *node, const css_qname *qname, void **parent)
{
    dom_element_named_parent_node(node, qname->name, (struct dom_element **)parent);
    dom_node_unref(*parent);

    return CSS_OK;
}

/**
 * Callback to find a named sibling node.
 *
 * \param pw       HTML document
 * \param node     DOM node
 * \param qname    Node name to search for
 * \param sibling  Pointer to location to receive sibling
 * \return CSS_OK.
 *
 * \post \a sibling will contain the result, or NULL if there is no match
 */
css_error named_sibling_node(void *pw, void *node, const css_qname *qname, void **sibling)
{
    dom_node *n = node;
    dom_node *prev;
    dom_exception err;

    *sibling = NULL;

    /* Find sibling element */
    err = dom_node_get_previous_sibling(n, &n);
    if (err != DOM_NO_ERR)
        return CSS_OK;

    while (n != NULL) {
        dom_node_type type;

        err = dom_node_get_node_type(n, &type);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_OK;
        }

        if (type == DOM_ELEMENT_NODE) {
            dom_string *name;

            err = dom_node_get_node_name(n, &name);
            if (err != DOM_NO_ERR) {
                dom_node_unref(n);
                return CSS_OK;
            }

            if (dom_string_caseless_lwc_isequal(name, qname->name)) {
                dom_string_unref(name);
                *sibling = n;
                break;
            }

            dom_string_unref(name);
            dom_node_unref(n);
            break;
        }

        err = dom_node_get_previous_sibling(n, &prev);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_OK;
        }

        dom_node_unref(n);
        n = prev;
    }

    return CSS_OK;
}

/**
 * Callback to find a named generic sibling node.
 *
 * \param pw       HTML document
 * \param node     DOM node
 * \param qname    Node name to search for
 * \param sibling  Pointer to location to receive ancestor
 * \return CSS_OK.
 *
 * \post \a sibling will contain the result, or NULL if there is no match
 */
css_error named_generic_sibling_node(void *pw, void *node, const css_qname *qname, void **sibling)
{
    dom_node *n = node;
    dom_node *prev;
    dom_exception err;

    *sibling = NULL;

    err = dom_node_get_previous_sibling(n, &n);
    if (err != DOM_NO_ERR)
        return CSS_OK;

    while (n != NULL) {
        dom_node_type type;
        dom_string *name;

        err = dom_node_get_node_type(n, &type);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_OK;
        }

        if (type == DOM_ELEMENT_NODE) {
            err = dom_node_get_node_name(n, &name);
            if (err != DOM_NO_ERR) {
                dom_node_unref(n);
                return CSS_OK;
            }

            if (dom_string_caseless_lwc_isequal(name, qname->name)) {
                dom_string_unref(name);
                *sibling = n;
                break;
            }
            dom_string_unref(name);
        }

        err = dom_node_get_previous_sibling(n, &prev);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_OK;
        }

        dom_node_unref(n);
        n = prev;
    }

    return CSS_OK;
}

/**
 * Callback to retrieve the parent of a node.
 *
 * \param pw      HTML document
 * \param node    DOM node
 * \param parent  Pointer to location to receive parent
 * \return CSS_OK.
 *
 * \post \a parent will contain the result, or NULL if there is no match
 */
css_error parent_node(void *pw, void *node, void **parent)
{
    dom_element_parent_node(node, (struct dom_element **)parent);
    dom_node_unref(*parent);

    return CSS_OK;
}

/**
 * Callback to retrieve the preceding sibling of a node.
 *
 * \param pw       HTML document
 * \param node     DOM node
 * \param sibling  Pointer to location to receive sibling
 * \return CSS_OK.
 *
 * \post \a sibling will contain the result, or NULL if there is no match
 */
css_error sibling_node(void *pw, void *node, void **sibling)
{
    dom_node *n = node;
    dom_node *prev;
    dom_exception err;

    *sibling = NULL;

    /* Find sibling element */
    err = dom_node_get_previous_sibling(n, &n);
    if (err != DOM_NO_ERR)
        return CSS_OK;

    while (n != NULL) {
        dom_node_type type;

        err = dom_node_get_node_type(n, &type);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_OK;
        }

        if (type == DOM_ELEMENT_NODE)
            break;

        err = dom_node_get_previous_sibling(n, &prev);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_OK;
        }

        dom_node_unref(n);
        n = prev;
    }

    if (n != NULL) {
        *sibling = n;
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node has the given name.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_name(void *pw, void *node, const css_qname *qname, bool *match)
{
    nscss_select_ctx *ctx = pw;
    dom_node *n = node;

    if (lwc_string_isequal(qname->name, ctx->universal, match) == lwc_error_ok && *match == false) {
        dom_string *name;
        dom_exception err;

        err = dom_node_get_node_name(n, &name);
        if (err != DOM_NO_ERR)
            return CSS_OK;

        /* Element names are case insensitive in HTML */
        *match = dom_string_caseless_lwc_isequal(name, qname->name);

        dom_string_unref(name);
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node has the given class.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param name   Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_class(void *pw, void *node, lwc_string *name, bool *match)
{
    dom_node *n = node;
    dom_exception err;

    err = dom_element_has_class(n, name, match);

    assert(err == DOM_NO_ERR);

    return CSS_OK;
}

/**
 * Callback to determine if a node has the given id.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param name   Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_id(void *pw, void *node, lwc_string *name, bool *match)
{
    dom_node *n = node;
    dom_string *attr;
    dom_exception err;

    *match = false;

    /* Retrieve element ID using generic dom_element_get_attribute instead of dom_html_element_get_id */
    err = dom_element_get_attribute(n, corestring_dom_id, &attr);
    if (err != DOM_NO_ERR)
        return CSS_OK;

    if (attr != NULL) {
        *match = dom_string_lwc_isequal(attr, name);

        dom_string_unref(attr);
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with the given name.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_attribute(void *pw, void *node, const css_qname *qname, bool *match)
{
    dom_node *n = node;
    dom_string *name;
    dom_exception err;

    err = dom_string_create_interned(
        (const uint8_t *)lwc_string_data(qname->name), lwc_string_length(qname->name), &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    err = dom_element_has_attribute(n, name, match);
    if (err != DOM_NO_ERR) {
        dom_string_unref(name);
        return CSS_OK;
    }

    dom_string_unref(name);

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with given name and value.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_attribute_equal(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match)
{
    dom_node *n = node;
    dom_string *name;
    dom_string *atr_val;
    dom_exception err;

    size_t vlen = lwc_string_length(value);

    if (vlen == 0) {
        *match = false;
        return CSS_OK;
    }

    err = dom_string_create_interned(
        (const uint8_t *)lwc_string_data(qname->name), lwc_string_length(qname->name), &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    err = dom_element_get_attribute(n, name, &atr_val);
    if ((err != DOM_NO_ERR) || (atr_val == NULL)) {
        dom_string_unref(name);
        *match = false;
        return CSS_OK;
    }

    dom_string_unref(name);

    *match = dom_string_caseless_lwc_isequal(atr_val, value);

    dom_string_unref(atr_val);

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value dashmatches that given.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_attribute_dashmatch(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match)
{
    dom_node *n = node;
    dom_string *name;
    dom_string *atr_val;
    dom_exception err;

    size_t vlen = lwc_string_length(value);

    if (vlen == 0) {
        *match = false;
        return CSS_OK;
    }

    err = dom_string_create_interned(
        (const uint8_t *)lwc_string_data(qname->name), lwc_string_length(qname->name), &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    err = dom_element_get_attribute(n, name, &atr_val);
    if ((err != DOM_NO_ERR) || (atr_val == NULL)) {
        dom_string_unref(name);
        *match = false;
        return CSS_OK;
    }

    dom_string_unref(name);

    /* check for exact match */
    *match = dom_string_caseless_lwc_isequal(atr_val, value);

    /* check for dashmatch */
    if (*match == false) {
        const char *vdata = lwc_string_data(value);
        const char *data = (const char *)dom_string_data(atr_val);
        size_t len = dom_string_byte_length(atr_val);

        if (len > vlen && data[vlen] == '-' && strncasecmp(data, vdata, vlen) == 0) {
            *match = true;
        }
    }

    dom_string_unref(atr_val);

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value includes that given.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_attribute_includes(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match)
{
    dom_node *n = node;
    dom_string *name;
    dom_string *atr_val;
    dom_exception err;
    size_t vlen = lwc_string_length(value);
    const char *p;
    const char *start;
    const char *end;

    *match = false;

    if (vlen == 0) {
        return CSS_OK;
    }

    err = dom_string_create_interned(
        (const uint8_t *)lwc_string_data(qname->name), lwc_string_length(qname->name), &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    err = dom_element_get_attribute(n, name, &atr_val);
    if ((err != DOM_NO_ERR) || (atr_val == NULL)) {
        dom_string_unref(name);
        *match = false;
        return CSS_OK;
    }

    dom_string_unref(name);

    /* check for match */
    start = (const char *)dom_string_data(atr_val);
    end = start + dom_string_byte_length(atr_val);

    for (p = start; p <= end; p++) {
        if (*p == ' ' || *p == '\0') {
            if ((size_t)(p - start) == vlen && strncasecmp(start, lwc_string_data(value), vlen) == 0) {
                *match = true;
                break;
            }

            start = p + 1;
        }
    }

    dom_string_unref(atr_val);

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value has the prefix given.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_attribute_prefix(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match)
{
    dom_node *n = node;
    dom_string *name;
    dom_string *atr_val;
    dom_exception err;

    size_t vlen = lwc_string_length(value);

    if (vlen == 0) {
        *match = false;
        return CSS_OK;
    }

    err = dom_string_create_interned(
        (const uint8_t *)lwc_string_data(qname->name), lwc_string_length(qname->name), &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    err = dom_element_get_attribute(n, name, &atr_val);
    if ((err != DOM_NO_ERR) || (atr_val == NULL)) {
        dom_string_unref(name);
        *match = false;
        return CSS_OK;
    }

    dom_string_unref(name);

    /* check for exact match */
    *match = dom_string_caseless_lwc_isequal(atr_val, value);

    /* check for prefix match */
    if (*match == false) {
        const char *data = (const char *)dom_string_data(atr_val);
        size_t len = dom_string_byte_length(atr_val);

        if ((len >= vlen) && (strncasecmp(data, lwc_string_data(value), vlen) == 0)) {
            *match = true;
        }
    }

    dom_string_unref(atr_val);

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value has the suffix given.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_attribute_suffix(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match)
{
    dom_node *n = node;
    dom_string *name;
    dom_string *atr_val;
    dom_exception err;

    size_t vlen = lwc_string_length(value);

    if (vlen == 0) {
        *match = false;
        return CSS_OK;
    }

    err = dom_string_create_interned(
        (const uint8_t *)lwc_string_data(qname->name), lwc_string_length(qname->name), &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    err = dom_element_get_attribute(n, name, &atr_val);
    if ((err != DOM_NO_ERR) || (atr_val == NULL)) {
        dom_string_unref(name);
        *match = false;
        return CSS_OK;
    }

    dom_string_unref(name);

    /* check for exact match */
    *match = dom_string_caseless_lwc_isequal(atr_val, value);

    /* check for prefix match */
    if (*match == false) {
        const char *data = (const char *)dom_string_data(atr_val);
        size_t len = dom_string_byte_length(atr_val);

        const char *start = (char *)data + len - vlen;

        if ((len >= vlen) && (strncasecmp(start, lwc_string_data(value), vlen) == 0)) {
            *match = true;
        }
    }

    dom_string_unref(atr_val);

    return CSS_OK;
}

/**
 * Callback to determine if a node has an attribute with the given name whose
 * value contains the substring given.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param qname  Name to match
 * \param value  Value to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK on success,
 *         CSS_NOMEM on memory exhaustion.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_has_attribute_substring(void *pw, void *node, const css_qname *qname, lwc_string *value, bool *match)
{
    dom_node *n = node;
    dom_string *name;
    dom_string *atr_val;
    dom_exception err;

    size_t vlen = lwc_string_length(value);

    if (vlen == 0) {
        *match = false;
        return CSS_OK;
    }

    err = dom_string_create_interned(
        (const uint8_t *)lwc_string_data(qname->name), lwc_string_length(qname->name), &name);
    if (err != DOM_NO_ERR)
        return CSS_NOMEM;

    err = dom_element_get_attribute(n, name, &atr_val);
    if ((err != DOM_NO_ERR) || (atr_val == NULL)) {
        dom_string_unref(name);
        *match = false;
        return CSS_OK;
    }

    dom_string_unref(name);

    /* check for exact match */
    *match = dom_string_caseless_lwc_isequal(atr_val, value);

    /* check for prefix match */
    if (*match == false) {
        const char *vdata = lwc_string_data(value);
        const char *start = (const char *)dom_string_data(atr_val);
        size_t len = dom_string_byte_length(atr_val);
        const char *last_start = start + len - vlen;

        if (len >= vlen) {
            while (start <= last_start) {
                if (strncasecmp(start, vdata, vlen) == 0) {
                    *match = true;
                    break;
                }

                start++;
            }
        }
    }

    dom_string_unref(atr_val);

    return CSS_OK;
}

/**
 * Callback to determine if a node is the root node of the document.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_is_root(void *pw, void *node, bool *match)
{
    dom_node *n = node;
    dom_node *parent;
    dom_node_type type;
    dom_exception err;

    err = dom_node_get_parent_node(n, &parent);
    if (err != DOM_NO_ERR) {
        return CSS_NOMEM;
    }

    if (parent != NULL) {
        err = dom_node_get_node_type(parent, &type);

        dom_node_unref(parent);

        if (err != DOM_NO_ERR)
            return CSS_NOMEM;

        if (type != DOM_DOCUMENT_NODE) {
            *match = false;
            return CSS_OK;
        }
    }

    *match = true;

    return CSS_OK;
}

static int node_count_siblings_check(dom_node *node, bool check_name, dom_string *name)
{
    dom_node_type type;
    int ret = 0;
    dom_exception exc;

    if (node == NULL)
        return 0;

    exc = dom_node_get_node_type(node, &type);
    if ((exc != DOM_NO_ERR) || (type != DOM_ELEMENT_NODE)) {
        return 0;
    }

    if (check_name) {
        dom_string *node_name = NULL;
        exc = dom_node_get_node_name(node, &node_name);

        if ((exc == DOM_NO_ERR) && (node_name != NULL)) {

            if (dom_string_caseless_isequal(name, node_name)) {
                ret = 1;
            }
            dom_string_unref(node_name);
        }
    } else {
        ret = 1;
    }

    return ret;
}

/**
 * Callback to count a node's siblings.
 *
 * \param pw         HTML document
 * \param n          DOM node
 * \param same_name  Only count siblings with the same name, or all
 * \param after      Count anteceding instead of preceding siblings
 * \param count      Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a count will contain the number of siblings
 */
css_error node_count_siblings(void *pw, void *n, bool same_name, bool after, int32_t *count)
{
    int32_t cnt = 0;
    dom_exception exc;
    dom_string *node_name = NULL;

    if (same_name) {
        dom_node *node = n;
        exc = dom_node_get_node_name(node, &node_name);
        if ((exc != DOM_NO_ERR) || (node_name == NULL)) {
            return CSS_NOMEM;
        }
    }

    if (after) {
        dom_node *node = dom_node_ref(n);
        dom_node *next;

        do {
            exc = dom_node_get_next_sibling(node, &next);
            if ((exc != DOM_NO_ERR))
                break;

            dom_node_unref(node);
            node = next;

            cnt += node_count_siblings_check(node, same_name, node_name);
        } while (node != NULL);
    } else {
        dom_node *node = dom_node_ref(n);
        dom_node *next;

        do {
            exc = dom_node_get_previous_sibling(node, &next);
            if ((exc != DOM_NO_ERR))
                break;

            dom_node_unref(node);
            node = next;

            cnt += node_count_siblings_check(node, same_name, node_name);

        } while (node != NULL);
    }

    if (node_name != NULL) {
        dom_string_unref(node_name);
    }

    *count = cnt;
    return CSS_OK;
}

/**
 * Callback to determine if a node is empty.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node is empty and false otherwise.
 */
css_error node_is_empty(void *pw, void *node, bool *match)
{
    dom_node *n = node, *next;
    dom_exception err;

    *match = true;

    err = dom_node_get_first_child(n, &n);
    if (err != DOM_NO_ERR) {
        return CSS_BADPARM;
    }

    while (n != NULL) {
        dom_node_type ntype;
        err = dom_node_get_node_type(n, &ntype);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_BADPARM;
        }

        if (ntype == DOM_ELEMENT_NODE || ntype == DOM_TEXT_NODE) {
            *match = false;
            dom_node_unref(n);
            break;
        }

        err = dom_node_get_next_sibling(n, &next);
        if (err != DOM_NO_ERR) {
            dom_node_unref(n);
            return CSS_BADPARM;
        }
        dom_node_unref(n);
        n = next;
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node is a linking element.
 *
 * \param pw     DOM document
 * \param n      DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_is_link(void *pw, void *n, bool *match)
{
    dom_node *node = n;
    dom_exception exc;
    dom_string *node_name = NULL;

    exc = dom_node_get_node_name(node, &node_name);
    if ((exc != DOM_NO_ERR) || (node_name == NULL)) {
        return CSS_NOMEM;
    }

    if (dom_string_caseless_lwc_isequal(node_name, corestring_lwc_a)) {
        bool has_href;
        exc = dom_element_has_attribute(node, corestring_dom_href, &has_href);
        if ((exc == DOM_NO_ERR) && (has_href)) {
            *match = true;
        } else {
            *match = false;
        }
    } else {
        *match = false;
    }
    dom_string_unref(node_name);

    return CSS_OK;
}

/**
 * Callback to determine if a node is a linking element whose target has been
 * visited.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_is_visited(void *pw, void *node, bool *match)
{
    nscss_select_ctx *ctx = pw;
    nsurl *url;
    nserror error;
    const struct url_data *data;

    dom_exception exc;
    dom_node *n = node;
    dom_string *s = NULL;

    *match = false;

    exc = dom_node_get_node_name(n, &s);
    if ((exc != DOM_NO_ERR) || (s == NULL)) {
        return CSS_NOMEM;
    }

    if (!dom_string_caseless_lwc_isequal(s, corestring_lwc_a)) {
        /* Can't be visited; not ancher element */
        dom_string_unref(s);
        return CSS_OK;
    }

    /* Finished with node name string */
    dom_string_unref(s);
    s = NULL;

    exc = dom_element_get_attribute(n, corestring_dom_href, &s);
    if ((exc != DOM_NO_ERR) || (s == NULL)) {
        /* Can't be visited; not got a URL */
        return CSS_OK;
    }

    /* Make href absolute */
    /* Resolve href against base URL */
    error = nsurl_join(ctx->base_url, dom_string_data(s), &url);

    /* Finished with href string */
    dom_string_unref(s);

    if (error != NSERROR_OK) {
        /* Couldn't make nsurl object */
        return CSS_NOMEM;
    }

    data = urldb_get_url_data(url);

    /* Visited if in the db and has
     * non-zero visit count */
    if (data != NULL && data->visits > 0)
        *match = true;

    nsurl_unref(url);

    return CSS_OK;
}

/**
 * Callback to determine if a node is currently being hovered over.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_is_hover(void *pw, void *node, bool *match)
{
    /* NOTE: Hover state requires tracking mouse movements and updating
     * the layout/styling dynamically, which touches many systems. */

    *match = false;

    return CSS_OK;
}

/**
 * Callback to determine if a node is currently activated.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_is_active(void *pw, void *node, bool *match)
{
    nscss_select_ctx *ctx = pw;

    *match = false;

    if (ctx == NULL || ctx->c == NULL || node == NULL) {
        return CSS_OK;
    }

    if (ctx->c->drag_type != HTML_DRAG_NONE) {
        dom_node *n = node;

        if (ctx->c->focus_type == HTML_FOCUS_CONTENT && ctx->c->focus_owner.content &&
            ctx->c->focus_owner.content->node == n) {
            *match = true;
        } else if (ctx->c->focus_type == HTML_FOCUS_TEXTAREA && ctx->c->focus_owner.textarea &&
                   ctx->c->focus_owner.textarea->node == n) {
            *match = true;
        } else if ((ctx->c->drag_type == HTML_DRAG_TEXTAREA_SELECTION ||
                    ctx->c->drag_type == HTML_DRAG_TEXTAREA_SCROLLBAR) &&
                   ctx->c->drag_owner.textarea && ctx->c->drag_owner.textarea->node == n) {
            *match = true;
        } else if ((ctx->c->drag_type == HTML_DRAG_CONTENT_SELECTION ||
                    ctx->c->drag_type == HTML_DRAG_CONTENT_SCROLL) &&
                   ctx->c->drag_owner.content && ctx->c->drag_owner.content->node == n) {
            *match = true;
        }
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node has the input focus.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_is_focus(void *pw, void *node, bool *match)
{
    nscss_select_ctx *ctx = pw;

    if (ctx == NULL || ctx->c == NULL) {
        *match = false;
        return CSS_OK;
    }

    dom_node *n = node;
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

/**
 * Callback to determine if a node is enabled.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node is enabled and false otherwise.
 */
static css_error is_disableable_element(dom_node *node, bool *is_disableable)
{
    dom_node_type type;
    dom_exception err;
    dom_string *name;

    *is_disableable = false;

    err = dom_node_get_node_type(node, &type);
    if (err != DOM_NO_ERR || type != DOM_ELEMENT_NODE) {
        return CSS_OK;
    }

    err = dom_node_get_node_name(node, &name);
    if (err != DOM_NO_ERR || name == NULL) {
        return CSS_OK;
    }

    if (dom_string_caseless_lwc_isequal(name, corestring_lwc_button) ||
        dom_string_caseless_lwc_isequal(name, corestring_lwc_input) ||
        dom_string_caseless_lwc_isequal(name, corestring_lwc_select) ||
        dom_string_caseless_lwc_isequal(name, corestring_lwc_textarea) ||
        dom_string_caseless_lwc_isequal(name, corestring_lwc_optgroup) ||
        dom_string_caseless_lwc_isequal(name, corestring_lwc_option) ||
        dom_string_caseless_lwc_isequal(name, corestring_lwc_fieldset)) {
        *is_disableable = true;
    }

    dom_string_unref(name);
    return CSS_OK;
}

css_error node_is_enabled(void *pw, void *node, bool *match)
{
    bool is_disableable = false;
    bool has_disabled = false;
    css_error err;

    *match = false;

    err = is_disableable_element((dom_node *)node, &is_disableable);
    if (err != CSS_OK)
        return err;

    if (is_disableable) {
        dom_exception exc = dom_element_has_attribute((dom_element *)node, corestring_dom_disabled, &has_disabled);
        if (exc == DOM_NO_ERR) {
            *match = !has_disabled;
        }
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node is disabled.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node is disabled and false otherwise.
 */
css_error node_is_disabled(void *pw, void *node, bool *match)
{
    bool is_disableable = false;
    bool has_disabled = false;
    css_error err;

    *match = false;

    err = is_disableable_element((dom_node *)node, &is_disableable);
    if (err != CSS_OK)
        return err;

    if (is_disableable) {
        dom_exception exc = dom_element_has_attribute((dom_element *)node, corestring_dom_disabled, &has_disabled);
        if (exc == DOM_NO_ERR) {
            *match = has_disabled;
        }
    }

    return CSS_OK;
}

/**
 * Callback to determine if a node is checked.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node is checked and false otherwise.
 */
css_error node_is_checked(void *pw, void *node, bool *match)
{
    dom_node *n = node;
    dom_string *node_name = NULL;
    dom_exception exc;

    *match = false;

    if (n == NULL) {
        return CSS_OK;
    }

    exc = dom_node_get_node_name(n, &node_name);
    if (exc != DOM_NO_ERR || node_name == NULL) {
        return CSS_OK;
    }

    if (dom_string_caseless_lwc_isequal(node_name, corestring_lwc_input)) {
        dom_string *input_type = NULL;
        bool is_checkable = true;

        exc = dom_html_input_element_get_type((dom_html_input_element *)n, &input_type);
        if (exc == DOM_NO_ERR && input_type != NULL) {
            if (!dom_string_caseless_lwc_isequal(input_type, corestring_lwc_checkbox) &&
                !dom_string_caseless_lwc_isequal(input_type, corestring_lwc_radio)) {
                /* Per W3C Selectors spec, :checked applies only to checkbox and radio inputs */
                is_checkable = false;
            }
            dom_string_unref(input_type);
        }

        if (is_checkable) {
            bool checked = false;
            exc = dom_html_input_element_get_checked((dom_html_input_element *)n, &checked);
            if (exc == DOM_NO_ERR) {
                *match = checked;
            } else {
                /* Fallback to checking for "checked" attribute if dom_html_input_element_get_checked fails */
                bool has_attr = false;
                exc = dom_element_has_attribute(n, corestring_dom_checked, &has_attr);
                if (exc == DOM_NO_ERR) {
                    *match = has_attr;
                }
            }
        }
    } else if (dom_string_caseless_lwc_isequal(node_name, corestring_lwc_option)) {
        bool selected = false;
        exc = dom_html_option_element_get_selected((dom_html_option_element *)n, &selected);
        if (exc == DOM_NO_ERR) {
            *match = selected;
        } else {
            /* Fallback to checking for "selected" attribute if dom_html_option_element_get_selected fails */
            bool has_attr = false;
            exc = dom_element_has_attribute(n, corestring_dom_selected, &has_attr);
            if (exc == DOM_NO_ERR) {
                *match = has_attr;
            }
        }
    }

    dom_string_unref(node_name);

    return CSS_OK;
}

/**
 * Callback to determine if a node is the target of the document URL.
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match with contain true if the node matches and false otherwise.
 */
css_error node_is_target(void *pw, void *node, bool *match)
{
    nscss_select_ctx *ctx = pw;
    dom_node *n = node;
    lwc_string *target = NULL;
    dom_string *attr = NULL;
    dom_exception err;

    *match = false;

    if (ctx == NULL || ctx->c == NULL || ctx->c->base_url == NULL) {
        return CSS_OK;
    }

    if (!nsurl_has_component(ctx->c->base_url, NSURL_FRAGMENT)) {
        return CSS_OK;
    }

    target = nsurl_get_component(ctx->c->base_url, NSURL_FRAGMENT);
    if (target == NULL) {
        return CSS_OK;
    }

    /* First check element id */
    err = dom_element_get_attribute(n, corestring_dom_id, &attr);
    if (err == DOM_NO_ERR && attr != NULL) {
        if (dom_string_lwc_isequal(attr, target)) {
            *match = true;
        }
        dom_string_unref(attr);
    }

    /* If id didn't match, check anchor name attribute for <a> elements */
    if (!*match) {
        dom_string *node_name = NULL;
        err = dom_node_get_node_name(n, &node_name);
        if (err == DOM_NO_ERR && node_name != NULL) {
            if (dom_string_caseless_lwc_isequal(node_name, corestring_lwc_a)) {
                err = dom_element_get_attribute(n, corestring_dom_name, &attr);
                if (err == DOM_NO_ERR && attr != NULL) {
                    if (dom_string_lwc_isequal(attr, target)) {
                        *match = true;
                    }
                    dom_string_unref(attr);
                }
            }
            dom_string_unref(node_name);
        }
    }

    lwc_string_unref(target);

    return CSS_OK;
}

/**
 * Callback to determine if a node has the given language
 *
 * \param pw     DOM document
 * \param node   DOM node
 * \param lang   Language specifier to match
 * \param match  Pointer to location to receive result
 * \return CSS_OK.
 *
 * \post \a match will contain true if the node matches and false otherwise.
 */
css_error node_is_lang(void *pw, void *node, lwc_string *lang, bool *match)
{
    /** \todo Support languages */

    *match = false;

    return CSS_OK;
}

/**
 * Callback to retrieve the User-Agent defaults for a CSS property.
 *
 * \param pw        HTML document
 * \param property  Property to retrieve defaults for
 * \param hint      Pointer to hint object to populate
 * \return CSS_OK       on success,
 *         CSS_INVALID  if the property should not have a user-agent default.
 */
css_error ua_default_for_property(void *pw, uint32_t property, css_hint *hint)
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
        extern void *wisp_get_default_voice_family_ptr(void);
        hint->data.strings = wisp_get_default_voice_family_ptr();
        hint->status = 0;
    } else {
        return CSS_INVALID;
    }

    return CSS_OK;
}

css_error set_libcss_node_data(void *pw, void *node, void *libcss_node_data)
{
    dom_node *n = node;
    dom_exception err;
    void *old_node_data = NULL;

    /* Set this node's node data */
    err = dom_node_set_user_data(n, corestring_dom___ns_key_libcss_node_data, libcss_node_data,
        nscss_dom_user_data_handler, (void *)&old_node_data);
    if (err != DOM_NO_ERR) {
        return CSS_NOMEM;
    }

    if (old_node_data != NULL) {
        /* Note: css_libcss_node_data_handler is the public LibCSS API which expects exactly 6 arguments:
         * (css_select_handler *handler, css_node_data_action action, void *pw, void *node, void *clone_node, void *libcss_node_data)
         * This invocation perfectly aligns with the function signature and correctly cleans up the old node data.
         */
        css_libcss_node_data_handler(&selection_handler, CSS_NODE_DELETED, NULL, n, NULL, old_node_data);
    }

    return CSS_OK;
}

css_error get_libcss_node_data(void *pw, void *node, void **libcss_node_data)
{
    dom_node *n = node;
    dom_exception err;

    /* Get this node's node data */
    err = dom_node_get_user_data(n, corestring_dom___ns_key_libcss_node_data, libcss_node_data);
    if (err != DOM_NO_ERR) {
        return CSS_NOMEM;
    }

    return CSS_OK;
}
