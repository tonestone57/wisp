#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/utils/core_buffer.h"
#include "wisp/utils/errors.h"

START_TEST(test_core_buffer_destroy_null)
{
    /* Should safely return without crashing on NULL */
    core_buffer_destroy(NULL);
}
END_TEST

START_TEST(test_core_buffer_destroy_unallocated)
{
    core_buffer buf;
    nserror err = core_buffer_init(&buf);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);

    /* Destroying an unallocated buffer should reset fields cleanly */
    core_buffer_destroy(&buf);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);
}
END_TEST

START_TEST(test_core_buffer_destroy_allocated)
{
    core_buffer buf;
    nserror err = core_buffer_init(&buf);
    ck_assert_int_eq(err, NSERROR_OK);

    const uint8_t sample_data[] = "Test data for core_buffer_destroy";
    size_t sample_len = sizeof(sample_data);

    err = core_buffer_append(&buf, sample_data, sample_len);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_nonnull(buf.data);
    ck_assert_uint_eq(buf.length, sample_len);
    ck_assert_uint_ge(buf.allocated, sample_len);

    /* Destroying an allocated buffer should free data and reset fields */
    core_buffer_destroy(&buf);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);
}
END_TEST

START_TEST(test_core_buffer_destroy_wrapped_external)
{
    core_buffer buf;
    nserror err = core_buffer_init(&buf);
    ck_assert_int_eq(err, NSERROR_OK);

    uint8_t external_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    size_t external_len = sizeof(external_data);

    err = core_buffer_wrap_external(&buf, external_data, external_len);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_eq(core_buffer_data(&buf), external_data);
    ck_assert_uint_eq(core_buffer_length(&buf), external_len);
    ck_assert_uint_eq(buf.allocated, 0);

    /* core_buffer_destroy on wrapped external buffer must NOT free external data */
    core_buffer_destroy(&buf);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);

    /* Confirm external memory is intact */
    ck_assert_uint_eq(external_data[0], 0x01);
    ck_assert_uint_eq(external_data[4], 0x05);
}
END_TEST

START_TEST(test_core_buffer_init)
{
    /* NULL pointer error handling */
    ck_assert_int_eq(core_buffer_init(NULL), NSERROR_BAD_PARAMETER);

    core_buffer buf;
    memset(&buf, 0xFF, sizeof(buf));
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);
}
END_TEST

START_TEST(test_core_buffer_append_and_reserve)
{
    core_buffer buf;
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* Parameter validation */
    const uint8_t chunk1[] = "Hello ";
    ck_assert_int_eq(core_buffer_append(NULL, chunk1, 6), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 6), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_reserve(NULL, 100), NSERROR_BAD_PARAMETER);

    /* Valid appends */
    ck_assert_int_eq(core_buffer_append(&buf, chunk1, 6), NSERROR_OK);
    ck_assert_uint_eq(core_buffer_length(&buf), 6);
    ck_assert_mem_eq(core_buffer_data(&buf), "Hello ", 6);

    const uint8_t chunk2[] = "World!";
    ck_assert_int_eq(core_buffer_append(&buf, chunk2, 6), NSERROR_OK);
    ck_assert_uint_eq(core_buffer_length(&buf), 12);
    ck_assert_mem_eq(core_buffer_data(&buf), "Hello World!", 12);

    /* Reserve larger buffer capacity */
    ck_assert_int_eq(core_buffer_reserve(&buf, 256), NSERROR_OK);
    ck_assert_uint_ge(buf.allocated, 256);
    ck_assert_uint_eq(core_buffer_length(&buf), 12);
    ck_assert_mem_eq(core_buffer_data(&buf), "Hello World!", 12);

    /* Reserve smaller capacity (no-op) */
    size_t cur_alloc = buf.allocated;
    ck_assert_int_eq(core_buffer_reserve(&buf, 50), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, cur_alloc);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_shrink)
{
    ck_assert_int_eq(core_buffer_shrink(NULL), NSERROR_BAD_PARAMETER);

    core_buffer buf;
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    const uint8_t data[] = "1234567890";
    ck_assert_int_eq(core_buffer_append(&buf, data, 10), NSERROR_OK);
    ck_assert_int_eq(core_buffer_reserve(&buf, 1024), NSERROR_OK);
    ck_assert_uint_ge(buf.allocated, 1024);

    /* Shrink to length */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, 10);
    ck_assert_uint_eq(buf.length, 10);
    ck_assert_mem_eq(core_buffer_data(&buf), "1234567890", 10);

    /* Clear and shrink to zero */
    core_buffer_clear(&buf);
    ck_assert_uint_eq(core_buffer_length(&buf), 0);
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.allocated, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_wrap_and_accessors)
{
    uint8_t dummy[] = "Dummy data";

    ck_assert_int_eq(core_buffer_wrap_external(NULL, dummy, 10), NSERROR_BAD_PARAMETER);
    ck_assert_ptr_null(core_buffer_data(NULL));
    ck_assert_uint_eq(core_buffer_length(NULL), 0);

    core_buffer_clear(NULL); /* Should safely do nothing */

    core_buffer buf;
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    const uint8_t msg[] = "Clear test";
    ck_assert_int_eq(core_buffer_append(&buf, msg, 10), NSERROR_OK);
    ck_assert_uint_eq(core_buffer_length(&buf), 10);

    core_buffer_clear(&buf);
    ck_assert_uint_eq(core_buffer_length(&buf), 0);
    ck_assert_ptr_nonnull(core_buffer_data(&buf));
    ck_assert_uint_gt(buf.allocated, 0);

    core_buffer_destroy(&buf);
}
END_TEST

static Suite *core_buffer_suite(void)
{
    Suite *s = suite_create("Core Buffer");
    TCase *tc_destroy = tcase_create("Destroy");

    tcase_add_test(tc_destroy, test_core_buffer_destroy_null);
    tcase_add_test(tc_destroy, test_core_buffer_destroy_unallocated);
    tcase_add_test(tc_destroy, test_core_buffer_destroy_allocated);
    tcase_add_test(tc_destroy, test_core_buffer_destroy_wrapped_external);
    suite_add_tcase(s, tc_destroy);

    TCase *tc_ops = tcase_create("Operations");
    tcase_add_test(tc_ops, test_core_buffer_init);
    tcase_add_test(tc_ops, test_core_buffer_append_and_reserve);
    tcase_add_test(tc_ops, test_core_buffer_shrink);
    tcase_add_test(tc_ops, test_core_buffer_wrap_and_accessors);
    suite_add_tcase(s, tc_ops);

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
