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

static TCase *ssl_certs_case_create(void)
{
    TCase *tc;
    tc = tcase_create("SSL Certificates");

    tcase_add_test(tc, cert_chain_alloc_test);

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
