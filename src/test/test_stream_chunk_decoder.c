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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "wisp/utils/stream_chunk_decoder.h"

START_TEST(test_simd_crlf_scanner)
{
    const uint8_t data1[] = "Hello World\r\nHeader: Value\r\n";
    ck_assert_int_eq(wisp_scan_crlf_simd(data1, sizeof(data1) - 1), 11);
    ck_assert_int_eq(wisp_scan_crlf_scalar(data1, sizeof(data1) - 1), 11);
    ck_assert_int_eq(wisp_scan_crlf_sse2(data1, sizeof(data1) - 1), 11);
    ck_assert_int_eq(wisp_scan_crlf_neon(data1, sizeof(data1) - 1), 11);
    ck_assert_int_eq(wisp_scan_crlf_rvv(data1, sizeof(data1) - 1), 11);

    const uint8_t data2[] = "12345678901234567890\r\n";
    ck_assert_int_eq(wisp_scan_crlf_simd(data2, sizeof(data2) - 1), 20);

    const uint8_t data_no_crlf[] = "12345678901234567890\rWithout newline";
    ck_assert_int_eq(wisp_scan_crlf_simd(data_no_crlf, sizeof(data_no_crlf) - 1), (size_t)-1);
}
END_TEST

START_TEST(test_standard_chunk_decoding)
{
    wisp_stream_chunk_decoder_t decoder;
    wisp_stream_chunk_decoder_init(&decoder);

    const uint8_t input[] = "5\r\nHello\r\n7\r\n World!\r\n0\r\n\r\n";
    uint8_t out[100];
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    int status = wisp_stream_chunk_decoder_decode(&decoder, input, sizeof(input) - 1, &bytes_read, out, sizeof(out), &bytes_written);
    ck_assert_int_eq(status, 1);
    ck_assert_int_eq(bytes_read, sizeof(input) - 1);
    ck_assert_int_eq(bytes_written, 12);
    out[bytes_written] = '\0';
    ck_assert_str_eq((char *)out, "Hello World!");
}
END_TEST

START_TEST(test_incremental_chunk_decoding)
{
    wisp_stream_chunk_decoder_t decoder;
    wisp_stream_chunk_decoder_init(&decoder);

    const uint8_t chunk1[] = "4\r\nWiki\r\n5\r\npedia\r\n";
    const uint8_t chunk2[] = "F\r\n in \r\n\r\nchunks.\r\n0\r\n\r\n";

    uint8_t out[100];
    size_t total_written = 0;
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    int res1 = wisp_stream_chunk_decoder_decode(&decoder, chunk1, sizeof(chunk1) - 1, &bytes_read, out + total_written, sizeof(out) - total_written, &bytes_written);
    ck_assert_int_eq(res1, 0);
    total_written += bytes_written;

    int res2 = wisp_stream_chunk_decoder_decode(&decoder, chunk2, sizeof(chunk2) - 1, &bytes_read, out + total_written, sizeof(out) - total_written, &bytes_written);
    ck_assert_int_eq(res2, 1);
    total_written += bytes_written;

    out[total_written] = '\0';
    ck_assert_str_eq((char *)out, "Wikipedia in \r\n\r\nchunks.");
}
END_TEST

START_TEST(test_chunk_extensions_and_trailers)
{
    wisp_stream_chunk_decoder_t decoder;
    wisp_stream_chunk_decoder_init(&decoder);

    const uint8_t input[] = "A;foo=bar;baz\r\n0123456789\r\n0;final=true\r\nX-Trailer: test\r\n\r\n";
    uint8_t out[100];
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    int status = wisp_stream_chunk_decoder_decode(&decoder, input, sizeof(input) - 1, &bytes_read, out, sizeof(out), &bytes_written);
    ck_assert_int_eq(status, 1);
    ck_assert_int_eq(bytes_written, 10);
    out[bytes_written] = '\0';
    ck_assert_str_eq((char *)out, "0123456789");
}
END_TEST

START_TEST(test_invalid_hex_chunk_header)
{
    wisp_stream_chunk_decoder_t decoder;
    wisp_stream_chunk_decoder_init(&decoder);

    const uint8_t input[] = "G\r\nInvalid\r\n";
    uint8_t out[100];
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    int status = wisp_stream_chunk_decoder_decode(&decoder, input, sizeof(input) - 1, &bytes_read, out, sizeof(out), &bytes_written);
    ck_assert_int_eq(status, -1);
}
END_TEST

static Suite *stream_chunk_decoder_suite(void)
{
    Suite *s = suite_create("stream_chunk_decoder");
    TCase *tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_simd_crlf_scanner);
    tcase_add_test(tc_core, test_standard_chunk_decoding);
    tcase_add_test(tc_core, test_incremental_chunk_decoding);
    tcase_add_test(tc_core, test_chunk_extensions_and_trailers);
    tcase_add_test(tc_core, test_invalid_hex_chunk_header);

    suite_add_tcase(s, tc_core);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s = stream_chunk_decoder_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
