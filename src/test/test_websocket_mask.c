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
#include "wisp/utils/websocket_mask.h"

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


START_TEST(test_websocket_masking_zero_length)
{
    const uint8_t mask_key[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t data[1] = {0x42};
    wisp_websocket_mask(data, 0, mask_key, 0);
    ck_assert_int_eq(data[0], 0x42); // Should not modify
}
END_TEST

START_TEST(test_websocket_masking_boundaries)
{
    const uint8_t mask_key[4] = {0x12, 0x34, 0x56, 0x78};

    // Boundary of 15
    uint8_t data_15[15];
    uint8_t expected_15[15];
    for (size_t i = 0; i < 15; i++) {
        data_15[i] = (uint8_t)i;
        expected_15[i] = (uint8_t)i ^ mask_key[i % 4];
    }
    wisp_websocket_mask(data_15, 15, mask_key, 0);
    ck_assert_mem_eq(data_15, expected_15, 15);

    // Boundary of 16 (often SSE2 block size)
    uint8_t data_16[16];
    uint8_t expected_16[16];
    for (size_t i = 0; i < 16; i++) {
        data_16[i] = (uint8_t)i;
        expected_16[i] = (uint8_t)i ^ mask_key[i % 4];
    }
    wisp_websocket_mask(data_16, 16, mask_key, 0);
    ck_assert_mem_eq(data_16, expected_16, 16);

    // Boundary of 17
    uint8_t data_17[17];
    uint8_t expected_17[17];
    for (size_t i = 0; i < 17; i++) {
        data_17[i] = (uint8_t)i;
        expected_17[i] = (uint8_t)i ^ mask_key[i % 4];
    }
    wisp_websocket_mask(data_17, 17, mask_key, 0);
    ck_assert_mem_eq(data_17, expected_17, 17);

    // Boundary of 31
    uint8_t data_31[31];
    uint8_t expected_31[31];
    for (size_t i = 0; i < 31; i++) {
        data_31[i] = (uint8_t)i;
        expected_31[i] = (uint8_t)i ^ mask_key[i % 4];
    }
    wisp_websocket_mask(data_31, 31, mask_key, 0);
    ck_assert_mem_eq(data_31, expected_31, 31);
}
END_TEST

START_TEST(test_websocket_masking_large)
{
    const uint8_t mask_key[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    size_t len = 10000;
    uint8_t *data = malloc(len);
    uint8_t *expected = malloc(len);

    ck_assert_ptr_nonnull(data);
    ck_assert_ptr_nonnull(expected);

    for (size_t i = 0; i < len; i++) {
        data[i] = (uint8_t)(i % 256);
        expected[i] = (uint8_t)(i % 256) ^ mask_key[i % 4];
    }

    wisp_websocket_mask(data, len, mask_key, 0);
    ck_assert_mem_eq(data, expected, len);

    free(data);
    free(expected);
}
END_TEST

START_TEST(test_websocket_masking_unaligned)
{
    const uint8_t mask_key[4] = {0x99, 0x88, 0x77, 0x66};

    // Allocate enough and start with an offset to force unaligned access
    size_t len = 200;
    uint8_t *base_ptr = malloc(len + 16);
    ck_assert_ptr_nonnull(base_ptr);

    for (size_t align_offset = 1; align_offset < 16; align_offset++) {
        uint8_t *data = base_ptr + align_offset;
        uint8_t *expected = malloc(len);
        ck_assert_ptr_nonnull(expected);

        for (size_t i = 0; i < len; i++) {
            data[i] = (uint8_t)(i % 256);
            expected[i] = (uint8_t)(i % 256) ^ mask_key[i % 4];
        }

        wisp_websocket_mask(data, len, mask_key, 0);
        ck_assert_mem_eq(data, expected, len);

        free(expected);
    }

    free(base_ptr);
}
END_TEST
static Suite *websocket_mask_suite(void)
{
    Suite *s;
    TCase *tc;

    s = suite_create("websocket_mask");
    tc = tcase_create("core");

    tcase_add_test(tc, test_websocket_masking);

    tcase_add_test(tc, test_websocket_masking_zero_length);
    tcase_add_test(tc, test_websocket_masking_boundaries);
    tcase_add_test(tc, test_websocket_masking_large);
    tcase_add_test(tc, test_websocket_masking_unaligned);
    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = websocket_mask_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_ENV);

    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
