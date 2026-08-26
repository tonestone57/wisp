/*
 * Test core_buffer utility functions.
 */

#include <check.h>
#include <stdlib.h>
#include <string.h>

#include <wisp/utils/core_buffer.h>
#include <wisp/utils/errors.h>

START_TEST(test_core_buffer_init_null)
{
    nserror err = core_buffer_init(NULL);
    ck_assert_int_eq(err, NSERROR_BAD_PARAMETER);
}
END_TEST

START_TEST(test_core_buffer_init_valid)
{
    core_buffer buf;
    memset(&buf, 0xFF, sizeof(buf)); /* Dirty memory */

    nserror err = core_buffer_init(&buf);
    ck_assert_int_eq(err, NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.length, 0);
    ck_assert_uint_eq(buf.allocated, 0);
}
END_TEST

START_TEST(test_core_buffer_append_null)
{
    core_buffer buf;
    const uint8_t test_data[] = "hello";

    core_buffer_init(&buf);

    /* NULL buffer */
    ck_assert_int_eq(core_buffer_append(NULL, test_data, 5), NSERROR_BAD_PARAMETER);

    /* NULL data with non-zero length */
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 5), NSERROR_BAD_PARAMETER);

    /* Zero length with NULL or valid data should succeed */
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, test_data, 0), NSERROR_OK);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_append_and_reserve)
{
    core_buffer buf;
    const uint8_t data1[] = "Hello ";
    const uint8_t data2[] = "World!";

    core_buffer_init(&buf);

    /* Reserve space */
    ck_assert_int_eq(core_buffer_reserve(NULL, 10), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_reserve(&buf, 100), NSERROR_OK);
    ck_assert_uint_ge(buf.allocated, 100);

    /* Append first slice */
    ck_assert_int_eq(core_buffer_append(&buf, data1, strlen((const char *)data1)), NSERROR_OK);
    ck_assert_uint_eq(core_buffer_length(&buf), strlen((const char *)data1));

    /* Append second slice */
    ck_assert_int_eq(core_buffer_append(&buf, data2, strlen((const char *)data2)), NSERROR_OK);
    ck_assert_uint_eq(core_buffer_length(&buf), strlen("Hello World!"));

    ck_assert_mem_eq(core_buffer_data(&buf), "Hello World!", strlen("Hello World!"));

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_wrap_external)
{
    core_buffer buf;
    uint8_t external_data[] = "External Buffer Data";
    size_t len = strlen((const char *)external_data);

    core_buffer_init(&buf);

    ck_assert_int_eq(core_buffer_wrap_external(NULL, external_data, len), NSERROR_BAD_PARAMETER);

    ck_assert_int_eq(core_buffer_wrap_external(&buf, external_data, len), NSERROR_OK);
    ck_assert_ptr_eq(core_buffer_data(&buf), external_data);
    ck_assert_uint_eq(core_buffer_length(&buf), len);
    ck_assert_uint_eq(buf.allocated, 0);

    /* Destroying wrapped external buffer should not free external memory */
    core_buffer_destroy(&buf);
    ck_assert_ptr_null(core_buffer_data(&buf));
    ck_assert_uint_eq(core_buffer_length(&buf), 0);
}
END_TEST

START_TEST(test_core_buffer_shrink_and_clear)
{
    core_buffer buf;
    const uint8_t data[] = "12345678901234567890";

    core_buffer_init(&buf);

    ck_assert_int_eq(core_buffer_shrink(NULL), NSERROR_BAD_PARAMETER);

    /* Shrinking empty unallocated buffer */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);

    /* Append and shrink */
    ck_assert_int_eq(core_buffer_append(&buf, data, 20), NSERROR_OK);
    ck_assert_uint_gt(buf.allocated, 20);

    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_uint_eq(buf.allocated, 20);

    /* Clear buffer length without freeing */
    core_buffer_clear(NULL); /* Should handle NULL safely */
    core_buffer_clear(&buf);
    ck_assert_uint_eq(core_buffer_length(&buf), 0);
    ck_assert_uint_eq(buf.allocated, 20);

    /* Shrinking cleared buffer (length == 0) frees data */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_ptr_null(buf.data);
    ck_assert_uint_eq(buf.allocated, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(test_core_buffer_accessors_and_destroy_null)
{
    ck_assert_ptr_null(core_buffer_data(NULL));
    ck_assert_uint_eq(core_buffer_length(NULL), 0);

    /* Should handle NULL gracefully */
    core_buffer_destroy(NULL);
}
END_TEST

static TCase *core_buffer_case_create(void)
{
    TCase *tc = tcase_create("Core Buffer");

    tcase_add_test(tc, test_core_buffer_init_null);
    tcase_add_test(tc, test_core_buffer_init_valid);
    tcase_add_test(tc, test_core_buffer_append_null);
    tcase_add_test(tc, test_core_buffer_append_and_reserve);
    tcase_add_test(tc, test_core_buffer_wrap_external);
    tcase_add_test(tc, test_core_buffer_shrink_and_clear);
    tcase_add_test(tc, test_core_buffer_accessors_and_destroy_null);

    return tc;
}

static Suite *core_buffer_suite_create(void)
{
    Suite *s = suite_create("Core Buffer");

    suite_add_tcase(s, core_buffer_case_create());

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr;

    sr = srunner_create(core_buffer_suite_create());

    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
