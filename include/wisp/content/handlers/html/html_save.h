/*
 * Copyright 2018 Vincent Sanders <vince@netsurf-browser.org>
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
 * Interface to HTML content handler to save documents.
 *
 * \todo Investigate altering this API as it is only used for
 *         exporting the html content to disc.
 */

#ifndef WISP_HTML_HTML_SAVE_H
#define WISP_HTML_HTML_SAVE_H

#include <stdbool.h>
#include <wisp/desktop/save_text.h>

struct dom_document;
struct hlcache_handle;
struct nsurl;

/**
 * get the dom document of a html content from a handle
 */
dom_document *html_get_document(struct hlcache_handle *h);

/**
 * extract text representation from an html content handle
 */
void html_extract_text(struct hlcache_handle *h, bool *first,
		save_text_whitespace *before, struct save_text_state *save);

/**
 * get the base url of an html content from a handle
 */
struct nsurl *html_get_base_url(struct hlcache_handle *h);

#endif
