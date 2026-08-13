/*
 * Copyright 2008 Daniel Silverstone <dsilvers@netsurf-browser.org>
 * Copyright 2009 Mark Benjamin <netsurf-browser.org.MarkBenjamin@dfgh.net>
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

#include <winsock2.h>
#include <windows.h>

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wisp/utils/filepath.h"
#include "wisp/utils/log.h"
#include "wisp/utils/nsurl.h"
#include "wisp/utils/utils.h"

#include "windows/findfile.h"

static char *realpath(const char *path, char *resolved_path)
{
    /* useless, but there we go */
    snprintf(resolved_path, PATH_MAX, "%s", path);
    return resolved_path;
}


/**
 * Locate a shared resource file by searching known places in order.
 *
 * Search order is: ~/.netsurf/, $NETSURFRES/ (where NETSURFRES is an
 * environment variable), then the path specified in
 * NETSURF_WINDOWS_RESPATH in the Makefile then .\\res\\ [windows paths]
 *
 * \param  buf      buffer to write to.  must be at least PATH_MAX chars
 * \param  filename file to look for
 * \param  def      default to return if file not found
 * \return The passed in buffer
 */

char *nsws_find_resource(char *buf, const char *filename, const char *def)
{
    char *cdir = getenv("HOME");
    char t[PATH_MAX];

    if (cdir != NULL) {
        NSLOG(wisp, INFO, "Found Home %s", cdir);
        snprintf(t, PATH_MAX, "%s/.wisp/%s", cdir, filename);
        if ((realpath(t, buf) != NULL) && (access(buf, R_OK) == 0))
            return buf;
    }

    cdir = getenv("WISPRES");

    if (cdir != NULL) {
        snprintf(t, PATH_MAX, "%s/%s", cdir, filename);
        if (realpath(t, buf) != NULL) {
            if (access(buf, R_OK) == 0)
                return buf;
        }
    }

    snprintf(t, PATH_MAX, "%s%s", WISP_WINDOWS_RESPATH, filename);
    if ((realpath(t, buf) != NULL) && (access(buf, R_OK) == 0))
        return buf;

    if (getcwd(t, PATH_MAX) != NULL) {
        char temp[PATH_MAX];
        snprintf(temp, PATH_MAX, "%s\\res\\%s", t, filename);
        snprintf(t, PATH_MAX, "%s", temp);
        NSLOG(wisp, INFO, "looking in %s", t);
        if ((realpath(t, buf) != NULL) && (access(buf, R_OK) == 0))
            return buf;
    }

    if (def[0] == '~') {
        const char *home = getenv("HOME");
        snprintf(t, PATH_MAX, "%s%s", home ? home : "", def + 1);
        if (realpath(t, buf) == NULL) {
            snprintf(buf, PATH_MAX, "%s", t);
        }
    } else {
        if (realpath(def, buf) == NULL) {
            snprintf(buf, PATH_MAX, "%s", def);
        }
    }

    return buf;
}


/*
 * Local Variables:
 * c-basic-offset: 8
 * End:
 */
