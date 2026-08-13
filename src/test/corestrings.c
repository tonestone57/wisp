/*
 * Copyright 2016 Vincent Sanders <vince@netsurf-browser.org>
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
 * Tests for corestrings.
 */

#include "utils/config.h"

#include <assert.h>
#include <check.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/corestrings.h"
#include <dom/dom.h>
#include "wisp/utils/nsurl.h"

#include "test/malloc_fig.h"

/**
 * The number of corestrings.
 *
 * This is used to test all the out of memory paths in initialisation.
 */
#define CORESTRING_TEST_COUNT 2000

START_TEST(corestrings_test)
{
    nserror ires;
    nserror res;

    malloc_limit(_i);

    ires = corestrings_init();
    res = corestrings_fini();

    malloc_limit(UINT_MAX);

    if (_i < CORESTRING_TEST_COUNT) {
        ck_assert_int_eq(ires, NSERROR_NOMEM);
    } else {
        ck_assert_int_eq(ires, NSERROR_OK);
    }
    ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST

START_TEST(corestrings_namespaces_test)
{
    nserror res;

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    /* Validate lwc_string namespace */
    ck_assert_ptr_nonnull(corestring_lwc_a);
    ck_assert_int_eq(lwc_string_length(corestring_lwc_a), 1);
    ck_assert_str_eq(lwc_string_data(corestring_lwc_a), "a");

    /* Validate dom_string namespace */
    ck_assert_ptr_nonnull(corestring_dom_a);
    ck_assert_int_eq(dom_string_byte_length(corestring_dom_a), 1);

    /* Validate nsurl namespace */
    ck_assert_ptr_nonnull(corestring_nsurl_about_blank);
    const char *about_blank_str = nsurl_access(corestring_nsurl_about_blank);
    ck_assert_ptr_nonnull(about_blank_str);
    ck_assert_str_eq(about_blank_str, "about:blank");

    res = corestrings_fini();
    ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST

START_TEST(corestrings_idempotency_test)
{
    nserror res;

    /* First initialization */
    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);
    ck_assert_ptr_nonnull(corestring_lwc_a);

    /* Second initialization (should be idempotent) */
    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    /* Ensure state is still valid */
    ck_assert_ptr_nonnull(corestring_lwc_a);
    ck_assert_int_eq(lwc_string_length(corestring_lwc_a), 1);

    /* Finalize */
    res = corestrings_fini();
    ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST


static TCase *corestrings_case_create(void)
{
    TCase *tc;
    tc = tcase_create("corestrings");

    tcase_add_loop_test(tc, corestrings_test, CORESTRING_TEST_COUNT, CORESTRING_TEST_COUNT + 1);
    tcase_add_test(tc, corestrings_namespaces_test);
    tcase_add_test(tc, corestrings_idempotency_test);

    return tc;
}


/*
 * corestrings test suite creation
 */
static Suite *corestrings_suite_create(void)
{
    Suite *s;
    s = suite_create("Corestrings API");

    suite_add_tcase(s, corestrings_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(corestrings_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
