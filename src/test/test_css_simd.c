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

#include <wisp/utils/css_delimiters.h>

START_TEST(test_css_delimiters_basic)
{
    const char *css = "body { margin: 0; padding: 0; }";
    size_t len = strlen(css);

    /* "body" is 4 characters before ' ' */
    ck_assert_int_eq(wisp_scan_css_delimiters((const uint8_t *)css, len), 4);
    ck_assert_int_eq(wisp_scan_css_delimiters_scalar((const uint8_t *)css, len), 4);
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    ck_assert_int_eq(wisp_scan_css_delimiters_sse2((const uint8_t *)css, len), 4);
#endif
#if defined(__arm__) || defined(__aarch64__)
    if (wisp_delim_has_neon()) {
        ck_assert_int_eq(wisp_scan_css_delimiters_neon((const uint8_t *)css, len), 4);
    }
#endif
#if defined(__riscv) && defined(__riscv_vector)
    if (wisp_delim_has_rvv()) {
        ck_assert_int_eq(wisp_scan_css_delimiters_rvv((const uint8_t *)css, len), 4);
    }
#endif
}
END_TEST

START_TEST(test_css_delimiters_long_ident)
{
    /* 32-byte identifier without delimiters */
    const char *css = "abcdefghijklmnopqrstuvwxyz123456;padding: 10px;";
    size_t len = strlen(css);

    /* Should stop at ';' at index 32 */
    ck_assert_int_eq(wisp_scan_css_delimiters((const uint8_t *)css, len), 32);
    ck_assert_int_eq(wisp_scan_css_delimiters_scalar((const uint8_t *)css, len), 32);
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    ck_assert_int_eq(wisp_scan_css_delimiters_sse2((const uint8_t *)css, len), 32);
#endif
#if defined(__arm__) || defined(__aarch64__)
    if (wisp_delim_has_neon()) {
        ck_assert_int_eq(wisp_scan_css_delimiters_neon((const uint8_t *)css, len), 32);
    }
#endif
#if defined(__riscv) && defined(__riscv_vector)
    if (wisp_delim_has_rvv()) {
        ck_assert_int_eq(wisp_scan_css_delimiters_rvv((const uint8_t *)css, len), 32);
    }
#endif
}
END_TEST

START_TEST(test_css_delimiters_all_delimiters)
{
    const char *delims = ";{}:(),/[]=*#\"'!%+>~ \t\n\f\r";
    size_t len = strlen(delims);

    for (size_t i = 0; i < len; i++) {
        size_t res_simd = wisp_scan_css_delimiters((const uint8_t *)(delims + i), len - i);
        size_t res_scalar = wisp_scan_css_delimiters_scalar((const uint8_t *)(delims + i), len - i);
        ck_assert_int_eq(res_simd, 0);
        ck_assert_int_eq(res_scalar, 0);
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        ck_assert_int_eq(wisp_scan_css_delimiters_sse2((const uint8_t *)(delims + i), len - i), 0);
#endif
    }
}
END_TEST

START_TEST(test_css_delimiters_utf8_nonascii)
{
    /* Non-ASCII UTF-8 characters e.g. "café { color: red; }" */
    const char *utf8_css = "caf\xc3\xa9 { color: red; }";
    size_t len = strlen(utf8_css);

    /* "café" is 5 bytes in UTF-8 before ' ' */
    ck_assert_int_eq(wisp_scan_css_delimiters((const uint8_t *)utf8_css, len), 5);
    ck_assert_int_eq(wisp_scan_css_delimiters_scalar((const uint8_t *)utf8_css, len), 5);
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    ck_assert_int_eq(wisp_scan_css_delimiters_sse2((const uint8_t *)utf8_css, len), 5);
#endif

    /* Longer UTF-8 string (>16 bytes) e.g. "header_caf\xc3\xa9_title_block { padding: 0; }" */
    const char *utf8_long = "header_caf\xc3\xa9_title_block { padding: 0; }";
    size_t long_len = strlen(utf8_long);

    /* 24 bytes before ' ' */
    ck_assert_int_eq(wisp_scan_css_delimiters((const uint8_t *)utf8_long, long_len), 24);
    ck_assert_int_eq(wisp_scan_css_delimiters_scalar((const uint8_t *)utf8_long, long_len), 24);
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    ck_assert_int_eq(wisp_scan_css_delimiters_sse2((const uint8_t *)utf8_long, long_len), 24);
#endif
}
END_TEST

START_TEST(test_css_delimiters_parity_long)
{
    /* Test input longer than 16 bytes containing various delimiters */
    const char *test_str = "abcdefghijklmno#0123456789abcdef";
    size_t len = strlen(test_str);

    size_t res_simd = wisp_scan_css_delimiters((const uint8_t *)test_str, len);
    size_t res_scalar = wisp_scan_css_delimiters_scalar((const uint8_t *)test_str, len);

    ck_assert_int_eq(res_simd, 15);
    ck_assert_int_eq(res_scalar, 15);
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    ck_assert_int_eq(wisp_scan_css_delimiters_sse2((const uint8_t *)test_str, len), 15);
#endif
}
END_TEST

START_TEST(test_css_delimiters_no_delimiters)
{
    const char *no_delim = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
    size_t len = strlen(no_delim);

    ck_assert_int_eq(wisp_scan_css_delimiters((const uint8_t *)no_delim, len), len);
    ck_assert_int_eq(wisp_scan_css_delimiters_scalar((const uint8_t *)no_delim, len), len);
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    ck_assert_int_eq(wisp_scan_css_delimiters_sse2((const uint8_t *)no_delim, len), len);
#endif
}
END_TEST

static Suite *css_simd_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("css_simd");
    tc = tcase_create("delimiters");

    tcase_add_test(tc, test_css_delimiters_basic);
    tcase_add_test(tc, test_css_delimiters_long_ident);
    tcase_add_test(tc, test_css_delimiters_all_delimiters);
    tcase_add_test(tc, test_css_delimiters_utf8_nonascii);
    tcase_add_test(tc, test_css_delimiters_parity_long);
    tcase_add_test(tc, test_css_delimiters_no_delimiters);

    suite_add_tcase(s, tc);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = css_simd_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
