#include <check.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/utils/filepath.h>

START_TEST(filepath_free_strvec_test)
{
    char **strvec;

    strvec = calloc(4, sizeof(char *));
    ck_assert_ptr_ne(strvec, NULL);

    strvec[0] = strdup("/usr/share/wisp");
    strvec[1] = strdup("/etc/wisp");
    strvec[2] = strdup("~/.wisp");
    strvec[3] = NULL;

    filepath_free_strvec(strvec);

    /* We can't strictly check for memory leaks here but ASAN will catch it
     * and we ensure no segmentation fault occurred. */
}
END_TEST

START_TEST(filepath_free_strvec_empty_test)
{
    char **strvec;

    strvec = calloc(1, sizeof(char *));
    ck_assert_ptr_ne(strvec, NULL);

    strvec[0] = NULL;

    filepath_free_strvec(strvec);
}
END_TEST

static TCase *filepath_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Filepath");

    tcase_add_test(tc, filepath_free_strvec_test);
    tcase_add_test(tc, filepath_free_strvec_empty_test);

    return tc;
}

static Suite *filepath_suite_create(void)
{
    Suite *s;
    s = suite_create("Filepath Utils");

    suite_add_tcase(s, filepath_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(filepath_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
