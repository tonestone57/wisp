/*
 * Copyright 2004 John M Bell <jmb202@ecs.soton.ac.uk>
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

/** \file
 * Text export of HTML (implementation).
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dom/dom.h>

#include <wisp/content/handlers/html/html.h>
#include <wisp/utils/config.h>
#include <wisp/utils/log.h>
#include <wisp/utils/utf8.h>
#include <wisp/utils/utils.h>
#include "wisp/content.h"

#include <wisp/desktop/gui_internal.h>
#include <wisp/desktop/save_text.h>
#include "wisp/utf8.h"

/**
 * Extract the text from an HTML content and save it as a text file. Text is
 * converted to the local encoding.
 *
 * \param  c		An HTML content.
 * \param  path		Path to save text file too.
 */

void save_as_text(struct hlcache_handle *c, char *path)
{
    FILE *out;
    struct save_text_state save = {NULL, 0, 0};
    save_text_whitespace before = WHITESPACE_NONE;
    bool first = true;
    nserror ret;
    char *result;

    if (!c || content_get_type(c) != CONTENT_HTML) {
        return;
    }

    html_extract_text(c, &first, &before, &save);
    if (!save.block)
        return;

    ret = guit->utf8->utf8_to_local(save.block, save.length, &result);
    free(save.block);

    if (ret != NSERROR_OK) {
        NSLOG(wisp, ERROR, "failed to convert to local encoding, return %d", ret);
        return;
    }

    out = fopen(path, "w");
    if (out) {
        int res = fputs(result, out);

        if (res < 0) {
            NSLOG(wisp, WARNING, "Warning: write failed");
        }

        res = fputs("\n", out);
        if (res < 0) {
            NSLOG(wisp, WARNING, "Warning: failed writing trailing newline");
        }

        fclose(out);
    }

    free(result);
}




