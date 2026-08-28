/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#ifndef _WISP_UTILS_CSS_DELIMITERS_H_
#define _WISP_UTILS_CSS_DELIMITERS_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#elif defined(__riscv) && defined(__riscv_vector)
#include <riscv_vector.h>
#endif

#if defined(__arm__) || defined(__aarch64__)
static inline bool wisp_delim_has_neon(void) {
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    return true;
#else
    return false;
#endif
}
#endif

#if defined(__riscv)
#ifdef __linux__
#include <sys/auxv.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifndef AT_HWCAP
#define AT_HWCAP 9
#endif
#ifndef COMPAT_HWCAP_ISA_V
#define COMPAT_HWCAP_ISA_V (1 << ('V' - 'A'))
#endif

#ifndef __NR_riscv_hwprobe
#define __NR_riscv_hwprobe 258
#endif
#ifndef RISCV_HWPROBE_KEY_IMA_EXT_0
#define RISCV_HWPROBE_KEY_IMA_EXT_0 4
#endif
#ifndef RISCV_HWPROBE_IMA_V
#define RISCV_HWPROBE_IMA_V (1 << 2)
#endif

struct wisp_delim_riscv_hwprobe {
    int64_t key;
    uint64_t value;
};

static inline bool wisp_delim_has_rvv(void) {
    static int cached_rvv = -1;
    if (cached_rvv != -1) {
        return (bool)cached_rvv;
    }
    struct wisp_delim_riscv_hwprobe request;
    request.key = RISCV_HWPROBE_KEY_IMA_EXT_0;
    request.value = 0;
    if (syscall(__NR_riscv_hwprobe, &request, 1, 0, NULL, 0) == 0) {
        if (request.value & RISCV_HWPROBE_IMA_V) {
            cached_rvv = 1;
            return true;
        }
    }
    unsigned long hwcap = getauxval(AT_HWCAP);
    if (hwcap & COMPAT_HWCAP_ISA_V) {
        cached_rvv = 1;
        return true;
    }
    cached_rvv = 0;
    return false;
}
#else
static inline bool wisp_delim_has_rvv(void) {
#ifdef __riscv_vector
    return true;
#else
    return false;
#endif
}
#endif
#endif

static inline bool is_css_delimiter_scalar(uint8_t c) {
    return !( (c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' ||
              c >= 0x80 );
}

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("sse2")))
#endif
static inline size_t wisp_scan_css_delimiters_sse2(const uint8_t *data, size_t len) {
    size_t i = 0;
    __m128i a_val = _mm_set1_epi8('a');
    __m128i z_val = _mm_set1_epi8('z');
    __m128i A_val = _mm_set1_epi8('A');
    __m128i Z_val = _mm_set1_epi8('Z');
    __m128i d0_val = _mm_set1_epi8('0');
    __m128i d9_val = _mm_set1_epi8('9');
    __m128i under_v = _mm_set1_epi8('_');
    __m128i dash_v  = _mm_set1_epi8('-');
    __m128i zero_v  = _mm_setzero_si128();

    while (i + 15 < len) {
        __m128i v = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i is_lower = _mm_andnot_si128(_mm_cmplt_epi8(v, a_val), _mm_andnot_si128(_mm_cmpgt_epi8(v, z_val), _mm_cmpeq_epi8(v, v)));
        __m128i is_upper = _mm_andnot_si128(_mm_cmplt_epi8(v, A_val), _mm_andnot_si128(_mm_cmpgt_epi8(v, Z_val), _mm_cmpeq_epi8(v, v)));
        __m128i is_digit = _mm_andnot_si128(_mm_cmplt_epi8(v, d0_val), _mm_andnot_si128(_mm_cmpgt_epi8(v, d9_val), _mm_cmpeq_epi8(v, v)));
        __m128i is_under = _mm_cmpeq_epi8(v, under_v);
        __m128i is_dash  = _mm_cmpeq_epi8(v, dash_v);
        __m128i is_nonascii = _mm_cmplt_epi8(v, zero_v); /* high bit set (c >= 0x80) */

        __m128i is_ident = _mm_or_si128(_mm_or_si128(_mm_or_si128(is_lower, is_upper),
                                                      _mm_or_si128(is_digit, is_nonascii)),
                                         _mm_or_si128(is_under, is_dash));

        int mask = (~_mm_movemask_epi8(is_ident)) & 0xFFFF;
        if (mask != 0) {
            return i + __builtin_ctz(mask);
        }
        i += 16;
    }
    while (i < len) {
        if (is_css_delimiter_scalar(data[i])) {
            return i;
        }
        i++;
    }
    return i;
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static inline size_t wisp_scan_css_delimiters_neon(const uint8_t *data, size_t len) {
    size_t i = 0;
    uint8x16_t a_val = vdupq_n_u8('a');
    uint8x16_t z_val = vdupq_n_u8('z');
    uint8x16_t A_val = vdupq_n_u8('A');
    uint8x16_t Z_val = vdupq_n_u8('Z');
    uint8x16_t d0_val = vdupq_n_u8('0');
    uint8x16_t d9_val = vdupq_n_u8('9');
    uint8x16_t under_v = vdupq_n_u8('_');
    uint8x16_t dash_v  = vdupq_n_u8('-');
    uint8x16_t high_bit = vdupq_n_u8(0x80);

    while (i + 15 < len) {
        uint8x16_t v = vld1q_u8(data + i);

        uint8x16_t is_lower = vandq_u8(vcgeq_u8(v, a_val), vcleq_u8(v, z_val));
        uint8x16_t is_upper = vandq_u8(vcgeq_u8(v, A_val), vcleq_u8(v, Z_val));
        uint8x16_t is_digit = vandq_u8(vcgeq_u8(v, d0_val), vcleq_u8(v, d9_val));
        uint8x16_t is_under = vceqq_u8(v, under_v);
        uint8x16_t is_dash  = vceqq_u8(v, dash_v);
        uint8x16_t is_nonascii = vtstq_u8(v, high_bit);

        uint8x16_t is_ident = vorrq_u8(vorrq_u8(vorrq_u8(is_lower, is_upper),
                                                 vorrq_u8(is_digit, is_nonascii)),
                                        vorrq_u8(is_under, is_dash));

        uint8x16_t not_ident = vmvnq_u8(is_ident);

        uint8x8_t low = vget_low_u8(not_ident);
        uint8x8_t high = vget_high_u8(not_ident);
        uint8x8_t combined = vorr_u8(low, high);
        uint32x2_t repr = vreinterpret_u32_u8(combined);
        if (vget_lane_u32(repr, 0) != 0 || vget_lane_u32(repr, 1) != 0) {
            uint8_t temp[16];
            vst1q_u8(temp, not_ident);
            for (int j = 0; j < 16; j++) {
                if (temp[j] != 0) {
                    return i + j;
                }
            }
        }
        i += 16;
    }
    while (i < len) {
        if (is_css_delimiter_scalar(data[i])) {
            return i;
        }
        i++;
    }
    return i;
}
#endif

#if defined(__riscv) && defined(__riscv_vector)
static inline size_t wisp_scan_css_delimiters_rvv(const uint8_t *data, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1(data + i, vl);

        vbool8_t ge_a = __riscv_vmsgeu_vx_u8m1_b8(chunk, 'a', vl);
        vbool8_t le_z = __riscv_vmsleu_vx_u8m1_b8(chunk, 'z', vl);
        vbool8_t is_lower = __riscv_vmand_mm_b8(ge_a, le_z, vl);

        vbool8_t ge_A = __riscv_vmsgeu_vx_u8m1_b8(chunk, 'A', vl);
        vbool8_t le_Z = __riscv_vmsleu_vx_u8m1_b8(chunk, 'Z', vl);
        vbool8_t is_upper = __riscv_vmand_mm_b8(ge_A, le_Z, vl);

        vbool8_t ge_0 = __riscv_vmsgeu_vx_u8m1_b8(chunk, '0', vl);
        vbool8_t le_9 = __riscv_vmsleu_vx_u8m1_b8(chunk, '9', vl);
        vbool8_t is_digit = __riscv_vmand_mm_b8(ge_0, le_9, vl);

        vbool8_t is_under = __riscv_vmseq_vx_u8m1_b8(chunk, '_', vl);
        vbool8_t is_dash  = __riscv_vmseq_vx_u8m1_b8(chunk, '-', vl);
        vbool8_t is_nonascii = __riscv_vmsgeu_vx_u8m1_b8(chunk, 0x80, vl);

        vbool8_t is_ident = __riscv_vmor_mm_b8(is_lower, is_upper, vl);
        is_ident = __riscv_vmor_mm_b8(is_ident, is_digit, vl);
        is_ident = __riscv_vmor_mm_b8(is_ident, is_under, vl);
        is_ident = __riscv_vmor_mm_b8(is_ident, is_dash, vl);
        is_ident = __riscv_vmor_mm_b8(is_ident, is_nonascii, vl);

        vbool8_t not_ident = __riscv_vmnot_m_b8(is_ident, vl);

        long first_non_ident = __riscv_vfirst_m_b8(not_ident, vl);
        if (first_non_ident >= 0) {
            return i + first_non_ident;
        }
        i += vl;
    }
    return i;
}
#endif

static inline size_t wisp_scan_css_delimiters_scalar(const uint8_t *data, size_t len) {
    size_t i = 0;
    while (i < len) {
        if (is_css_delimiter_scalar(data[i])) {
            return i;
        }
        i++;
    }
    return i;
}

static inline size_t wisp_scan_css_delimiters(const uint8_t *data, size_t len) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    return wisp_scan_css_delimiters_sse2(data, len);
#elif defined(__arm__) || defined(__aarch64__)
    if (wisp_delim_has_neon()) {
        return wisp_scan_css_delimiters_neon(data, len);
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (wisp_delim_has_rvv()) {
        return wisp_scan_css_delimiters_rvv(data, len);
    }
#endif
    return wisp_scan_css_delimiters_scalar(data, len);
}

#endif /* _WISP_UTILS_CSS_DELIMITERS_H_ */
