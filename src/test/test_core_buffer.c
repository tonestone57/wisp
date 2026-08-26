/*
 * Copyright 2026 NeoSurf Project
 *
 * This file is part of NetSurf / Wisp.
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file
 * Tests for core_buffer operations.
 */

#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/core_buffer.h"

/* --- Core Buffer API & Parameter Validation Tests --- */

START_TEST(core_buffer_null_parameter_test)
{
    core_buffer buf;
    const uint8_t sample_data[] = "hello";

    /* Test null buffer parameter for all functions */
    ck_assert_int_eq(core_buffer_init(NULL), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_append(NULL, sample_data, 5), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_reserve(NULL, 100), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_shrink(NULL), NSERROR_BAD_PARAMETER);
    ck_assert_int_eq(core_buffer_wrap_external(NULL, (uint8_t *)sample_data, 5), NSERROR_BAD_PARAMETER);
    ck_assert(core_buffer_data(NULL) == NULL);
    ck_assert_int_eq(core_buffer_length(NULL), 0);

    /* core_buffer_destroy and core_buffer_clear should handle NULL gracefully without crashing */
    core_buffer_destroy(NULL);
    core_buffer_clear(NULL);

    /* Test append with NULL data pointer when length > 0 */
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 10), NSERROR_BAD_PARAMETER);

    /* Append NULL data with 0 length should succeed */
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_append_zero_length_test)
{
    core_buffer buf;
    const uint8_t data[] = "initial";

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* Append 0 length with non-NULL pointer to empty buffer */
    ck_assert_int_eq(core_buffer_append(&buf, data, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert(buf.data == NULL);

    /* Append 0 length with NULL pointer to empty buffer */
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert(buf.data == NULL);

    /* Populate buffer */
    ck_assert_int_eq(core_buffer_append(&buf, data, 7), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 7);

    /* Append 0 length to pre-populated buffer */
    ck_assert_int_eq(core_buffer_append(&buf, data, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, NULL, 0), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 7);
    ck_assert_mem_eq(core_buffer_data(&buf), "initial", 7);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_append_binary_data_test)
{
    core_buffer buf;
    const uint8_t bin1[] = { 0x00, 0xFF, 0xFE, 0x00, 0x42 };
    const uint8_t bin2[] = { 0x12, 0x34, 0x00, 0x56, 0x78, 0x90 };

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    ck_assert_int_eq(core_buffer_append(&buf, bin1, sizeof(bin1)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), sizeof(bin1));
    ck_assert_mem_eq(core_buffer_data(&buf), bin1, sizeof(bin1));

    ck_assert_int_eq(core_buffer_append(&buf, bin2, sizeof(bin2)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), sizeof(bin1) + sizeof(bin2));

    uint8_t expected[sizeof(bin1) + sizeof(bin2)];
    memcpy(expected, bin1, sizeof(bin1));
    memcpy(expected + sizeof(bin1), bin2, sizeof(bin2));

    ck_assert_mem_eq(core_buffer_data(&buf), expected, sizeof(expected));

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_append_wrapped_external_test)
{
    core_buffer buf;
    uint8_t ext_data[] = "external_data_prefix";
    size_t ext_len = strlen((char *)ext_data);
    const uint8_t suffix[] = "_appended_suffix";
    size_t suf_len = strlen((char *)suffix);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, ext_data, ext_len), NSERROR_OK);

    /* Verify wrapped state */
    ck_assert(buf.data == ext_data);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_int_eq(buf.length, ext_len);

    /* Appending to wrapped external buffer should transition it to heap allocated memory */
    ck_assert_int_eq(core_buffer_append(&buf, suffix, suf_len), NSERROR_OK);
    ck_assert(buf.data != ext_data);
    ck_assert(buf.data != NULL);
    ck_assert(buf.allocated >= ext_len + suf_len);
    ck_assert_int_eq(core_buffer_length(&buf), ext_len + suf_len);

    char expected[64];
    snprintf(expected, sizeof(expected), "%s%s", (char *)ext_data, (char *)suffix);
    ck_assert_mem_eq(core_buffer_data(&buf), expected, ext_len + suf_len);

    /* Original external data buffer must remain untouched */
    ck_assert_str_eq((char *)ext_data, "external_data_prefix");

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_append_multiple_reallocations_test)
{
    core_buffer buf;
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    uint8_t pattern[128];
    for (size_t i = 0; i < sizeof(pattern); i++) {
        pattern[i] = (uint8_t)(i & 0xFF);
    }

    /* Perform repeated appends to force multiple buffer reallocations */
    size_t total_expected_len = 0;
    for (int iter = 0; iter < 50; iter++) {
        ck_assert_int_eq(core_buffer_append(&buf, pattern, sizeof(pattern)), NSERROR_OK);
        total_expected_len += sizeof(pattern);
        ck_assert_int_eq(core_buffer_length(&buf), total_expected_len);
        ck_assert(buf.allocated >= total_expected_len);
    }

    /* Verify contents across all appended blocks */
    const uint8_t *data = core_buffer_data(&buf);
    for (int iter = 0; iter < 50; iter++) {
        ck_assert_mem_eq(data + (iter * sizeof(pattern)), pattern, sizeof(pattern));
    }

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_init_destroy_test)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert(buf.data == NULL);
    ck_assert_int_eq(buf.length, 0);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert(core_buffer_data(&buf) == NULL);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    core_buffer_destroy(&buf);
    ck_assert(buf.data == NULL);
    ck_assert_int_eq(buf.length, 0);
    ck_assert_int_eq(buf.allocated, 0);
}
END_TEST

START_TEST(core_buffer_length_test)
{
    core_buffer buf;
    const uint8_t chunk1[] = "Hello";
    const uint8_t chunk2[] = ", World!";
    uint8_t ext_data[] = "external_buffer_content";
    size_t ext_len = strlen((char *)ext_data);

    /* 1. NULL buffer pointer */
    ck_assert_int_eq(core_buffer_length(NULL), 0);

    /* 2. Initialized empty buffer */
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    /* 3. Buffer after reserve (capacity allocated, length remains 0) */
    ck_assert_int_eq(core_buffer_reserve(&buf, 128), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    /* 4. Sequential appends */
    ck_assert_int_eq(core_buffer_append(&buf, chunk1, strlen((char *)chunk1)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 5);

    ck_assert_int_eq(core_buffer_append(&buf, chunk2, strlen((char *)chunk2)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 13);

    /* 5. Clear buffer resets length to 0 */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    /* Append again after clear */
    ck_assert_int_eq(core_buffer_append(&buf, chunk1, strlen((char *)chunk1)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 5);

    /* 6. Shrink non-empty buffer preserves length */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 5);

    /* Clear and shrink keeps length at 0 */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    core_buffer_destroy(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    /* 7. Wrapped external buffer length */
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, ext_data, ext_len), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), ext_len);

    core_buffer_destroy(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
}
END_TEST

/* --- core_buffer_reserve Allocation Logic Tests --- */

START_TEST(core_buffer_reserve_initial_allocation_test)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* Reserving length <= 64 when allocated == 0 should allocate 64 bytes (default starting size) */
    ck_assert_int_eq(core_buffer_reserve(&buf, 10), NSERROR_OK);
    ck_assert(buf.data != NULL);
    ck_assert_int_eq(buf.allocated, 64);
    ck_assert_int_eq(buf.length, 0);

    /* Reserving length equal to or smaller than current allocated capacity should be a no-op */
    ck_assert_int_eq(core_buffer_reserve(&buf, 64), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 64);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_reserve_power_of_two_scaling_test)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* Reserving length 100 on an unallocated buffer:
     * Start at 64 -> doubling loop: 64 < 100 -> 128 >= 100.
     * So allocation should be 128.
     */
    ck_assert_int_eq(core_buffer_reserve(&buf, 100), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 128);

    core_buffer_destroy(&buf);

    /* Reserving length 300 on an unallocated buffer:
     * Start at 64 -> 128 -> 256 -> 512 >= 300.
     * So allocation should be 512.
     */
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_reserve(&buf, 300), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 512);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_reserve_exponential_growth_test)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* Step 1: Initial reserve for 10 bytes -> 64 allocated */
    ck_assert_int_eq(core_buffer_reserve(&buf, 10), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 64);

    /* Step 2: Request 70 bytes when current allocated is 64.
     * new_alloc candidate = 64 * 2 = 128.
     * Since 128 >= 70, new allocation should be 128.
     */
    ck_assert_int_eq(core_buffer_reserve(&buf, 70), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 128);

    /* Step 3: Request 129 bytes when current allocated is 128.
     * candidate = 128 * 2 = 256.
     * Since 256 >= 129, new allocation should be 256.
     */
    ck_assert_int_eq(core_buffer_reserve(&buf, 129), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 256);

    /* Step 4: Request 1000 bytes when current allocated is 256.
     * candidate = 256 * 2 = 512 < 1000 -> 1024 >= 1000.
     * So allocation should double to 1024.
     */
    ck_assert_int_eq(core_buffer_reserve(&buf, 1000), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 1024);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_reserve_external_wrapped_transition_test)
{
    core_buffer buf;
    uint8_t external_data[] = "external static buffer content";
    size_t ext_len = strlen((char *)external_data);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, external_data, ext_len), NSERROR_OK);

    ck_assert_int_eq(buf.allocated, 0);
    ck_assert(buf.data == external_data);
    ck_assert_int_eq(buf.length, ext_len);

    /* Reserving extra capacity beyond external buffer (ext_len < 64)
     * should transition the buffer to heap ownership:
     * allocated == 0 && data != NULL branch in core_buffer_reserve.
     * It allocates new memory (new_alloc = 64), copies existing data,
     * and sets allocated = 64.
     */
    ck_assert_int_eq(core_buffer_reserve(&buf, 50), NSERROR_OK);
    ck_assert(buf.data != external_data);
    ck_assert(buf.data != NULL);
    ck_assert_int_eq(buf.allocated, 64);
    ck_assert_int_eq(buf.length, ext_len);
    ck_assert_mem_eq(buf.data, external_data, ext_len);

    /* Now destroy should free the newly allocated heap memory */
    core_buffer_destroy(&buf);
}
END_TEST

/* --- Append, Clear, Shrink, and Utility Tests --- */

START_TEST(core_buffer_append_test)
{
    core_buffer buf;
    const uint8_t chunk1[] = "Hello, ";
    const uint8_t chunk2[] = "World!";
    const uint8_t chunk3[] = " Adding more text to verify dynamic resizing behavior.";

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* Append first chunk */
    ck_assert_int_eq(core_buffer_append(&buf, chunk1, strlen((char *)chunk1)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), strlen((char *)chunk1));
    ck_assert_int_eq(buf.allocated, 64);

    /* Append second chunk */
    ck_assert_int_eq(core_buffer_append(&buf, chunk2, strlen((char *)chunk2)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 13);
    ck_assert_mem_eq(core_buffer_data(&buf), "Hello, World!", 13);

    /* Append third chunk */
    ck_assert_int_eq(core_buffer_append(&buf, chunk3, strlen((char *)chunk3)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), 13 + strlen((char *)chunk3));
    ck_assert_mem_eq(core_buffer_data(&buf), "Hello, World! Adding more text to verify dynamic resizing behavior.", core_buffer_length(&buf));

    core_buffer_destroy(&buf);
}
END_TEST

/* --- Dedicated core_buffer_clear Unit Tests --- */

START_TEST(core_buffer_clear_null_test)
{
    /* Clear handles NULL pointer without crashing */
    core_buffer_clear(NULL);
}
END_TEST

START_TEST(core_buffer_clear_unallocated_test)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert(buf.data == NULL);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_int_eq(buf.length, 0);

    /* Clear on unallocated empty buffer */
    core_buffer_clear(&buf);
    ck_assert(buf.data == NULL);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_int_eq(buf.length, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_clear_reserved_test)
{
    core_buffer buf;

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_reserve(&buf, 100), NSERROR_OK);

    uint8_t *allocated_ptr = buf.data;
    size_t allocated_size = buf.allocated;

    ck_assert(allocated_ptr != NULL);
    ck_assert_int_eq(buf.length, 0);
    ck_assert(allocated_size >= 100);

    /* Clear on reserved empty buffer should preserve data pointer and capacity */
    core_buffer_clear(&buf);
    ck_assert_ptr_eq(buf.data, allocated_ptr);
    ck_assert_int_eq(buf.allocated, allocated_size);
    ck_assert_int_eq(buf.length, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_clear_appended_heap_test)
{
    core_buffer buf;
    const uint8_t initial_data[] = "abcdefghijklmnopqrstuvwxyz";
    const uint8_t replacement_data[] = "12345";

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, initial_data, sizeof(initial_data)), NSERROR_OK);

    uint8_t *allocated_ptr = buf.data;
    size_t allocated_size = buf.allocated;

    ck_assert_int_eq(core_buffer_length(&buf), sizeof(initial_data));

    /* Clear resets length to 0 while maintaining allocation */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert_ptr_eq(buf.data, allocated_ptr);
    ck_assert_int_eq(buf.allocated, allocated_size);

    /* Appending after clear writes at offset 0 */
    ck_assert_int_eq(core_buffer_append(&buf, replacement_data, sizeof(replacement_data)), NSERROR_OK);
    ck_assert_int_eq(core_buffer_length(&buf), sizeof(replacement_data));
    ck_assert_mem_eq(core_buffer_data(&buf), replacement_data, sizeof(replacement_data));

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_clear_wrapped_external_test)
{
    core_buffer buf;
    uint8_t external_mem[] = "external_read_only_buffer";
    size_t ext_len = strlen((char *)external_mem);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, external_mem, ext_len), NSERROR_OK);

    ck_assert_ptr_eq(buf.data, external_mem);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_int_eq(core_buffer_length(&buf), ext_len);

    /* Clearing wrapped external buffer resets length to 0 without altering data pointer or allocated size (0) */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert_ptr_eq(buf.data, external_mem);
    ck_assert_int_eq(buf.allocated, 0);

    /* External memory remains unchanged */
    ck_assert_str_eq((char *)external_mem, "external_read_only_buffer");

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_clear_wrapped_external_then_append_test)
{
    core_buffer buf;
    uint8_t external_mem[] = "static_buffer";
    size_t ext_len = strlen((char *)external_mem);
    const uint8_t new_payload[] = "heap_payload";
    size_t pay_len = strlen((char *)new_payload);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, external_mem, ext_len), NSERROR_OK);

    /* Clear the external buffer */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    /* Appending after clear should transition to heap allocation starting at offset 0 */
    ck_assert_int_eq(core_buffer_append(&buf, new_payload, pay_len), NSERROR_OK);
    ck_assert(buf.data != external_mem);
    ck_assert(buf.allocated >= pay_len);
    ck_assert_int_eq(core_buffer_length(&buf), pay_len);
    ck_assert_mem_eq(core_buffer_data(&buf), new_payload, pay_len);

    /* External memory must remain unmodified */
    ck_assert_str_eq((char *)external_mem, "static_buffer");

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_clear_idempotency_test)
{
    core_buffer buf;
    const uint8_t sample[] = "idempotency_test_data";

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, sample, strlen((char *)sample)), NSERROR_OK);

    uint8_t *allocated_ptr = buf.data;
    size_t allocated_size = buf.allocated;

    /* Multiple consecutive core_buffer_clear calls */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert_ptr_eq(buf.data, allocated_ptr);
    ck_assert_int_eq(buf.allocated, allocated_size);

    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert_ptr_eq(buf.data, allocated_ptr);
    ck_assert_int_eq(buf.allocated, allocated_size);

    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_length(&buf), 0);
    ck_assert_ptr_eq(buf.data, allocated_ptr);
    ck_assert_int_eq(buf.allocated, allocated_size);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_clear_reuse_cycles_test)
{
    core_buffer buf;
    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    for (int cycle = 0; cycle < 10; cycle++) {
        char cycle_data[32];
        snprintf(cycle_data, sizeof(cycle_data), "cycle_payload_%d", cycle);
        size_t len = strlen(cycle_data);

        ck_assert_int_eq(core_buffer_append(&buf, (const uint8_t *)cycle_data, len), NSERROR_OK);
        ck_assert_int_eq(core_buffer_length(&buf), len);
        ck_assert_mem_eq(core_buffer_data(&buf), cycle_data, len);

        core_buffer_clear(&buf);
        ck_assert_int_eq(core_buffer_length(&buf), 0);
    }

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_shrink_test)
{
    core_buffer buf;
    const uint8_t data[] = "1234567890";

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_append(&buf, data, 10), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 64);
    ck_assert_int_eq(buf.length, 10);

    /* Shrink buffer with non-zero length -> reallocates buffer->data down to buffer->length */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 10);
    ck_assert_int_eq(buf.length, 10);
    ck_assert_mem_eq(buf.data, "1234567890", 10);

    /* Shrink when length == allocated should be a no-op */
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert_int_eq(buf.allocated, 10);

    /* Clear and shrink -> length == 0 frees data and resets allocated to 0 */
    core_buffer_clear(&buf);
    ck_assert_int_eq(core_buffer_shrink(&buf), NSERROR_OK);
    ck_assert(buf.data == NULL);
    ck_assert_int_eq(buf.allocated, 0);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_wrap_external_test)
{
    core_buffer buf;
    uint8_t ext_data1[] = "hello external buffer";
    size_t len1 = strlen((char *)ext_data1);
    uint8_t ext_data2[] = "secondary buffer data";
    size_t len2 = strlen((char *)ext_data2);

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);

    /* 1. Wrapping valid non-NULL external data */
    ck_assert_int_eq(core_buffer_wrap_external(&buf, ext_data1, len1), NSERROR_OK);
    ck_assert_ptr_eq(buf.data, ext_data1);
    ck_assert_int_eq(buf.length, len1);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_ptr_eq(core_buffer_data(&buf), ext_data1);
    ck_assert_int_eq(core_buffer_length(&buf), len1);

    /* 2. Wrapping NULL data with 0 length */
    ck_assert_int_eq(core_buffer_wrap_external(&buf, NULL, 0), NSERROR_OK);
    ck_assert_ptr_eq(buf.data, NULL);
    ck_assert_int_eq(buf.length, 0);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_ptr_eq(core_buffer_data(&buf), NULL);
    ck_assert_int_eq(core_buffer_length(&buf), 0);

    /* 3. Re-wrapping buffer with different external data and length */
    ck_assert_int_eq(core_buffer_wrap_external(&buf, ext_data2, len2), NSERROR_OK);
    ck_assert_ptr_eq(buf.data, ext_data2);
    ck_assert_int_eq(buf.length, len2);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_ptr_eq(core_buffer_data(&buf), ext_data2);
    ck_assert_int_eq(core_buffer_length(&buf), len2);

    core_buffer_destroy(&buf);
}
END_TEST

START_TEST(core_buffer_wrap_external_destroy_test)
{
    core_buffer buf;
    uint8_t external_buf[32] = "don't free me";

    ck_assert_int_eq(core_buffer_init(&buf), NSERROR_OK);
    ck_assert_int_eq(core_buffer_wrap_external(&buf, external_buf, 13), NSERROR_OK);
    ck_assert(core_buffer_data(&buf) == external_buf);
    ck_assert_int_eq(core_buffer_length(&buf), 13);
    ck_assert_int_eq(buf.allocated, 0);

    /* Destroying a wrapped external buffer with allocated == 0 must not free external_buf */
    core_buffer_destroy(&buf);
    ck_assert(buf.data == NULL);
    ck_assert_int_eq(buf.length, 0);
    ck_assert_int_eq(buf.allocated, 0);
    ck_assert_str_eq((char *)external_buf, "don't free me");
}
END_TEST

/* --- Suite Definition --- */

static Suite *core_buffer_suite(void)
{
    Suite *s = suite_create("core_buffer");

    TCase *tc_api = tcase_create("API & Validation");
    tcase_add_test(tc_api, core_buffer_null_parameter_test);
    tcase_add_test(tc_api, core_buffer_init_destroy_test);
    tcase_add_test(tc_api, core_buffer_length_test);
    suite_add_tcase(s, tc_api);

    TCase *tc_reserve = tcase_create("Reserve & Allocation");
    tcase_add_test(tc_reserve, core_buffer_reserve_initial_allocation_test);
    tcase_add_test(tc_reserve, core_buffer_reserve_power_of_two_scaling_test);
    tcase_add_test(tc_reserve, core_buffer_reserve_exponential_growth_test);
    tcase_add_test(tc_reserve, core_buffer_reserve_external_wrapped_transition_test);
    suite_add_tcase(s, tc_reserve);

    TCase *tc_ops = tcase_create("Append & Shrink");
    tcase_add_test(tc_ops, core_buffer_append_test);
    tcase_add_test(tc_ops, core_buffer_append_zero_length_test);
    tcase_add_test(tc_ops, core_buffer_append_binary_data_test);
    tcase_add_test(tc_ops, core_buffer_append_wrapped_external_test);
    tcase_add_test(tc_ops, core_buffer_append_multiple_reallocations_test);
    tcase_add_test(tc_ops, core_buffer_shrink_test);
    tcase_add_test(tc_ops, core_buffer_wrap_external_test);
    tcase_add_test(tc_ops, core_buffer_wrap_external_destroy_test);
    suite_add_tcase(s, tc_ops);

    TCase *tc_clear = tcase_create("Clear Operations");
    tcase_add_test(tc_clear, core_buffer_clear_null_test);
    tcase_add_test(tc_clear, core_buffer_clear_unallocated_test);
    tcase_add_test(tc_clear, core_buffer_clear_reserved_test);
    tcase_add_test(tc_clear, core_buffer_clear_appended_heap_test);
    tcase_add_test(tc_clear, core_buffer_clear_wrapped_external_test);
    tcase_add_test(tc_clear, core_buffer_clear_wrapped_external_then_append_test);
    tcase_add_test(tc_clear, core_buffer_clear_idempotency_test);
    tcase_add_test(tc_clear, core_buffer_clear_reuse_cycles_test);
    suite_add_tcase(s, tc_clear);

    return s;
}

int main(int argc, char **argv)
{
    int number_failed;
    Suite *s = core_buffer_suite();
    SRunner *sr = srunner_create(s);

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
