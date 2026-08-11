#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <wisp/utils/shm_dom.h>

// Do NOT define the global variables here because they are already defined in libwisp.so

START_TEST(test_shm_dom_create)
{
    const char *test_name = "/test_shm_dom_create";
    uint32_t capacity = 100;
    bool is_server = true;

    shm_dom_t *shm = shm_dom_create(test_name, capacity, is_server);

    ck_assert_ptr_nonnull(shm);
    ck_assert_int_eq(shm->is_server, is_server);
    ck_assert_int_eq(shm->node_capacity, capacity);
    ck_assert_str_eq(shm->shm_name, test_name);

    shm_dom_destroy(shm, test_name, is_server);
}
END_TEST

START_TEST(test_shm_dom_destroy_null)
{
    // Test that destroying a NULL pointer doesn't crash
    shm_dom_destroy(NULL, "/wisp_test_shm_dom", true);
}
END_TEST


START_TEST(test_shm_alloc_string_nulls)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_nulls", 100, true);
    ck_assert_ptr_nonnull(shm);

    // Test null shm
    WispStringRef ref1 = wisp_shm_alloc_string(NULL, "test");
    ck_assert_int_eq(ref1, 0);

    // Test null str
    WispStringRef ref2 = wisp_shm_alloc_string(shm, NULL);
    ck_assert_int_eq(ref2, 0);

    // Test both null
    WispStringRef ref3 = wisp_shm_alloc_string(NULL, NULL);
    ck_assert_int_eq(ref3, 0);

    shm_dom_destroy(shm, "/test_shm_alloc_string_nulls", true);
}
END_TEST

START_TEST(test_shm_alloc_string_sso)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_sso", 100, true);
    ck_assert_ptr_nonnull(shm);

    uint32_t initial_heap_top = shm->string_heap_top;

    // Length 0
    WispStringRef ref0 = wisp_shm_alloc_string(shm, "");
    ck_assert(wisp_string_ref_eq(shm, ref0, ""));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    // Length 1
    WispStringRef ref1 = wisp_shm_alloc_string(shm, "A");
    ck_assert(wisp_string_ref_eq(shm, ref1, "A"));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    // Length 2
    WispStringRef ref2 = wisp_shm_alloc_string(shm, "AB");
    ck_assert(wisp_string_ref_eq(shm, ref2, "AB"));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    // Length 3
    WispStringRef ref3 = wisp_shm_alloc_string(shm, "ABC");
    ck_assert(wisp_string_ref_eq(shm, ref3, "ABC"));
    ck_assert_int_eq(shm->string_heap_top, initial_heap_top);

    shm_dom_destroy(shm, "/test_shm_alloc_string_sso", true);
}
END_TEST

START_TEST(test_shm_alloc_string_heap)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_heap_tmp", 100, true);
    memset(shm->string_hash_table, 0, sizeof(shm->string_hash_table));
    ck_assert_ptr_nonnull(shm);

    uint32_t initial_heap_top = shm->string_heap_top;

    // Length > 3
    WispStringRef ref1 = wisp_shm_alloc_string(shm, "LongStringHere");
    ck_assert_int_ne(ref1, 0);
    ck_assert(wisp_string_ref_eq(shm, ref1, "LongStringHere"));

    uint32_t after_alloc_heap_top = shm->string_heap_top;
    ck_assert_int_gt(after_alloc_heap_top, initial_heap_top);

    // Allocate again, should intern
    WispStringRef ref2 = wisp_shm_alloc_string(shm, "LongStringHere");
    ck_assert_int_eq(ref1, ref2); // Should return same ref
    ck_assert_int_eq(shm->string_heap_top, after_alloc_heap_top); // Heap top should not increase

    shm_dom_destroy(shm, "/test_shm_alloc_string_heap_tmp", true);
}
END_TEST

START_TEST(test_shm_alloc_string_oom)
{
    shm_dom_t *shm = shm_dom_create("/test_shm_alloc_string_oom", 100, true);
    ck_assert_ptr_nonnull(shm);

    // Artificially restrict heap
    shm->string_heap_top = SHM_STRING_HEAP_SIZE - 2;

    // Try allocating string that needs > 2 bytes
    WispStringRef ref = wisp_shm_alloc_string(shm, "WillOOM");
    ck_assert_int_eq(ref, 0);

    shm_dom_destroy(shm, "/test_shm_alloc_string_oom", true);
}
END_TEST

static Suite *shm_dom_suite(void)
{
    Suite *s = suite_create("shm_dom");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_shm_dom_create);
    tcase_add_test(tc_core, test_shm_dom_destroy_null);
    tcase_add_test(tc_core, test_shm_alloc_string_nulls);
    tcase_add_test(tc_core, test_shm_alloc_string_sso);
    tcase_add_test(tc_core, test_shm_alloc_string_heap);
    tcase_add_test(tc_core, test_shm_alloc_string_oom);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = shm_dom_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
