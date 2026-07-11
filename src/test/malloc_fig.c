/*
 * Copyright 2017 Vincent Sanders <vince@netsurf-browser.org>
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
 *
 * heap fault injection generation.
 *
 * This library inject allocation faults into NetSurf tests
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "test/malloc_fig.h"

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define WISP_SANITIZER_ENABLED
#elif defined(__has_feature)
  #if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
  #define WISP_SANITIZER_ENABLED
  #endif
#endif

static unsigned int count = UINT_MAX;

void malloc_limit(unsigned int newcount)
{
    count = newcount;
}

#ifndef WISP_SANITIZER_ENABLED

#ifdef __GLIBC__
extern void *__libc_malloc(size_t size);
#endif

void *malloc(size_t size)
{
    static void *(*real_malloc)(size_t) = NULL;
    void *p = NULL;

#ifdef __GLIBC__
    real_malloc = __libc_malloc;
#else
    if (real_malloc == NULL) {
        static int in_dlsym = 0;
        if (in_dlsym) {
            static char fallback_buf[8192];
            static size_t fallback_ptr = 0;
            if (fallback_ptr + size > sizeof(fallback_buf)) {
                return NULL;
            }
            void *ret = &fallback_buf[fallback_ptr];
            fallback_ptr += (size + 7) & ~7;
            return ret;
        }
        in_dlsym = 1;
        real_malloc = dlsym(RTLD_NEXT, "malloc");
        in_dlsym = 0;
    }
#endif

    if (count > 0) {
        p = real_malloc(size);
        count--;
    }
    return p;
}

#endif /* !WISP_SANITIZER_ENABLED */
