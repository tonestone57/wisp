/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include "wisp/utils/stream_chunk_decoder.h"
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

struct wisp_stream_riscv_hwprobe {
    int64_t key;
    uint64_t value;
};

static inline bool has_rvv(void) {
    struct wisp_stream_riscv_hwprobe request;
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

/* Scalar CRLF Scanner Fallback */
size_t wisp_scan_crlf_scalar(const uint8_t *data, size_t len) {
    if (len < 2 || !data) return (size_t)-1;
    for (size_t i = 0; i + 1 < len; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }
    return (size_t)-1;
}

/* SSE2 CRLF Fastpath */
#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)) && (defined(__GNUC__) || defined(__clang__))
__attribute__((target("sse2")))
size_t wisp_scan_crlf_sse2(const uint8_t *data, size_t len) {
    if (len < 2 || !data) return (size_t)-1;
    size_t i = 0;
    __m128i cr_v = _mm_set1_epi8('\r');
    while (i + 15 < len) {
        __m128i v = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i eq_cr = _mm_cmpeq_epi8(v, cr_v);
        int mask = _mm_movemask_epi8(eq_cr);
        while (mask != 0) {
            int pos = __builtin_ctz(mask);
            size_t idx = i + pos;
            if (idx + 1 < len) {
                if (data[idx + 1] == '\n') {
                    return idx;
                }
            }
            mask &= mask - 1; /* clear lowest set bit */
        }
        i += 16;
    }
    while (i + 1 < len) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
        i++;
    }
    return (size_t)-1;
}
#else
size_t wisp_scan_crlf_sse2(const uint8_t *data, size_t len) {
    return wisp_scan_crlf_scalar(data, len);
}
#endif

/* NEON CRLF Fastpath */
#if defined(__arm__) || defined(__aarch64__)
size_t wisp_scan_crlf_neon(const uint8_t *data, size_t len) {
    if (len < 2 || !data) return (size_t)-1;
    size_t i = 0;
    uint8x16_t cr_v = vdupq_n_u8('\r');
    while (i + 15 < len) {
        uint8x16_t v = vld1q_u8(data + i);
        uint8x16_t eq_cr = vceqq_u8(v, cr_v);
        uint8x8_t low = vget_low_u8(eq_cr);
        uint8x8_t high = vget_high_u8(eq_cr);
        uint8x8_t combined = vorr_u8(low, high);
        uint32x2_t repr = vreinterpret_u32_u8(combined);
        if (vget_lane_u32(repr, 0) != 0 || vget_lane_u32(repr, 1) != 0) {
            uint8_t temp[16];
            vst1q_u8(temp, eq_cr);
            for (int j = 0; j < 16; j++) {
                if (temp[j] != 0) {
                    size_t idx = i + j;
                    if (idx + 1 < len && data[idx + 1] == '\n') {
                        return idx;
                    }
                }
            }
        }
        i += 16;
    }
    while (i + 1 < len) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
        i++;
    }
    return (size_t)-1;
}
#else
size_t wisp_scan_crlf_neon(const uint8_t *data, size_t len) {
    return wisp_scan_crlf_scalar(data, len);
}
#endif

/* RISC-V Vector 1.0 CRLF Fastpath */
#if defined(__riscv) && defined(__riscv_vector)
size_t wisp_scan_crlf_rvv(const uint8_t *data, size_t len) {
    if (len < 2 || !data) return (size_t)-1;
    size_t i = 0;
    while (i + 1 < len) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1(data + i, vl);
        vbool8_t eq_cr = __riscv_vmseq_vx_u8m1_b8(chunk, '\r', vl);
        long pos = __riscv_vfirst_m_b8(eq_cr, vl);
        while (pos >= 0) {
            size_t idx = i + pos;
            if (idx + 1 < len && data[idx + 1] == '\n') {
                return idx;
            }
            pos = pos + 1;
            if ((size_t)pos >= vl) break;
            while ((size_t)pos < vl) {
                if (data[i + pos] == '\r') break;
                pos++;
            }
            if ((size_t)pos >= vl) break;
        }
        i += vl;
    }
    return (size_t)-1;
}
#else
size_t wisp_scan_crlf_rvv(const uint8_t *data, size_t len) {
    return wisp_scan_crlf_scalar(data, len);
}
#endif

/* Public SIMD Byte Scanner dispatch */
size_t wisp_scan_crlf_simd(const uint8_t *data, size_t len) {
#if (defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)) && (defined(__GNUC__) || defined(__clang__))
    if (has_sse2()) {
        return wisp_scan_crlf_sse2(data, len);
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        return wisp_scan_crlf_neon(data, len);
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        return wisp_scan_crlf_rvv(data, len);
    }
#endif

    return wisp_scan_crlf_scalar(data, len);
}

/* Helper hex parser */
static inline int hex_char_to_val(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

void wisp_stream_chunk_decoder_init(wisp_stream_chunk_decoder_t *decoder) {
    if (!decoder) return;
    decoder->state = WISP_CHUNK_STATE_SIZE;
    decoder->chunk_bytes_remaining = 0;
    decoder->hex_digits_count = 0;
    decoder->is_final_chunk = false;
}

int wisp_stream_chunk_decoder_decode(wisp_stream_chunk_decoder_t *decoder,
                                      const uint8_t *in_buf, size_t in_len, size_t *bytes_read,
                                      uint8_t *out_buf, size_t out_capacity, size_t *bytes_written) {
    if (!decoder || !bytes_read || !bytes_written) return -1;

    size_t r = 0;
    size_t w = 0;
    *bytes_read = 0;
    *bytes_written = 0;

    if (!in_buf || in_len == 0) {
        if (decoder->state == WISP_CHUNK_STATE_DONE) return 1;
        return 0;
    }

    while (r < in_len) {
        switch (decoder->state) {
        case WISP_CHUNK_STATE_SIZE: {
            uint8_t c = in_buf[r];

            if (c == ';') {
                if (decoder->hex_digits_count == 0) {
                    decoder->state = WISP_CHUNK_STATE_ERROR;
                    *bytes_read = r;
                    *bytes_written = w;
                    return -1;
                }
                decoder->state = WISP_CHUNK_STATE_EXT;
                r++;
                break;
            }

            if (c == '\r') {
                if (r + 1 < in_len) {
                    if (in_buf[r + 1] == '\n') {
                        if (decoder->hex_digits_count == 0) {
                            decoder->state = WISP_CHUNK_STATE_ERROR;
                            *bytes_read = r;
                            *bytes_written = w;
                            return -1;
                        }
                        r += 2;
                        if (decoder->chunk_bytes_remaining == 0) {
                            decoder->is_final_chunk = true;
                            decoder->state = WISP_CHUNK_STATE_TRAILER;
                        } else {
                            decoder->state = WISP_CHUNK_STATE_DATA;
                        }
                        break;
                    } else {
                        decoder->state = WISP_CHUNK_STATE_ERROR;
                        *bytes_read = r;
                        *bytes_written = w;
                        return -1;
                    }
                } else {
                    /* Wait for next byte to check for '\n' */
                    *bytes_read = r;
                    *bytes_written = w;
                    return 0;
                }
            }

            if (c == ' ' || c == '\t') {
                /* Allow whitespace before ';' or CRLF */
                r++;
                break;
            }

            int hv = hex_char_to_val(c);
            if (hv >= 0) {
                /* Guard overflow */
                if (decoder->chunk_bytes_remaining > (SIZE_MAX >> 4)) {
                    decoder->state = WISP_CHUNK_STATE_ERROR;
                    *bytes_read = r;
                    *bytes_written = w;
                    return -1;
                }
                decoder->chunk_bytes_remaining = (decoder->chunk_bytes_remaining << 4) | (size_t)hv;
                decoder->hex_digits_count++;
                r++;
            } else {
                decoder->state = WISP_CHUNK_STATE_ERROR;
                *bytes_read = r;
                *bytes_written = w;
                return -1;
            }
            break;
        }

        case WISP_CHUNK_STATE_EXT: {
            size_t crlf_off = wisp_scan_crlf_simd(in_buf + r, in_len - r);
            if (crlf_off != (size_t)-1) {
                r += crlf_off + 2;
                if (decoder->chunk_bytes_remaining == 0) {
                    decoder->is_final_chunk = true;
                    decoder->state = WISP_CHUNK_STATE_TRAILER;
                } else {
                    decoder->state = WISP_CHUNK_STATE_DATA;
                }
            } else {
                /* Skip extension characters until buffer end except potential '\r' */
                size_t rem = in_len - r;
                if (rem > 0 && in_buf[in_len - 1] == '\r') {
                    r = in_len - 1;
                } else {
                    r = in_len;
                }
                *bytes_read = r;
                *bytes_written = w;
                return 0;
            }
            break;
        }

        case WISP_CHUNK_STATE_DATA: {
            size_t avail_in = in_len - r;
            size_t avail_out = (out_buf && out_capacity > w) ? (out_capacity - w) : 0;
            size_t to_copy = decoder->chunk_bytes_remaining;

            if (to_copy > avail_in) to_copy = avail_in;
            if (out_buf && to_copy > avail_out) to_copy = avail_out;

            if (to_copy > 0 && out_buf) {
                memcpy(out_buf + w, in_buf + r, to_copy);
                r += to_copy;
                w += to_copy;
                decoder->chunk_bytes_remaining -= to_copy;
            } else if (to_copy > 0 && !out_buf) {
                /* Skip payload if out_buf is NULL */
                r += to_copy;
                decoder->chunk_bytes_remaining -= to_copy;
            }

            if (decoder->chunk_bytes_remaining == 0) {
                decoder->state = WISP_CHUNK_STATE_DATA_CRLF;
            }

            if (out_buf && w == out_capacity && decoder->chunk_bytes_remaining > 0) {
                *bytes_read = r;
                *bytes_written = w;
                return 0;
            }
            break;
        }

        case WISP_CHUNK_STATE_DATA_CRLF: {
            if (r + 1 < in_len) {
                if (in_buf[r] == '\r' && in_buf[r + 1] == '\n') {
                    r += 2;
                    decoder->hex_digits_count = 0;
                    decoder->chunk_bytes_remaining = 0;
                    decoder->state = WISP_CHUNK_STATE_SIZE;
                } else {
                    decoder->state = WISP_CHUNK_STATE_ERROR;
                    *bytes_read = r;
                    *bytes_written = w;
                    return -1;
                }
            } else if (r < in_len) {
                if (in_buf[r] != '\r') {
                    decoder->state = WISP_CHUNK_STATE_ERROR;
                    *bytes_read = r;
                    *bytes_written = w;
                    return -1;
                }
                /* Wait for '\n' in next call */
                *bytes_read = r;
                *bytes_written = w;
                return 0;
            }
            break;
        }

        case WISP_CHUNK_STATE_TRAILER: {
            if (r + 1 < in_len && in_buf[r] == '\r' && in_buf[r + 1] == '\n') {
                /* Empty line finishes trailers */
                r += 2;
                decoder->state = WISP_CHUNK_STATE_DONE;
                *bytes_read = r;
                *bytes_written = w;
                return 1;
            }

            size_t crlf_off = wisp_scan_crlf_simd(in_buf + r, in_len - r);
            if (crlf_off != (size_t)-1) {
                r += crlf_off + 2;
            } else {
                size_t rem = in_len - r;
                if (rem > 0 && in_buf[in_len - 1] == '\r') {
                    r = in_len - 1;
                } else {
                    r = in_len;
                }
                *bytes_read = r;
                *bytes_written = w;
                return 0;
            }
            break;
        }

        case WISP_CHUNK_STATE_DONE:
            *bytes_read = r;
            *bytes_written = w;
            return 1;

        case WISP_CHUNK_STATE_ERROR:
        default:
            *bytes_read = r;
            *bytes_written = w;
            return -1;
        }
    }

    *bytes_read = r;
    *bytes_written = w;
    return (decoder->state == WISP_CHUNK_STATE_DONE) ? 1 : 0;
}
