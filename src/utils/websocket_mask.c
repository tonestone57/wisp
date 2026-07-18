/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include "wisp/utils/websocket_mask.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#elif defined(__riscv) && defined(__riscv_vector)
#include <riscv_vector.h>
#endif

/* Dynamic CPU Feature Detection */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
static inline bool has_sse2(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return true;
#elif defined(__GNUC__) || defined(__clang__)
    return __builtin_cpu_supports("sse2");
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

struct wisp_riscv_hwprobe {
    int64_t key;
    uint64_t value;
};

static inline bool has_rvv(void) {
    struct wisp_riscv_hwprobe request;
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
#else /* non-Linux RISC-V */
static inline bool has_rvv(void) {
#ifdef __riscv_vector
    return true;
#else
    return false;
#endif
}
#endif
#endif

/* SSE2 Optimization */
#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)) && (defined(__GNUC__) || defined(__clang__))
__attribute__((target("sse2")))
static void wisp_websocket_mask_sse2(uint8_t *data, size_t len, const uint8_t mask_key[4], size_t key_offset) {
    size_t i = 0;
    if (len >= 16) {
        uint8_t pattern[16];
        size_t start_offset = key_offset % 4;
        for (size_t j = 0; j < 16; j++) {
            pattern[j] = mask_key[(j + start_offset) % 4];
        }
        __m128i v_mask = _mm_loadu_si128((const __m128i *)pattern);
        for (; i + 15 < len; i += 16) {
            __m128i v_data = _mm_loadu_si128((const __m128i *)(data + i));
            __m128i v_res = _mm_xor_si128(v_data, v_mask);
            _mm_storeu_si128((__m128i *)(data + i), v_res);
        }
    }
    for (; i < len; i++) {
        data[i] ^= mask_key[(i + key_offset) % 4];
    }
}
#endif

/* NEON Optimization */
#if defined(__arm__) || defined(__aarch64__)
static void wisp_websocket_mask_neon(uint8_t *data, size_t len, const uint8_t mask_key[4], size_t key_offset) {
    size_t i = 0;
    if (len >= 16) {
        uint8_t pattern[16];
        size_t start_offset = key_offset % 4;
        for (size_t j = 0; j < 16; j++) {
            pattern[j] = mask_key[(j + start_offset) % 4];
        }
        uint8x16_t v_mask = vld1q_u8(pattern);
        for (; i + 15 < len; i += 16) {
            uint8x16_t v_data = vld1q_u8(data + i);
            uint8x16_t v_res = veorq_u8(v_data, v_mask);
            vst1q_u8(data + i, v_res);
        }
    }
    for (; i < len; i++) {
        data[i] ^= mask_key[(i + key_offset) % 4];
    }
}
#endif

/* RISC-V Vector 1.0 Optimization */
#if defined(__riscv) && defined(__riscv_vector)
static void wisp_websocket_mask_rvv(uint8_t *data, size_t len, const uint8_t mask_key[4], size_t key_offset) {
    /* Define safe bounds for VLEN length patterns */
    uint8_t key_pattern[512];
    for (size_t j = 0; j < 512; j++) {
        key_pattern[j] = mask_key[j % 4];
    }

    size_t i = 0;
    while (i < len) {
        /* Cap dynamic VL length to ensure memory safety on massive VLEN implementations */
        size_t remaining = len - i;
        if (remaining > 256) {
            remaining = 256;
        }
        size_t vl = __riscv_vsetvl_e8m1(remaining);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1((const uint8_t *)(data + i), vl);
        size_t pattern_offset = (key_offset + i) % 4;
        vuint8m1_t v_mask = __riscv_vle8_v_u8m1(key_pattern + pattern_offset, vl);
        vuint8m1_t v_res = __riscv_vxor_vv_u8m1(chunk, v_mask, vl);
        __riscv_vse8_v_u8m1((uint8_t *)(data + i), v_res, vl);
        i += vl;
    }
}
#endif

/* Public API */
void wisp_websocket_mask(uint8_t *data, size_t len, const uint8_t mask_key[4], size_t key_offset) {
#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)) && (defined(__GNUC__) || defined(__clang__))
    if (has_sse2()) {
        wisp_websocket_mask_sse2(data, len, mask_key, key_offset);
        return;
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        wisp_websocket_mask_neon(data, len, mask_key, key_offset);
        return;
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        wisp_websocket_mask_rvv(data, len, mask_key, key_offset);
        return;
    }
#endif

    /* Scalar fallback */
    for (size_t i = 0; i < len; i++) {
        data[i] ^= mask_key[(i + key_offset) % 4];
    }
}
