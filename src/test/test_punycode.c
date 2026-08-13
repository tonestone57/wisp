#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/punycode.h"

struct test_case {
    punycode_uint input[32];
    size_t input_length;
    const char *expected_output;
};

static const struct test_case test_cases[] = {
    // München -> Mnchen-3ya
    {
        .input = { 0x4d, 0xfc, 0x6e, 0x63, 0x68, 0x65, 0x6e },
        .input_length = 7,
        .expected_output = "Mnchen-3ya",
    },
    // MajiでKoiする5秒前 -> MajiKoi5-783gue6qz075azm5e
    {
        .input = { 0x4d, 0x61, 0x6a, 0x69, 0x3067, 0x4b, 0x6f, 0x69, 0x3059, 0x308b, 0x35, 0x79d2, 0x524d },
        .input_length = 13,
        .expected_output = "MajiKoi5-783gue6qz075azm5e",
    },
    // 「ひゃっほう」 -> 16jc4ole2crcwc
    {
        .input = { 0x300c, 0x3072, 0x3083, 0x3063, 0x307b, 0x3046, 0x300d },
        .input_length = 7,
        .expected_output = "16jc4ole2crcwc",
    },
    // hello-world -> hello-world-
    {
        .input = { 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2d, 0x77, 0x6f, 0x72, 0x6c, 0x64 },
        .input_length = 11,
        .expected_output = "hello-world-",
    },
    // 你好世界 -> rhq34a65tw32a
    {
        .input = { 0x4f60, 0x597d, 0x4e16, 0x754c },
        .input_length = 4,
        .expected_output = "rhq34a65tw32a",
    },
    // αβγδε -> mxacdef
    {
        .input = { 0x3b1, 0x3b2, 0x3b3, 0x3b4, 0x3b5 },
        .input_length = 5,
        .expected_output = "mxacdef",
    },
};

START_TEST(test_punycode_encode_basic)
{
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        char output[256];
        size_t output_length = sizeof(output);
        enum punycode_status status = punycode_encode(
            test_cases[i].input_length,
            test_cases[i].input,
            NULL,
            &output_length,
            output
        );
        ck_assert_int_eq(status, punycode_success);

        // Null-terminate the output for string comparison
        output[output_length] = '\0';
        ck_assert_str_eq(output, test_cases[i].expected_output);
    }
}
END_TEST

START_TEST(test_punycode_decode_basic)
{
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        punycode_uint output[256];
        size_t output_length = sizeof(output) / sizeof(output[0]);
        enum punycode_status status = punycode_decode(
            strlen(test_cases[i].expected_output),
            test_cases[i].expected_output,
            &output_length,
            output,
            NULL
        );
        ck_assert_int_eq(status, punycode_success);

        ck_assert_int_eq(output_length, test_cases[i].input_length);
        for (size_t j = 0; j < output_length; j++) {
            ck_assert_int_eq(output[j], test_cases[i].input[j]);
        }
    }
}
END_TEST

static Suite *punycode_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("punycode");
    tc = tcase_create("core");

    tcase_add_test(tc, test_punycode_encode_basic);
    tcase_add_test(tc, test_punycode_decode_basic);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = punycode_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
