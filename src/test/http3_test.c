/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp, http://www.netsurf-browser.org/
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
 * Tests for QUIC & HTTP/3 transport options.
 */

#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/errors.h"
#include "utils/nsoption.h"

/* Stubs */
nserror nslog_set_filter_by_options(void)
{
    return NSERROR_OK;
}

START_TEST(test_http3_options_defaults)
{
    nserror err = nsoption_init(NULL, NULL, NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Verify defaults */
    ck_assert_int_eq(nsoption_bool(enable_http3), true);
    ck_assert_int_eq(nsoption_bool(force_http3), false);
    ck_assert_ptr_eq(nsoption_charp(altsvc_cache_path), NULL);
    ck_assert_int_eq(nsoption_bool(enable_quic_0rtt), true);
    ck_assert_int_eq(nsoption_int(quic_connection_cache_size), 16);

    nsoption_finalise(NULL, NULL);
}
END_TEST

START_TEST(test_http3_options_modify)
{
    nserror err = nsoption_init(NULL, NULL, NULL);
    ck_assert_int_eq(err, NSERROR_OK);

    /* Modify options */
    nsoption_set_bool(enable_http3, false);
    nsoption_set_bool(force_http3, true);
    nsoption_set_charp(altsvc_cache_path, strdup("/tmp/test_altsvc.txt"));
    nsoption_set_bool(enable_quic_0rtt, false);
    nsoption_set_int(quic_connection_cache_size, 32);

    ck_assert_int_eq(nsoption_bool(enable_http3), false);
    ck_assert_int_eq(nsoption_bool(force_http3), true);
    ck_assert_str_eq(nsoption_charp(altsvc_cache_path), "/tmp/test_altsvc.txt");
    ck_assert_int_eq(nsoption_bool(enable_quic_0rtt), false);
    ck_assert_int_eq(nsoption_int(quic_connection_cache_size), 32);

    nsoption_finalise(NULL, NULL);
}
END_TEST

static Suite *http3_suite_create(void)
{
    Suite *s = suite_create("QUIC & HTTP/3 Transport Options");
    TCase *tc = tcase_create("Options");

    tcase_add_test(tc, test_http3_options_defaults);
    tcase_add_test(tc, test_http3_options_modify);
    suite_add_tcase(s, tc);

    return s;
}

int main(void)
{
    int number_failed;
    SRunner *sr = srunner_create(http3_suite_create());

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
