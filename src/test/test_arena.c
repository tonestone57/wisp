#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "utils/arena.h"

START_TEST(test_arena_destroy_null)
{
    /* Should not crash when passing NULL */
    arena_destroy(NULL);
}
END_TEST

START_TEST(test_arena_destroy_unaligned)
{
    /* Should not crash when passing an unaligned pointer */
    struct arena *unaligned = (struct arena *)1;
    arena_destroy(unaligned);
}
END_TEST

static void dummy_destructor(void *ptr)
{
    int *call_count = (int *)ptr;
    (*call_count)++;
}

START_TEST(test_arena_destroy_normal)
{
    struct arena *a = arena_create(4096);
    ck_assert_ptr_nonnull(a);

    int dtor1_calls = 0;
    int dtor2_calls = 0;

    /* Register destructors */
    arena_register_destructor(a, &dtor1_calls, dummy_destructor);
    arena_register_destructor(a, &dtor2_calls, dummy_destructor);

    /* Allocate some memory so chunks are created and need to be freed */
    void *ptr1 = arena_alloc(a, 128);
    ck_assert_ptr_nonnull(ptr1);
    void *ptr2 = arena_alloc(a, 256);
    ck_assert_ptr_nonnull(ptr2);

    arena_destroy(a);

    /* Verify destructors were called exactly once each */
    ck_assert_int_eq(dtor1_calls, 1);
    ck_assert_int_eq(dtor2_calls, 1);
}
END_TEST

static Suite *arena_suite(void)
{
    Suite *s = suite_create("arena");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_arena_destroy_null);
    tcase_add_test(tc_core, test_arena_destroy_unaligned);
    tcase_add_test(tc_core, test_arena_destroy_normal);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = arena_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
