/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wisp/utils/utf8proc_wrapper.h"

/* 1. ASCII Detection Tests */
START_TEST(test_wisp_is_ascii)
{
    /* Empty string */
    ck_assert(wisp_is_ascii("", 0));

    /* Short ASCII */
    const char *ascii_short = "Hello";
    ck_assert(wisp_is_ascii(ascii_short, strlen(ascii_short)));

    /* Exact 16-byte SIMD boundary ASCII */
    const char *ascii_16 = "1234567890123456";
    ck_assert(wisp_is_ascii(ascii_16, 16));

    /* Exact 32-byte ASCII */
    const char *ascii_32 = "12345678901234567890123456789012";
    ck_assert(wisp_is_ascii(ascii_32, 32));

    /* Longer ASCII */
    const char *ascii_long = "This is a longer ASCII string exceeding multiple SIMD chunks.";
    ck_assert(wisp_is_ascii(ascii_long, strlen(ascii_long)));

    /* Non-ASCII at start, middle, end, boundaries */
    const char *non_ascii_start = "\x80Hello";
    const char *non_ascii_mid = "Hello\xFFWorld";
    const char *non_ascii_end = "HelloWorld\xC0";
    const char *non_ascii_at_15 = "123456789012345\x80";
    const char *non_ascii_at_16 = "1234567890123456\x80";

    ck_assert(!wisp_is_ascii(non_ascii_start, strlen(non_ascii_start)));
    ck_assert(!wisp_is_ascii(non_ascii_mid, strlen(non_ascii_mid)));
    ck_assert(!wisp_is_ascii(non_ascii_end, strlen(non_ascii_end)));
    ck_assert(!wisp_is_ascii(non_ascii_at_15, strlen(non_ascii_at_15)));
    ck_assert(!wisp_is_ascii(non_ascii_at_16, strlen(non_ascii_at_16)));
}
END_TEST

/* 2. UTF-8 Validation Tests */
START_TEST(test_wisp_validate_utf8)
{
    /* Valid ASCII */
    const char *ascii = "Simple ASCII string";
    ck_assert(wisp_validate_utf8(ascii, strlen(ascii)));

    /* Valid 2-byte sequence (e.g. Greek / Spanish é) */
    const char *valid_2byte = "\xC3\xA9"; // é
    ck_assert(wisp_validate_utf8(valid_2byte, strlen(valid_2byte)));

    /* Valid 3-byte sequence (e.g. Euro sign U+20AC: \xE2\x82\xAC) */
    const char *valid_3byte = "\xE2\x82\xAC";
    ck_assert(wisp_validate_utf8(valid_3byte, strlen(valid_3byte)));

    /* Valid 4-byte sequence (e.g. Emoji U+1F600: \xF0\x9F\x98\x80) */
    const char *valid_4byte = "\xF0\x9F\x98\x80";
    ck_assert(wisp_validate_utf8(valid_4byte, strlen(valid_4byte)));

    /* Invalid: Truncated 2-byte sequence */
    ck_assert(!wisp_validate_utf8("\xC3", 1));

    /* Invalid: Truncated 3-byte sequence */
    ck_assert(!wisp_validate_utf8("\xE2\x82", 2));

    /* Invalid: Truncated 4-byte sequence */
    ck_assert(!wisp_validate_utf8("\xF0\x9F\x98", 3));

    /* Invalid: Invalid continuation byte */
    ck_assert(!wisp_validate_utf8("\xC3\x20", 2));
    ck_assert(!wisp_validate_utf8("\xE2\x20\xAC", 3));
    ck_assert(!wisp_validate_utf8("\xF0\x9F\x20\x80", 4));

    /* Invalid: Overlong encodings */
    ck_assert(!wisp_validate_utf8("\xC0\xAF", 2)); // overlong '/'
    ck_assert(!wisp_validate_utf8("\xC1\x80", 2)); // overlong
    ck_assert(!wisp_validate_utf8("\xE0\x9F\x80", 3)); // overlong 3-byte
    ck_assert(!wisp_validate_utf8("\xF0\x8F\x80\x80", 4)); // overlong 4-byte

    /* Invalid: UTF-16 surrogates (U+D800 - U+DFFF) */
    ck_assert(!wisp_validate_utf8("\xED\xA0\x80", 3)); // U+D800

    /* Invalid: Beyond U+10FFFF */
    ck_assert(!wisp_validate_utf8("\xF4\x90\x80\x80", 4)); // U+110000
    ck_assert(!wisp_validate_utf8("\xF5\x80\x80\x80", 4)); // > 0xF4

    /* Invalid: Unexpected continuation byte as lead byte */
    ck_assert(!wisp_validate_utf8("\x80", 1));
}
END_TEST

/* 3. SIMD strcmp & streq Tests */
START_TEST(test_wisp_simd_strcmp_and_streq)
{
    /* NULL handling */
    ck_assert_int_eq(wisp_simd_strcmp(NULL, NULL), 0);
    ck_assert(wisp_simd_streq(NULL, NULL));
    ck_assert(wisp_simd_strcmp("a", NULL) > 0);
    ck_assert(!wisp_simd_streq("a", NULL));
    ck_assert(wisp_simd_strcmp(NULL, "a") < 0);
    ck_assert(!wisp_simd_streq(NULL, "a"));

    /* Equal strings */
    ck_assert_int_eq(wisp_simd_strcmp("", ""), 0);
    ck_assert(wisp_simd_streq("", ""));
    ck_assert_int_eq(wisp_simd_strcmp("hello", "hello"), 0);
    ck_assert(wisp_simd_streq("hello", "hello"));

    /* Long equal strings (across SIMD 16/32 byte boundaries) */
    const char *long1 = "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const char *long2 = "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    ck_assert_int_eq(wisp_simd_strcmp(long1, long2), 0);
    ck_assert(wisp_simd_streq(long1, long2));

    /* Mismatches at various offsets */
    ck_assert(wisp_simd_strcmp("hello", "hellx") < 0);
    ck_assert(!wisp_simd_streq("hello", "hellx"));
    ck_assert(wisp_simd_strcmp("hellx", "hello") > 0);
    ck_assert(!wisp_simd_streq("hellx", "hello"));

    /* Length differences */
    ck_assert(wisp_simd_strcmp("hello", "hello_world") < 0);
    ck_assert(!wisp_simd_streq("hello", "hello_world"));
    ck_assert(wisp_simd_strcmp("hello_world", "hello") > 0);
    ck_assert(!wisp_simd_streq("hello_world", "hello"));

    /* Mismatch beyond 16 bytes */
    const char *mismatch1 = "abcdefghijklmnopqrstuvwxyz1234567890_A";
    const char *mismatch2 = "abcdefghijklmnopqrstuvwxyz1234567890_B";
    ck_assert(wisp_simd_strcmp(mismatch1, mismatch2) < 0);
    ck_assert(!wisp_simd_streq(mismatch1, mismatch2));
}
END_TEST

/* 4. ASCII Case Mapping Tests */
START_TEST(test_wisp_ascii_case_mapping)
{
    const char *src = "Hello World 123! @#$ ABCxyz";
    size_t len = strlen(src);
    char dst_lower[64];
    char dst_upper[64];

    wisp_ascii_tolower(src, dst_lower, len);
    dst_lower[len] = '\0';
    ck_assert_str_eq(dst_lower, "hello world 123! @#$ abcxyz");

    wisp_ascii_toupper(src, dst_upper, len);
    dst_upper[len] = '\0';
    ck_assert_str_eq(dst_upper, "HELLO WORLD 123! @#$ ABCXYZ");

    /* SIMD chunk boundaries (15, 16, 32 bytes) */
    const char *src_16 = "ABCDEFGHIJKLMNOP"; // 16 chars
    char dst_16[17];
    wisp_ascii_tolower(src_16, dst_16, 16);
    dst_16[16] = '\0';
    ck_assert_str_eq(dst_16, "abcdefghijklmnop");

    const char *src_32 = "abcdefghijklmnopqrstuvwxyz123456"; // 32 chars
    char dst_32[33];
    wisp_ascii_toupper(src_32, dst_32, 32);
    dst_32[32] = '\0';
    ck_assert_str_eq(dst_32, "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456");

    /* Empty string */
    char empty_buf[1] = {'X'};
    wisp_ascii_tolower("", empty_buf, 0);
    ck_assert_int_eq(empty_buf[0], 'X');
}
END_TEST

/* 5. ASCII <-> UTF-32 Conversion Tests */
START_TEST(test_wisp_ascii_utf32_conversion)
{
    const char *ascii_src = "Testing 123!";
    size_t len = strlen(ascii_src);

    int32_t utf32_buf[32];
    char ascii_dst[32];

    /* ASCII -> UTF32 */
    wisp_ascii_to_utf32(ascii_src, utf32_buf, len);
    for (size_t i = 0; i < len; i++) {
        ck_assert_int_eq(utf32_buf[i], (int32_t)(unsigned char)ascii_src[i]);
    }

    /* UTF32 -> ASCII roundtrip */
    wisp_utf32_to_ascii(utf32_buf, ascii_dst, len);
    ascii_dst[len] = '\0';
    ck_assert_str_eq(ascii_dst, ascii_src);

    /* Test lengths > 8 (SIMD loop boundary in sse2/neon) */
    const char *long_ascii = "12345678901234567890"; // 20 chars
    size_t long_len = strlen(long_ascii);
    int32_t long_utf32[32];
    char long_dst[32];

    wisp_ascii_to_utf32(long_ascii, long_utf32, long_len);
    wisp_utf32_to_ascii(long_utf32, long_dst, long_len);
    long_dst[long_len] = '\0';
    ck_assert_str_eq(long_dst, long_ascii);

    /* Length 0 */
    wisp_ascii_to_utf32("", utf32_buf, 0);
    wisp_utf32_to_ascii(utf32_buf, ascii_dst, 0);
}
END_TEST

/* 6. Whitespace Skipping Tests */
START_TEST(test_wisp_skip_whitespaces)
{
    /* Empty string */
    ck_assert_int_eq(wisp_skip_whitespaces((const uint8_t *)"", 0), 0);

    /* No whitespace */
    const uint8_t *no_ws = (const uint8_t *)"hello";
    ck_assert_int_eq(wisp_skip_whitespaces(no_ws, 5), 0);

    /* All whitespace (space, tab, newline, CR, FF) */
    const uint8_t *all_ws = (const uint8_t *)"  \t\n\r\f  ";
    size_t all_len = strlen((const char *)all_ws);
    ck_assert_int_eq(wisp_skip_whitespaces(all_ws, all_len), all_len);

    /* Mixed whitespace and non-whitespace */
    const uint8_t *mixed = (const uint8_t *)" \t\nABC \t";
    ck_assert_int_eq(wisp_skip_whitespaces(mixed, strlen((const char *)mixed)), 3);

    /* Test SIMD boundaries (> 16 bytes of whitespace) */
    const uint8_t *long_ws = (const uint8_t *)"                    hello"; // 20 spaces
    ck_assert_int_eq(wisp_skip_whitespaces(long_ws, strlen((const char *)long_ws)), 20);
}
END_TEST

/* 7. utf8proc Wrapper Function Tests */
START_TEST(test_wisp_utf8proc_decompose)
{
    /* ASCII fast-path */
    const char *ascii_str = "hello_world";
    int32_t buf[32];
    utf8proc_ssize_t res = wisp_utf8proc_decompose((const uint8_t *)ascii_str, -1, buf, 32, UTF8PROC_STABLE);
    ck_assert_int_eq(res, (utf8proc_ssize_t)strlen(ascii_str));
    for (size_t i = 0; i < strlen(ascii_str); i++) {
        ck_assert_int_eq(buf[i], (int32_t)ascii_str[i]);
    }

    /* Non-ASCII fallback */
    const char *non_ascii = "\xC3\xA9"; // é
    res = wisp_utf8proc_decompose((const uint8_t *)non_ascii, -1, buf, 32, UTF8PROC_STABLE);
    ck_assert_int_gt(res, 0);

    /* Modifying options on ASCII (should defer to utf8proc_decompose) */
    res = wisp_utf8proc_decompose((const uint8_t *)ascii_str, -1, buf, 32, UTF8PROC_CASEFOLD);
    ck_assert_int_eq(res, (utf8proc_ssize_t)strlen(ascii_str));
}
END_TEST

START_TEST(test_wisp_utf8proc_normalize_utf32)
{
    /* Negative length check */
    ck_assert_int_eq(wisp_utf8proc_normalize_utf32(NULL, -1, 0), UTF8PROC_ERROR_INVALIDOPTS);

    /* Length 0 check */
    int32_t buf[10] = {'a', 'b', 'c'};
    ck_assert_int_eq(wisp_utf8proc_normalize_utf32(buf, 0, 0), 0);

    /* NULL buffer check */
    ck_assert_int_eq(wisp_utf8proc_normalize_utf32(NULL, 5, 0), UTF8PROC_ERROR_INVALIDOPTS);

    /* ASCII UTF-32 fast path */
    int32_t ascii_utf32[5] = {'h', 'e', 'l', 'l', 'o'};
    utf8proc_ssize_t res = wisp_utf8proc_normalize_utf32(ascii_utf32, 5, 0);
    ck_assert_int_eq(res, 5);

    /* Non-ASCII UTF-32 fallback */
    int32_t non_ascii_utf32[2] = {0x00E9, 0x0061}; // é, a
    res = wisp_utf8proc_normalize_utf32(non_ascii_utf32, 2, UTF8PROC_STABLE);
    ck_assert_int_gt(res, 0);
}
END_TEST

START_TEST(test_wisp_utf8proc_reencode)
{
    /* Negative length check */
    ck_assert_int_eq(wisp_utf8proc_reencode(NULL, -1, 0), UTF8PROC_ERROR_INVALIDOPTS);

    /* Length 0 check */
    int32_t buf[10] = {'a', 'b', 'c'};
    ck_assert_int_eq(wisp_utf8proc_reencode(buf, 0, 0), 0);

    /* NULL buffer check */
    ck_assert_int_eq(wisp_utf8proc_reencode(NULL, 5, 0), UTF8PROC_ERROR_INVALIDOPTS);

    /* ASCII UTF-32 reencode fast path (in-place cast and reencode) */
    int32_t ascii_utf32[16] = {'h', 'e', 'l', 'l', 'o', '\0'};
    utf8proc_ssize_t res = wisp_utf8proc_reencode(ascii_utf32, 5, 0);
    ck_assert_int_eq(res, 5);
    ck_assert_str_eq((const char *)ascii_utf32, "hello");

    /* Non-ASCII fallback */
    int32_t non_ascii_utf32[16] = {0x00E9, '\0'}; // é
    res = wisp_utf8proc_reencode(non_ascii_utf32, 1, 0);
    ck_assert_int_gt(res, 0);
}
END_TEST

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

    tcase_add_test(tc, test_wisp_is_ascii);
    tcase_add_test(tc, test_wisp_validate_utf8);
    tcase_add_test(tc, test_wisp_simd_strcmp_and_streq);
    tcase_add_test(tc, test_wisp_ascii_case_mapping);
    tcase_add_test(tc, test_wisp_ascii_utf32_conversion);
    tcase_add_test(tc, test_wisp_skip_whitespaces);
    tcase_add_test(tc, test_wisp_utf8proc_decompose);
    tcase_add_test(tc, test_wisp_utf8proc_normalize_utf32);
    tcase_add_test(tc, test_wisp_utf8proc_reencode);
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
