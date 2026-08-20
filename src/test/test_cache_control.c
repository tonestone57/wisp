/*
 * Copyright 2019 John-Mark Bell <jmb@netsurf-browser.org>
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

#include <check.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/types.h"
#include "wisp/utils/errors.h"
#include "wisp/utils/corestrings.h"
#include "utils/http/cache-control.h"

static void setup(void)
{
    corestrings_init();
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(test_cache_control_parse_max_age)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Standard valid max-age */
    err = http_parse_cache_control("max-age=3600", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 3600);
    ck_assert_int_eq(http_cache_control_no_cache(cc), false);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Zero max-age */
    err = http_parse_cache_control("max-age=0", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 0);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Max-age integer overflow (clamped to UINT_MAX) */
    err = http_parse_cache_control("max-age=4294967296", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), UINT_MAX);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Non-digit max-age value */
    err = http_parse_cache_control("max-age=invalid", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Empty max-age value */
    err = http_parse_cache_control("max-age=", &cc);
    ck_assert_int_ne(err, NSERROR_OK);
}
END_TEST

START_TEST(test_cache_control_parse_flags)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* no-cache flag only */
    err = http_parse_cache_control("no-cache", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* no-store flag only */
    err = http_parse_cache_control("no-store", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    ck_assert_int_eq(http_cache_control_no_cache(cc), false);
    ck_assert_int_eq(http_cache_control_no_store(cc), true);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Combined no-cache, no-store, and max-age */
    err = http_parse_cache_control("no-cache, no-store, max-age=123", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 123);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), true);
    http_cache_control_destroy(cc);
    cc = NULL;
}
END_TEST

START_TEST(test_cache_control_leading_zero_and_spaces)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Max-age with leading zeros */
    err = http_parse_cache_control("max-age=007200", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 7200);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Max-age with whitespace around equals */
    err = http_parse_cache_control("max-age = 1800", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 1800);
    http_cache_control_destroy(cc);
    cc = NULL;
}
END_TEST

START_TEST(test_cache_control_parse_case_insensitivity)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Upper-case directives */
    err = http_parse_cache_control("MAX-AGE=7200, NO-CACHE, NO-STORE", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 7200);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), true);
    http_cache_control_destroy(cc);
    cc = NULL;
}
END_TEST

START_TEST(test_cache_control_parse_duplicates)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Duplicate max-age */
    err = http_parse_cache_control("max-age=10, max-age=20", &cc);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    /* Duplicate no-cache */
    err = http_parse_cache_control("no-cache, no-cache", &cc);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);

    /* Duplicate extension directive */
    err = http_parse_cache_control("public, public", &cc);
    ck_assert_int_eq(err, NSERROR_NOT_FOUND);
}
END_TEST

START_TEST(test_cache_control_parse_syntax_and_quoted)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Quoted parameter values */
    err = http_parse_cache_control("private=\"foo\", no-cache", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Extra whitespace around tokens and separators */
    err = http_parse_cache_control("  max-age  =  3600  ,  no-cache  ", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 3600);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Extension directives (s-maxage, must-revalidate) */
    err = http_parse_cache_control("s-maxage=3600, must-revalidate, public", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    ck_assert_int_eq(http_cache_control_no_cache(cc), false);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;
}
END_TEST

START_TEST(test_cache_control_quoted_max_age)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Quoted max-age value e.g. max-age="3600" */
    err = http_parse_cache_control("max-age=\"3600\"", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), true);
    ck_assert_int_eq(http_cache_control_max_age(cc), 3600);
    http_cache_control_destroy(cc);
    cc = NULL;
}
END_TEST

START_TEST(test_cache_control_invalid_max_age)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Negative max-age value */
    err = http_parse_cache_control("max-age=-5", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Decimal max-age value */
    err = http_parse_cache_control("max-age=1.5", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;

    /* Trailing non-digits */
    err = http_parse_cache_control("max-age=100s", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;
}
END_TEST

START_TEST(test_cache_control_malformed_headers)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Unterminated string literal */
    err = http_parse_cache_control("max-age=\"3600", &cc);
    ck_assert_int_ne(err, NSERROR_OK);

    /* Invalid token character in directive name */
    err = http_parse_cache_control("max-age=@123", &cc);
    ck_assert_int_ne(err, NSERROR_OK);

    /* Invalid equals syntax without value token */
    err = http_parse_cache_control("no-cache==", &cc);
    ck_assert_int_ne(err, NSERROR_OK);
}
END_TEST

START_TEST(test_cache_control_complex_directives)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* no-cache with field name list and private with field name list */
    err = http_parse_cache_control("no-cache=\"Set-Cookie\", private=\"location\", no-transform, stale-while-revalidate=60", &cc);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(cc);
    ck_assert_int_eq(http_cache_control_no_cache(cc), true);
    ck_assert_int_eq(http_cache_control_no_store(cc), false);
    ck_assert_int_eq(http_cache_control_has_max_age(cc), false);
    http_cache_control_destroy(cc);
    cc = NULL;
}
END_TEST

START_TEST(test_cache_control_empty_header)
{
    nserror err;
    http_cache_control *cc = NULL;

    /* Empty header string */
    err = http_parse_cache_control("", &cc);
    ck_assert_int_ne(err, NSERROR_OK);

    /* Whitespace-only header string */
    err = http_parse_cache_control("   ", &cc);
    ck_assert_int_ne(err, NSERROR_OK);
}
END_TEST

static Suite *cache_control_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("cache-control");
    tc = tcase_create("Core");

    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_cache_control_parse_max_age);
    tcase_add_test(tc, test_cache_control_parse_flags);
    tcase_add_test(tc, test_cache_control_parse_case_insensitivity);
    tcase_add_test(tc, test_cache_control_parse_duplicates);
    tcase_add_test(tc, test_cache_control_parse_syntax_and_quoted);
    tcase_add_test(tc, test_cache_control_quoted_max_age);
    tcase_add_test(tc, test_cache_control_leading_zero_and_spaces);
    tcase_add_test(tc, test_cache_control_invalid_max_age);
    tcase_add_test(tc, test_cache_control_malformed_headers);
    tcase_add_test(tc, test_cache_control_complex_directives);
    tcase_add_test(tc, test_cache_control_empty_header);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(cache_control_suite_create());
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
