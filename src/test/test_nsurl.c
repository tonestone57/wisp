/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of NetSurf / Wisp.
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
 * Comprehensive unit tests for nsurl operations in src/utils/nsurl/nsurl.c.
 */

#include <assert.h>
#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libwapcaplet/libwapcaplet.h>

#include "utils/corestrings.h"
#include "utils/errors.h"
#include "utils/nsurl.h"

static void setup(void)
{
    ck_assert_int_eq(corestrings_init(), NSERROR_OK);
}

static void teardown(void)
{
    corestrings_fini();
}

START_TEST(test_nsurl_ref_unref)
{
    nserror err;
    nsurl *url = NULL;
    nsurl *ref = NULL;

    err = nsurl_create("http://www.example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(url);

    ref = nsurl_ref(url);
    ck_assert_ptr_eq(ref, url);

    nsurl_unref(ref);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_access_length)
{
    nserror err;
    nsurl *url = NULL;

    err = nsurl_create("http://www.example.com/test/page.html", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_str_eq(nsurl_access(url), "http://www.example.com/test/page.html");
    ck_assert_uint_eq(nsurl_length(url), strlen("http://www.example.com/test/page.html"));

    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_access_log)
{
    nserror err;
    nsurl *url = NULL;

    /* NULL URL log string */
    ck_assert_str_eq(nsurl_access_log(NULL), "(null)");

    /* Data scheme URL log string */
    err = nsurl_create("data:text/plain,Hello%20World", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access_log(url), "[data url]");
    nsurl_unref(url);

    /* Regular HTTP URL log string */
    err = nsurl_create("http://www.example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access_log(url), "http://www.example.com/");
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_hash)
{
    nserror err;
    nsurl *url1 = NULL;
    nsurl *url2 = NULL;
    nsurl *url3 = NULL;

    err = nsurl_create("http://www.example.com/foo", &url1);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_create("http://www.example.com/foo", &url2);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_create("http://www.example.com/bar", &url3);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_uint_ne(nsurl_hash(url1), 0);
    ck_assert_uint_eq(nsurl_hash(url1), nsurl_hash(url2));
    ck_assert_uint_ne(nsurl_hash(url1), nsurl_hash(url3));

    nsurl_unref(url1);
    nsurl_unref(url2);
    nsurl_unref(url3);
}
END_TEST

START_TEST(test_nsurl_components_has_get)
{
    nserror err;
    nsurl *url = NULL;
    lwc_string *comp = NULL;

    /* Full URL with user, pass, host, port, path, query, fragment */
    err = nsurl_create("http://user:pass@example.com:8080/path/to/res?q=1#frag", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Check presence of components */
    ck_assert(nsurl_has_component(url, NSURL_SCHEME));
    ck_assert(nsurl_has_component(url, NSURL_USERNAME));
    ck_assert(nsurl_has_component(url, NSURL_PASSWORD));
    ck_assert(nsurl_has_component(url, NSURL_CREDENTIALS));
    ck_assert(nsurl_has_component(url, NSURL_HOST));
    ck_assert(nsurl_has_component(url, NSURL_PORT));
    ck_assert(nsurl_has_component(url, NSURL_PATH));
    ck_assert(nsurl_has_component(url, NSURL_QUERY));
    ck_assert(nsurl_has_component(url, NSURL_FRAGMENT));

    /* Check invalid component enum value */
    ck_assert(!nsurl_has_component(url, (nsurl_component)9999));

    /* Check component getters */
    comp = nsurl_get_component(url, NSURL_SCHEME);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "http");
    lwc_string_unref(comp);

    comp = nsurl_get_component(url, NSURL_USERNAME);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "user");
    lwc_string_unref(comp);

    comp = nsurl_get_component(url, NSURL_PASSWORD);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "pass");
    lwc_string_unref(comp);

    comp = nsurl_get_component(url, NSURL_HOST);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "example.com");
    lwc_string_unref(comp);

    comp = nsurl_get_component(url, NSURL_PORT);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "8080");
    lwc_string_unref(comp);

    comp = nsurl_get_component(url, NSURL_PATH);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "/path/to/res");
    lwc_string_unref(comp);

    comp = nsurl_get_component(url, NSURL_QUERY);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "q=1");
    lwc_string_unref(comp);

    comp = nsurl_get_component(url, NSURL_FRAGMENT);
    ck_assert_ptr_nonnull(comp);
    ck_assert_str_eq(lwc_string_data(comp), "frag");
    lwc_string_unref(comp);

    /* Invalid component getter */
    comp = nsurl_get_component(url, (nsurl_component)9999);
    ck_assert_ptr_null(comp);

    nsurl_unref(url);

    /* Minimal URL without optional components */
    err = nsurl_create("http://example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert(!nsurl_has_component(url, NSURL_USERNAME));
    ck_assert(!nsurl_has_component(url, NSURL_PASSWORD));
    ck_assert(!nsurl_has_component(url, NSURL_CREDENTIALS));
    ck_assert(!nsurl_has_component(url, NSURL_PORT));
    ck_assert(!nsurl_has_component(url, NSURL_QUERY));
    ck_assert(!nsurl_has_component(url, NSURL_FRAGMENT));

    ck_assert_ptr_null(nsurl_get_component(url, NSURL_USERNAME));
    ck_assert_ptr_null(nsurl_get_component(url, NSURL_PASSWORD));
    ck_assert_ptr_null(nsurl_get_component(url, NSURL_PORT));
    ck_assert_ptr_null(nsurl_get_component(url, NSURL_QUERY));
    ck_assert_ptr_null(nsurl_get_component(url, NSURL_FRAGMENT));

    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_get_scheme_type)
{
    nserror err;
    nsurl *url = NULL;

    ck_assert_int_eq(nsurl_get_scheme_type(NULL), NSURL_SCHEME_OTHER);

    err = nsurl_create("http://example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_get_scheme_type(url), NSURL_SCHEME_HTTP);
    nsurl_unref(url);

    err = nsurl_create("https://example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_get_scheme_type(url), NSURL_SCHEME_HTTPS);
    nsurl_unref(url);

    err = nsurl_create("file:///etc/hosts", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_get_scheme_type(url), NSURL_SCHEME_FILE);
    nsurl_unref(url);

    err = nsurl_create("ftp://ftp.example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_get_scheme_type(url), NSURL_SCHEME_FTP);
    nsurl_unref(url);

    err = nsurl_create("mailto:user@example.com", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_get_scheme_type(url), NSURL_SCHEME_MAILTO);
    nsurl_unref(url);

    err = nsurl_create("data:text/plain,test", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_get_scheme_type(url), NSURL_SCHEME_DATA);
    nsurl_unref(url);

    err = nsurl_create("about:blank", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_get_scheme_type(url), NSURL_SCHEME_OTHER);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_get_string)
{
    nserror err;
    nsurl *url = NULL;
    char *out_s = NULL;
    size_t out_l = 0;

    err = nsurl_create("http://user:pass@example.com:8080/path?q=1#frag", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Scheme + Host */
    err = nsurl_get(url, NSURL_SCHEME | NSURL_HOST, &out_s, &out_l);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(out_s, "http://example.com");
    ck_assert_uint_eq(out_l, strlen("http://example.com"));
    free(out_s);

    /* Path + Query */
    err = nsurl_get(url, NSURL_PATH | NSURL_QUERY, &out_s, &out_l);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(out_s, "/path?q=1");
    free(out_s);

    /* Complete minus password */
    err = nsurl_get(url, NSURL_COMPLETE & ~NSURL_PASSWORD, &out_s, &out_l);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(out_s, "http://user@example.com:8080/path?q=1");
    free(out_s);

    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_get_utf8)
{
    nserror err;
    nsurl *url = NULL;
    char *out_s = NULL;
    size_t out_l = 0;

    /* Parameter validation */
    err = nsurl_create("http://example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert_int_eq(nsurl_get_utf8(NULL, &out_s, &out_l), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(nsurl_get_utf8(url, NULL, &out_l), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(nsurl_get_utf8(url, &out_s, NULL), NSERROR_BAD_PARAMETER);

    nsurl_unref(url);

    /* Hostless URL (file scheme) */
    err = nsurl_create("file:///etc/fstab", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = nsurl_get_utf8(url, &out_s, &out_l);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(out_s, "file:///etc/fstab");
    ck_assert_uint_eq(out_l, strlen("file:///etc/fstab"));
    free(out_s);
    nsurl_unref(url);

    /* Standard ASCII URL */
    err = nsurl_create("http://example.com/test", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = nsurl_get_utf8(url, &out_s, &out_l);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(out_s, "http://example.com/test");
    free(out_s);
    nsurl_unref(url);

    /* IDN punycode URL */
    err = nsurl_create("http://a.xn--11b4c3d/a", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    err = nsurl_get_utf8(url, &out_s, &out_l);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(out_s, "http://a.कॉम/a");
    free(out_s);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_access_leaf)
{
    nserror err;
    nsurl *url = NULL;

    /* URL with normal path and leaf */
    err = nsurl_create("http://example.com/a/b/document.pdf", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access_leaf(url), "document.pdf");
    nsurl_unref(url);

    /* URL ending in slash */
    err = nsurl_create("http://example.com/a/b/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access_leaf(url), "");
    nsurl_unref(url);

    /* Root path */
    err = nsurl_create("http://example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access_leaf(url), "/");
    nsurl_unref(url);

    /* Scheme with path segment */
    err = nsurl_create("about:blank", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access_leaf(url), "blank");
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_defragment)
{
    nserror err;
    nsurl *url = NULL;
    nsurl *no_frag = NULL;

    /* Defragmenting a URL that HAS a fragment */
    err = nsurl_create("http://example.com/page.html#section2", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_defragment(url, &no_frag);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(no_frag), "http://example.com/page.html");
    ck_assert(!nsurl_has_component(no_frag, NSURL_FRAGMENT));

    nsurl_unref(no_frag);
    nsurl_unref(url);

    /* Defragmenting a URL that HAS NO fragment */
    err = nsurl_create("http://example.com/page.html", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_defragment(url, &no_frag);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_eq(no_frag, url); /* Returns same pointer with increased refcount */

    nsurl_unref(no_frag);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_refragment)
{
    nserror err;
    nsurl *url = NULL;
    nsurl *new_url = NULL;
    lwc_string *frag = NULL;

    err = lwc_intern_string("newfrag", 7, &frag);
    ck_assert_int_eq(err, lwc_error_ok);

    /* Refragmenting a URL that already has a fragment */
    err = nsurl_create("http://example.com/page.html#oldfrag", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_refragment(url, frag, &new_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(new_url), "http://example.com/page.html#newfrag");

    nsurl_unref(new_url);
    nsurl_unref(url);

    /* Refragmenting a URL that has no fragment */
    err = nsurl_create("http://example.com/page.html", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_refragment(url, frag, &new_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(new_url), "http://example.com/page.html#newfrag");

    nsurl_unref(new_url);
    nsurl_unref(url);

    lwc_string_unref(frag);
}
END_TEST

START_TEST(test_nsurl_replace_query)
{
    nserror err;
    nsurl *url = NULL;
    nsurl *new_url = NULL;

    /* Replacing existing query */
    err = nsurl_create("http://example.com/search?q=old#frag", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_replace_query(url, "q=new&lang=en", &new_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(new_url), "http://example.com/search?q=new&lang=en#frag");
    nsurl_unref(new_url);

    /* Replacing query with empty string (removing query) */
    err = nsurl_replace_query(url, "", &new_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(new_url), "http://example.com/search#frag");
    nsurl_unref(new_url);

    nsurl_unref(url);

    /* Adding query to URL without query */
    err = nsurl_create("http://example.com/search#frag", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_replace_query(url, "a=1", &new_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(new_url), "http://example.com/search?a=1#frag");

    nsurl_unref(new_url);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_replace_scheme)
{
    nserror err;
    nsurl *url = NULL;
    nsurl *new_url = NULL;
    lwc_string *https_scheme = NULL;
    lwc_string *file_scheme = NULL;

    /* NULL parameters */
    err = lwc_intern_string("https", 5, &https_scheme);
    ck_assert_int_eq(err, lwc_error_ok);
    ck_assert_int_eq(nsurl_replace_scheme(NULL, https_scheme, &new_url), NSERROR_BAD_PARAMETER);

    err = nsurl_create("http://example.com/path", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_int_eq(nsurl_replace_scheme(url, NULL, &new_url), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(nsurl_replace_scheme(url, https_scheme, NULL), NSERROR_BAD_PARAMETER);

    /* Replace HTTP with HTTPS */
    err = nsurl_replace_scheme(url, https_scheme, &new_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(new_url), "https://example.com/path");
    ck_assert_int_eq(nsurl_get_scheme_type(new_url), NSURL_SCHEME_HTTPS);
    nsurl_unref(new_url);

    /* Replace with file scheme */
    err = lwc_intern_string("file", 4, &file_scheme);
    ck_assert_int_eq(err, lwc_error_ok);
    err = nsurl_replace_scheme(url, file_scheme, &new_url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(new_url), "file://example.com/path");
    ck_assert_int_eq(nsurl_get_scheme_type(new_url), NSURL_SCHEME_FILE);
    nsurl_unref(new_url);

    nsurl_unref(url);
    lwc_string_unref(https_scheme);
    lwc_string_unref(file_scheme);
}
END_TEST

START_TEST(test_nsurl_nice)
{
    nserror err;
    nsurl *url = NULL;
    char *res = NULL;

    /* Normal URL with path */
    err = nsurl_create("http://www.example.com/downloads/manual.pdf", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_nice(url, &res, false);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(res, "manual.pdf");
    free(res);

    err = nsurl_nice(url, &res, true);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(res, "manual");
    free(res);
    nsurl_unref(url);

    /* URL with host only */
    err = nsurl_create("http://www.example.org/", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_nice(url, &res, false);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(res, "www_example_org");
    free(res);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_parent)
{
    nserror err;
    nsurl *url = NULL;
    nsurl *parent = NULL;

    /* Parent of subpath with query and fragment */
    err = nsurl_create("http://example.com/dir1/dir2/page.html?q=1#section", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_parent(url, &parent);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(parent), "http://example.com/dir1/dir2/");

    nsurl_unref(parent);
    nsurl_unref(url);

    /* Parent of single path segment */
    err = nsurl_create("http://example.com/dir1", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_parent(url, &parent);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_str_eq(nsurl_access(parent), "http://example.com/");

    nsurl_unref(parent);
    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_compare)
{
    nserror err;
    nsurl *url1 = NULL;
    nsurl *url2 = NULL;

    err = nsurl_create("http://user:pass@example.com:8080/path?q=1#frag", &url1);
    ck_assert_int_eq(err, NSERROR_OK);

    err = nsurl_create("http://user:pass@example.com:8080/path?q=1#frag", &url2);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Compare identical URLs */
    ck_assert(nsurl_compare(url1, url2, NSURL_COMPLETE));
    ck_assert(nsurl_compare(url1, url2, NSURL_WITH_FRAGMENT));

    nsurl_unref(url2);

    /* Compare URL with different query */
    err = nsurl_create("http://user:pass@example.com:8080/path?q=2#frag", &url2);
    ck_assert_int_eq(err, NSERROR_OK);

    ck_assert(!nsurl_compare(url1, url2, NSURL_QUERY));
    ck_assert(!nsurl_compare(url1, url2, NSURL_COMPLETE));
    ck_assert(nsurl_compare(url1, url2, NSURL_HOST));
    ck_assert(nsurl_compare(url1, url2, NSURL_PATH));

    nsurl_unref(url1);
    nsurl_unref(url2);
}
END_TEST

START_TEST(test_nsurl_dump)
{
    nserror err;
    nsurl *url = NULL;

    err = nsurl_create("http://user:pass@example.com:8080/path?q=1#frag", &url);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Call nsurl_dump to ensure it runs cleanly */
    nsurl_dump(url);

    nsurl_unref(url);
}
END_TEST

START_TEST(test_nsurl_has_component_comprehensive)
{
    nserror err;
    nsurl *url = NULL;

    /* 1. Full URL with all components present */
    err = nsurl_create("http://user:pass@example.com:8080/path/to/res?q=1#frag", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert(nsurl_has_component(url, NSURL_SCHEME));
    ck_assert(nsurl_has_component(url, NSURL_USERNAME));
    ck_assert(nsurl_has_component(url, NSURL_PASSWORD));
    ck_assert(nsurl_has_component(url, NSURL_CREDENTIALS));
    ck_assert(nsurl_has_component(url, NSURL_HOST));
    ck_assert(nsurl_has_component(url, NSURL_PORT));
    ck_assert(nsurl_has_component(url, NSURL_PATH));
    ck_assert(nsurl_has_component(url, NSURL_QUERY));
    ck_assert(nsurl_has_component(url, NSURL_FRAGMENT));

    /* Unsupported / invalid part parameters for switch */
    ck_assert(!nsurl_has_component(url, (nsurl_component)0));
    ck_assert(!nsurl_has_component(url, (nsurl_component)-1));
    ck_assert(!nsurl_has_component(url, (nsurl_component)9999));
    ck_assert(!nsurl_has_component(url, NSURL_HOST | NSURL_PORT));
    nsurl_unref(url);

    /* 2. Username only (no password) */
    err = nsurl_create("http://user@example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert(nsurl_has_component(url, NSURL_USERNAME));
    ck_assert(!nsurl_has_component(url, NSURL_PASSWORD));
    ck_assert(nsurl_has_component(url, NSURL_CREDENTIALS));
    nsurl_unref(url);

    /* 4. Minimal URL with no optional components */
    err = nsurl_create("http://example.com/", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert(nsurl_has_component(url, NSURL_SCHEME));
    ck_assert(nsurl_has_component(url, NSURL_HOST));
    ck_assert(nsurl_has_component(url, NSURL_PATH));
    ck_assert(!nsurl_has_component(url, NSURL_USERNAME));
    ck_assert(!nsurl_has_component(url, NSURL_PASSWORD));
    ck_assert(!nsurl_has_component(url, NSURL_CREDENTIALS));
    ck_assert(!nsurl_has_component(url, NSURL_PORT));
    ck_assert(!nsurl_has_component(url, NSURL_QUERY));
    ck_assert(!nsurl_has_component(url, NSURL_FRAGMENT));
    nsurl_unref(url);

    /* 5. Hostless URL (file scheme) */
    err = nsurl_create("file:///etc/hosts", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert(nsurl_has_component(url, NSURL_SCHEME));
    ck_assert(!nsurl_has_component(url, NSURL_HOST));
    ck_assert(nsurl_has_component(url, NSURL_PATH));
    ck_assert(!nsurl_has_component(url, NSURL_PORT));
    ck_assert(!nsurl_has_component(url, NSURL_QUERY));
    ck_assert(!nsurl_has_component(url, NSURL_FRAGMENT));
    nsurl_unref(url);

    /* 6. Query present without fragment */
    err = nsurl_create("http://example.com/search?q=test", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert(nsurl_has_component(url, NSURL_QUERY));
    ck_assert(!nsurl_has_component(url, NSURL_FRAGMENT));
    nsurl_unref(url);

    /* 7. Fragment present without query */
    err = nsurl_create("http://example.com/doc#section", &url);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert(!nsurl_has_component(url, NSURL_QUERY));
    ck_assert(nsurl_has_component(url, NSURL_FRAGMENT));
    nsurl_unref(url);
}
END_TEST

static TCase *nsurl_core_case_create(void)
{
    TCase *tc;
    tc = tcase_create("NSURL Core");

    tcase_add_checked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_nsurl_ref_unref);
    tcase_add_test(tc, test_nsurl_access_length);
    tcase_add_test(tc, test_nsurl_access_log);
    tcase_add_test(tc, test_nsurl_hash);
    tcase_add_test(tc, test_nsurl_components_has_get);
    tcase_add_test(tc, test_nsurl_get_scheme_type);
    tcase_add_test(tc, test_nsurl_get_string);
    tcase_add_test(tc, test_nsurl_get_utf8);
    tcase_add_test(tc, test_nsurl_access_leaf);
    tcase_add_test(tc, test_nsurl_defragment);
    tcase_add_test(tc, test_nsurl_refragment);
    tcase_add_test(tc, test_nsurl_replace_query);
    tcase_add_test(tc, test_nsurl_replace_scheme);
    tcase_add_test(tc, test_nsurl_nice);
    tcase_add_test(tc, test_nsurl_parent);
    tcase_add_test(tc, test_nsurl_compare);
    tcase_add_test(tc, test_nsurl_dump);
    tcase_add_test(tc, test_nsurl_has_component_comprehensive);

    return tc;
}

static Suite *test_nsurl_suite_create(void)
{
    Suite *s;
    s = suite_create("test_nsurl");

    suite_add_tcase(s, nsurl_core_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(test_nsurl_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
