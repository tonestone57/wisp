/*
 * Copyright 2024 NetSurf Contributors
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
 * Test nsurl parse operations.
 */

#include <assert.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libwapcaplet/libwapcaplet.h>

#include "utils/corestrings.h"
#include "utils/nsurl.h"
#include "utils/errors.h"

static void setup(void)
{
    ck_assert(corestrings_init() == NSERROR_OK);
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(nsurl_parse_components_char_test)
{
    nserror err;
    nsurl *url = NULL;

    lwc_string *scheme;
    lwc_string *host;

    lwc_intern_string("http", 4, &scheme);
    lwc_intern_string("www.example.com", 15, &host);

    err = nsurl_create_from_components_char(scheme, host, "8080", "/path?query=1#frag", &url);
    ck_assert(err == NSERROR_OK);
    ck_assert(url != NULL);
    ck_assert_str_eq(nsurl_access(url), "http://www.example.com:8080/path?query=1#frag");
    nsurl_unref(url);

    /* Test invalid host */
    lwc_string *bad_host;
    lwc_intern_string("invalid host", 12, &bad_host);
    err = nsurl_create_from_components_char(scheme, bad_host, "8080", "/path", &url);
    ck_assert(err == NSERROR_BAD_URL);
    lwc_string_unref(bad_host);

    /* Test missing host for HTTP */
    err = nsurl_create_from_components_char(scheme, NULL, NULL, "/path", &url);
    ck_assert(err == NSERROR_BAD_PARAMETER);

    lwc_string_unref(scheme);
    lwc_string_unref(host);
}
END_TEST

START_TEST(nsurl_parse_components_str_test)
{
    nserror err;
    nsurl *url = NULL;

    lwc_string *scheme;
    lwc_string *host;
    lwc_string *port;
    lwc_string *path;
    lwc_string *query;
    lwc_string *fragment;

    lwc_intern_string("https", 5, &scheme);
    lwc_intern_string("example.org", 11, &host);
    lwc_intern_string("443", 3, &port);
    lwc_intern_string("/foo/bar", 8, &path);
    lwc_intern_string("q=test", 6, &query);
    lwc_intern_string("section1", 8, &fragment);

    err = nsurl_create_from_components_str(scheme, host, port, path, query, fragment, &url);
    ck_assert(err == NSERROR_OK);
    ck_assert(url != NULL);
    ck_assert_str_eq(nsurl_access(url), "https://example.org:443/foo/bar?q=test#section1");
    nsurl_unref(url);

    lwc_string_unref(scheme);
    lwc_string_unref(host);
    lwc_string_unref(port);
    lwc_string_unref(path);
    lwc_string_unref(query);
    lwc_string_unref(fragment);
}
END_TEST

START_TEST(nsurl_parse_join_test)
{
    nserror err;
    nsurl *base = NULL;
    nsurl *joined = NULL;

    err = nsurl_create("http://example.com/dir/page.html", &base);
    ck_assert(err == NSERROR_OK);
    ck_assert(base != NULL);

    /* Join absolute URL */
    err = nsurl_join(base, "https://other.com/foo", &joined);
    ck_assert(err == NSERROR_OK);
    ck_assert_str_eq(nsurl_access(joined), "https://other.com/foo");
    nsurl_unref(joined);

    /* Join relative path */
    err = nsurl_join(base, "other.html", &joined);
    ck_assert(err == NSERROR_OK);
    ck_assert_str_eq(nsurl_access(joined), "http://example.com/dir/other.html");
    nsurl_unref(joined);

    /* Join absolute path */
    err = nsurl_join(base, "/root.html", &joined);
    ck_assert(err == NSERROR_OK);
    ck_assert_str_eq(nsurl_access(joined), "http://example.com/root.html");
    nsurl_unref(joined);

    /* Join query */
    err = nsurl_join(base, "?q=1", &joined);
    ck_assert(err == NSERROR_OK);
    ck_assert_str_eq(nsurl_access(joined), "http://example.com/dir/page.html?q=1");
    nsurl_unref(joined);

    /* Join fragment */
    err = nsurl_join(base, "#frag", &joined);
    ck_assert(err == NSERROR_OK);
    ck_assert_str_eq(nsurl_access(joined), "http://example.com/dir/page.html#frag");
    nsurl_unref(joined);

    nsurl_unref(base);
}
END_TEST

static TCase *nsurl_parse_case_create(void)
{
    TCase *tc;
    tc = tcase_create("NSURL Parse");

    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, nsurl_parse_components_char_test);
    tcase_add_test(tc, nsurl_parse_components_str_test);
    tcase_add_test(tc, nsurl_parse_join_test);

    return tc;
}

static Suite *nsurl_parse_suite_create(void)
{
    Suite *s;
    s = suite_create("nsurl_parse");

    suite_add_tcase(s, nsurl_parse_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(nsurl_parse_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
