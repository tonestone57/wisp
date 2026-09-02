/*
 * Copyright 2026 Neosurf contributors
 *
 * This file is part of Neosurf, http://www.netsurf-browser.org/
 *
 * Neosurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * Neosurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * Unit test for stylesheet gate checking and late-arriving stylesheet re-cascading.
 */

#include "utils/config.h"

#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/content_type.h"
#include "wisp/content/content.h"

struct mock_html_stylesheet {
    void *node;
    void *sheet; /* hlcache_handle pointer */
    char *media;
    bool modified;
    bool unused;
};

struct mock_html_content {
    int active;
    int scripts_active;
    bool data_complete;
    uint64_t data_complete_time_ms;
    bool conversion_begun;
    unsigned int stylesheet_count;
    struct mock_html_stylesheet *stylesheets;
    int active_bg_tasks;
};

/* Mock content status helper */
static content_status mock_get_status(void *sheet, content_status default_status)
{
    if (sheet == NULL) return CONTENT_STATUS_DONE;
    return default_status;
}

static bool mock_html_can_begin_conversion(struct mock_html_content *htmlc, content_status *sheet_statuses)
{
    unsigned int i;

    if (htmlc->conversion_begun)
        return false;

    if (htmlc->active_bg_tasks > 0)
        return false;

    if (!htmlc->data_complete)
        return false;

    bool bypass_active_gate = false;
    if (htmlc->data_complete_time_ms != 0 && htmlc->data_complete_time_ms > 5000) {
        bypass_active_gate = true;
    }

    if (htmlc->active != htmlc->scripts_active && !bypass_active_gate) {
        return false;
    }

    for (i = 0; i != htmlc->stylesheet_count; i++) {
        if (htmlc->stylesheets[i].modified && !bypass_active_gate) {
            return false;
        }
        if (htmlc->stylesheets[i].sheet != NULL && !bypass_active_gate) {
            content_status status = mock_get_status(htmlc->stylesheets[i].sheet, sheet_statuses ? sheet_statuses[i] : CONTENT_STATUS_DONE);
            if (status == CONTENT_STATUS_LOADING) {
                return false;
            }
        }
    }

    return true;
}

static bool mock_html_convert_css_callback_done(struct mock_html_content *parent, unsigned int sheet_index, content_status *sheet_statuses, bool *restart_scheduled)
{
    if (sheet_statuses) {
        sheet_statuses[sheet_index] = CONTENT_STATUS_DONE;
    }
    parent->active--;

    if (parent->conversion_begun) {
        parent->conversion_begun = false;
        if (restart_scheduled) *restart_scheduled = true;
        return true;
    } else if (mock_html_can_begin_conversion(parent, sheet_statuses)) {
        parent->conversion_begun = true;
        return true;
    }
    return false;
}

START_TEST(test_stylesheet_gate_blocks_while_stylesheet_loading)
{
    struct mock_html_stylesheet sheets[2] = {
        {.sheet = (void *)0x1000, .modified = false},
        {.sheet = (void *)0x2000, .modified = false}
    };
    content_status statuses[2] = { CONTENT_STATUS_DONE, CONTENT_STATUS_LOADING };

    struct mock_html_content htmlc = {
        .active = 1,
        .scripts_active = 0,
        .data_complete = true,
        .data_complete_time_ms = 0,
        .conversion_begun = false,
        .stylesheet_count = 2,
        .stylesheets = sheets,
        .active_bg_tasks = 0
    };

    /* Gate must block while sheet[1] (e.g. shijin4.css) is loading */
    ck_assert_msg(!mock_html_can_begin_conversion(&htmlc, statuses),
        "Conversion gate should block when a stylesheet is still loading");

    /* When sheet[1] finishes loading, active decrements and statuses[1] becomes DONE */
    htmlc.active = 0;
    statuses[1] = CONTENT_STATUS_DONE;
    ck_assert_msg(mock_html_can_begin_conversion(&htmlc, statuses),
        "Conversion gate should allow conversion when all stylesheets are done");
}
END_TEST

START_TEST(test_late_stylesheet_triggers_conversion_restart)
{
    struct mock_html_stylesheet sheets[2] = {
        {.sheet = (void *)0x1000, .modified = false},
        {.sheet = (void *)0x2000, .modified = false}
    };
    content_status statuses[2] = { CONTENT_STATUS_DONE, CONTENT_STATUS_LOADING };

    struct mock_html_content htmlc = {
        .active = 2,
        .scripts_active = 0,
        .data_complete = true,
        .data_complete_time_ms = 0,
        .conversion_begun = true, /* Initial conversion ran earlier */
        .stylesheet_count = 2,
        .stylesheets = sheets,
        .active_bg_tasks = 0
    };

    bool restart_scheduled = false;
    bool handled = mock_html_convert_css_callback_done(&htmlc, 1, statuses, &restart_scheduled);

    ck_assert_msg(handled, "CSS completion callback should be handled");
    ck_assert_msg(restart_scheduled, "Late-arriving stylesheet MUST trigger conversion restart");
    ck_assert_msg(!htmlc.conversion_begun, "conversion_begun flag MUST be reset to allow re-cascade pass");
}
END_TEST

Suite *stylesheet_cascade_gate_suite(void)
{
    Suite *s = suite_create("StylesheetCascadeGate");
    TCase *tc = tcase_create("Core");

    tcase_add_test(tc, test_stylesheet_gate_blocks_while_stylesheet_loading);
    tcase_add_test(tc, test_late_stylesheet_triggers_conversion_restart);
    suite_add_tcase(s, tc);

    return s;
}

int main(void)
{
    int number_failed;
    SRunner *sr = srunner_create(stylesheet_cascade_gate_suite());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
