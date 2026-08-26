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
#include "wisp/utils/stream_simd.h"

START_TEST(test_simd_find_crlf)
{
    const char *str1 = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    size_t off1 = wisp_simd_find_crlf((const uint8_t *)str1, strlen(str1));
    ck_assert_int_eq(off1, 15);

    const char *str_no_crlf = "No CRLF sequence here at all!";
    size_t off2 = wisp_simd_find_crlf((const uint8_t *)str_no_crlf, strlen(str_no_crlf));
    ck_assert_int_eq(off2, (size_t)-1);

    /* Small buffer under vector width (16 bytes) bounds safety test */
    const char *short_crlf = "abc\r\ndef";
    size_t off_short = wisp_simd_find_crlf((const uint8_t *)short_crlf, strlen(short_crlf));
    ck_assert_int_eq(off_short, 3);

    /* Alignment & 32-byte boundary check */
    char long_str[128];
    memset(long_str, 'A', sizeof(long_str));
    long_str[45] = '\r';
    long_str[46] = '\n';
    size_t off3 = wisp_simd_find_crlf((const uint8_t *)long_str, sizeof(long_str));
    ck_assert_int_eq(off3, 45);
}
END_TEST

START_TEST(test_simd_parse_chunk_header)
{
    size_t csize = 0, hlen = 0;

    const char *chunk1 = "1A\r\nHello World!";
    bool ok1 = wisp_simd_parse_chunk_header((const uint8_t *)chunk1, strlen(chunk1), &csize, &hlen);
    ck_assert(ok1);
    ck_assert_int_eq(csize, 26);
    ck_assert_int_eq(hlen, 4);

    const char *chunk_ext = "f;foo=bar\r\n";
    bool ok2 = wisp_simd_parse_chunk_header((const uint8_t *)chunk_ext, strlen(chunk_ext), &csize, &hlen);
    ck_assert(ok2);
    ck_assert_int_eq(csize, 15);
    ck_assert_int_eq(hlen, 11);

    const char *invalid_hex = "1G\r\n";
    bool ok3 = wisp_simd_parse_chunk_header((const uint8_t *)invalid_hex, strlen(invalid_hex), &csize, &hlen);
    ck_assert(!ok3);

    const char *incomplete = "1A";
    bool ok4 = wisp_simd_parse_chunk_header((const uint8_t *)incomplete, strlen(incomplete), &csize, &hlen);
    ck_assert(!ok4);
}
END_TEST

START_TEST(test_simd_decode_chunked_stream)
{
    const char *stream = "5\r\nHello\r\n7\r\n World!\r\n0\r\n\r\n";
    uint8_t out[128];
    memset(out, 0, sizeof(out));

    wisp_chunk_decode_result res = wisp_simd_decode_chunked_stream((const uint8_t *)stream, strlen(stream), out, sizeof(out));

    ck_assert(!res.is_invalid);
    ck_assert(!res.is_incomplete);
    ck_assert(res.is_final_chunk);
    ck_assert_int_eq(res.decoded_bytes, 12);
    out[res.decoded_bytes] = '\0';
    ck_assert_str_eq((const char *)out, "Hello World!");

    /* Progressive stream fragmentation test */
    const char *inc_stream = "5\r\nHell";
    wisp_chunk_decode_result res_inc = wisp_simd_decode_chunked_stream((const uint8_t *)inc_stream, strlen(inc_stream), out, sizeof(out));
    ck_assert(res_inc.is_incomplete);
    ck_assert(!res_inc.is_invalid);
}
END_TEST

START_TEST(test_simd_validate_http_header_rfc7230)
{
    /* Valid headers & long status lines (>= 16 bytes) */
    ck_assert(wisp_simd_validate_http_header("Content-Type: text/html\r\n", 25));
    ck_assert(wisp_simd_validate_http_header("HTTP/1.1 200 OK\r\n", 17));
    ck_assert(wisp_simd_validate_http_header("HTTP/1.1 200 OK - Request Succeeded\r\n", 37));
    ck_assert(wisp_simd_validate_http_header("HTTP/1.1 404 Not Found\r\n", 24));
    ck_assert(wisp_simd_validate_http_header("Server: Wisp/1.0\r\n", 18));

    /* Small buffer safety check (< 16 bytes) */
    ck_assert(wisp_simd_validate_http_header("A: B\r\n", 6));

    /* Rejection of space before colon in header field name */
    ck_assert(!wisp_simd_validate_http_header("Content-Type : text/html\r\n", 26));
    ck_assert(!wisp_simd_validate_http_header("Header-Name\t: val\r\n", 19));

    /* Rejection of obsolete line folding (obs-fold: header starting with space or tab) */
    ck_assert(!wisp_simd_validate_http_header(" continuation line\r\n", 21));
    ck_assert(!wisp_simd_validate_http_header("\tfolded line\r\n", 15));

    /* Rejection of bare CR or bare LF at interior or end of header */
    ck_assert(!wisp_simd_validate_http_header("Header: val\rsub\r\n", 18));
    ck_assert(!wisp_simd_validate_http_header("Header: val\nsub\r\n", 18));
    ck_assert(!wisp_simd_validate_http_header("Header: val\r", 12));
    ck_assert(!wisp_simd_validate_http_header("Header: val\n", 12));

    /* UTF-8 / non-ASCII character in header value (0x80..0xFF) parity check */
    const char *utf8_hdr = "Location: https://example.com/path/\xC3\xA9\r\n";
    ck_assert(wisp_simd_validate_http_header(utf8_hdr, strlen(utf8_hdr)));

    /* Control character in header value */
    char bad_hdr[32] = "X-Header: bad\x01value\r\n";
    ck_assert(!wisp_simd_validate_http_header(bad_hdr, strlen(bad_hdr)));

    /* Missing colon in standard header */
    ck_assert(!wisp_simd_validate_http_header("InvalidHeaderLine\r\n", 19));

    /* Header block validation & body termination check (\r\n\r\n) */
    const char *block = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 100\r\n\r\n<html><body>Body</body></html>";
    ck_assert(wisp_simd_validate_http_header_block(block, strlen(block)));
}
END_TEST

static Suite *stream_simd_suite(void)
{
    Suite *s = suite_create("stream_simd");
    TCase *tc = tcase_create("core");

    tcase_add_test(tc, test_simd_find_crlf);
    tcase_add_test(tc, test_simd_parse_chunk_header);
    tcase_add_test(tc, test_simd_decode_chunked_stream);
    tcase_add_test(tc, test_simd_validate_http_header_rfc7230);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = stream_simd_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
