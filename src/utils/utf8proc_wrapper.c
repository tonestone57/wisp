/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include "wisp/utils/utf8proc_wrapper.h"
#include <string.h>
#include <stdlib.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#endif

/* Dynamic CPU Feature Detection */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
static inline bool has_avx2(void) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_cpu_supports("avx2");
#else
    return false;
#endif
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static inline bool has_neon(void) {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    return true;
#else
    return false;
#endif
}
#endif

/* 1. is_ascii implementations */

static bool wisp_is_ascii_scalar(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if ((unsigned char)str[i] >= 0x80) {
            return false;
        }
    }
    return true;
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
__attribute__((target("avx2")))
static bool wisp_is_ascii_avx2(const char *str, size_t len) {
    size_t i = 0;
    if (len >= 32) {
        __m256i mask = _mm256_set1_epi8(0x80);
        for (; i + 31 < len; i += 32) {
            __m256i chunk = _mm256_loadu_si256((const __m256i *)(str + i));
            __m256i test = _mm256_and_si256(chunk, mask);
            if (!_mm256_testz_si256(test, test)) {
                return false;
            }
        }
    }
    for (; i < len; i++) {
        if ((unsigned char)str[i] >= 0x80) {
            return false;
        }
    }
    return true;
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static bool wisp_is_ascii_neon(const char *str, size_t len) {
    size_t i = 0;
    if (len >= 16) {
        uint8x16_t high_bit_mask = vdupq_n_u8(0x80);
        for (; i + 15 < len; i += 16) {
            uint8x16_t chunk = vld1q_u8((const uint8_t *)(str + i));
            uint8x16_t test = vandq_u8(chunk, high_bit_mask);
            #if defined(__aarch64__)
                if (vmaxvq_u8(test) != 0) {
                    return false;
                }
            #else
                uint8x8_t low = vget_low_u8(test);
                uint8x8_t high = vget_high_u8(test);
                uint8x8_t combined = vorr_u8(low, high);
                uint32x2_t repr = vreinterpret_u32_u8(combined);
                if (vget_lane_u32(repr, 0) != 0 || vget_lane_u32(repr, 1) != 0) {
                    return false;
                }
            #endif
        }
    }
    for (; i < len; i++) {
        if ((unsigned char)str[i] >= 0x80) {
            return false;
        }
    }
    return true;
}
#endif

void wisp_is_ascii_func_for_test(void) {
    /* Stub for test harness referencing */
}

bool wisp_is_ascii(const char *str, size_t len) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_avx2()) {
        return wisp_is_ascii_avx2(str, len);
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        return wisp_is_ascii_neon(str, len);
    }
#endif
    return wisp_is_ascii_scalar(str, len);
}

/* 2. UTF-8 Validation */

bool wisp_validate_utf8(const char *str, size_t len) {
    if (wisp_is_ascii(str, len)) {
        return true;
    }
    size_t i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)str[i];
        if (c < 0x80) {
            i++;
        } else if ((c & 0xE0) == 0xC0) {
            if (i + 1 >= len) return false;
            if (((unsigned char)str[i+1] & 0xC0) != 0x80) return false;
            if (c < 0xC2) return false;
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            if (i + 2 >= len) return false;
            unsigned char c1 = (unsigned char)str[i+1];
            unsigned char c2 = (unsigned char)str[i+2];
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) return false;
            if (c == 0xE0 && c1 < 0xA0) return false;
            if (c == 0xED && c1 >= 0xA0) return false;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            if (i + 3 >= len) return false;
            unsigned char c1 = (unsigned char)str[i+1];
            unsigned char c2 = (unsigned char)str[i+2];
            unsigned char c3 = (unsigned char)str[i+3];
            if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return false;
            if (c == 0xF0 && c1 < 0x90) return false;
            if (c == 0xF4 && c1 >= 0x90) return false;
            if (c > 0xF4) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

/* 3. ascii_tolower implementations */

static void wisp_ascii_tolower_scalar(const char *src, char *dst, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') {
            dst[i] = c + 32;
        } else {
            dst[i] = c;
        }
    }
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
__attribute__((target("avx2")))
static void wisp_ascii_tolower_avx2(const char *src, char *dst, size_t len) {
    size_t i = 0;
    if (len >= 32) {
        __m256i a_bound = _mm256_set1_epi8('A' - 1);
        __m256i z_bound = _mm256_set1_epi8('Z' + 1);
        __m256i offset = _mm256_set1_epi8(32);
        for (; i + 31 < len; i += 32) {
            __m256i chunk = _mm256_loadu_si256((const __m256i *)(src + i));
            __m256i gt_A = _mm256_cmpgt_epi8(chunk, a_bound);
            __m256i lt_Z = _mm256_cmpgt_epi8(z_bound, chunk);
            __m256i is_upper = _mm256_and_si256(gt_A, lt_Z);
            __m256i add_mask = _mm256_and_si256(is_upper, offset);
            __m256i result = _mm256_add_epi8(chunk, add_mask);
            _mm256_storeu_si256((__m256i *)(dst + i), result);
        }
    }
    for (; i < len; i++) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') {
            dst[i] = c + 32;
        } else {
            dst[i] = c;
        }
    }
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static void wisp_ascii_tolower_neon(const char *src, char *dst, size_t len) {
    size_t i = 0;
    if (len >= 16) {
        int8x16_t a_bound = vdupq_n_s8('A' - 1);
        int8x16_t z_bound = vdupq_n_s8('Z' + 1);
        int8x16_t offset = vdupq_n_s8(32);
        for (; i + 15 < len; i += 16) {
            int8x16_t chunk = vld1q_s8((const int8_t *)(src + i));
            uint8x16_t gt_A = vcgtq_s8(chunk, a_bound);
            uint8x16_t lt_Z = vcgtq_s8(z_bound, chunk);
            uint8x16_t is_upper = vandq_u8(gt_A, lt_Z);
            int8x16_t add_mask = vandq_s8(vreinterpretq_s8_u8(is_upper), offset);
            int8x16_t result = vaddq_s8(chunk, add_mask);
            vst1q_s8((int8_t *)(dst + i), result);
        }
    }
    for (; i < len; i++) {
        char c = src[i];
        if (c >= 'A' && c <= 'Z') {
            dst[i] = c + 32;
        } else {
            dst[i] = c;
        }
    }
}
#endif

void wisp_ascii_tolower(const char *src, char *dst, size_t len) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_avx2()) {
        wisp_ascii_tolower_avx2(src, dst, len);
        return;
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        wisp_ascii_tolower_neon(src, dst, len);
        return;
    }
#endif
    wisp_ascii_tolower_scalar(src, dst, len);
}

/* 4. ascii_toupper implementations */

static void wisp_ascii_toupper_scalar(const char *src, char *dst, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char c = src[i];
        if (c >= 'a' && c <= 'z') {
            dst[i] = c - 32;
        } else {
            dst[i] = c;
        }
    }
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
__attribute__((target("avx2")))
static void wisp_ascii_toupper_avx2(const char *src, char *dst, size_t len) {
    size_t i = 0;
    if (len >= 32) {
        __m256i a_bound = _mm256_set1_epi8('a' - 1);
        __m256i z_bound = _mm256_set1_epi8('z' + 1);
        __m256i offset = _mm256_set1_epi8(32);
        for (; i + 31 < len; i += 32) {
            __m256i chunk = _mm256_loadu_si256((const __m256i *)(src + i));
            __m256i gt_a = _mm256_cmpgt_epi8(chunk, a_bound);
            __m256i lt_z = _mm256_cmpgt_epi8(z_bound, chunk);
            __m256i is_lower = _mm256_and_si256(gt_a, lt_z);
            __m256i sub_mask = _mm256_and_si256(is_lower, offset);
            __m256i result = _mm256_sub_epi8(chunk, sub_mask);
            _mm256_storeu_si256((__m256i *)(dst + i), result);
        }
    }
    for (; i < len; i++) {
        char c = src[i];
        if (c >= 'a' && c <= 'z') {
            dst[i] = c - 32;
        } else {
            dst[i] = c;
        }
    }
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static void wisp_ascii_toupper_neon(const char *src, char *dst, size_t len) {
    size_t i = 0;
    if (len >= 16) {
        int8x16_t a_bound = vdupq_n_s8('a' - 1);
        int8x16_t z_bound = vdupq_n_s8('z' + 1);
        int8x16_t offset = vdupq_n_s8(32);
        for (; i + 15 < len; i += 16) {
            int8x16_t chunk = vld1q_s8((const int8_t *)(src + i));
            uint8x16_t gt_a = vcgtq_s8(chunk, a_bound);
            uint8x16_t lt_z = vcgtq_s8(z_bound, chunk);
            uint8x16_t is_lower = vandq_u8(gt_a, lt_z);
            int8x16_t sub_mask = vandq_s8(vreinterpretq_s8_u8(is_lower), offset);
            int8x16_t result = vsubq_s8(chunk, sub_mask);
            vst1q_s8((int8_t *)(dst + i), result);
        }
    }
    for (; i < len; i++) {
        char c = src[i];
        if (c >= 'a' && c <= 'z') {
            dst[i] = c - 32;
        } else {
            dst[i] = c;
        }
    }
}
#endif

void wisp_ascii_toupper(const char *src, char *dst, size_t len) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_avx2()) {
        wisp_ascii_toupper_avx2(src, dst, len);
        return;
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        wisp_ascii_toupper_neon(src, dst, len);
        return;
    }
#endif
    wisp_ascii_toupper_scalar(src, dst, len);
}

/* 5. ascii_to_utf32 implementations */

static void wisp_ascii_to_utf32_scalar(const char *src, int32_t *dst, size_t len) {
    for (size_t i = 0; i < len; i++) {
        dst[i] = (unsigned char)src[i];
    }
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
__attribute__((target("avx2")))
static void wisp_ascii_to_utf32_avx2(const char *src, int32_t *dst, size_t len) {
    size_t i = 0;
    if (len >= 8) {
        for (; i + 7 < len; i += 8) {
            __m128i chunk = _mm_loadl_epi64((const __m128i *)(src + i));
            __m256i extended = _mm256_cvtepu8_epi32(chunk);
            _mm256_storeu_si256((__m256i *)(dst + i), extended);
        }
    }
    for (; i < len; i++) {
        dst[i] = (unsigned char)src[i];
    }
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static void wisp_ascii_to_utf32_neon(const char *src, int32_t *dst, size_t len) {
    size_t i = 0;
    if (len >= 8) {
        for (; i + 7 < len; i += 8) {
            uint8x8_t chunk = vld1_u8((const uint8_t *)(src + i));
            uint16x8_t chunk_16 = vmovl_u8(chunk);
            uint32x4_t low = vmovl_u16(vget_low_u16(chunk_16));
            uint32x4_t high = vmovl_u16(vget_high_u16(chunk_16));
            vst1q_s32(dst + i, vreinterpretq_s32_u32(low));
            vst1q_s32(dst + i + 4, vreinterpretq_s32_u32(high));
        }
    }
    for (; i < len; i++) {
        dst[i] = (unsigned char)src[i];
    }
}
#endif

void wisp_ascii_to_utf32(const char *src, int32_t *dst, size_t len) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_avx2()) {
        wisp_ascii_to_utf32_avx2(src, dst, len);
        return;
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        wisp_ascii_to_utf32_neon(src, dst, len);
        return;
    }
#endif
    wisp_ascii_to_utf32_scalar(src, dst, len);
}

/* 6. utf32_to_ascii implementations */

static void wisp_utf32_to_ascii_scalar(const int32_t *src, char *dst, size_t len) {
    for (size_t i = 0; i < len; i++) {
        dst[i] = (char)(src[i] & 0xFF);
    }
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
__attribute__((target("avx2")))
static void wisp_utf32_to_ascii_avx2(const int32_t *src, char *dst, size_t len) {
    size_t i = 0;
    if (len >= 8) {
        for (; i + 7 < len; i += 8) {
            __m256i v = _mm256_loadu_si256((const __m256i *)(src + i));
            __m256i packed_16 = _mm256_packus_epi32(v, v);
            __m256i permuted = _mm256_permute4x64_epi64(packed_16, 0x08);
            __m128i low_128 = _mm256_castsi256_si128(permuted);
            __m128i packed_8 = _mm_packus_epi16(low_128, low_128);
            _mm_storel_epi64((__m128i *)(dst + i), packed_8);
        }
    }
    for (; i < len; i++) {
        dst[i] = (char)(src[i] & 0xFF);
    }
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static void wisp_utf32_to_ascii_neon(const int32_t *src, char *dst, size_t len) {
    size_t i = 0;
    if (len >= 8) {
        for (; i + 7 < len; i += 8) {
            uint32x4_t low = vreinterpretq_u32_s32(vld1q_s32(src + i));
            uint32x4_t high = vreinterpretq_u32_s32(vld1q_s32(src + i + 4));
            uint16x4_t low_16 = vmovn_u32(low);
            uint16x4_t high_16 = vmovn_u32(high);
            uint16x8_t combined_16 = vcombine_u16(low_16, high_16);
            uint8x8_t result = vmovn_u16(combined_16);
            vst1_u8((uint8_t *)(dst + i), result);
        }
    }
    for (; i < len; i++) {
        dst[i] = (char)(src[i] & 0xFF);
    }
}
#endif

void wisp_utf32_to_ascii(const int32_t *src, char *dst, size_t len) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_avx2()) {
        wisp_utf32_to_ascii_avx2(src, dst, len);
        return;
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        wisp_utf32_to_ascii_neon(src, dst, len);
        return;
    }
#endif
    wisp_utf32_to_ascii_scalar(src, dst, len);
}

/* Optimized wrappers for utf8proc standard functions */

utf8proc_ssize_t wisp_utf8proc_decompose(
    const utf8proc_uint8_t *str, utf8proc_ssize_t len,
    utf8proc_int32_t *buffer, utf8proc_ssize_t bufsize, utf8proc_option_t options)
{
    if (len < 0) {
        len = (utf8proc_ssize_t)strlen((const char *)str);
    }

    if (wisp_is_ascii((const char *)str, len)) {
        utf8proc_option_t modifying_flags = UTF8PROC_CASEFOLD | UTF8PROC_STRIPCC |
                                            UTF8PROC_NLF2LS | UTF8PROC_NLF2PS |
                                            UTF8PROC_LUMP | UTF8PROC_CHARBOUND |
                                            UTF8PROC_STRIPMARK | UTF8PROC_IGNORE;
        if ((options & modifying_flags) == 0) {
            if (bufsize >= len) {
                wisp_ascii_to_utf32((const char *)str, buffer, len);
            }
            return len;
        }
    }

    return utf8proc_decompose(str, len, buffer, bufsize, options);
}

utf8proc_ssize_t wisp_utf8proc_normalize_utf32(
    utf8proc_int32_t *buffer, utf8proc_ssize_t length, utf8proc_option_t options)
{
    if (length < 0) return UTF8PROC_ERROR_INVALIDOPTS;

    bool is_ascii = true;
    for (utf8proc_ssize_t i = 0; i < length; i++) {
        if (buffer[i] >= 0x80 || buffer[i] < 0) {
            is_ascii = false;
            break;
        }
    }

    if (is_ascii) {
        utf8proc_option_t modifying_flags = UTF8PROC_STRIPCC | UTF8PROC_NLF2LS | UTF8PROC_NLF2PS;
        if ((options & modifying_flags) == 0) {
            return length;
        }
    }

    return utf8proc_normalize_utf32(buffer, length, options);
}

utf8proc_ssize_t wisp_utf8proc_reencode(
    utf8proc_int32_t *buffer, utf8proc_ssize_t length, utf8proc_option_t options)
{
    if (length < 0) return UTF8PROC_ERROR_INVALIDOPTS;

    bool is_ascii = true;
    for (utf8proc_ssize_t i = 0; i < length; i++) {
        if (buffer[i] >= 0x80 || buffer[i] < 0) {
            is_ascii = false;
            break;
        }
    }

    if (is_ascii) {
        utf8proc_option_t modifying_flags = UTF8PROC_STRIPCC | UTF8PROC_NLF2LS | UTF8PROC_NLF2PS | UTF8PROC_CHARBOUND;
        if ((options & modifying_flags) == 0) {
            char *dst = (char *)buffer;
            wisp_utf32_to_ascii(buffer, dst, length);
            dst[length] = '\0';
            return length;
        }
    }

    return utf8proc_reencode(buffer, length, options);
}

utf8proc_uint8_t *wisp_utf8proc_NFD(const utf8proc_uint8_t *str) {
    size_t len = strlen((const char *)str);
    if (wisp_is_ascii((const char *)str, len)) {
        return (utf8proc_uint8_t *)strdup((const char *)str);
    }
    return utf8proc_NFD(str);
}

utf8proc_uint8_t *wisp_utf8proc_NFC(const utf8proc_uint8_t *str) {
    size_t len = strlen((const char *)str);
    if (wisp_is_ascii((const char *)str, len)) {
        return (utf8proc_uint8_t *)strdup((const char *)str);
    }
    return utf8proc_NFC(str);
}

utf8proc_uint8_t *wisp_utf8proc_NFKD(const utf8proc_uint8_t *str) {
    size_t len = strlen((const char *)str);
    if (wisp_is_ascii((const char *)str, len)) {
        return (utf8proc_uint8_t *)strdup((const char *)str);
    }
    return utf8proc_NFKD(str);
}

utf8proc_uint8_t *wisp_utf8proc_NFKC(const utf8proc_uint8_t *str) {
    size_t len = strlen((const char *)str);
    if (wisp_is_ascii((const char *)str, len)) {
        return (utf8proc_uint8_t *)strdup((const char *)str);
    }
    return utf8proc_NFKC(str);
}
