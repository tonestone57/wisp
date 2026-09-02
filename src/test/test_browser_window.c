/*
 * Copyright 2026 Wisp Contributors
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/**
 * \file
 * Unit tests for browser_window functions (specifically browser_window_show_certificates).
 */

#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/browser_window.h>
#include <wisp/desktop/gui_table.h>
#include <wisp/misc.h>
#include <wisp/ssl_certs.h>
#include <wisp/utils/corestrings.h>
#include <wisp/utils/errors.h>
#include <wisp/utils/messages.h>
#include <wisp/utils/nsoption.h>
#include <wisp/window.h>
#include "desktop/browser_private.h"
#include "content/handlers/javascript/js.h"

extern struct wisp_table *guit;

static struct browser_window *last_created_bw = NULL;

static nserror mock_schedule(int delay, void (*callback)(void *p), void *p)
{
    (void)delay;
    (void)callback;
    (void)p;
    return NSERROR_OK;
}

static struct gui_misc_table mock_misc = {
    .schedule = mock_schedule,
};

static struct gui_window *mock_gui_window_create(
    struct browser_window *bw, struct gui_window *existing, gui_window_create_flags flags)
{
    (void)existing;
    (void)flags;
    last_created_bw = bw;
    return (struct gui_window *)0x12345;
}

static void mock_gui_window_destroy(struct gui_window *gw)
{
    (void)gw;
}

static nserror mock_gui_window_event(struct gui_window *gw, enum gui_window_event event)
{
    (void)gw;
    (void)event;
    return NSERROR_OK;
}

static nserror mock_gui_window_set_url(struct gui_window *gw, struct nsurl *url)
{
    (void)gw;
    (void)url;
    return NSERROR_OK;
}

static void mock_gui_window_set_title(struct gui_window *gw, const char *title)
{
    (void)gw;
    (void)title;
}

static void mock_gui_window_set_status(struct gui_window *gw, const char *text)
{
    (void)gw;
    (void)text;
}

static void mock_gui_window_set_icon(struct gui_window *gw, struct hlcache_handle *icon)
{
    (void)gw;
    (void)icon;
}

static struct gui_window *mock_gui_window_create_fail(
    struct browser_window *bw, struct gui_window *existing, gui_window_create_flags flags)
{
    (void)bw;
    (void)existing;
    (void)flags;
    return NULL;
}

static struct gui_window_table mock_window_table_success = {
    .create = mock_gui_window_create,
    .destroy = mock_gui_window_destroy,
    .event = mock_gui_window_event,
    .set_url = mock_gui_window_set_url,
    .set_title = mock_gui_window_set_title,
    .set_status = mock_gui_window_set_status,
    .set_icon = mock_gui_window_set_icon,
};

static struct gui_window_table mock_window_table_fail = {
    .create = mock_gui_window_create_fail,
    .destroy = mock_gui_window_destroy,
    .event = mock_gui_window_event,
    .set_url = mock_gui_window_set_url,
    .set_title = mock_gui_window_set_title,
    .set_status = mock_gui_window_set_status,
    .set_icon = mock_gui_window_set_icon,
};

static struct wisp_table mock_guit_success = {
    .misc = &mock_misc,
    .window = &mock_window_table_success,
};

static struct wisp_table mock_guit_fail = {
    .misc = &mock_misc,
    .window = &mock_window_table_fail,
};

static void setup(void)
{
    last_created_bw = NULL;
    corestrings_init();
    nsoption_init(NULL, NULL, NULL);
    js_initialise();
}

static void teardown(void)
{
    js_finalise();
    nsoption_finalise(NULL, NULL);
    corestrings_fini();
}

START_TEST(test_show_certificates_null_chain)
{
    struct browser_window bw;
    memset(&bw, 0, sizeof(bw));

    nserror res = browser_window_show_certificates(&bw);
    ck_assert_int_eq(res, NSERROR_NOT_FOUND);
}
END_TEST

START_TEST(test_show_certificates_success)
{
    struct wisp_table *saved_guit = guit;
    guit = &mock_guit_success;
    last_created_bw = NULL;

    struct browser_window bw;
    memset(&bw, 0, sizeof(bw));

    nserror err = cert_chain_alloc(1, &bw.current_cert_chain);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(bw.current_cert_chain);

    bw.current_cert_chain->certs[0].der = (uint8_t *)strdup("test_cert_data");
    bw.current_cert_chain->certs[0].der_length = 14;
    bw.current_cert_chain->certs[0].err = SSL_CERT_ERR_OK;

    nserror res = browser_window_show_certificates(&bw);
    ck_assert(res == NSERROR_OK || res == NSERROR_NO_FETCH_HANDLER);
    ck_assert_ptr_nonnull(last_created_bw);

    browser_window_destroy(last_created_bw);
    last_created_bw = NULL;

    cert_chain_free(bw.current_cert_chain);
    bw.current_cert_chain = NULL;

    guit = saved_guit;
}
END_TEST

START_TEST(test_show_certificates_create_failure)
{
    struct wisp_table *saved_guit = guit;
    guit = &mock_guit_fail;
    last_created_bw = NULL;

    struct browser_window bw;
    memset(&bw, 0, sizeof(bw));

    nserror err = cert_chain_alloc(1, &bw.current_cert_chain);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(bw.current_cert_chain);

    nserror res = browser_window_show_certificates(&bw);
    ck_assert_int_eq(res, NSERROR_BAD_PARAMETER);
    ck_assert_ptr_null(last_created_bw);

    cert_chain_free(bw.current_cert_chain);
    bw.current_cert_chain = NULL;

    guit = saved_guit;
}
END_TEST

static Suite *browser_window_suite_create(void)
{
    Suite *s = suite_create("Browser Window");
    TCase *tc = tcase_create("browser_window_show_certificates");

    tcase_add_unchecked_fixture(tc, setup, teardown);

    tcase_add_test(tc, test_show_certificates_null_chain);
    tcase_add_test(tc, test_show_certificates_success);
    tcase_add_test(tc, test_show_certificates_create_failure);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = browser_window_suite_create();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_VERBOSE);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
