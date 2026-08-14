#include <check.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/utils/utf8proc_wrapper.h"

START_TEST(test_wisp_utf8proc_NFD)
{
    const char *ascii_str = "hello_world";
    uint8_t *result_ascii = wisp_utf8proc_NFD((const uint8_t *)ascii_str);
    ck_assert_ptr_nonnull(result_ascii);
    ck_assert_str_eq((const char *)result_ascii, ascii_str);
    free(result_ascii);

    const char *non_ascii_str = "\xC3\xA9"; // é
    uint8_t *result_non_ascii = wisp_utf8proc_NFD((const uint8_t *)non_ascii_str);
    ck_assert_ptr_nonnull(result_non_ascii);
    ck_assert_str_eq((const char *)result_non_ascii, "e\xCC\x81"); // e + combining acute accent
    free(result_non_ascii);
}
END_TEST

START_TEST(test_wisp_utf8proc_NFC)
{
    const char *ascii_str = "hello_world";
    uint8_t *result_ascii = wisp_utf8proc_NFC((const uint8_t *)ascii_str);
    ck_assert_ptr_nonnull(result_ascii);
    ck_assert_str_eq((const char *)result_ascii, ascii_str);
    free(result_ascii);

    const char *non_ascii_str = "e\xCC\x81"; // e + combining acute accent
    uint8_t *result_non_ascii = wisp_utf8proc_NFC((const uint8_t *)non_ascii_str);
    ck_assert_ptr_nonnull(result_non_ascii);
    ck_assert_str_eq((const char *)result_non_ascii, "\xC3\xA9"); // é
    free(result_non_ascii);
}
END_TEST

START_TEST(test_wisp_utf8proc_NFKD)
{
    const char *ascii_str = "hello_world";
    uint8_t *result_ascii = wisp_utf8proc_NFKD((const uint8_t *)ascii_str);
    ck_assert_ptr_nonnull(result_ascii);
    ck_assert_str_eq((const char *)result_ascii, ascii_str);
    free(result_ascii);

    const char *non_ascii_str = "\xE2\x85\xA3"; // Roman numeral IV (U+2163)
    uint8_t *result_non_ascii = wisp_utf8proc_NFKD((const uint8_t *)non_ascii_str);
    ck_assert_ptr_nonnull(result_non_ascii);
    ck_assert_str_eq((const char *)result_non_ascii, "IV");
    free(result_non_ascii);
}
END_TEST

START_TEST(test_wisp_utf8proc_NFKC)
{
    const char *ascii_str = "hello_world";
    uint8_t *result_ascii = wisp_utf8proc_NFKC((const uint8_t *)ascii_str);
    ck_assert_ptr_nonnull(result_ascii);
    ck_assert_str_eq((const char *)result_ascii, ascii_str);
    free(result_ascii);

    const char *non_ascii_str = "\xE2\x85\xA3"; // Roman numeral IV (U+2163)
    uint8_t *result_non_ascii = wisp_utf8proc_NFKC((const uint8_t *)non_ascii_str);
    ck_assert_ptr_nonnull(result_non_ascii);
    ck_assert_str_eq((const char *)result_non_ascii, "IV");
    free(result_non_ascii);
}
END_TEST

static Suite *utf8proc_wrapper_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("utf8proc_wrapper");
    tc = tcase_create("core");

    tcase_add_test(tc, test_wisp_utf8proc_NFD);
    tcase_add_test(tc, test_wisp_utf8proc_NFC);
    tcase_add_test(tc, test_wisp_utf8proc_NFKD);
    tcase_add_test(tc, test_wisp_utf8proc_NFKC);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = utf8proc_wrapper_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
