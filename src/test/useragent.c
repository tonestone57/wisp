/*
 * Copyright 2024 Jules
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils/useragent.h"
#include "test/malloc_fig.h"

START_TEST(test_user_agent_normal)
{
    const char *ua;
    ua = user_agent_string();
    ck_assert(ua != NULL);
    ck_assert(strlen(ua) > 0);
    ck_assert(strstr(ua, "Wisp") != NULL);

    free_user_agent_string();
}
END_TEST

START_TEST(test_user_agent_oom)
{
    const char *ua;

    /* Force malloc to fail on first call */
    malloc_limit(0);

    ua = user_agent_string();

    ck_assert(ua != NULL);
    ck_assert_str_eq(ua, "Mozilla/5.0 (Unknown) Wisp/0");

    /* Reset limit */
    malloc_limit(-1);

    free_user_agent_string();
}
END_TEST

static Suite *useragent_suite_create(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("UserAgent");

    tc_core = tcase_create("Core");
    tcase_add_test(tc_core, test_user_agent_normal);
    tcase_add_test(tc_core, test_user_agent_oom);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = useragent_suite_create();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
