#ifndef QUICKJS_JSON_SIMD_H
#define QUICKJS_JSON_SIMD_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#elif defined(__riscv) && defined(__riscv_vector)
#include <riscv_vector.h>
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

struct wisp_riscv_hwprobe_json {
    int64_t key;
    uint64_t value;
};

static inline bool has_rvv(void) {
    struct wisp_riscv_hwprobe_json request;
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
static inline bool has_rvv(void) {
#ifdef __riscv_vector
    return true;
#else
    return false;
#endif
}
#endif
#endif

/* 1. Scalar Fallback Implementation */
static void wisp_json_preparse_scalar(const uint8_t *buf, size_t len, const uint8_t **offsets, size_t *p_count) {
    size_t count = *p_count;
    bool in_string = false;
    bool escaped = false;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = buf[i];
        bool char_inside_string = in_string;
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else if (c == '"') {
            in_string = true;
            escaped = false;
            char_inside_string = false;
        }
        if (!char_inside_string && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            offsets[count++] = buf + i;
        }
    }
    *p_count = count;
}

/* 2. AVX2 Implementation */
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
__attribute__((target("avx2")))
static void wisp_json_preparse_avx2(const uint8_t *buf, size_t len, const uint8_t **offsets, size_t *p_count) {
    size_t count = *p_count;
    size_t i = 0;
    bool in_string = false;
    bool escaped = false;

    if (len >= 32) {
        __m256i v_quote = _mm256_set1_epi8('"');
        __m256i v_backslash = _mm256_set1_epi8('\\');
        __m256i v_space = _mm256_set1_epi8(' ');
        __m256i v_tab = _mm256_set1_epi8('\t');
        __m256i v_cr = _mm256_set1_epi8('\r');
        __m256i v_lf = _mm256_set1_epi8('\n');

        for (; i + 31 < len; i += 32) {
            __m256i chunk = _mm256_loadu_si256((const __m256i *)(buf + i));
            __m256i cmp_quote = _mm256_cmpeq_epi8(chunk, v_quote);
            __m256i cmp_backslash = _mm256_cmpeq_epi8(chunk, v_backslash);
            __m256i cmp_any = _mm256_or_si256(cmp_quote, cmp_backslash);
            uint32_t mask_any = _mm256_movemask_epi8(cmp_any);

            if (mask_any == 0) {
                if (!in_string) {
                    __m256i cmp_space = _mm256_cmpeq_epi8(chunk, v_space);
                    __m256i cmp_tab = _mm256_cmpeq_epi8(chunk, v_tab);
                    __m256i cmp_cr = _mm256_cmpeq_epi8(chunk, v_cr);
                    __m256i cmp_lf = _mm256_cmpeq_epi8(chunk, v_lf);
                    __m256i ws = _mm256_or_si256(_mm256_or_si256(cmp_space, cmp_tab),
                                                 _mm256_or_si256(cmp_cr, cmp_lf));
                    uint32_t ws_mask = _mm256_movemask_epi8(ws);
                    uint32_t non_ws_mask = ~ws_mask;

                    while (non_ws_mask != 0) {
                        int idx = __builtin_ctz(non_ws_mask);
                        offsets[count++] = buf + i + idx;
                        non_ws_mask &= non_ws_mask - 1;
                    }
                }
            } else {
                /* Slow path: contains quotes or backslashes. Fallback to scalar for this chunk. */
                for (int j = 0; j < 32; j++) {
                    uint8_t c = buf[i + j];
                    bool char_inside_string = in_string;
                    if (in_string) {
                        if (escaped) {
                            escaped = false;
                        } else if (c == '\\') {
                            escaped = true;
                        } else if (c == '"') {
                            in_string = false;
                        }
                    } else if (c == '"') {
                        in_string = true;
                        escaped = false;
                        char_inside_string = false;
                    }
                    if (!char_inside_string && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                        offsets[count++] = buf + i + j;
                    }
                }
            }
        }
    }

    /* Process remaining elements */
    for (; i < len; i++) {
        uint8_t c = buf[i];
        bool char_inside_string = in_string;
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else if (c == '"') {
            in_string = true;
            escaped = false;
            char_inside_string = false;
        }
        if (!char_inside_string && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            offsets[count++] = buf + i;
        }
    }

    *p_count = count;
}
#endif

/* 3. NEON Implementation */
#if defined(__arm__) || defined(__aarch64__)
static inline uint16_t neon_movemask(uint8x16_t input) {
    uint16x8_t high_bits = vreinterpretq_u16_u8(vshrq_n_u8(input, 7));
    uint32x4_t paired1 = vreinterpretq_u32_u16(vsriq_n_u16(high_bits, high_bits, 7));
    uint64x2_t paired2 = vreinterpretq_u64_u32(vsriq_n_u32(paired1, paired1, 14));
    uint8x16_t paired3 = vreinterpretq_u8_u64(vsriq_n_u64(paired2, paired2, 28));
    return vgetq_lane_u8(paired3, 0) | ((uint16_t)vgetq_lane_u8(paired3, 8) << 8);
}

static void wisp_json_preparse_neon(const uint8_t *buf, size_t len, const uint8_t **offsets, size_t *p_count) {
    size_t count = *p_count;
    size_t i = 0;
    bool in_string = false;
    bool escaped = false;

    if (len >= 16) {
        uint8x16_t v_quote = vdupq_n_u8('"');
        uint8x16_t v_backslash = vdupq_n_u8('\\');
        uint8x16_t v_space = vdupq_n_u8(' ');
        uint8x16_t v_tab = vdupq_n_u8('\t');
        uint8x16_t v_cr = vdupq_n_u8('\r');
        uint8x16_t v_lf = vdupq_n_u8('\n');

        for (; i + 15 < len; i += 16) {
            uint8x16_t chunk = vld1q_u8((const uint8_t *)(buf + i));
            uint8x16_t cmp_quote = vceqq_u8(chunk, v_quote);
            uint8x16_t cmp_backslash = vceqq_u8(chunk, v_backslash);
            uint8x16_t cmp_any = vorrq_u8(cmp_quote, cmp_backslash);

#if defined(__aarch64__)
            bool any_set = vmaxvq_u8(cmp_any) != 0;
#else
            uint8x8_t low = vget_low_u8(cmp_any);
            uint8x8_t high = vget_high_u8(cmp_any);
            uint8x8_t combined = vorr_u8(low, high);
            uint32x2_t repr = vreinterpret_u32_u8(combined);
            bool any_set = (vget_lane_u32(repr, 0) != 0 || vget_lane_u32(repr, 1) != 0);
#endif

            if (!any_set) {
                if (!in_string) {
                    uint8x16_t cmp_space = vceqq_u8(chunk, v_space);
                    uint8x16_t cmp_tab = vceqq_u8(chunk, v_tab);
                    uint8x16_t cmp_cr = vceqq_u8(chunk, v_cr);
                    uint8x16_t cmp_lf = vceqq_u8(chunk, v_lf);
                    uint8x16_t ws = vorrq_u8(vorrq_u8(cmp_space, cmp_tab), vorrq_u8(cmp_cr, cmp_lf));
                    uint16_t ws_mask = neon_movemask(ws);
                    uint16_t non_ws_mask = ~ws_mask;

                    while (non_ws_mask != 0) {
                        int idx = __builtin_ctz(non_ws_mask);
                        if (idx < 16) {
                            offsets[count++] = buf + i + idx;
                        }
                        non_ws_mask &= non_ws_mask - 1;
                    }
                }
            } else {
                /* Slow path: contains quotes or backslashes. Fallback to scalar for this chunk. */
                for (int j = 0; j < 16; j++) {
                    uint8_t c = buf[i + j];
                    bool char_inside_string = in_string;
                    if (in_string) {
                        if (escaped) {
                            escaped = false;
                        } else if (c == '\\') {
                            escaped = true;
                        } else if (c == '"') {
                            in_string = false;
                        }
                    } else if (c == '"') {
                        in_string = true;
                        escaped = false;
                        char_inside_string = false;
                    }
                    if (!char_inside_string && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                        offsets[count++] = buf + i + j;
                    }
                }
            }
        }
    }

    /* Process remaining elements */
    for (; i < len; i++) {
        uint8_t c = buf[i];
        bool char_inside_string = in_string;
        if (in_string) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
        } else if (c == '"') {
            in_string = true;
            escaped = false;
            char_inside_string = false;
        }
        if (!char_inside_string && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            offsets[count++] = buf + i;
        }
    }

    *p_count = count;
}
#endif

/* 4. RVV Implementation */
#if defined(__riscv) && defined(__riscv_vector)
static void wisp_json_preparse_rvv(const uint8_t *buf, size_t len, const uint8_t **offsets, size_t *p_count) {
    size_t count = *p_count;
    size_t i = 0;
    bool in_string = false;
    bool escaped = false;

    while (i < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1(buf + i, vl);

        vbool8_t cmp_quote = __riscv_vmseq_vx_u8m1_b8(chunk, '"', vl);
        vbool8_t cmp_backslash = __riscv_vmseq_vx_u8m1_b8(chunk, '\\', vl);
        vbool8_t cmp_any = __riscv_vmor_mm_b8(cmp_quote, cmp_backslash, vl);

        long any_idx = __riscv_vfirst_m_b8(cmp_any, vl);
        if (any_idx < 0) {
            /* Fast path: No quotes or backslashes in this vector chunk */
            if (!in_string) {
                for (size_t j = 0; j < vl; j++) {
                    uint8_t c = buf[i + j];
                    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                        offsets[count++] = buf + i + j;
                    }
                }
            }
            i += vl;
        } else {
            /* Slow path: contains quotes or backslashes. Fallback to scalar for this chunk. */
            for (size_t j = 0; j < vl; j++) {
                uint8_t c = buf[i + j];
                bool char_inside_string = in_string;
                if (in_string) {
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_string = false;
                    }
                } else if (c == '"') {
                    in_string = true;
                    escaped = false;
                    char_inside_string = false;
                }
                if (!char_inside_string && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                    offsets[count++] = buf + i + j;
                }
            }
            i += vl;
        }
    }
    *p_count = count;
}
#endif

/* Forward declarations of QuickJS-ng allocation APIs so they're visible if included early */
void *js_malloc(JSContext *ctx, size_t size);
void js_free(JSContext *ctx, void *ptr);

/* Global Dispatcher */
static inline const uint8_t **wisp_json_preparse(JSContext *ctx, const char *buf, size_t len, size_t *p_count) {
    if (len == 0) {
        *p_count = 0;
        return NULL;
    }
    const uint8_t **offsets = (const uint8_t **)js_malloc(ctx, sizeof(const uint8_t *) * (len + 1));
    if (!offsets) {
        *p_count = 0;
        return NULL;
    }
    size_t count = 0;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_avx2()) {
        wisp_json_preparse_avx2((const uint8_t *)buf, len, offsets, &count);
    } else {
        wisp_json_preparse_scalar((const uint8_t *)buf, len, offsets, &count);
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        wisp_json_preparse_neon((const uint8_t *)buf, len, offsets, &count);
    } else {
        wisp_json_preparse_scalar((const uint8_t *)buf, len, offsets, &count);
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        wisp_json_preparse_rvv((const uint8_t *)buf, len, offsets, &count);
    } else {
        wisp_json_preparse_scalar((const uint8_t *)buf, len, offsets, &count);
    }
#else
    wisp_json_preparse_scalar((const uint8_t *)buf, len, offsets, &count);
#endif

    *p_count = count;
    return offsets;
}

#endif /* QUICKJS_JSON_SIMD_H */
