#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/utils/utf8.h"

START_TEST(test_utf8_to_ucs4_ascii)
{
    ck_assert_int_eq(utf8_to_ucs4("A", 1), 65);
    ck_assert_int_eq(utf8_to_ucs4("a", 1), 97);
}
END_TEST

START_TEST(test_utf8_to_ucs4_multibyte)
{
    // 2-byte UTF-8 for '¢' (U+00A2) -> C2 A2
    ck_assert_int_eq(utf8_to_ucs4("\xc2\xa2", 2), 0x00A2);
    // 3-byte UTF-8 for '€' (U+20AC) -> E2 82 AC
    ck_assert_int_eq(utf8_to_ucs4("\xe2\x82\xac", 3), 0x20AC);
    // 4-byte UTF-8 for '𐍈' (U+10348) -> F0 90 8D 88
    ck_assert_int_eq(utf8_to_ucs4("\xf0\x90\x8d\x88", 4), 0x10348);
}
END_TEST

START_TEST(test_utf8_to_ucs4_invalid)
{
    // Invalid continuation byte
    ck_assert_int_eq(utf8_to_ucs4("\x80", 1), 0xfffd);
    // Overlong encoding for 'A' -> C1 81
    ck_assert_int_eq(utf8_to_ucs4("\xc1\x81", 2), 0xfffd);
}
END_TEST

START_TEST(test_utf8_to_ucs4_empty)
{
    ck_assert_int_eq(utf8_to_ucs4("", 0), 0xfffd);
}
END_TEST


START_TEST(test_utf8_convert_empty)
{
    char *result = NULL;
    size_t result_len = 0;

    // Empty string (should bypass iconv)
    ck_assert_int_eq(utf8_from_enc("", "ISO-8859-1", 0, &result, &result_len), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result_len, 0);
    ck_assert_int_eq(result[0], '\0');
    free(result);
    result = NULL;
}
END_TEST

START_TEST(test_utf8_convert_same_encoding)
{
    char *result = NULL;
    size_t result_len = 0;

    // Source and dest are the same (should bypass iconv)
    ck_assert_int_eq(utf8_from_enc("Hello", "UTF-8", 5, &result, &result_len), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result_len, 5);
    ck_assert_str_eq(result, "Hello");
    free(result);
    result = NULL;
}
END_TEST

START_TEST(test_utf8_convert_length_zero)
{
    char *result = NULL;
    size_t result_len = 0;

    // slen = 0 forces strlen internal call
    ck_assert_int_eq(utf8_from_enc("Test", "UTF-8", 0, &result, &result_len), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result_len, 4);
    ck_assert_str_eq(result, "Test");
    free(result);
    result = NULL;
}
END_TEST

START_TEST(test_utf8_convert_valid_conversion)
{
    char *result = NULL;
    size_t result_len = 0;

    // Valid conversion from ISO-8859-1 to UTF-8
    // '¢' in ISO-8859-1 is 0xA2. In UTF-8 it's C2 A2.
    ck_assert_int_eq(utf8_from_enc("\xA2", "ISO-8859-1", 1, &result, &result_len), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_int_eq(result_len, 2);
    ck_assert_str_eq(result, "\xC2\xA2");
    free(result);
    result = NULL;

    // Valid conversion from UTF-8 to ISO-8859-1
    ck_assert_int_eq(utf8_to_enc("\xC2\xA2", "ISO-8859-1", 2, &result), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    // length is not returned by utf8_to_enc, we check the string
    ck_assert_int_eq((unsigned char)result[0], 0xA2);
    ck_assert_int_eq(result[1], '\0');
    free(result);
    result = NULL;
}
END_TEST

START_TEST(test_utf8_convert_invalid_encoding_name)
{
    char *result = NULL;
    size_t result_len = 0;

    // Invalid encoding name should fail (either EINVAL->BAD_ENCODING or NOMEM)
    nserror err = utf8_from_enc("Test", "NONEXISTENT-ENCODING", 4, &result, &result_len);
    ck_assert(err == NSERROR_BAD_ENCODING || err == NSERROR_NOMEM);
}
END_TEST

START_TEST(test_utf8_convert_bad_encoding_data)
{
    char *result = NULL;

    // Invalid UTF-8 to ISO-8859-1 (0x80 is an invalid continuation byte in UTF-8 context when standing alone)
    ck_assert_int_eq(utf8_to_enc("\x80", "ISO-8859-1", 1, &result), NSERROR_BAD_ENCODING);
}
END_TEST


START_TEST(test_utf8_to_html_empty)
{
    char *result = NULL;
    ck_assert_int_eq(utf8_to_html("", "UTF-8", 0, &result), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "");
    free(result);

    ck_assert_int_eq(utf8_to_html("test", "UTF-8", 0, &result), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "test");
    free(result);
}
END_TEST

START_TEST(test_utf8_to_html_no_escape)
{
    char *result = NULL;
    ck_assert_int_eq(utf8_to_html("Hello World", "UTF-8", 0, &result), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "Hello World");
    free(result);
}
END_TEST

START_TEST(test_utf8_to_html_escape_basic)
{
    char *result = NULL;
    ck_assert_int_eq(utf8_to_html("a < b & c > d", "UTF-8", 0, &result), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "a &#x00003c; b &#x000026; c &#x00003e; d");
    free(result);
}
END_TEST

START_TEST(test_utf8_to_html_escape_unrepresentable)
{
    char *result = NULL;
    ck_assert_int_eq(utf8_to_html("Price: \xc2\xa2", "US-ASCII", 0, &result), NSERROR_OK);
    ck_assert_ptr_nonnull(result);
    ck_assert_str_eq(result, "Price: &#x0000a2;");
    free(result);
}
END_TEST

START_TEST(test_utf8_to_html_bad_encoding)
{
    char *result = NULL;
    nserror err = utf8_to_html("test", "NONEXISTENT-ENCODING", 0, &result);
    ck_assert(err == NSERROR_BAD_ENCODING || err == NSERROR_NOMEM);
}
END_TEST

static Suite *utf8_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("utf8");
    tc = tcase_create("core");

    tcase_add_test(tc, test_utf8_to_ucs4_ascii);
    tcase_add_test(tc, test_utf8_to_ucs4_multibyte);
    tcase_add_test(tc, test_utf8_to_ucs4_invalid);
    tcase_add_test(tc, test_utf8_to_ucs4_empty);

    tcase_add_test(tc, test_utf8_convert_empty);
    tcase_add_test(tc, test_utf8_convert_same_encoding);
    tcase_add_test(tc, test_utf8_convert_length_zero);
    tcase_add_test(tc, test_utf8_convert_valid_conversion);
    tcase_add_test(tc, test_utf8_convert_invalid_encoding_name);
    tcase_add_test(tc, test_utf8_convert_bad_encoding_data);

    tcase_add_test(tc, test_utf8_to_html_empty);
    tcase_add_test(tc, test_utf8_to_html_no_escape);
    tcase_add_test(tc, test_utf8_to_html_escape_basic);
    tcase_add_test(tc, test_utf8_to_html_escape_unrepresentable);
    tcase_add_test(tc, test_utf8_to_html_bad_encoding);
    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = utf8_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
