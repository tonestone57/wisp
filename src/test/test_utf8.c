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
