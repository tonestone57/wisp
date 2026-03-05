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

#include <libcss/libcss.h>
#include <string.h>

#include <wisp/utils/nsurl.h>

#include "content/handlers/css/internal.h"

/* exported interface documented in content/handlers/css/internal.h */
css_error nscss_resolve_url(void *pw, const char *base, lwc_string *rel, lwc_string **abs)
{
    lwc_error lerror;
    nserror error;
    nsurl *nsbase = pw;
    nsurl *nsabs;
    bool created_nsbase = false;

    if (nsbase == NULL) {
        error = nsurl_create(base, &nsbase);
        if (error != NSERROR_OK) {
            return error == NSERROR_NOMEM ? CSS_NOMEM : CSS_INVALID;
        }
        created_nsbase = true;
    }

    /* Resolve URI */
    error = nsurl_join(nsbase, lwc_string_data(rel), &nsabs);
    if (error != NSERROR_OK) {
        if (created_nsbase) {
            nsurl_unref(nsbase);
        }
        return error == NSERROR_NOMEM ? CSS_NOMEM : CSS_INVALID;
    }

    if (created_nsbase) {
        nsurl_unref(nsbase);
    }

    /* Intern it */
    lerror = lwc_intern_string(nsurl_access(nsabs), nsurl_length(nsabs), abs);
    if (lerror != lwc_error_ok) {
        *abs = NULL;
        nsurl_unref(nsabs);
        return lerror == lwc_error_oom ? CSS_NOMEM : CSS_INVALID;
    }

    nsurl_unref(nsabs);

    return CSS_OK;
}
