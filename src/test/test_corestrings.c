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

#include <dom/dom.h>

#include "utils/corestrings.h"
#include "utils/nsurl.h"
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

START_TEST(corestrings_validity_test)
{
    nserror res;

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    ck_assert_ptr_nonnull(corestring_lwc_a);
    ck_assert_ptr_nonnull(corestring_lwc_html);
    ck_assert_ptr_nonnull(corestring_lwc_body);
    ck_assert_ptr_nonnull(corestring_lwc_div);

    ck_assert_ptr_nonnull(corestring_dom_a);
    ck_assert_ptr_nonnull(corestring_dom_abort);
    ck_assert_ptr_nonnull(corestring_dom_afterprint);
    ck_assert_ptr_nonnull(corestring_dom_align);

    ck_assert_ptr_nonnull(corestring_nsurl_about_blank);
    ck_assert_ptr_nonnull(corestring_nsurl_about_query_ssl);

    size_t len = 0;
    const char *str = lwc_string_data(corestring_lwc_html);
    len = lwc_string_length(corestring_lwc_html);
    ck_assert_int_eq(len, 4);
    ck_assert_int_eq(strncmp(str, "html", 4), 0);

    str = lwc_string_data(corestring_lwc_body);
    len = lwc_string_length(corestring_lwc_body);
    ck_assert_int_eq(len, 4);
    ck_assert_int_eq(strncmp(str, "body", 4), 0);

    lwc_string *test_str;
    lwc_error lerror;

    lerror = lwc_intern_string("div", 3, &test_str);
    ck_assert_int_eq(lerror, lwc_error_ok);

    bool match = false;
    lerror = lwc_string_isequal(corestring_lwc_div, test_str, &match);
    ck_assert_int_eq(lerror, lwc_error_ok);
    ck_assert(match == true);

    lwc_string_unref(test_str);

    dom_string *dstr;
    dom_exception exc;

    exc = dom_string_create_interned((const uint8_t *)"align", 5, &dstr);
    ck_assert_int_eq(exc, DOM_NO_ERR);

    ck_assert(dom_string_isequal(corestring_dom_align, dstr));

    dom_string_unref(dstr);

    ck_assert_int_eq(nsurl_compare(corestring_nsurl_about_blank, corestring_nsurl_about_query_ssl, NSURL_COMPLETE), false);

    res = corestrings_fini();
    ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST

START_TEST(corestrings_idempotency_test)
{
    nserror res;

    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    // Initialising again should be a no-op
    res = corestrings_init();
    ck_assert_int_eq(res, NSERROR_OK);

    res = corestrings_fini();
    ck_assert_int_eq(res, NSERROR_OK);

    // Finalising again should be safe
    res = corestrings_fini();
    ck_assert_int_eq(res, NSERROR_OK);
}
END_TEST


static TCase *corestrings_case_create(void)
{
    TCase *tc;
    tc = tcase_create("corestrings");

    tcase_add_loop_test(tc, corestrings_test, CORESTRING_TEST_COUNT, CORESTRING_TEST_COUNT + 1);
    tcase_add_test(tc, corestrings_validity_test);
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
