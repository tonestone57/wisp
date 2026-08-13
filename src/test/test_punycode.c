#include <check.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/punycode.h"

START_TEST(test_punycode_encode_basic)
{
    punycode_uint input[] = { 109, 252, 110, 99, 104, 101, 110 }; /* münchen */
    unsigned char case_flags[] = { 1, 0, 0, 0, 0, 1, 0 }; /* MnchEn */
    size_t input_len = 7;
    char output[256];
    size_t output_len = 256;

    int status = punycode_encode(input_len, input, case_flags, &output_len, output);
    ck_assert_int_eq(status, punycode_success);
    output[output_len] = '\0';
    ck_assert_str_eq(output, "MnchEn-3ya");
}
END_TEST

START_TEST(test_punycode_decode_basic)
{
    char input[] = "MnchEn-3ya";
    size_t input_len = strlen(input);
    punycode_uint dec_out[256];
    unsigned char dec_flags[256];
    size_t dec_len = 256;

    int status = punycode_decode(input_len, input, &dec_len, dec_out, dec_flags);
    ck_assert_int_eq(status, punycode_success);
    ck_assert_int_eq(dec_len, 7);

    ck_assert_int_eq(dec_out[0], 77);
    ck_assert_int_eq(dec_flags[0], 1);
    ck_assert_int_eq(dec_out[1], 252);
    ck_assert_int_eq(dec_flags[1], 0);
    ck_assert_int_eq(dec_out[5], 69);
    ck_assert_int_eq(dec_flags[5], 1);
}
END_TEST

START_TEST(test_punycode_encode_big_output)
{
    punycode_uint input[] = { 109, 252, 110, 99, 104, 101, 110 };
    size_t input_len = 7;
    char output[5];
    size_t output_len = 5; /* Too small */

    int status = punycode_encode(input_len, input, NULL, &output_len, output);
    ck_assert_int_eq(status, punycode_big_output);
}
END_TEST

START_TEST(test_punycode_decode_big_output)
{
    char input[] = "mnchen-3ya";
    size_t input_len = strlen(input);
    punycode_uint dec_out[2];
    size_t dec_len = 2; /* Too small */

    int status = punycode_decode(input_len, input, &dec_len, dec_out, NULL);
    ck_assert_int_eq(status, punycode_big_output);
}
END_TEST

START_TEST(test_punycode_decode_bad_input)
{
    char input[] = "mnchen-3ya\xFF";
    size_t input_len = strlen(input);
    punycode_uint dec_out[256];
    size_t dec_len = 256;

    int status = punycode_decode(input_len, input, &dec_len, dec_out, NULL);
    ck_assert_int_eq(status, punycode_bad_input);
}
END_TEST

static Suite *punycode_suite_create(void)
{
    Suite *s = suite_create("Punycode");
    TCase *tc = tcase_create("punycode");
    tcase_add_test(tc, test_punycode_encode_basic);
    tcase_add_test(tc, test_punycode_decode_basic);
    tcase_add_test(tc, test_punycode_encode_big_output);
    tcase_add_test(tc, test_punycode_decode_big_output);
    tcase_add_test(tc, test_punycode_decode_bad_input);
    suite_add_tcase(s, tc);
    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    SRunner *sr = srunner_create(punycode_suite_create());
    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
