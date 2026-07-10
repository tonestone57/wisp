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
#include <utf8proc.h>
#include "wisp/utils/utf8proc_wrapper.h"
#include "wisp/utils/websocket_mask.h"

#define NELEMS(x) (sizeof(x) / sizeof((x)[0]))

/* Test cases for ASCII check */
START_TEST(test_ascii_detection)
{
    /* Pure ASCII strings of different lengths */
    const char *ascii_short = "Hello";
    const char *ascii_32 = "12345678901234567890123456789012"; /* 32 bytes */
    const char *ascii_long = "This is a longer ASCII string that exceeds 32 characters in length to test AVX2 chunking.";

    ck_assert(wisp_is_ascii(ascii_short, strlen(ascii_short)));
    ck_assert(wisp_is_ascii(ascii_32, strlen(ascii_32)));
    ck_assert(wisp_is_ascii(ascii_long, strlen(ascii_long)));

    /* Non-ASCII strings */
    const char *non_ascii_start = "\x80Hello";
    const char *non_ascii_mid = "Hello\xffWorld";
    const char *non_ascii_end = "HelloWorld\xce";
    const char *non_ascii_at_31 = "1234567890123456789012345678901\xce"; /* non-ASCII at index 31 */
    const char *non_ascii_at_32 = "12345678901234567890123456789012\xce"; /* non-ASCII at index 32 */

    ck_assert(!wisp_is_ascii(non_ascii_start, strlen(non_ascii_start)));
    ck_assert(!wisp_is_ascii(non_ascii_mid, strlen(non_ascii_mid)));
    ck_assert(!wisp_is_ascii(non_ascii_end, strlen(non_ascii_end)));
    ck_assert(!wisp_is_ascii(non_ascii_at_31, strlen(non_ascii_at_31)));
    ck_assert(!wisp_is_ascii(non_ascii_at_32, strlen(non_ascii_at_32)));
}
END_TEST

/* Test cases for UTF-8 Validation */
START_TEST(test_utf8_validation)
{
    const char *valid_ascii = "Standard ASCII is valid UTF-8.";
    const char *valid_greek = "Ελληνικά (Greek)";
    const char *valid_emoji = "Emoji \xf0\x9f\x98\x80 is valid UTF-8.";
    const char *invalid_overlong = "\xc0\xaf"; /* Overlong ASCII '/' representation */
    const char *invalid_surrogate = "\xed\xa0\x80"; /* High surrogate U+D800 */
    const char *invalid_too_large = "\xf5\x80\x80\x80"; /* Code point above U+10FFFF */

    ck_assert(wisp_validate_utf8(valid_ascii, strlen(valid_ascii)));
    ck_assert(wisp_validate_utf8(valid_greek, strlen(valid_greek)));
    ck_assert(wisp_validate_utf8(valid_emoji, strlen(valid_emoji)));

    ck_assert(!wisp_validate_utf8(invalid_overlong, strlen(invalid_overlong)));
    ck_assert(!wisp_validate_utf8(invalid_surrogate, strlen(invalid_surrogate)));
    ck_assert(!wisp_validate_utf8(invalid_too_large, strlen(invalid_too_large)));
}
END_TEST

/* Test ASCII case mappings */
START_TEST(test_ascii_case_mapping)
{
    const char *src = "AbCdEfGhIjKlMnOpQrStUvWxYz 1234!@#$";
    char lower[100];
    char upper[100];

    wisp_ascii_tolower(src, lower, strlen(src));
    lower[strlen(src)] = '\0';
    ck_assert_str_eq(lower, "abcdefghijklmnopqrstuvwxyz 1234!@#$");

    wisp_ascii_toupper(src, upper, strlen(src));
    upper[strlen(src)] = '\0';
    ck_assert_str_eq(upper, "ABCDEFGHIJKLMNOPQRSTUVWXYZ 1234!@#$");

    /* Test 32-byte exact boundary */
    const char *src_32 = "ABCDEFGHabcdefgh12345678!@#$%^&*";
    char lower_32[33];
    wisp_ascii_tolower(src_32, lower_32, 32);
    lower_32[32] = '\0';
    ck_assert_str_eq(lower_32, "abcdefghabcdefgh12345678!@#$%^&*");
}
END_TEST

/* Test ASCII <-> UTF-32 encoding conversions */
START_TEST(test_encoding_conversions)
{
    const char *src = "ASCII_1234";
    size_t len = strlen(src);
    int32_t ucs4[20];
    char dest[20];

    wisp_ascii_to_utf32(src, ucs4, len);
    for (size_t i = 0; i < len; i++) {
        ck_assert_int_eq(ucs4[i], (int32_t)src[i]);
    }

    wisp_utf32_to_ascii(ucs4, dest, len);
    dest[len] = '\0';
    ck_assert_str_eq(dest, src);
}
END_TEST

/* Test WebSocket Payload Masking SIMD Acceleration */
START_TEST(test_websocket_masking)
{
    const uint8_t mask_key[4] = {0xDE, 0xAD, 0xBE, 0xEF};

    /* Test Case 1: Short payload (less than SIMD alignment) */
    uint8_t short_data[7] = {1, 2, 3, 4, 5, 6, 7};
    uint8_t short_data_expected[7] = {1, 2, 3, 4, 5, 6, 7};
    for (size_t i = 0; i < 7; i++) {
        short_data_expected[i] ^= mask_key[i % 4];
    }
    wisp_websocket_mask(short_data, 7, mask_key, 0);
    ck_assert_mem_eq(short_data, short_data_expected, 7);

    /* Test Case 2: Exact 32 bytes payload */
    uint8_t data_32[32];
    uint8_t data_32_expected[32];
    for (size_t i = 0; i < 32; i++) {
        data_32[i] = (uint8_t)i;
        data_32_expected[i] = (uint8_t)i ^ mask_key[i % 4];
    }
    wisp_websocket_mask(data_32, 32, mask_key, 0);
    ck_assert_mem_eq(data_32, data_32_expected, 32);

    /* Test Case 3: 100 bytes payload with various key offsets */
    for (size_t offset = 0; offset < 4; offset++) {
        uint8_t data_100[100];
        uint8_t data_100_expected[100];
        for (size_t i = 0; i < 100; i++) {
            data_100[i] = (uint8_t)(i * 3);
            data_100_expected[i] = (uint8_t)(i * 3) ^ mask_key[(i + offset) % 4];
        }
        wisp_websocket_mask(data_100, 100, mask_key, offset);
        ck_assert_mem_eq(data_100, data_100_expected, 100);
    }

    /* Test Case 4: Reversibility (masking twice yields original data) */
    uint8_t raw_data[123];
    uint8_t copy_data[123];
    for (size_t i = 0; i < 123; i++) {
        raw_data[i] = (uint8_t)(i ^ 0x55);
        copy_data[i] = raw_data[i];
    }
    wisp_websocket_mask(copy_data, 123, mask_key, 1);
    wisp_websocket_mask(copy_data, 123, mask_key, 1);
    ck_assert_mem_eq(copy_data, raw_data, 123);
}
END_TEST

/* Test wrapper compatibility with original libutf8proc */
START_TEST(test_utf8proc_wrapper_compatibility)
{
    /* ASCII text */
    const char *ascii_str = "hello_world_123";
    int32_t buf_wisp[64];
    int32_t buf_orig[64];
    ssize_t size_wisp, size_orig;

    size_wisp = wisp_utf8proc_decompose((const uint8_t *)ascii_str, strlen(ascii_str), buf_wisp, 64, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
    size_orig = utf8proc_decompose((const uint8_t *)ascii_str, strlen(ascii_str), buf_orig, 64, UTF8PROC_STABLE | UTF8PROC_COMPOSE);

    ck_assert_int_eq(size_wisp, size_orig);
    for (ssize_t i = 0; i < size_orig; i++) {
        ck_assert_int_eq(buf_wisp[i], buf_orig[i]);
    }

    /* Normalization shortcut check */
    uint8_t *norm_wisp = wisp_utf8proc_NFC((const uint8_t *)ascii_str);
    uint8_t *norm_orig = utf8proc_NFC((const uint8_t *)ascii_str);
    ck_assert_str_eq((const char *)norm_wisp, (const char *)norm_orig);
    free(norm_wisp);
    free(norm_orig);

    /* Non-ASCII (Greek) to ensure wrapper falls back to standard correctly and produces correct output */
    const char *greek_str = "Ελληνικά";
    int32_t greek_wisp[64];
    int32_t greek_orig[64];

    ssize_t g_wisp = wisp_utf8proc_decompose((const uint8_t *)greek_str, strlen(greek_str), greek_wisp, 64, UTF8PROC_STABLE | UTF8PROC_COMPOSE);
    ssize_t g_orig = utf8proc_decompose((const uint8_t *)greek_str, strlen(greek_str), greek_orig, 64, UTF8PROC_STABLE | UTF8PROC_COMPOSE);

    ck_assert_int_eq(g_wisp, g_orig);
    for (ssize_t i = 0; i < g_orig; i++) {
        ck_assert_int_eq(greek_wisp[i], greek_orig[i]);
    }
}
END_TEST

static Suite *utf8proc_simd_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("utf8proc_simd");
    tc = tcase_create("core");

    tcase_add_test(tc, test_ascii_detection);
    tcase_add_test(tc, test_utf8_validation);
    tcase_add_test(tc, test_ascii_case_mapping);
    tcase_add_test(tc, test_encoding_conversions);
    tcase_add_test(tc, test_websocket_masking);
    tcase_add_test(tc, test_utf8proc_wrapper_compatibility);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = utf8proc_simd_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
