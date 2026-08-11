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

START_TEST(test_arena_merge_null)
{
    struct arena *a = arena_create(4096);
    ck_assert_ptr_nonnull(a);

    /* Should not crash when passing NULL */
    arena_merge(NULL, NULL);
    arena_merge(a, NULL);
    arena_merge(NULL, a);

    arena_destroy(a);
}
END_TEST

START_TEST(test_arena_merge_normal)
{
    struct arena *m = arena_create(4096);
    struct arena *w = arena_create(4096);
    ck_assert_ptr_nonnull(m);
    ck_assert_ptr_nonnull(w);

    int m_dtor_calls = 0;
    int w_dtor1_calls = 0;
    int w_dtor2_calls = 0;

    /* Register destructors in both arenas */
    arena_register_destructor(m, &m_dtor_calls, dummy_destructor);
    arena_register_destructor(w, &w_dtor1_calls, dummy_destructor);
    arena_register_destructor(w, &w_dtor2_calls, dummy_destructor);

    /* Allocate some memory so chunks are created */
    void *m_ptr = arena_alloc(m, 128);
    ck_assert_ptr_nonnull(m_ptr);

    void *w_ptr1 = arena_alloc(w, 256);
    ck_assert_ptr_nonnull(w_ptr1);
    void *w_ptr2 = arena_alloc(w, 512);
    ck_assert_ptr_nonnull(w_ptr2);

    /* Merge worker arena into main arena */
    arena_merge(m, w);

    /* Destroy worker arena - its destructors and chunks should have been moved */
    arena_destroy(w);
    ck_assert_int_eq(w_dtor1_calls, 0);
    ck_assert_int_eq(w_dtor2_calls, 0);
    ck_assert_int_eq(m_dtor_calls, 0);

    /* Destroy main arena - it should now call all destructors and free all chunks */
    arena_destroy(m);
    ck_assert_int_eq(m_dtor_calls, 1);
    ck_assert_int_eq(w_dtor1_calls, 1);
    ck_assert_int_eq(w_dtor2_calls, 1);
}
END_TEST

static Suite *arena_suite(void)
{
    Suite *s = suite_create("arena");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_arena_destroy_null);
    tcase_add_test(tc_core, test_arena_destroy_unaligned);
    tcase_add_test(tc_core, test_arena_destroy_normal);
    tcase_add_test(tc_core, test_arena_merge_null);
    tcase_add_test(tc_core, test_arena_merge_normal);

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
