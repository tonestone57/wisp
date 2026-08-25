/*
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
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
 * implementation of content handling for text/html scripts.
 */

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <wisp/content.h>
#include <wisp/content/content_protected.h>
#include <wisp/content/fetch.h>
#include <wisp/content/hlcache.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/misc.h>
#include <wisp/utils/config.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/log.h>
#include <wisp/utils/messages.h>
#include "content/content_factory.h"
#include "content/handlers/javascript/js.h"

#include <wisp/content/handlers/html/html.h>
#include <wisp/content/handlers/html/private.h>
#include <wisp/content/csp.h>

#include <nsutils/time.h>

/* Performance tracing - enable via CMake: -DNEOSURF_ENABLE_PERF_TRACE=ON */
#include <wisp/utils/perf.h>

typedef bool(script_handler_t)(struct jsthread *jsthread, const uint8_t *data, size_t size, const char *name);


static script_handler_t *select_script_handler(content_type ctype)
{
    switch (ctype) {
    case CONTENT_JS:
        return js_exec;
    default:
        return NULL;
    }
}

void script_resume_conversion_cb(void *p)
{
    html_content *htmlc = p;
    html_begin_conversion(htmlc);
}


/* exported internal interface documented in html/html_internal.h */
nserror html_script_exec(html_content *c, bool allow_defer)
{
    unsigned int i;
    struct html_script *s;
    script_handler_t *script_handler;
    bool have_run_something = false;

    if (c->jsthread == NULL) {
        return NSERROR_BAD_PARAMETER;
    }

    for (i = 0, s = c->scripts; i != c->scripts_count; i++, s++) {
        if (s->already_started) {
            continue;
        }

        if ((s->type == HTML_SCRIPT_ASYNC) || (allow_defer && (s->type == HTML_SCRIPT_DEFER))) {
            /* ensure script content is present */
            if (s->data.handle == NULL)
                continue;

            /* ensure script content fetch status is not an error */
            if (content_get_status(s->data.handle) == CONTENT_STATUS_ERROR)
                continue;

            /* ensure script handler for content type */
            script_handler = select_script_handler(content_get_type(s->data.handle));
            if (script_handler == NULL)
                continue; /* unsupported type */

            if (content_get_status(s->data.handle) == CONTENT_STATUS_DONE) {
                /* external script is now available */
                const uint8_t *data;
                size_t size;
                data = content_get_source_data(s->data.handle, &size);

                doc_rwlock_wrlock(&c->doc_mutex);
                script_handler(c->jsthread, data, size, nsurl_access(hlcache_handle_get_url(s->data.handle)));
                doc_rwlock_wrunlock(&c->doc_mutex);

                have_run_something = true;
                /* We have to re-acquire this here since the
                 * c->scripts array may have been reallocated
                 * as a result of executing this script.
                 */
                s = &(c->scripts[i]);

                s->already_started = true;
            }
        }
    }

    if (have_run_something) {
        return html_proceed_to_done(c);
    }

    return NSERROR_OK;
}

/* create new html script entry */
static struct html_script *html_process_new_script(html_content *c, dom_string *mimetype, enum html_script_type type)
{
    struct html_script *nscript;
    /* add space for new script entry */
    nscript = realloc(c->scripts, sizeof(struct html_script) * (c->scripts_count + 1));
    if (nscript == NULL) {
        return NULL;
    }

    c->scripts = nscript;

    /* increment script entry count */
    nscript = &c->scripts[c->scripts_count];
    c->scripts_count++;

    memset(nscript, 0, sizeof(struct html_script));

    nscript->already_started = false;
    nscript->parser_inserted = false;
    nscript->force_async = true;
    nscript->ready_exec = false;
    nscript->async = false;
    nscript->defer = false;

    nscript->type = type;

    nscript->mimetype = dom_string_ref(mimetype); /* reference mimetype */

    return nscript;
}

/**
 * Callback for asyncronous scripts
 */
static nserror convert_script_async_cb(hlcache_handle *script, const hlcache_event *event, void *pw)
{
    html_content *parent = pw;
    unsigned int i;
    struct html_script *s;
    nserror ret_val = NSERROR_OK;

    doc_rwlock_wrlock(&parent->doc_mutex);

    /* Find script */
    bool found = false;
    for (i = 0, s = parent->scripts; i != parent->scripts_count; i++, s++) {
        if (s->type == HTML_SCRIPT_ASYNC && s->data.handle == script) {
            found = true;
            break;
        }
    }

    if (!found) {
        for (int idx = (int)parent->scripts_count - 1; idx >= 0; idx--) {
            s = &parent->scripts[idx];
            if (s->type == HTML_SCRIPT_ASYNC && s->data.handle == NULL) {
                s->data.handle = script;
                i = idx;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        i = parent->scripts_count;
    }

    if (i == parent->scripts_count) {
        /* If not found, check if this is a synchronous callback executing during retrieval
         * where s->data.handle was not yet assigned from the retrieve caller. */
        for (int k = (int)parent->scripts_count - 1; k >= 0; k--) {
            if (parent->scripts[k].type == HTML_SCRIPT_ASYNC && parent->scripts[k].data.handle == NULL) {
                parent->scripts[k].data.handle = script;
                i = k;
                s = &parent->scripts[k];
                break;
            }
        }
    }

    if (i == parent->scripts_count) {
        NSLOG(wisp, ERROR, "convert_script_async_cb: script not found!");
        doc_rwlock_wrunlock(&parent->doc_mutex);
        return NSERROR_OK;
    }

    switch (event->type) {
    case CONTENT_MSG_LOADING:
        break;

    case CONTENT_MSG_READY:
        break;

    case CONTENT_MSG_DONE:
        PERF("SCRIPT ASYNC DONE %d '%s' (active=%d)", i, nsurl_access(hlcache_handle_get_url(script)),
            parent->base.active - 1);
        NSLOG(wisp, INFO, "script %d done '%s'", i, nsurl_access(hlcache_handle_get_url(script)));
        {
            const uint8_t *data;
            size_t size;
            data = content_get_source_data(script, &size);
            extern void wisp_queue_precompile(const uint8_t *txt, size_t txtlen);
            wisp_queue_precompile(data, size);
        }
        if (parent->base.active == 0) {
            NSLOG(wisp, CRITICAL,
                "ACTIVE UNDERFLOW! async_cb DONE decrement when 0 "
                "[content=%p script=%s]",
                parent, nsurl_access(hlcache_handle_get_url(script)));
        }
        parent->base.active--;
        parent->scripts_active--;
        nsu_getmonotonic_ms(&parent->data_complete_time_ms);
        NSLOG(wisp, INFO, "%d fetches active", parent->base.active);

        break;

    case CONTENT_MSG_ERROR:
        NSLOG(wisp, WARNING, "script %s failed: %s", nsurl_access(hlcache_handle_get_url(script)),
            event->data.errordata.errormsg);

        hlcache_handle_release(script);
        s->data.handle = NULL;
        if (parent->base.active == 0) {
            NSLOG(wisp, CRITICAL,
                "ACTIVE UNDERFLOW! async_cb ERROR decrement when 0 "
                "[content=%p script=%s]",
                parent, nsurl_access(hlcache_handle_get_url(script)));
        }
        parent->base.active--;
        parent->scripts_active--;
        NSLOG(wisp, INFO, "%d fetches active", parent->base.active);

        break;

    default:
        break;
    }

    /* if there are no active fetches remaining begin post parse
     * conversion
     */
    if (html_can_begin_conversion(parent)) {
        guit->misc->schedule(0, script_resume_conversion_cb, parent);
    }

    /* if we have already started converting though, then we can handle the
     * scripts as they come in.
     */
    else if (parent->conversion_begun) {
        ret_val = html_script_exec(parent, false);
    }

    doc_rwlock_wrunlock(&parent->doc_mutex);
    return ret_val;
}

/**
 * Callback for defer scripts
 */
static nserror convert_script_defer_cb(hlcache_handle *script, const hlcache_event *event, void *pw)
{
    html_content *parent = pw;
    unsigned int i;
    struct html_script *s;

    doc_rwlock_wrlock(&parent->doc_mutex);

    /* Find script */
    bool found = false;
    for (i = 0, s = parent->scripts; i != parent->scripts_count; i++, s++) {
        if (s->type == HTML_SCRIPT_DEFER && s->data.handle == script) {
            found = true;
            break;
        }
    }

    if (!found) {
        for (int idx = (int)parent->scripts_count - 1; idx >= 0; idx--) {
            s = &parent->scripts[idx];
            if (s->type == HTML_SCRIPT_DEFER && s->data.handle == NULL) {
                s->data.handle = script;
                i = idx;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        i = parent->scripts_count;
    }

    if (i == parent->scripts_count) {
        /* If not found, check if this is a synchronous callback executing during retrieval
         * where s->data.handle was not yet assigned from the retrieve caller. */
        for (int k = (int)parent->scripts_count - 1; k >= 0; k--) {
            if (parent->scripts[k].type == HTML_SCRIPT_DEFER && parent->scripts[k].data.handle == NULL) {
                parent->scripts[k].data.handle = script;
                i = k;
                s = &parent->scripts[k];
                break;
            }
        }
    }

    if (i == parent->scripts_count) {
        NSLOG(wisp, ERROR, "convert_script_defer_cb: script not found!");
        doc_rwlock_wrunlock(&parent->doc_mutex);
        return NSERROR_OK;
    }

    switch (event->type) {

    case CONTENT_MSG_DONE:
        PERF("SCRIPT DEFER DONE %d '%s' (active=%d)", i, nsurl_access(hlcache_handle_get_url(script)),
            parent->base.active - 1);
        NSLOG(wisp, INFO, "script %d done '%s'", i, nsurl_access(hlcache_handle_get_url(script)));
        {
            const uint8_t *data;
            size_t size;
            data = content_get_source_data(script, &size);
            extern void wisp_queue_precompile(const uint8_t *txt, size_t txtlen);
            wisp_queue_precompile(data, size);
        }
        if (parent->base.active == 0) {
            NSLOG(wisp, CRITICAL,
                "ACTIVE UNDERFLOW! defer_cb DONE decrement when 0 "
                "[content=%p script=%s]",
                parent, nsurl_access(hlcache_handle_get_url(script)));
        }
        parent->base.active--;
        parent->scripts_active--;
        NSLOG(wisp, INFO, "%d fetches active", parent->base.active);

        break;

    case CONTENT_MSG_ERROR:
        NSLOG(wisp, WARNING, "script %s failed: %s", nsurl_access(hlcache_handle_get_url(script)),
            event->data.errordata.errormsg);

        hlcache_handle_release(script);
        s->data.handle = NULL;
        if (parent->base.active == 0) {
            NSLOG(wisp, CRITICAL,
                "ACTIVE UNDERFLOW! defer_cb ERROR decrement when 0 "
                "[content=%p script=%s]",
                parent, nsurl_access(hlcache_handle_get_url(script)));
        }
        parent->base.active--;
        parent->scripts_active--;
        NSLOG(wisp, INFO, "%d fetches active", parent->base.active);

        break;

    default:
        break;
    }

    /* if there are no active fetches remaining begin post parse
     * conversion
     */
    if (html_can_begin_conversion(parent)) {
        guit->misc->schedule(0, script_resume_conversion_cb, parent);
    }

    doc_rwlock_wrunlock(&parent->doc_mutex);
    return NSERROR_OK;
}

static void html_execute_pending_sync_scripts(html_content *parent)
{
    unsigned int i;
    struct html_script *s;
    script_handler_t *script_handler;

    for (i = 0; i < parent->scripts_count; i++) {
        s = &parent->scripts[i];

        if (s->type != HTML_SCRIPT_SYNC) {
            continue;
        }

        if (s->already_started) {
            continue;
        }

        /* If this script is not ready, we must block execution of subsequent scripts */
        if (s->data.handle == NULL || content_get_status(s->data.handle) != CONTENT_STATUS_DONE) {
            break;
        }

        s->already_started = true;

        script_handler = select_script_handler(content_get_type(s->data.handle));
        if (script_handler != NULL) {
            const uint8_t *data;
            size_t size;
            data = content_get_source_data(s->data.handle, &size);

            doc_rwlock_wrlock(&parent->doc_mutex);
            if (parent->jsthread != NULL) {
                script_handler(parent->jsthread, data, size, nsurl_access(hlcache_handle_get_url(s->data.handle)));
            }
            doc_rwlock_wrunlock(&parent->doc_mutex);
        }
    }
}

/**
 * Callback for syncronous scripts
 */
static nserror convert_script_sync_cb(hlcache_handle *script, const hlcache_event *event, void *pw)
{
    html_content *parent = pw;
    unsigned int i;
    struct html_script *s;
    dom_hubbub_error err;
    unsigned int active_sync_scripts = 0;
    nserror ret_val = NSERROR_OK;

    doc_rwlock_wrlock(&parent->doc_mutex);

    /* Count sync scripts which have yet to complete downloading (other than us) */
    for (i = 0, s = parent->scripts; i != parent->scripts_count; i++, s++) {
        if (s->type == HTML_SCRIPT_SYNC && s->data.handle != script && s->already_started == false) {
            if (s->data.handle == NULL || content_get_status(s->data.handle) != CONTENT_STATUS_DONE) {
                active_sync_scripts++;
            }
        }
    }

    /* Find script */
    bool found = false;
    for (i = 0, s = parent->scripts; i != parent->scripts_count; i++, s++) {
        if (s->type == HTML_SCRIPT_SYNC && s->data.handle == script) {
            found = true;
            break;
        }
    }

    if (!found) {
        for (int idx = (int)parent->scripts_count - 1; idx >= 0; idx--) {
            s = &parent->scripts[idx];
            if (s->type == HTML_SCRIPT_SYNC && s->data.handle == NULL) {
                s->data.handle = script;
                i = idx;
                found = true;
                break;
            }
        }
    }

    if (!found) {
        i = parent->scripts_count;
    }

    if (i == parent->scripts_count) {
        /* If not found, check if this is a synchronous callback executing during retrieval
         * where s->data.handle was not yet assigned from the retrieve caller. */
        for (int k = (int)parent->scripts_count - 1; k >= 0; k--) {
            if (parent->scripts[k].type == HTML_SCRIPT_SYNC && parent->scripts[k].data.handle == NULL) {
                parent->scripts[k].data.handle = script;
                i = k;
                s = &parent->scripts[k];
                break;
            }
        }
    }

    if (i == parent->scripts_count) {
        NSLOG(wisp, ERROR, "convert_script_sync_cb: script not found!");
        doc_rwlock_wrunlock(&parent->doc_mutex);
        return NSERROR_OK;
    }

    switch (event->type) {
    case CONTENT_MSG_DONE:
        PERF("SCRIPT SYNC DONE %d '%s' (active=%d)", i, nsurl_access(hlcache_handle_get_url(script)),
            parent->base.active - 1);
        NSLOG(wisp, INFO, "script %d done '%s'", i, nsurl_access(hlcache_handle_get_url(script)));
        {
            const uint8_t *data;
            size_t size;
            data = content_get_source_data(script, &size);
            extern void wisp_queue_precompile(const uint8_t *txt, size_t txtlen);
            wisp_queue_precompile(data, size);
        }
        NSLOG(wisp, INFO, "DIAG: sync_cb DONE: parent=%p active=%d->%d", parent, parent->base.active,
            parent->base.active - 1);
        if (parent->base.active == 0) {
            NSLOG(wisp, CRITICAL,
                "ACTIVE UNDERFLOW! sync_cb DONE decrement when 0 "
                "[content=%p script=%s]",
                parent, nsurl_access(hlcache_handle_get_url(script)));
        }
        parent->base.active--;
        parent->scripts_active--;
        nsu_getmonotonic_ms(&parent->data_complete_time_ms);
        NSLOG(wisp, INFO, "%d fetches active", parent->base.active);

        html_execute_pending_sync_scripts(parent);

        /* continue parse */
        if (parent->parser != NULL && active_sync_scripts == 0) {
            err = dom_hubbub_parser_pause(parent->parser, false);
            if (err != DOM_HUBBUB_OK) {
                if (err == DOM_HUBBUB_HUBBUB_ERR_PAUSED) {
                    NSLOG(wisp, INFO, "unpause re-paused for subsequent script (0x%x)", err);
                } else {
                    NSLOG(wisp, WARNING, "unpause returned error 0x%x", err);
                }
            }
        }

        break;

    case CONTENT_MSG_ERROR:
        NSLOG(wisp, WARNING, "script %s failed: %s", nsurl_access(hlcache_handle_get_url(script)),
            event->data.errordata.errormsg);

        hlcache_handle_release(script);
        s->data.handle = NULL;
        if (parent->base.active == 0) {
            NSLOG(wisp, CRITICAL,
                "ACTIVE UNDERFLOW! sync_cb ERROR decrement when 0 "
                "[content=%p script=%s]",
                parent, nsurl_access(hlcache_handle_get_url(script)));
        }
        parent->base.active--;
        parent->scripts_active--;
        nsu_getmonotonic_ms(&parent->data_complete_time_ms);

        NSLOG(wisp, INFO, "%d fetches active", parent->base.active);

        s->already_started = true;

        /* continue parse */
        if (parent->parser != NULL && active_sync_scripts == 0) {
            err = dom_hubbub_parser_pause(parent->parser, false);
            if (err != DOM_HUBBUB_OK) {
                if (err == DOM_HUBBUB_HUBBUB_ERR_PAUSED) {
                    NSLOG(wisp, INFO, "unpause re-paused for subsequent script (0x%x)", err);
                } else {
                    NSLOG(wisp, WARNING, "unpause returned error 0x%x", err);
                }
            }
        }

        break;

    default:
        break;
    }

    /* if there are no active fetches remaining begin post parse
     * conversion
     */
    if (html_can_begin_conversion(parent)) {
        guit->misc->schedule(0, script_resume_conversion_cb, parent);
    }

    /* if we have already started converting, execute pending scripts
     * and proceed to done state if all scripts have finished
     */
    else if (parent->conversion_begun) {
        ret_val = html_script_exec(parent, false);
    }

    doc_rwlock_wrunlock(&parent->doc_mutex);
    return ret_val;
}

/**
 * process a script with a src tag
 */
static dom_hubbub_error exec_src_script(html_content *c, dom_node *node, dom_string *mimetype, dom_string *src)
{
    nserror ns_error;
    nsurl *joined;
    hlcache_child_context child = { 0 };
    struct html_script *nscript;
    bool async;
    bool defer;
    enum html_script_type script_type;
    hlcache_handle_callback script_cb;
    dom_hubbub_error ret = DOM_HUBBUB_OK;
    dom_exception exc; /* returned by libdom functions */

    /* src url */
    ns_error = nsurl_join(c->base_url, dom_string_data(src), &joined);
    if (ns_error != NSERROR_OK) {
        content_broadcast_error(&c->base, NSERROR_NOMEM, NULL);
        return DOM_HUBBUB_NOMEM;
    }

    NSLOG(wisp, INFO, "script %i '%s'", c->scripts_count, nsurl_access(joined));

    /* there are three ways to process the script tag at this point:
     *
     * Syncronously  pause the parent parse and continue after
     *                 the script has downloaded and executed. (default)
     * Async         Start the script downloading and execute it when it
     *                 becomes available.
     * Defered       Start the script downloading and execute it when
     *                 the page has completed parsing, may be set along
     *                 with async where it is ignored.
     */

    /* we interpret the presence of the async and defer attribute
     * as true and ignore its value, technically only the empty
     * value or the attribute name itself are valid. However
     * various browsers interpret this in various ways the most
     * compatible approach is to be liberal and accept any
     * value. Note setting the values to "false" still makes them true!
     */
    exc = dom_element_has_attribute(node, corestring_dom_async, &async);
    if (exc != DOM_NO_ERR) {
        return DOM_HUBBUB_OK; /* dom error */
    }

    if (c->parse_completed) {
        /* After parse completed, all scripts are essentially async */
        async = true;
        defer = false;
    }

    if (async) {
        /* asyncronous script */
        script_type = HTML_SCRIPT_ASYNC;
        script_cb = convert_script_async_cb;

    } else {
        exc = dom_element_has_attribute(node, corestring_dom_defer, &defer);
        if (exc != DOM_NO_ERR) {
            return DOM_HUBBUB_OK; /* dom error */
        }

        if (defer) {
            /* defered script */
            script_type = HTML_SCRIPT_DEFER;
            script_cb = convert_script_defer_cb;
        } else {
            /* syncronous script */
            script_type = HTML_SCRIPT_SYNC;
            script_cb = convert_script_sync_cb;
        }
    }

    nscript = html_process_new_script(c, mimetype, script_type);
    if (nscript == NULL) {
        nsurl_unref(joined);
        content_broadcast_error(&c->base, NSERROR_NOMEM, NULL);
        return DOM_HUBBUB_NOMEM;
    }

    dom_string *nonce_attr = NULL;
    dom_string *nonce_str = NULL;
    dom_string_create((const uint8_t *)"nonce", 5, &nonce_str);
    if (nonce_str) {
        dom_element_get_attribute(node, nonce_str, &nonce_attr);
        dom_string_unref(nonce_str);
    }

    /* set up child fetch encoding and quirks */
    child.charset = c->encoding;
    child.quirks = c->base.quirks;
    child.csp = c->csp;
    child.coep = c->coep;
    child.parent_url = c->base_url;
    if (nonce_attr != NULL) {
        child.nonce = dom_string_data(nonce_attr);
    }

    /* Increment active fetch count BEFORE hlcache_handle_retrieve.
     * This is critical because the callback can be called synchronously
     * for cached content, which would decrement active before we get
     * a chance to increment it, causing an underflow.
     */
    c->base.active++;
    c->scripts_active++; /* Track script fetches separately */
    NSLOG(wisp, INFO, "DIAG: exec_src_script: content=%p active=%d scripts_active=%d (pre-retrieve)", c,
        c->base.active, c->scripts_active);

    PERF("SCRIPT FETCH START '%s' type=%s (active=%d)", nsurl_access(joined),
        script_type == HTML_SCRIPT_SYNC ? "SYNC" : (script_type == HTML_SCRIPT_ASYNC ? "ASYNC" : "DEFER"),
        c->base.active);

    unsigned int script_idx = c->scripts_count - 1;
    struct hlcache_handle *local_handle = NULL;

    ns_error = hlcache_handle_retrieve(
        joined, 0, content_get_url(&c->base), NULL, script_cb, c, &child, CONTENT_SCRIPT, &local_handle);

    if (nonce_attr != NULL) {
        dom_string_unref(nonce_attr);
    }

    /* Re-acquire nscript since c->scripts may have been reallocated during synchronous script_cb */
    nscript = &c->scripts[script_idx];
    nscript->data.handle = local_handle;

    nsurl_unref(joined);

    if (ns_error != NSERROR_OK) {
        /* Fetch failed - decrement the counter we just incremented */
        c->base.active--;
        c->scripts_active--;
        /* mark duff script fetch as already started */
        nscript->already_started = true;
        NSLOG(wisp, INFO, "Fetch failed with error %d, active=%d", ns_error, c->base.active);
    } else {
        NSLOG(wisp, INFO, "%d fetches active", c->base.active);

        switch (script_type) {
        case HTML_SCRIPT_SYNC:
            if (c->parser != NULL && !nscript->already_started) {
                dom_hubbub_parser_pause(c->parser, true);
                ret = DOM_HUBBUB_HUBBUB_ERR | HUBBUB_PAUSED;
            }
            break;

        case HTML_SCRIPT_ASYNC:
            break;

        case HTML_SCRIPT_DEFER:
            break;

        default:
            assert(0);
        }
    }

    return ret;
}

static bool is_javascript_mime_type(const char *mime)
{
    if (mime == NULL || mime[0] == '\0') {
        return true;
    }

    /* Trim leading whitespace */
    while (*mime && isspace((unsigned char)*mime)) {
        mime++;
    }
    size_t len = strlen(mime);
    /* Trim trailing whitespace */
    while (len > 0 && isspace((unsigned char)mime[len - 1])) {
        len--;
    }

    if (len == 0) {
        return true;
    }

    /* Check legacy/standard mime types */
    static const char *const legacy_js_mimetypes[] = {
        "text/javascript",
        "application/javascript",
        "application/ecmascript",
        "text/ecmascript",
        "application/x-javascript",
        "text/jscript",
        "text/livescript",
        "javascript",
        "js",
        "text/js",
        "module"
    };
    for (size_t i = 0; i < sizeof(legacy_js_mimetypes) / sizeof(legacy_js_mimetypes[0]); i++) {
        if (strncasecmp(mime, legacy_js_mimetypes[i], len) == 0 && legacy_js_mimetypes[i][len] == '\0') {
            return true;
        }
    }
    return false;
}

static dom_hubbub_error exec_inline_script(html_content *c, dom_node *node, dom_string *mimetype)
{
    dom_string *script;
    dom_exception exc; /* returned by libdom functions */
    struct lwc_string_s *lwcmimetype;
    script_handler_t *script_handler;
    struct html_script *nscript;

    /* does not appear to be a src so script is inline content */
    exc = dom_node_get_text_content(node, &script);
    if ((exc != DOM_NO_ERR) || (script == NULL)) {
        return DOM_HUBBUB_OK; /* no contents, skip */
    }

    nscript = html_process_new_script(c, mimetype, HTML_SCRIPT_INLINE);
    if (nscript == NULL) {
        dom_string_unref(script);

        content_broadcast_error(&c->base, NSERROR_NOMEM, NULL);
        return DOM_HUBBUB_NOMEM;
    }

    nscript->data.string = script;
    nscript->already_started = true;

    content_type ctype = CONTENT_NONE;
    const char *mime_cstr = mimetype ? (const char *)dom_string_data(mimetype) : NULL;
    if (is_javascript_mime_type(mime_cstr)) {
        ctype = CONTENT_JS;
    } else {
        /* ensure script handler for content type */
        exc = dom_string_intern(mimetype, &lwcmimetype);
        if (exc != DOM_NO_ERR) {
            NSLOG(wisp, WARNING, "exec_inline_script: dom_string_intern failed");
            return DOM_HUBBUB_DOM;
        }

        ctype = content_factory_type_from_mime_type(lwcmimetype);
        lwc_string_unref(lwcmimetype);
        /* Re-retrieve mime_cstr since dom_string_intern might free the old memory block
         * and reallocate / intern it, rendering the previous pointer invalid. */
        mime_cstr = mimetype ? (const char *)dom_string_data(mimetype) : NULL;
    }

    NSLOG(wisp, INFO, "exec_inline_script: mimetype_cstr='%s' -> content_type=%d (CONTENT_JS=%d)",
        mime_cstr ? mime_cstr : "", ctype, CONTENT_JS);

    script_handler = select_script_handler(ctype);

    bool allowed = false;
    dom_string *nonce_attr = NULL;
    dom_string *nonce_str = NULL;
    dom_string_create((const uint8_t *)"nonce", 5, &nonce_str);
    if (nonce_str) {
        dom_element_get_attribute(node, nonce_str, &nonce_attr);
        dom_string_unref(nonce_str);
    }

    if (nonce_attr != NULL) {
        const char *nonce_val = dom_string_data(nonce_attr);
        allowed = csp_check_nonce(c->csp, CSP_SCRIPT_SRC, nonce_val);
        dom_string_unref(nonce_attr);
    } else {
        allowed = csp_check_inline(c->csp, CSP_SCRIPT_SRC);
    }

    NSLOG(wisp, INFO, "exec_inline_script: script_handler=%p, jsthread=%p", script_handler, c->jsthread);

    if (!allowed) {
        NSLOG(wisp, INFO, "CSP BLOCKED inline script execution");
    } else if (script_handler != NULL && c->jsthread != NULL) {
        NSLOG(
            wisp, INFO, "exec_inline_script: calling script_handler with %zu bytes", dom_string_byte_length(script));
        script_handler(
            c->jsthread, (const uint8_t *)dom_string_data(script), dom_string_byte_length(script), "?inline script?");
    } else {
        NSLOG(wisp, DEBUG, "exec_inline_script: script_handler is NULL, skipping execution");
    }
    return DOM_HUBBUB_OK;
}


/**
 * process script node parser callback
 *
 *
 */
dom_hubbub_error html_process_script(void *ctx, dom_node *node)
{
    html_content *c = (html_content *)ctx;
    dom_exception exc; /* returned by libdom functions */
    dom_string *src, *mimetype;
    dom_hubbub_error err = DOM_HUBBUB_OK;

    /* ensure javascript context is available */
    /* We should only ever be here if scripting was enabled for this
     * content so it's correct to make a javascript context if there
     * isn't one already. */
    if (c->jsthread == NULL) {
        union content_msg_data msg_data;

        msg_data.jsthread = &c->jsthread;
        content_broadcast(&c->base, CONTENT_MSG_GETTHREAD, &msg_data);
        NSLOG(wisp, INFO, "javascript context %p ", c->jsthread);
        if (c->jsthread == NULL) {
            /* no context and it could not be created, abort */
            return DOM_HUBBUB_OK;
        }
    }

    NSLOG(wisp, INFO, "html_process_script: content %p parser %p node %p", c, c->parser, node);
    NSLOG(wisp, DEBUG, "PROFILER: START JS execute %p", node);

    exc = dom_element_get_attribute(node, corestring_dom_type, &mimetype);
    if (exc != DOM_NO_ERR || mimetype == NULL) {
        mimetype = dom_string_ref(corestring_dom_text_javascript);
    }
    NSLOG(wisp, INFO, "html_process_script: mimetype='%s'", dom_string_data(mimetype));

    exc = dom_element_get_attribute(node, corestring_dom_src, &src);
    if (exc != DOM_NO_ERR || src == NULL) {
        NSLOG(wisp, INFO, "html_process_script: executing INLINE script");
        err = exec_inline_script(c, node, mimetype);
    } else {
        NSLOG(wisp, INFO, "html_process_script: fetching EXTERNAL script: %s", dom_string_data(src));
        err = exec_src_script(c, node, mimetype, src);
        dom_string_unref(src);
    }

    dom_string_unref(mimetype);
    NSLOG(wisp, DEBUG, "PROFILER: STOP JS execute %p", node);

    return err;
}

/* exported internal interface documented in html/html_internal.h */
bool html_saw_insecure_scripts(html_content *htmlc)
{
    struct html_script *s;
    unsigned int i;

    for (i = 0, s = htmlc->scripts; i != htmlc->scripts_count; i++, s++) {
        if (s->type == HTML_SCRIPT_INLINE) {
            /* Inline scripts are no less secure than their
             * containing HTML content
             */
            continue;
        }
        if (s->data.handle == NULL) {
            /* We've not begun loading this? */
            continue;
        }
        if (content_saw_insecure_objects(s->data.handle)) {
            return true;
        }
    }

    return false;
}

/* exported internal interface documented in html/html_internal.h */
nserror html_script_free(html_content *html)
{
    unsigned int i;

    for (i = 0; i != html->scripts_count; i++) {
        if (html->scripts[i].mimetype != NULL) {
            dom_string_unref(html->scripts[i].mimetype);
        }

        switch (html->scripts[i].type) {
        case HTML_SCRIPT_INLINE:
            if (html->scripts[i].data.string != NULL) {
                dom_string_unref(html->scripts[i].data.string);
            }
            break;
        case HTML_SCRIPT_SYNC:
            /* fallthrough */
        case HTML_SCRIPT_ASYNC:
            /* fallthrough */
        case HTML_SCRIPT_DEFER:
            if (html->scripts[i].data.handle != NULL) {
                hlcache_handle_release(html->scripts[i].data.handle);
            }
            break;
        }
    }
    free(html->scripts);

    return NSERROR_OK;
}
