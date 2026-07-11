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
#elif defined(__riscv) && defined(__riscv_vector)
#include <riscv_vector.h>
#endif

#if defined(__clang__) || defined(__GNUC__)
#define WISP_NO_SANITIZE __attribute__((no_sanitize("address", "thread")))
#elif defined(_MSC_VER)
#define WISP_NO_SANITIZE __declspec(no_sanitize_address)
#else
#define WISP_NO_SANITIZE
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

#if defined(__riscv)
#ifdef __linux__
#include <sys/auxv.h>
#include <sys/syscall.h>
#include <unistd.h>

/* Define constants in case they are missing in older headers */
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
    /* Try riscv_hwprobe first (modern Linux standard) */
    struct wisp_riscv_hwprobe request;
    request.key = RISCV_HWPROBE_KEY_IMA_EXT_0;
    request.value = 0;
    if (syscall(__NR_riscv_hwprobe, &request, 1, 0, NULL, 0) == 0) {
        if (request.value & RISCV_HWPROBE_IMA_V) {
            return true;
        }
    }

    /* Fallback to AT_HWCAP */
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

#if defined(__riscv) && defined(__riscv_vector)
static bool wisp_is_ascii_rvv(const char *str, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1((const uint8_t *)(str + i), vl);
        vbool8_t is_nonascii = __riscv_vmsgeu_vx_u8m1_b8(chunk, 0x80, vl);
        if (__riscv_vfirst_m_b8(is_nonascii, vl) >= 0) {
            return false;
        }
        i += vl;
    }
    return true;
}
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("avx2")))
#endif
WISP_NO_SANITIZE
static int wisp_simd_strcmp_avx2(const char *s1, const char *s2) {
    size_t offset = 0;
    while (1) {
        bool s1_safe = (((uintptr_t)(s1 + offset)) & 4095) <= (4096 - 32);
        bool s2_safe = (((uintptr_t)(s2 + offset)) & 4095) <= (4096 - 32);
        if (s1_safe && s2_safe) {
            __m256i v1 = _mm256_loadu_si256((const __m256i *)(s1 + offset));
            __m256i v2 = _mm256_loadu_si256((const __m256i *)(s2 + offset));
            __m256i cmp = _mm256_cmpeq_epi8(v1, v2);
            int eq_mask = _mm256_movemask_epi8(cmp);

            __m256i zero = _mm256_setzero_si256();
            __m256i nulls = _mm256_cmpeq_epi8(v1, zero);
            int null_mask = _mm256_movemask_epi8(nulls);

            if (eq_mask != -1) {
                int mismatch_idx = __builtin_ctz(~eq_mask);
                if (null_mask != 0) {
                    int null_idx = __builtin_ctz(null_mask);
                    if (mismatch_idx > null_idx) {
                        return 0;
                    }
                }
                unsigned char c1 = (unsigned char)s1[offset + mismatch_idx];
                unsigned char c2 = (unsigned char)s2[offset + mismatch_idx];
                return (int)c1 - (int)c2;
            }

            if (null_mask != 0) {
                return 0;
            }

            offset += 32;
        } else {
            while (1) {
                char c1 = s1[offset];
                char c2 = s2[offset];
                if (c1 != c2) {
                    return (int)(unsigned char)c1 - (int)(unsigned char)c2;
                }
                if (c1 == '\0') return 0;
                offset++;
                if ((((uintptr_t)(s1 + offset)) & 4095) <= (4096 - 32) &&
                    (((uintptr_t)(s2 + offset)) & 4095) <= (4096 - 32)) {
                    break;
                }
            }
        }
    }
}
#endif

#if defined(__arm__) || defined(__aarch64__)
WISP_NO_SANITIZE
static int wisp_simd_strcmp_neon(const char *s1, const char *s2) {
    size_t offset = 0;
    while (1) {
        bool s1_safe = (((uintptr_t)(s1 + offset)) & 4095) <= (4096 - 16);
        bool s2_safe = (((uintptr_t)(s2 + offset)) & 4095) <= (4096 - 16);
        if (s1_safe && s2_safe) {
            uint8x16_t v1 = vld1q_u8((const uint8_t *)(s1 + offset));
            uint8x16_t v2 = vld1q_u8((const uint8_t *)(s2 + offset));

            uint8x16_t diff = veorq_u8(v1, v2);
            uint64x2_t diff64 = vreinterpretq_u64_u8(diff);
            uint64_t d1 = vgetq_lane_u64(diff64, 0);
            uint64_t d2 = vgetq_lane_u64(diff64, 1);

            uint8x16_t zero = vdupq_n_u8(0);
            uint8x16_t nulls = vceqq_u8(v1, zero);
            uint64x2_t nulls64 = vreinterpretq_u64_u8(nulls);
            uint64_t n1 = vgetq_lane_u64(nulls64, 0);
            uint64_t n2 = vgetq_lane_u64(nulls64, 1);

            if ((d1 | d2) == 0 && (n1 | n2) == 0) {
                offset += 16;
                continue;
            }

            for (int i = 0; i < 16; i++) {
                char c1 = s1[offset + i];
                char c2 = s2[offset + i];
                if (c1 != c2) {
                    return (int)(unsigned char)c1 - (int)(unsigned char)c2;
                }
                if (c1 == '\0') return 0;
            }
            offset += 16;
        } else {
            while (1) {
                char c1 = s1[offset];
                char c2 = s2[offset];
                if (c1 != c2) {
                    return (int)(unsigned char)c1 - (int)(unsigned char)c2;
                }
                if (c1 == '\0') return 0;
                offset++;
                if ((((uintptr_t)(s1 + offset)) & 4095) <= (4096 - 16) &&
                    (((uintptr_t)(s2 + offset)) & 4095) <= (4096 - 16)) {
                    break;
                }
            }
        }
    }
}
#endif

#if defined(__riscv) && defined(__riscv_vector)
WISP_NO_SANITIZE
static int wisp_simd_strcmp_rvv(const char *s1, const char *s2) {
    size_t offset = 0;
    while (1) {
        bool s1_safe = (((uintptr_t)(s1 + offset)) & 4095) <= (4096 - 64);
        bool s2_safe = (((uintptr_t)(s2 + offset)) & 4095) <= (4096 - 64);
        if (s1_safe && s2_safe) {
            size_t vl = __riscv_vsetvl_e8m1(64);
            vuint8m1_t v1 = __riscv_vle8_v_u8m1((const uint8_t *)(s1 + offset), vl);
            vuint8m1_t v2 = __riscv_vle8_v_u8m1((const uint8_t *)(s2 + offset), vl);

            vbool8_t ne_mask = __riscv_vmsne_vv_u8m1_b8(v1, v2, vl);
            long first_ne = __riscv_vfirst_m_b8(ne_mask, vl);

            vbool8_t null_mask = __riscv_vmseq_vx_u8m1_b8(v1, 0, vl);
            long first_null = __riscv_vfirst_m_b8(null_mask, vl);

            if (first_ne < 0 && first_null < 0) {
                offset += vl;
                continue;
            }

            for (size_t i = 0; i < vl; i++) {
                char c1 = s1[offset + i];
                char c2 = s2[offset + i];
                if (c1 != c2) {
                    return (int)(unsigned char)c1 - (int)(unsigned char)c2;
                }
                if (c1 == '\0') return 0;
            }
            offset += vl;
        } else {
            while (1) {
                char c1 = s1[offset];
                char c2 = s2[offset];
                if (c1 != c2) {
                    return (int)(unsigned char)c1 - (int)(unsigned char)c2;
                }
                if (c1 == '\0') return 0;
                offset++;
                if ((((uintptr_t)(s1 + offset)) & 4095) <= (4096 - 64) &&
                    (((uintptr_t)(s2 + offset)) & 4095) <= (4096 - 64)) {
                    break;
                }
            }
        }
    }
}
#endif

int wisp_simd_strcmp(const char *s1, const char *s2) {
    if (!s1 || !s2) {
        if (s1 == s2) return 0;
        return s1 ? 1 : -1;
    }

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_avx2()) {
        return wisp_simd_strcmp_avx2(s1, s2);
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        return wisp_simd_strcmp_neon(s1, s2);
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        return wisp_simd_strcmp_rvv(s1, s2);
    }
#endif
    return strcmp(s1, s2);
}

bool wisp_simd_streq(const char *s1, const char *s2) {
    return wisp_simd_strcmp(s1, s2) == 0;
}

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
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        return wisp_is_ascii_rvv(str, len);
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

#if defined(__riscv) && defined(__riscv_vector)
static void wisp_ascii_tolower_rvv(const char *src, char *dst, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1((const uint8_t *)(src + i), vl);
        vbool8_t ge_A = __riscv_vmsgeu_vx_u8m1_b8(chunk, 'A', vl);
        vbool8_t le_Z = __riscv_vmsleu_vx_u8m1_b8(chunk, 'Z', vl);
        vbool8_t is_upper = __riscv_vmand_mm_b8(ge_A, le_Z, vl);
        vuint8m1_t lower = __riscv_vadd_vx_u8m1_m(is_upper, chunk, chunk, 32, vl);
        __riscv_vse8_v_u8m1((uint8_t *)(dst + i), lower, vl);
        i += vl;
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
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        wisp_ascii_tolower_rvv(src, dst, len);
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

#if defined(__riscv) && defined(__riscv_vector)
static void wisp_ascii_toupper_rvv(const char *src, char *dst, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1((const uint8_t *)(src + i), vl);
        vbool8_t ge_a = __riscv_vmsgeu_vx_u8m1_b8(chunk, 'a', vl);
        vbool8_t le_z = __riscv_vmsleu_vx_u8m1_b8(chunk, 'z', vl);
        vbool8_t is_lower = __riscv_vmand_mm_b8(ge_a, le_z, vl);
        vuint8m1_t upper = __riscv_vsub_vx_u8m1_m(is_lower, chunk, chunk, 32, vl);
        __riscv_vse8_v_u8m1((uint8_t *)(dst + i), upper, vl);
        i += vl;
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
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        wisp_ascii_toupper_rvv(src, dst, len);
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

#if defined(__riscv) && defined(__riscv_vector)
static void wisp_ascii_to_utf32_rvv(const char *src, int32_t *dst, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vint8m1_t chunk = __riscv_vle8_v_i8m1((const int8_t *)(src + i), vl);
        vint32m4_t extended = __riscv_vsext_vf4_i32m4(chunk, vl);
        __riscv_vse32_v_i32m4(dst + i, extended, vl);
        i += vl;
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
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        wisp_ascii_to_utf32_rvv(src, dst, len);
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

#if defined(__riscv) && defined(__riscv_vector)
static void wisp_utf32_to_ascii_rvv(const int32_t *src, char *dst, size_t len) {
    size_t i = 0;
    while (i < len) {
        size_t vl = __riscv_vsetvl_e32m4(len - i);
        vint32m4_t chunk = __riscv_vle32_v_i32m4(src + i, vl);
        vuint32m4_t u_chunk = __riscv_vreinterpret_v_i32m4_u32m4(chunk);
        vuint16m2_t chunk_16 = __riscv_vnsrl_wx_u16m2(u_chunk, 0, vl);
        vuint8m1_t chunk_8 = __riscv_vnsrl_wx_u8m1(chunk_16, 0, vl);
        __riscv_vse8_v_u8m1((uint8_t *)(dst + i), chunk_8, vl);
        i += vl;
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
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        wisp_utf32_to_ascii_rvv(src, dst, len);
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
    if (length == 0) return 0;
    if (buffer == NULL) return UTF8PROC_ERROR_INVALIDOPTS;

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
    if (length == 0) return 0;
    if (buffer == NULL) return UTF8PROC_ERROR_INVALIDOPTS;

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
