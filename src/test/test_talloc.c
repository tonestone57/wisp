#include <check.h>
#include <stdlib.h>
#include <string.h>

/* Use correct path for internal header */
#include "utils/talloc.h"

static int destructor_called = 0;

static int success_destructor(void *ptr)
{
    destructor_called = 1;
    return 0;
}

static int fail_destructor(void *ptr)
{
    destructor_called = 1;
    return -1;
}

START_TEST(test_talloc_set_destructor_success)
{
    void *ctx = talloc_new(NULL);
    void *ptr = talloc_size(ctx, 10);

    destructor_called = 0;
    talloc_set_destructor(ptr, success_destructor);

    int ret = talloc_free(ptr);
    ck_assert_int_eq(ret, 0);
    ck_assert_int_eq(destructor_called, 1);

    talloc_free(ctx);
}
END_TEST

START_TEST(test_talloc_set_destructor_fail)
{
    void *ctx = talloc_new(NULL);
    void *ptr = talloc_size(ctx, 10);

    destructor_called = 0;
    talloc_set_destructor(ptr, fail_destructor);

    int ret = talloc_free(ptr);
    ck_assert_int_eq(ret, -1);
    ck_assert_int_eq(destructor_called, 1);

    /* In talloc, if a destructor returns -1, the free operation fails, and the memory remains allocated.
       We have to explicitly clear the destructor to allow the context to be freed successfully,
       otherwise talloc_free(ctx) will still see the failed child destructor and abort or leak. */
    talloc_set_destructor(ptr, NULL);
    talloc_free(ctx);
}
END_TEST

static TCase *talloc_case_create(void)
{
    TCase *tc;
    tc = tcase_create("Talloc");
    tcase_add_test(tc, test_talloc_set_destructor_success);
    tcase_add_test(tc, test_talloc_set_destructor_fail);
    return tc;
}

static Suite *talloc_suite_create(void)
{
    Suite *s;
    s = suite_create("Talloc Utils");
    suite_add_tcase(s, talloc_case_create());
    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;
    sr = srunner_create(talloc_suite_create());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
