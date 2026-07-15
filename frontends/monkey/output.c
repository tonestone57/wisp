/*
 * Copyright 2018 Vincent Sanders <vince@nexturf-browser.org>
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

#include <stdarg.h>
#include <stdio.h>

#include "monkey/output.h"

/**
 * output type prefixes
 */
static const char *type_text[] = {
    "DIE",
    "ERROR",
    "WARN",
    "GENERIC",
    "WINDOW",
    "LOGIN",
    "DOWNLOAD",
    "PLOT",
};

static int critical_error_count = 0;

/* exported interface documented in monkey/output.h */
int moutf(enum monkey_output_type mout_type, const char *fmt, ...)
{
    va_list ap;
    int res;

    if (mout_type == MOUT_ERROR || mout_type == MOUT_DIE) {
        critical_error_count++;
    }

    res = fprintf(stdout, "%s ", type_text[mout_type]);

    va_start(ap, fmt);
    res += vfprintf(stdout, fmt, ap);
    va_end(ap);

    fputc('\n', stdout);

    return res + 1;
}

int monkey_get_critical_error_count(void)
{
    return critical_error_count;
}
