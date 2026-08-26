/*
 * Unit tests for core_buffer.
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <wisp/utils/core_buffer.h>
#include <wisp/utils/errors.h>

START_TEST(test_core_buffer_init)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(NULL), NSERROR_BAD_PARAMETER);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_append_and_accessors)
{
    core_buffer buf;
    const uint8_t test_data[] = "Hello, Wisp!";
    size_t test_len = sizeof(test_data);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* Error cases */
    ck_assert_int_eq(core_buffer_append(NULL, test_data, test_len), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_append(&buf, NULL, test_len), NSERROR_BAD_PARAMETER);

    /* Null data with 0 length is OK */
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 0), NSERROR_OK);

    /* Accessors on empty/NULL */
    ck_assert_ptr_null(core_buffer_data(NULL));
    ck_assert_uint_eq(core_buffer_length(NULL), 0);

    /* Append valid data */
    ck_assert_int_eq(core_buffer_append(&buf, test_data, test_len), NSERROR_OK);
    ck_assert_ptr_nonnull(core_buffer_data(&buf));
    ck_assert_uint_eq(core_buffer_length(&buf), test_len);
    ck_assert_mem_eq(core_buffer_data(&buf), test_data, test_len);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_reserve)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_reserve(NULL, 100), NSERROR_BAD_PARAMETER);

    ck_assert_int_eq(core_buffer_reserve(&buf, 100), NSERROR_OK);
    ck_assert_uint_ge(buf.allocated, 100);

    size_t prev_alloc = buf.allocated;
    /* Reserving less than or equal to current allocated size is no-op */
    ck_assert_int_eq(core_buffer_reserve(&buf, 50), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, prev_alloc);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_shrink_null)
{
    ck_assert_int_eq(core_buffer_shrink(NULL), NSERROR_BAD_PARAMETER);
}
END_TEST

START_TEST(test_core_buffer_shrink_zero_length)
{
    core_buffer buf;

    /* Unallocated 0-length buffer */
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);

    /* Pre-allocated buffer with 0 length */
    ck_assert_int_eq(core_buffer_reserve(&buf, 128), NSERROR_OK);
    ck_assert_uint_ge(buf.allocated, 128);
    ck_assert_ptr_nonnull(buf.data);

    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_shrink_excess_capacity)
{
    core_buffer buf;
    const uint8_t test_data[] = "Shrink test payload data";
    size_t test_len = strlen((const char *)test_data);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_reserve(&buf, 256), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, test_data, test_len), NSERROR_OK);

    ck_assert_uint_gt(buf.allocated, test_len);
    ck_assert_uint_eq(buf.length, test_len);

    /* Shrink to exact length */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, test_len);
    ck_assert_uint_eq(buf.length, test_len);
    ck_assert_mem_eq(buf.data, test_data, test_len);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_shrink_exact_capacity)
{
    core_buffer buf;
    const uint8_t test_data[] = "Exact capacity buffer";
    size_t test_len = strlen((const char *)test_data);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, test_data, test_len), NSERROR_OK);

    /* First shrink to make capacity exact */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, test_len);

    /* Second shrink when allocated == length */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, test_len);
    ck_assert_uint_eq(buf.length, test_len);
    ck_assert_mem_eq(buf.data, test_data, test_len);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_shrink_external)
{
    core_buffer buf;
    uint8_t external_data[] = "External buffer memory";
    size_t ext_len = strlen((const char *)external_data);

    ck_assert_int_eq(core_buffer_wrap_external(NULL, external_data, ext_len), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, external_data, ext_len), NSERROR_OK);

    ck_assert_ptr_eq(buf.data, external_data);
    ck_assert_uint_eq(buf.length, ext_len);
    ck_assert_uint_eq(buf.allocated, 0);

    /* Shrink on external buffer (allocated == 0) should do nothing */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_ptr_eq(buf.data, external_data);
    ck_assert_uint_eq(buf.length, ext_len);
    ck_assert_uint_eq(buf.allocated, 0);

    /* Destroying unowned buffer should not attempt to free data */
    core_buffer_destroy(&buf);
    ck_assert_ptr_null(buf.data);
}
END_TEST

START_TEST(test_core_buffer_clear)
{
    core_buffer buf;
    const uint8_t test_data[] = "Clear test payload";
    size_t test_len = strlen((const char *)test_data);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, test_data, test_len), NSERROR_OK);

    size_t alloc_before = buf.allocated;
    core_buffer_clear(NULL); /* Should handle gracefully */
    core_buffer_clear(&buf);

    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, alloc_before);
    ck_assert_ptr_nonnull(buf.data);

    /* Shrinking after clear should free memory */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, 0);
    ck_assert_ptr_null(buf.data);

    core_buffer_destroy(&buf);
}
END_TEST

static Suite *core_buffer_suite(void)
{
    Suite *s = suite_create("core_buffer");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_core_buffer_init);
    tcase_add_test(tc_core, test_core_buffer_append_and_accessors);
    tcase_add_test(tc_core, test_core_buffer_reserve);
    tcase_add_test(tc_core, test_core_buffer_shrink_null);
    tcase_add_test(tc_core, test_core_buffer_shrink_zero_length);
    tcase_add_test(tc_core, test_core_buffer_shrink_excess_capacity);
    tcase_add_test(tc_core, test_core_buffer_shrink_exact_capacity);
    tcase_add_test(tc_core, test_core_buffer_shrink_external);
    tcase_add_test(tc_core, test_core_buffer_clear);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    Suite *s = core_buffer_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
