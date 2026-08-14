#include <check.h>
#include <stdlib.h>
#include <stdbool.h>

#define main network_process_main
#include "../processes/network/main.c"
#undef main

START_TEST(test_default_filetype)
{
    ck_assert_str_eq(default_filetype("style.css"), "text/css");
    ck_assert_str_eq(default_filetype("page.html"), "text/html");
    ck_assert_str_eq(default_filetype("image.png"), "image/png");
    ck_assert_str_eq(default_filetype("photo.jpg"), "image/jpeg");
    ck_assert_str_eq(default_filetype("photo.jpeg"), "image/jpeg");
    ck_assert_str_eq(default_filetype("document.txt"), "text/plain");
    ck_assert_str_eq(default_filetype("unknown.xyz"), "text/plain");
}
END_TEST

static Suite *network_main_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("NetworkMain");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_default_filetype);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = network_main_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
