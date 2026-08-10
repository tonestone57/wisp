#include <check.h>
#include <stdlib.h>
#include <stdbool.h>
#include "wisp/utils/shm_dom.h"

// Do NOT define the global variables here because they are already defined in libwisp.so

START_TEST(test_shm_dom_create_destroy)
{
    const char *name = "/wisp_test_shm_dom";

    // Test creating server side SHM
    shm_dom_t *shm = shm_dom_create(name, 1024, true);
    ck_assert_ptr_ne(shm, NULL);

    ck_assert_int_eq(shm->node_capacity, 1024);
    ck_assert_int_eq(shm->is_server, true);
    ck_assert_str_eq(shm->shm_name, name);

    // Destroy the SHM
    shm_dom_destroy(shm, name, true);
}
END_TEST

START_TEST(test_shm_dom_destroy_null)
{
    // Test that destroying a NULL pointer doesn't crash
    shm_dom_destroy(NULL, "/wisp_test_shm_dom", true);
}
END_TEST

static Suite *shm_dom_suite(void)
{
    Suite *s = suite_create("shm_dom");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_shm_dom_create_destroy);
    tcase_add_test(tc_core, test_shm_dom_destroy_null);
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
