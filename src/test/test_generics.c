#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils/http/generics.h"
#include "wisp/utils/errors.h"

static int free_count = 0;

typedef struct test_item {
    http__item base;
    int data;
} test_item;

static void mock_free(http__item *self)
{
    test_item *item = (test_item *)self;
    free_count++;
    free(item);
}

static test_item *create_test_item(int data, test_item *next)
{
    test_item *item = calloc(1, sizeof(test_item));
    item->base.next = (http__item *)next;
    item->base.free = mock_free;
    item->data = data;
    return item;
}

static void setup(void)
{
    free_count = 0;
}

static void teardown(void)
{
    /* Nothing to do */
}

START_TEST(test_item_list_destroy_null)
{
    http___item_list_destroy(NULL);
    ck_assert_int_eq(free_count, 0);
}
END_TEST

START_TEST(test_item_list_destroy_single)
{
    test_item *item = create_test_item(1, NULL);
    http___item_list_destroy((http__item *)item);
    ck_assert_int_eq(free_count, 1);
}
END_TEST

START_TEST(test_item_list_destroy_multiple)
{
    test_item *item3 = create_test_item(3, NULL);
    test_item *item2 = create_test_item(2, item3);
    test_item *item1 = create_test_item(1, item2);

    http___item_list_destroy((http__item *)item1);
    ck_assert_int_eq(free_count, 3);
}
END_TEST

static Suite *test_suite(void)
{
    Suite *s = suite_create("http-generics");
    TCase *tc_core = tcase_create("Core");

    tcase_add_checked_fixture(tc_core, setup, teardown);
    tcase_add_test(tc_core, test_item_list_destroy_null);
    tcase_add_test(tc_core, test_item_list_destroy_single);
    tcase_add_test(tc_core, test_item_list_destroy_multiple);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = test_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
