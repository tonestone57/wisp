/*
 * Copyright 2024
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
 * Test ssl_certs operations.
 */

#include <assert.h>
#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/utils/errors.h>
#include <wisp/utils/nsurl.h>
#include "wisp/ssl_certs.h"

START_TEST(cert_chain_alloc_test)
{
    nserror err;
    struct cert_chain *chain;

    /* Test depth 0 */
    err = cert_chain_alloc(0, &chain);
    ck_assert(err == NSERROR_OK);
    ck_assert_ptr_nonnull(chain);
    ck_assert_int_eq(chain->depth, 0);
    ck_assert_ptr_null(chain->certs);

    cert_chain_free(chain);

    /* Test depth > 0 */
    err = cert_chain_alloc(3, &chain);
    ck_assert(err == NSERROR_OK);
    ck_assert_ptr_nonnull(chain);
    ck_assert_int_eq(chain->depth, 3);
    ck_assert_ptr_nonnull(chain->certs);

    cert_chain_free(chain);
}
END_TEST


START_TEST(cert_chain_to_query_empty_test)
{
    nserror err;
    struct cert_chain *chain;
    struct nsurl *url;

    err = cert_chain_alloc(0, &chain);
    ck_assert(err == NSERROR_OK);

    err = cert_chain_to_query(chain, &url);
    ck_assert(err == NSERROR_OK);

    const char *url_str = nsurl_access(url);
    ck_assert_str_eq(url_str, "about:certificate");

    nsurl_unref(url);
    cert_chain_free(chain);
}
END_TEST

START_TEST(cert_chain_to_query_single_cert_test)
{
    nserror err;
    struct cert_chain *chain;
    struct nsurl *url;

    err = cert_chain_alloc(1, &chain);
    ck_assert(err == NSERROR_OK);

    chain->certs[0].der = (uint8_t *)malloc(3);
    memcpy(chain->certs[0].der, "ABC", 3);
    chain->certs[0].der_length = 3;
    chain->certs[0].err = SSL_CERT_ERR_OK;

    err = cert_chain_to_query(chain, &url);
    ck_assert(err == NSERROR_OK);

    const char *url_str = nsurl_access(url);
    ck_assert_str_eq(url_str, "about:certificate?cert=QUJD");

    nsurl_unref(url);
    cert_chain_free(chain);
}
END_TEST

START_TEST(cert_chain_to_query_multiple_certs_with_error_test)
{
    nserror err;
    struct cert_chain *chain;
    struct nsurl *url;

    err = cert_chain_alloc(2, &chain);
    ck_assert(err == NSERROR_OK);

    chain->certs[0].der = (uint8_t *)malloc(3);
    memcpy(chain->certs[0].der, "ABC", 3);
    chain->certs[0].der_length = 3;
    chain->certs[0].err = SSL_CERT_ERR_OK;

    chain->certs[1].der = (uint8_t *)malloc(3);
    memcpy(chain->certs[1].der, "DEF", 3);
    chain->certs[1].der_length = 3;
    chain->certs[1].err = 123;

    err = cert_chain_to_query(chain, &url);
    ck_assert(err == NSERROR_OK);

    const char *url_str = nsurl_access(url);
    ck_assert_str_eq(url_str, "about:certificate?cert=QUJD&cert=REVG&certerr=123");

    nsurl_unref(url);
    cert_chain_free(chain);
}
END_TEST

START_TEST(cert_chain_to_query_empty_cert_data_test)
{
    nserror err;
    struct cert_chain *chain;
    struct nsurl *url;

    err = cert_chain_alloc(1, &chain);
    ck_assert(err == NSERROR_OK);

    chain->certs[0].der = NULL;
    chain->certs[0].der_length = 0;
    chain->certs[0].err = SSL_CERT_ERR_OK;

    err = cert_chain_to_query(chain, &url);
    ck_assert(err == NSERROR_OK);

    const char *url_str = nsurl_access(url);
    ck_assert_str_eq(url_str, "about:certificate?cert=");

    nsurl_unref(url);
    cert_chain_free(chain);
}
END_TEST


START_TEST(cert_chain_dup_into_increase_depth)
{
    nserror err;
    struct cert_chain *src;
    struct cert_chain *dst;

    err = cert_chain_alloc(2, &src);
    ck_assert(err == NSERROR_OK);
    src->certs[0].der = (uint8_t *)malloc(3);
    memcpy(src->certs[0].der, "ABC", 3);
    src->certs[0].der_length = 3;
    src->certs[0].err = SSL_CERT_ERR_OK;
    src->certs[1].der = (uint8_t *)malloc(3);
    memcpy(src->certs[1].der, "DEF", 3);
    src->certs[1].der_length = 3;
    src->certs[1].err = 123;

    err = cert_chain_alloc(1, &dst);
    ck_assert(err == NSERROR_OK);
    dst->certs[0].der = (uint8_t *)malloc(3);
    memcpy(dst->certs[0].der, "XYZ", 3);
    dst->certs[0].der_length = 3;
    dst->certs[0].err = SSL_CERT_ERR_OK;

    err = cert_chain_dup_into(src, dst);
    ck_assert(err == NSERROR_OK);

    ck_assert_int_eq(dst->depth, 2);
    ck_assert_int_eq(dst->certs[0].der_length, 3);
    ck_assert_mem_eq(dst->certs[0].der, "ABC", 3);
    ck_assert_int_eq(dst->certs[0].err, SSL_CERT_ERR_OK);
    ck_assert_int_eq(dst->certs[1].der_length, 3);
    ck_assert_mem_eq(dst->certs[1].der, "DEF", 3);
    ck_assert_int_eq(dst->certs[1].err, 123);

    cert_chain_free(src);
    cert_chain_free(dst);
}
END_TEST

START_TEST(cert_chain_dup_into_decrease_depth)
{
    nserror err;
    struct cert_chain *src;
    struct cert_chain *dst;

    err = cert_chain_alloc(1, &src);
    ck_assert(err == NSERROR_OK);
    src->certs[0].der = (uint8_t *)malloc(3);
    memcpy(src->certs[0].der, "ABC", 3);
    src->certs[0].der_length = 3;
    src->certs[0].err = SSL_CERT_ERR_OK;

    err = cert_chain_alloc(2, &dst);
    ck_assert(err == NSERROR_OK);
    dst->certs[0].der = (uint8_t *)malloc(3);
    memcpy(dst->certs[0].der, "XYZ", 3);
    dst->certs[0].der_length = 3;
    dst->certs[0].err = SSL_CERT_ERR_OK;
    dst->certs[1].der = (uint8_t *)malloc(3);
    memcpy(dst->certs[1].der, "UVW", 3);
    dst->certs[1].der_length = 3;
    dst->certs[1].err = SSL_CERT_ERR_OK;

    err = cert_chain_dup_into(src, dst);
    ck_assert(err == NSERROR_OK);

    ck_assert_int_eq(dst->depth, 1);
    ck_assert_int_eq(dst->certs[0].der_length, 3);
    ck_assert_mem_eq(dst->certs[0].der, "ABC", 3);
    ck_assert_int_eq(dst->certs[0].err, SSL_CERT_ERR_OK);

    cert_chain_free(src);
    cert_chain_free(dst);
}
END_TEST

START_TEST(cert_chain_dup_into_same_depth)
{
    nserror err;
    struct cert_chain *src;
    struct cert_chain *dst;

    err = cert_chain_alloc(1, &src);
    ck_assert(err == NSERROR_OK);
    src->certs[0].der = (uint8_t *)malloc(3);
    memcpy(src->certs[0].der, "ABC", 3);
    src->certs[0].der_length = 3;
    src->certs[0].err = SSL_CERT_ERR_OK;

    err = cert_chain_alloc(1, &dst);
    ck_assert(err == NSERROR_OK);
    dst->certs[0].der = (uint8_t *)malloc(3);
    memcpy(dst->certs[0].der, "XYZ", 3);
    dst->certs[0].der_length = 3;
    dst->certs[0].err = 999;

    err = cert_chain_dup_into(src, dst);
    ck_assert(err == NSERROR_OK);

    ck_assert_int_eq(dst->depth, 1);
    ck_assert_int_eq(dst->certs[0].der_length, 3);
    ck_assert_mem_eq(dst->certs[0].der, "ABC", 3);
    ck_assert_int_eq(dst->certs[0].err, SSL_CERT_ERR_OK);

    cert_chain_free(src);
    cert_chain_free(dst);
}
END_TEST

START_TEST(cert_chain_dup_into_null_der)
{
    nserror err;
    struct cert_chain *src;
    struct cert_chain *dst;

    err = cert_chain_alloc(1, &src);
    ck_assert(err == NSERROR_OK);
    src->certs[0].der = NULL;
    src->certs[0].der_length = 0;
    src->certs[0].err = 123;

    err = cert_chain_alloc(1, &dst);
    ck_assert(err == NSERROR_OK);
    dst->certs[0].der = (uint8_t *)malloc(3);
    memcpy(dst->certs[0].der, "XYZ", 3);
    dst->certs[0].der_length = 3;
    dst->certs[0].err = SSL_CERT_ERR_OK;

    err = cert_chain_dup_into(src, dst);
    ck_assert(err == NSERROR_OK);

    ck_assert_int_eq(dst->depth, 1);
    ck_assert_ptr_null(dst->certs[0].der);
    ck_assert_int_eq(dst->certs[0].der_length, 0);
    ck_assert_int_eq(dst->certs[0].err, 123);

    cert_chain_free(src);
    cert_chain_free(dst);
}
END_TEST

START_TEST(cert_chain_dup_into_from_empty)
{
    nserror err;
    struct cert_chain *src;
    struct cert_chain *dst;

    err = cert_chain_alloc(0, &src);
    ck_assert(err == NSERROR_OK);

    err = cert_chain_alloc(2, &dst);
    ck_assert(err == NSERROR_OK);
    dst->certs[0].der = (uint8_t *)malloc(3);
    memcpy(dst->certs[0].der, "XYZ", 3);
    dst->certs[0].der_length = 3;
    dst->certs[0].err = SSL_CERT_ERR_OK;
    dst->certs[1].der = (uint8_t *)malloc(3);
    memcpy(dst->certs[1].der, "UVW", 3);
    dst->certs[1].der_length = 3;
    dst->certs[1].err = SSL_CERT_ERR_OK;

    err = cert_chain_dup_into(src, dst);
    ck_assert(err == NSERROR_OK);

    ck_assert_int_eq(dst->depth, 0);

    cert_chain_free(src);
    cert_chain_free(dst);
}
END_TEST

static TCase *ssl_certs_case_create(void)

{
    TCase *tc;
    tc = tcase_create("SSL Certificates");

    tcase_add_test(tc, cert_chain_alloc_test);
    tcase_add_test(tc, cert_chain_to_query_empty_test);
    tcase_add_test(tc, cert_chain_to_query_single_cert_test);
    tcase_add_test(tc, cert_chain_to_query_multiple_certs_with_error_test);
    tcase_add_test(tc, cert_chain_to_query_empty_cert_data_test);
    tcase_add_test(tc, cert_chain_dup_into_increase_depth);
    tcase_add_test(tc, cert_chain_dup_into_decrease_depth);
    tcase_add_test(tc, cert_chain_dup_into_same_depth);
    tcase_add_test(tc, cert_chain_dup_into_null_der);
    tcase_add_test(tc, cert_chain_dup_into_from_empty);


    return tc;
}

static Suite *ssl_certs_suite_create(void)
{
    Suite *s;
    s = suite_create("SSL Certificates");

    suite_add_tcase(s, ssl_certs_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(ssl_certs_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
