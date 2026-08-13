#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "utils/useragent.h"

START_TEST(test_useragent_string)
{
    const char *ua = user_agent_string();
    ck_assert_ptr_nonnull(ua);
    ck_assert_int_gt(strlen(ua), 0);

    // Check that it contains "Wisp/"
    ck_assert_ptr_nonnull(strstr(ua, "Wisp/"));

    // Should be able to free and re-init
    free_user_agent_string();

    const char *ua2 = user_agent_string();
    ck_assert_ptr_nonnull(ua2);

    // Test repeated frees
    free_user_agent_string();
    free_user_agent_string();
}
END_TEST

static Suite *useragent_suite_create(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("useragent");
    tc = tcase_create("Core");

    tcase_add_test(tc, test_useragent_string);

    suite_add_tcase(s, tc);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(useragent_suite_create());
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
