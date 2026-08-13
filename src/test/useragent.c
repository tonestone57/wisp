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

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define WISP_SANITIZER_ENABLED
#elif defined(__has_feature)
  #if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
  #define WISP_SANITIZER_ENABLED
  #endif
#endif

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

#ifndef WISP_SANITIZER_ENABLED
START_TEST(test_user_agent_oom)
{
    const char *ua;

    /* Force malloc to fail on first call */
    malloc_limit(0);

    ua = user_agent_string();

    /* Reset limit */
    malloc_limit(-1);

    ck_assert(ua != NULL);
    ck_assert_str_eq(ua, "Mozilla/5.0 (Unknown) Wisp/0");

    free_user_agent_string();
}
END_TEST
#endif

START_TEST(test_user_agent_caching)
{
    const char *ua1;
    const char *ua2;
    ua1 = user_agent_string();
    ua2 = user_agent_string();
    ck_assert(ua1 != NULL);
    ck_assert_ptr_eq(ua1, ua2);
    free_user_agent_string();
}
END_TEST

START_TEST(test_user_agent_rebuild_after_free)
{
    const char *ua1;
    const char *ua2;
    ua1 = user_agent_string();
    ck_assert(ua1 != NULL);
    free_user_agent_string();
    ua2 = user_agent_string();
    ck_assert(ua2 != NULL);
    ck_assert(strlen(ua2) > 0);
    ck_assert(strstr(ua2, "Wisp") != NULL);
    free_user_agent_string();
}
END_TEST

START_TEST(test_free_user_agent_string_idempotent)
{
    const char *ua;
    ua = user_agent_string();
    ck_assert(ua != NULL);
    free_user_agent_string();
    free_user_agent_string();
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
    tcase_add_test(tc_core, test_user_agent_caching);
    tcase_add_test(tc_core, test_user_agent_rebuild_after_free);
    tcase_add_test(tc_core, test_free_user_agent_string_idempotent);
#ifndef WISP_SANITIZER_ENABLED
    tcase_add_test(tc_core, test_user_agent_oom);
#endif
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
