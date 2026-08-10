/*
 * Copyright 2004-2007 James Bursa <bursa@users.sourceforge.net>
 * Copyright 2004 John Tytgat <joty@netsurf-browser.org>
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
 * \brief Interface to a number of general purpose functionality.
 */

#ifndef WISP_UTILS_UTILS_H
#define WISP_UTILS_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wisp/ns_inttypes.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/macros.h>
#include <wisp/utils/math.h>

/* Windows does not have POSIX mkdir so work around that */
#if defined(_WIN32)
/** windows mkdir function */
#define nsmkdir(dir, mode) mkdir((dir))
#else
/** POSIX mkdir function */
#define nsmkdir(dir, mode) mkdir((dir), (mode))
#endif

/**
 * Format a float into a string with '.' decimal separator, regardless of locale.
 *
 * Some locales use ',' as the decimal separator in snprintf %f output, but
 * data-interchange formats (SVG, XML, JSON, CSS) always require '.'.
 *
 * \param buf   Output buffer
 * \param size  Buffer size
 * \param fmt   printf format string (e.g. "%.2f")
 * \param val   Value to format
 * \return      Number of characters written (same as snprintf)
 */
int nsfmt_float(char *buf, size_t size, const char *fmt, double val);

/**
 * Stable sort using insertion sort algorithm.
 *
 * Unlike qsort, this maintains relative order of elements with equal keys.
 *
 * \param base   Pointer to the array to sort
 * \param nmemb  Number of elements in the array
 * \param size   Size of each element in bytes
 * \param compar Comparison function (same signature as qsort)
 */
void stable_sort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));

/**
 * Check if a directory exists.
 */
bool is_dir(const char *path);

/**
 * Convert string to integer safely.
 *
 * \param s      String to convert
 * \param base   Base to use
 * \param result Pointer to store result
 * \return NSERROR_OK on success, or error code
 */
nserror ns_strtoint(const char *s, int base, int *result);

/**
 * Convert string to unsigned integer safely.
 *
 * \param s      String to convert
 * \param base   Base to use
 * \param result Pointer to store result
 * \return NSERROR_OK on success, or error code
 */
nserror ns_strtouint(const char *s, int base, unsigned int *result);

/**
 * Convert string to long long safely.
 */
nserror ns_strtoll(const char *s, int base, long long *result);

/**
 * Convert string to unsigned long long safely.
 */
nserror ns_strtoull(const char *s, int base, unsigned long long *result);

#endif
