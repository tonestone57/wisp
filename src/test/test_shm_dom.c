#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
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

START_TEST(test_shm_dom_lock_read)
{
    const char *test_name = "/test_shm_dom_lock_read";
    shm_dom_t *shm = shm_dom_create(test_name, 100, true);
    ck_assert_ptr_nonnull(shm);

    // Ensure initial lock state is 0
    ck_assert_int_eq(shm->lock, 0);

    // Test a read lock increments the counter
    shm_dom_lock_read(shm);
    ck_assert_int_eq(shm->lock, 1);

    // Test a second read lock increments it again
    shm_dom_lock_read(shm);
    ck_assert_int_eq(shm->lock, 2);

    // Test a read unlock decrements the counter
    shm_dom_unlock_read(shm);
    ck_assert_int_eq(shm->lock, 1);

    // Final unlock
    shm_dom_unlock_read(shm);
    ck_assert_int_eq(shm->lock, 0);

    shm_dom_destroy(shm, test_name, true);

    // Test NULL does not crash
    shm_dom_lock_read(NULL);
    shm_dom_unlock_read(NULL);
}
END_TEST

static Suite *shm_dom_suite(void)
{
    Suite *s = suite_create("shm_dom");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_shm_dom_create);
    tcase_add_test(tc_core, test_shm_dom_destroy_null);
    tcase_add_test(tc_core, test_shm_dom_lock_read);
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
