/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#ifndef _WISP_UTILS_WHITESPACES_H_
#define _WISP_UTILS_WHITESPACES_H_

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

#if defined(__clang__) || defined(__GNUC__)
#define WISP_WS_NO_SANITIZE __attribute__((no_sanitize("address", "thread")))
#elif defined(_MSC_VER)
#define WISP_WS_NO_SANITIZE __declspec(no_sanitize_address)
#else
#define WISP_WS_NO_SANITIZE
#endif

#if defined(__arm__) || defined(__aarch64__)
static inline bool wisp_ws_has_neon(void) {
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

struct wisp_ws_riscv_hwprobe {
    int64_t key;
    uint64_t value;
};

static inline bool wisp_ws_has_rvv(void) {
    struct wisp_ws_riscv_hwprobe request;
    request.key = RISCV_HWPROBE_KEY_IMA_EXT_0;
    request.value = 0;
    if (syscall(__NR_riscv_hwprobe, &request, 1, 0, NULL, 0) == 0) {
        if (request.value & RISCV_HWPROBE_IMA_V) {
            return true;
        }
    }
    unsigned long hwcap = getauxval(AT_HWCAP);
    if (hwcap & COMPAT_HWCAP_ISA_V) {
        return true;
    }
    return false;
}
#else
static inline bool wisp_ws_has_rvv(void) {
#ifdef __riscv_vector
    return true;
#else
    return false;
#endif
}
#endif
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("sse2")))
#endif
static inline size_t wisp_skip_whitespaces_sse2(const uint8_t *data, size_t len) {
    size_t i = 0;
    __m128i space_v = _mm_set1_epi8(' ');
    __m128i tab_v = _mm_set1_epi8('\t');
    __m128i nl_v = _mm_set1_epi8('\n');
    __m128i ff_v = _mm_set1_epi8('\f');
    __m128i cr_v = _mm_set1_epi8('\r');
    while (i + 15 < len) {
        __m128i v = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i eq_space = _mm_cmpeq_epi8(v, space_v);
        __m128i eq_tab = _mm_cmpeq_epi8(v, tab_v);
        __m128i eq_nl = _mm_cmpeq_epi8(v, nl_v);
        __m128i eq_ff = _mm_cmpeq_epi8(v, ff_v);
        __m128i eq_cr = _mm_cmpeq_epi8(v, cr_v);
        __m128i is_ws = _mm_or_si128(_mm_or_si128(eq_space, eq_tab),
                                     _mm_or_si128(eq_nl, _mm_or_si128(eq_ff, eq_cr)));
        int mask = _mm_movemask_epi8(is_ws);
        int not_ws_mask = (~mask) & 0xFFFF;
        if (not_ws_mask == 0) {
            i += 16;
        } else {
            i += __builtin_ctz(not_ws_mask);
            return i;
        }
    }
    while (i < len) {
        uint8_t c = data[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r') {
            i++;
        } else {
            break;
        }
    }
    return i;
}
#endif

#if defined(__arm__) || defined(__aarch64__)
static inline size_t wisp_skip_whitespaces_neon(const uint8_t *data, size_t len) {
    size_t i = 0;
    uint8x16_t space_v = vdupq_n_u8(' ');
    uint8x16_t tab_v = vdupq_n_u8('\t');
    uint8x16_t nl_v = vdupq_n_u8('\n');
    uint8x16_t ff_v = vdupq_n_u8('\f');
    uint8x16_t cr_v = vdupq_n_u8('\r');
    while (i + 15 < len) {
        uint8x16_t v = vld1q_u8(data + i);
        uint8x16_t eq_space = vceqq_u8(v, space_v);
        uint8x16_t eq_tab = vceqq_u8(v, tab_v);
        uint8x16_t eq_nl = vceqq_u8(v, nl_v);
        uint8x16_t eq_ff = vceqq_u8(v, ff_v);
        uint8x16_t eq_cr = vceqq_u8(v, cr_v);
        uint8x16_t is_ws = vorrq_u8(vorrq_u8(eq_space, eq_tab),
                                    vorrq_u8(eq_nl, vorrq_u8(eq_ff, eq_cr)));
        uint8x16_t not_ws = vmvnq_u8(is_ws);
        uint8x8_t low = vget_low_u8(not_ws);
        uint8x8_t high = vget_high_u8(not_ws);
        uint8x8_t combined = vorr_u8(low, high);
        uint32x2_t repr = vreinterpret_u32_u8(combined);
        if (vget_lane_u32(repr, 0) == 0 && vget_lane_u32(repr, 1) == 0) {
            i += 16;
        } else {
            uint8_t temp[16];
            vst1q_u8(temp, not_ws);
            for (int j = 0; j < 16; j++) {
                if (temp[j] != 0) {
                    i += j;
                    break;
                }
            }
            return i;
        }
    }
    while (i < len) {
        uint8_t c = data[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r') {
            i++;
        } else {
            break;
        }
    }
    return i;
}
#endif

#if defined(__riscv) && defined(__riscv_vector)
static inline size_t wisp_skip_whitespaces_rvv(const uint8_t *data, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1(data + i, vl);
        vbool8_t eq_space = __riscv_vmseq_vx_u8m1_b8(chunk, ' ', vl);
        vbool8_t eq_tab = __riscv_vmseq_vx_u8m1_b8(chunk, '\t', vl);
        vbool8_t eq_nl = __riscv_vmseq_vx_u8m1_b8(chunk, '\n', vl);
        vbool8_t eq_ff = __riscv_vmseq_vx_u8m1_b8(chunk, '\f', vl);
        vbool8_t eq_cr = __riscv_vmseq_vx_u8m1_b8(chunk, '\r', vl);
        vbool8_t is_ws = __riscv_vmor_mm_b8(eq_space, eq_tab, vl);
        is_ws = __riscv_vmor_mm_b8(is_ws, eq_nl, vl);
        is_ws = __riscv_vmor_mm_b8(is_ws, eq_ff, vl);
        is_ws = __riscv_vmor_mm_b8(is_ws, eq_cr, vl);

        vbool8_t not_ws = __riscv_vmnot_m_b8(is_ws, vl);
        long first_non_ws = __riscv_vfirst_m_b8(not_ws, vl);
        if (first_non_ws < 0) {
            i += vl;
        } else {
            i += first_non_ws;
            return i;
        }
    }
    return i;
}
#endif

static inline size_t wisp_skip_whitespaces_impl(const uint8_t *data, size_t len) {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    return wisp_skip_whitespaces_sse2(data, len);
#elif defined(__arm__) || defined(__aarch64__)
    if (wisp_ws_has_neon()) {
        return wisp_skip_whitespaces_neon(data, len);
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (wisp_ws_has_rvv()) {
        return wisp_skip_whitespaces_rvv(data, len);
    }
#endif

    size_t i = 0;
    while (i < len) {
        uint8_t c = data[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r') {
            i++;
        } else {
            break;
        }
    }
    return i;
}

#endif /* _WISP_UTILS_WHITESPACES_H_ */
