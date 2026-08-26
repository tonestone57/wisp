/*
 * Test core_buffer operations.
 */

#include <check.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "utils/core_buffer.h"
#include "utils/errors.h"

START_TEST(test_core_buffer_init)
{
    core_buffer buf;

    /* NULL parameter check */
    ck_assert_int_eq(core_buffer_init(NULL), NSERROR_BAD_PARAMETER);

    /* Normal initialization */
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_int_eq(buf.length, 0);
    ck_assert_int_eq(buf.allocated, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_append_invalid)
{
    core_buffer buf;
    const uint8_t data[] = "test data";

    core_buffer_init(&buf);

    /* NULL buffer */
    ck_assert_int_eq(core_buffer_append(NULL, data, 9), NSERROR_BAD_PARAMETER);

    /* NULL data with non-zero length */
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 9), NSERROR_BAD_PARAMETER);

    /* NULL data with zero length should succeed */
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 0), NSERROR_OK);

    /* Data with zero length should succeed */
    ck_assert_int_eq(core_buffer_append(&buf, data, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_append_basic)
{
    core_buffer buf;
    const uint8_t data1[] = "Hello";
    const uint8_t data2[] = ", World!";

    core_buffer_init(&buf);

    /* Append first chunk */
    ck_assert_int_eq(core_buffer_append(&buf, data1, 5), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 5);
    ck_assert_ptr_nonnull(core_buffer_data(&buf));
    ck_assert_mem_eq(core_buffer_data(&buf), "Hello", 5);

    /* Append second chunk */
    ck_assert_int_eq(core_buffer_append(&buf, data2, 8), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 13);
    ck_assert_mem_eq(core_buffer_data(&buf), "Hello, World!", 13);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_append_growth)
{
    core_buffer buf;
    uint8_t chunk[100];
    memset(chunk, 0xAB, sizeof(chunk));

    core_buffer_init(&buf);

    /* Append data larger than default initial allocation (64 bytes) */
    ck_assert_int_eq(core_buffer_append(&buf, chunk, sizeof(chunk)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 100);
    ck_assert_uint_ge(buf.allocated, 100);
    ck_assert_mem_eq(core_buffer_data(&buf), chunk, 100);

    /* Append another large chunk to force realloc growth */
    ck_assert_int_eq(core_buffer_append(&buf, chunk, sizeof(chunk)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 200);
    ck_assert_uint_ge(buf.allocated, 200);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_append_wrapped_external)
{
    core_buffer buf;
    uint8_t ext_data[] = "Wrapped";
    const uint8_t append_data[] = " Data";

    core_buffer_init(&buf);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, ext_data, 7), NSERROR_OK);

    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_int_eq(core_buffer_length(&buf), 7);
    ck_assert_ptr_eq(core_buffer_data(&buf), ext_data);

    /* Appending to a wrapped buffer causes allocation & copying of existing data */
    ck_assert_int_eq(core_buffer_append(&buf, append_data, 5), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 12);
    ck_assert_uint_ge(buf.allocated, 12);
    ck_assert_ptr_ne(core_buffer_data(&buf), ext_data);
    ck_assert_mem_eq(core_buffer_data(&buf), "Wrapped Data", 12);

    /* External buffer should be untouched */
    ck_assert_mem_eq(ext_data, "Wrapped", 7);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_getters_clear_shrink)
{
    core_buffer buf;
    const uint8_t data[] = "Sample Data";

    /* Getters with NULL */
    ck_assert_ptr_null(core_buffer_data(NULL));
    ck_assert_int_eq(core_buffer_length(NULL), 0);

    core_buffer_init(&buf);
    ck_assert_int_eq(core_buffer_append(&buf, data, 11), NSERROR_OK);

    /* Shrink buffer to match length */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 11);
    ck_assert_int_eq(buf.length, 11);

    /* Clear buffer length without freeing memory */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert_int_eq(buf.allocated, 11);

    /* Shrink empty buffer frees memory */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_ptr_null(buf.data);

    core_buffer_destroy(&buf);
}
END_TEST

static Suite *core_buffer_suite(void)
{
    Suite *s = suite_create("core_buffer");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_core_buffer_init);
    tcase_add_test(tc_core, test_core_buffer_append_invalid);
    tcase_add_test(tc_core, test_core_buffer_append_basic);
    tcase_add_test(tc_core, test_core_buffer_append_growth);
    tcase_add_test(tc_core, test_core_buffer_append_wrapped_external);
    tcase_add_test(tc_core, test_core_buffer_getters_clear_shrink);

    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = core_buffer_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
