/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include "wisp/utils/stream_simd.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#if !defined(WISP_NO_SIMD)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_neon.h>
#elif defined(__riscv) && defined(__riscv_vector)
#include <riscv_vector.h>
#endif
#endif

#if defined(__clang__) || defined(__GNUC__)
#define WISP_NO_SANITIZE __attribute__((no_sanitize("address", "thread")))
#elif defined(_MSC_VER)
#define WISP_NO_SANITIZE __declspec(no_sanitize_address)
#else
#define WISP_NO_SANITIZE
#endif

/* Dynamic CPU Feature Detection */
#if !defined(WISP_NO_SIMD)
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
#endif /* !defined(WISP_NO_SIMD) */

/* -------------------------------------------------------------------------
 * 1. wisp_simd_find_crlf
 * ------------------------------------------------------------------------- */

static size_t wisp_simd_find_crlf_scalar(const uint8_t *data, size_t len) {
    if (!data || len < 2) return (size_t)-1;
    for (size_t i = 0; i < len - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }
    return (size_t)-1;
}

#if !defined(WISP_NO_SIMD)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("sse2")))
#endif
WISP_NO_SANITIZE
static size_t wisp_simd_find_crlf_sse2(const uint8_t *data, size_t len) {
    if (!data || len < 2) return (size_t)-1;
    size_t i = 0;
    __m128i target_r = _mm_set1_epi8('\r');

    /* Strict buffer bounds protection: only use 16-byte vector loads when remaining length >= 16 */
    if (len >= 16) {
        for (; i <= len - 16; i += 16) {
            __m128i v = _mm_loadu_si128((const __m128i *)(data + i));
            __m128i cmp = _mm_cmpeq_epi8(v, target_r);
            int mask = _mm_movemask_epi8(cmp);

            while (mask != 0) {
                int idx = __builtin_ctz(mask);
                size_t cr_idx = i + idx;
                if (cr_idx + 1 < len) {
                    if (data[cr_idx + 1] == '\n') {
                        return cr_idx;
                    }
                }
                mask &= mask - 1; /* Clear LSB */
            }
        }
    }

    for (; i < len - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }
    return (size_t)-1;
}
#endif

#if defined(__arm__) || defined(__aarch64__)
WISP_NO_SANITIZE
static size_t wisp_simd_find_crlf_neon(const uint8_t *data, size_t len) {
    if (!data || len < 2) return (size_t)-1;
    size_t i = 0;
    uint8x16_t target_r = vdupq_n_u8('\r');

    if (len >= 16) {
        for (; i <= len - 16; i += 16) {
            uint8x16_t v = vld1q_u8(data + i);
            uint8x16_t cmp = vceqq_u8(v, target_r);

            uint64x2_t cmp64 = vreinterpretq_u64_u8(cmp);
            uint64_t d1 = vgetq_lane_u64(cmp64, 0);
            uint64_t d2 = vgetq_lane_u64(cmp64, 1);

            if ((d1 | d2) == 0) continue;

            for (int k = 0; k < 16; k++) {
                size_t cr_idx = i + k;
                if (data[cr_idx] == '\r' && cr_idx + 1 < len && data[cr_idx + 1] == '\n') {
                    return cr_idx;
                }
            }
        }
    }

    for (; i < len - 1; i++) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return i;
        }
    }
    return (size_t)-1;
}
#endif

#if defined(__riscv) && defined(__riscv_vector)
WISP_NO_SANITIZE
static size_t wisp_simd_find_crlf_rvv(const uint8_t *data, size_t len) {
    if (!data || len < 2) return (size_t)-1;
    size_t i = 0;
    while (i < len - 1) {
        size_t vl = __riscv_vsetvl_e8m1(len - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1(data + i, vl);
        vbool8_t eq_r = __riscv_vmseq_vx_u8m1_b8(chunk, '\r', vl);

        long first_r = __riscv_vfirst_m_b8(eq_r, vl);
        bool match_found = false;
        while (first_r >= 0) {
            match_found = true;
            size_t cr_idx = i + (size_t)first_r;
            if (cr_idx + 1 < len && data[cr_idx + 1] == '\n') {
                return cr_idx;
            }
            if ((size_t)first_r + 1 >= vl) break;
            i += (size_t)first_r + 1;
            vl = __riscv_vsetvl_e8m1(len - i);
            chunk = __riscv_vle8_v_u8m1(data + i, vl);
            eq_r = __riscv_vmseq_vx_u8m1_b8(chunk, '\r', vl);
            first_r = __riscv_vfirst_m_b8(eq_r, vl);
        }
        if (!match_found) {
            i += vl;
        }
    }
    return (size_t)-1;
}
#endif
#endif /* !defined(WISP_NO_SIMD) */

size_t wisp_simd_find_crlf(const uint8_t *data, size_t len) {
#if !defined(WISP_NO_SIMD)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_sse2()) {
        return wisp_simd_find_crlf_sse2(data, len);
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        return wisp_simd_find_crlf_neon(data, len);
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        return wisp_simd_find_crlf_rvv(data, len);
    }
#endif
#endif /* !defined(WISP_NO_SIMD) */
    return wisp_simd_find_crlf_scalar(data, len);
}

/* -------------------------------------------------------------------------
 * 2. wisp_simd_parse_chunk_header & wisp_simd_decode_chunked_stream
 * ------------------------------------------------------------------------- */

static inline int hex_val(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool wisp_simd_parse_chunk_header(const uint8_t *data, size_t len, size_t *chunk_size, size_t *header_len) {
    if (!data || !chunk_size || !header_len) return false;

    size_t crlf_off = wisp_simd_find_crlf(data, len);
    if (crlf_off == (size_t)-1) {
        return false; /* Incomplete header */
    }

    size_t parsed_size = 0;
    size_t i = 0;
    bool digit_found = false;

    for (; i < crlf_off; i++) {
        uint8_t c = data[i];
        if (c == ';' || c == ' ' || c == '\t') {
            break; /* Beginning of chunk extension or trailing spaces */
        }
        int v = hex_val(c);
        if (v < 0) {
            return false; /* Invalid hex character */
        }
        if (parsed_size > (SIZE_MAX - 15) / 16) {
            return false; /* Overflow safeguard */
        }
        parsed_size = (parsed_size * 16) + v;
        digit_found = true;
    }

    if (!digit_found) {
        return false;
    }

    *chunk_size = parsed_size;
    *header_len = crlf_off + 2; /* Include CRLF */
    return true;
}

wisp_chunk_decode_result wisp_simd_decode_chunked_stream(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_capacity) {
    wisp_chunk_decode_result res = {0};
    if (!in || !out) {
        res.is_invalid = true;
        return res;
    }

    size_t in_pos = 0;
    size_t out_pos = 0;

    while (in_pos < in_len) {
        size_t csize = 0;
        size_t hlen = 0;
        if (!wisp_simd_parse_chunk_header(in + in_pos, in_len - in_pos, &csize, &hlen)) {
            size_t crlf = wisp_simd_find_crlf(in + in_pos, in_len - in_pos);
            if (crlf == (size_t)-1) {
                res.is_incomplete = true;
            } else {
                res.is_invalid = true;
            }
            break;
        }

        if (csize == 0) {
            /* Final 0-length chunk */
            in_pos += hlen;
            /* Check if trailer CRLF is present */
            if (in_pos + 2 <= in_len && in[in_pos] == '\r' && in[in_pos + 1] == '\n') {
                in_pos += 2;
            }
            res.is_final_chunk = true;
            break;
        }

        /* Check if full chunk payload + trailing CRLF is available in stream */
        if (in_pos + hlen + csize + 2 > in_len) {
            res.is_incomplete = true;
            break;
        }

        if (out_pos + csize > out_capacity) {
            res.is_invalid = true;
            break;
        }

        /* Copy payload data */
        const uint8_t *payload_src = in + in_pos + hlen;
        uint8_t *payload_dst = out + out_pos;

        if (payload_dst != payload_src) {
            memmove(payload_dst, payload_src, csize);
        }

        /* Verify trailing CRLF after chunk payload */
        size_t payload_end = in_pos + hlen + csize;
        if (in[payload_end] != '\r' || in[payload_end + 1] != '\n') {
            res.is_invalid = true;
            break;
        }

        in_pos += hlen + csize + 2;
        out_pos += csize;
    }

    res.consumed_bytes = in_pos;
    res.decoded_bytes = out_pos;
    return res;
}

/* -------------------------------------------------------------------------
 * 3. wisp_simd_validate_http_header & wisp_simd_validate_http_header_block
 * ------------------------------------------------------------------------- */

static bool wisp_simd_validate_http_header_scalar(const char *header, size_t len) {
    if (!header || len == 0) return false;

    /* Reject obsolete line folding (obs-fold: CRLF or LF/CR starting with space or tab) */
    if (header[0] == ' ' || header[0] == '\t') {
        return false;
    }

    /* Ignore trailing \r\n or \n for validation */
    size_t end = len;
    while (end > 0 && (header[end - 1] == '\r' || header[end - 1] == '\n')) {
        end--;
    }
    if (end == 0) return true; /* Empty CRLF line */

    bool is_status_line = (end >= 5 && strncasecmp(header, "HTTP/", 5) == 0);
    bool found_colon = false;

    for (size_t i = 0; i < end; i++) {
        unsigned char c = (unsigned char)header[i];

        /* RFC 7230: Rejection of bare CR or bare LF inside header value */
        if (c == '\r') {
            if (i + 1 >= end || header[i + 1] != '\n') return false;
        } else if (c == '\n') {
            if (i == 0 || header[i - 1] != '\r') return false;
        }

        /* RFC 7230: Control characters (0x00-0x1F except HT 0x09, and 0x7F) are invalid */
        if ((c < 0x20 && c != 0x09) || c == 0x7F) {
            return false;
        }

        if (!is_status_line && !found_colon) {
            if (c == ':') {
                if (i == 0) return false; /* Empty header field name */
                found_colon = true;
            } else if (c == ' ' || c == '\t') {
                return false; /* RFC 7230: Space before colon in header name is forbidden */
            }
        }
    }

    return is_status_line || found_colon;
}

#if !defined(WISP_NO_SIMD)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("sse2")))
#endif
WISP_NO_SANITIZE
static bool wisp_simd_validate_http_header_sse2(const char *header, size_t len) {
    if (!header || len == 0) return false;
    if (header[0] == ' ' || header[0] == '\t') return false; /* Rejection of obs-fold */

    size_t end = len;
    while (end > 0 && (header[end - 1] == '\r' || header[end - 1] == '\n')) {
        end--;
    }
    if (end == 0) return true;

    bool is_status_line = (end >= 5 && strncasecmp(header, "HTTP/", 5) == 0);
    size_t i = 0;
    bool found_colon = false;
    __m128i space = _mm_set1_epi8(' ');
    __m128i tab = _mm_set1_epi8('\t');
    __m128i colon = _mm_set1_epi8(':');
    __m128i del = _mm_set1_epi8(0x7F);

    /* Strict buffer bounds protection: only use 16-byte vector loads when remaining length >= 16 */
    if (end >= 16) {
        for (; i <= end - 16; i += 16) {
            __m128i v = _mm_loadu_si128((const __m128i *)(header + i));

            /* Check for control chars < 0x20 using unsigned comparison via unsigned minimum */
            __m128i max_ctrl = _mm_set1_epi8(0x1F);
            __m128i is_lt_space = _mm_cmpeq_epi8(_mm_min_epu8(v, max_ctrl), v);
            __m128i is_tab = _mm_cmpeq_epi8(v, tab);
            __m128i invalid_ctrl = _mm_andnot_si128(is_tab, is_lt_space);
            __m128i is_del = _mm_cmpeq_epi8(v, del);
            __m128i invalid = _mm_or_si128(invalid_ctrl, is_del);

            if (_mm_movemask_epi8(invalid) != 0) {
                return false;
            }

            if (!is_status_line && !found_colon) {
                __m128i has_colon = _mm_cmpeq_epi8(v, colon);
                int colon_mask = _mm_movemask_epi8(has_colon);
                if (colon_mask != 0) {
                    int colon_idx = __builtin_ctz(colon_mask);
                    /* Validate characters before colon */
                    for (size_t k = i; k < i + colon_idx; k++) {
                        char c = header[k];
                        if (c == ' ' || c == '\t') return false;
                    }
                    if (i + colon_idx == 0) return false;
                    found_colon = true;
                } else {
                    /* If colon not yet found, check if space or tab appears */
                    __m128i has_space = _mm_cmpeq_epi8(v, space);
                    __m128i has_tab = _mm_cmpeq_epi8(v, tab);
                    __m128i space_or_tab = _mm_or_si128(has_space, has_tab);
                    if (_mm_movemask_epi8(space_or_tab) != 0) {
                        return false;
                    }
                }
            }
        }
    }

    for (; i < end; i++) {
        unsigned char c = (unsigned char)header[i];
        if (c == '\r') {
            if (i + 1 >= end || header[i + 1] != '\n') return false;
        } else if (c == '\n') {
            if (i == 0 || header[i - 1] != '\r') return false;
        }
        if ((c < 0x20 && c != 0x09) || c == 0x7F) return false;
        if (!is_status_line && !found_colon) {
            if (c == ':') {
                if (i == 0) return false;
                found_colon = true;
            } else if (c == ' ' || c == '\t') {
                return false;
            }
        }
    }

    return is_status_line || found_colon;
}
#endif

#if defined(__arm__) || defined(__aarch64__)
WISP_NO_SANITIZE
static bool wisp_simd_validate_http_header_neon(const char *header, size_t len) {
    if (!header || len == 0) return false;
    if (header[0] == ' ' || header[0] == '\t') return false;

    size_t end = len;
    while (end > 0 && (header[end - 1] == '\r' || header[end - 1] == '\n')) {
        end--;
    }
    if (end == 0) return true;

    bool is_status_line = (end >= 5 && strncasecmp(header, "HTTP/", 5) == 0);
    size_t i = 0;
    bool found_colon = false;
    uint8x16_t space = vdupq_n_u8(' ');
    uint8x16_t tab = vdupq_n_u8('\t');
    uint8x16_t colon = vdupq_n_u8(':');
    uint8x16_t del = vdupq_n_u8(0x7F);

    if (end >= 16) {
        for (; i <= end - 16; i += 16) {
            uint8x16_t v = vld1q_u8((const uint8_t *)(header + i));

            uint8x16_t is_lt_space = vcltq_u8(v, space);
            uint8x16_t is_tab = vceqq_u8(v, tab);
            uint8x16_t invalid_ctrl = vbicq_u8(is_lt_space, is_tab);
            uint8x16_t is_del = vceqq_u8(v, del);
            uint8x16_t invalid = vorrq_u8(invalid_ctrl, is_del);

            uint64x2_t inv64 = vreinterpretq_u64_u8(invalid);
            if ((vgetq_lane_u64(inv64, 0) | vgetq_lane_u64(inv64, 1)) != 0) {
                return false;
            }

            if (!is_status_line && !found_colon) {
                uint8x16_t has_colon = vceqq_u8(v, colon);
                uint64x2_t col64 = vreinterpretq_u64_u8(has_colon);
                if ((vgetq_lane_u64(col64, 0) | vgetq_lane_u64(col64, 1)) != 0) {
                    for (int k = 0; k < 16; k++) {
                        char c = header[i + k];
                        if (c == ':') {
                            if (i + k == 0) return false;
                            found_colon = true;
                            break;
                        } else if (c == ' ' || c == '\t') {
                            return false;
                        }
                    }
                } else {
                    uint8x16_t has_space = vceqq_u8(v, space);
                    uint8x16_t has_tab = vceqq_u8(v, tab);
                    uint8x16_t sp_tb = vorrq_u8(has_space, has_tab);
                    uint64x2_t sp64 = vreinterpretq_u64_u8(sp_tb);
                    if ((vgetq_lane_u64(sp64, 0) | vgetq_lane_u64(sp64, 1)) != 0) {
                        return false;
                    }
                }
            }
        }
    }

    for (; i < end; i++) {
        unsigned char c = (unsigned char)header[i];
        if (c == '\r') {
            if (i + 1 >= end || header[i + 1] != '\n') return false;
        } else if (c == '\n') {
            if (i == 0 || header[i - 1] != '\r') return false;
        }
        if ((c < 0x20 && c != 0x09) || c == 0x7F) return false;
        if (!is_status_line && !found_colon) {
            if (c == ':') {
                if (i == 0) return false;
                found_colon = true;
            } else if (c == ' ' || c == '\t') {
                return false;
            }
        }
    }

    return is_status_line || found_colon;
}
#endif

#if defined(__riscv) && defined(__riscv_vector)
WISP_NO_SANITIZE
static bool wisp_simd_validate_http_header_rvv(const char *header, size_t len) {
    if (!header || len == 0) return false;
    if (header[0] == ' ' || header[0] == '\t') return false;

    size_t end = len;
    while (end > 0 && (header[end - 1] == '\r' || header[end - 1] == '\n')) {
        end--;
    }
    if (end == 0) return true;

    bool is_status_line = (end >= 5 && strncasecmp(header, "HTTP/", 5) == 0);
    size_t i = 0;
    bool found_colon = false;

    while (i < end) {
        size_t vl = __riscv_vsetvl_e8m1(end - i);
        vuint8m1_t chunk = __riscv_vle8_v_u8m1((const uint8_t *)(header + i), vl);

        vbool8_t lt_space = __riscv_vmsltu_vx_u8m1_b8(chunk, 0x20, vl);
        vbool8_t is_tab = __riscv_vmseq_vx_u8m1_b8(chunk, 0x09, vl);
        vbool8_t invalid_ctrl = __riscv_vmandn_mm_b8(lt_space, is_tab, vl);
        vbool8_t is_del = __riscv_vmseq_vx_u8m1_b8(chunk, 0x7F, vl);
        vbool8_t invalid = __riscv_vmor_mm_b8(invalid_ctrl, is_del, vl);

        if (__riscv_vfirst_m_b8(invalid, vl) >= 0) {
            return false;
        }

        if (!is_status_line && !found_colon) {
            vbool8_t has_colon = __riscv_vmseq_vx_u8m1_b8(chunk, ':', vl);
            long col_idx = __riscv_vfirst_m_b8(has_colon, vl);
            if (col_idx >= 0) {
                for (size_t k = i; k < i + (size_t)col_idx; k++) {
                    char c = header[k];
                    if (c == ' ' || c == '\t') return false;
                }
                if (i + (size_t)col_idx == 0) return false;
                found_colon = true;
            } else {
                vbool8_t has_space = __riscv_vmseq_vx_u8m1_b8(chunk, ' ', vl);
                vbool8_t has_tb = __riscv_vmseq_vx_u8m1_b8(chunk, '\t', vl);
                vbool8_t sp_tb = __riscv_vmor_mm_b8(has_space, has_tb, vl);
                if (__riscv_vfirst_m_b8(sp_tb, vl) >= 0) {
                    return false;
                }
            }
        }
        i += vl;
    }

    return is_status_line || found_colon;
}
#endif
#endif /* !defined(WISP_NO_SIMD) */

bool wisp_simd_validate_http_header(const char *header, size_t len) {
#if !defined(WISP_NO_SIMD)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    if (has_sse2()) {
        return wisp_simd_validate_http_header_sse2(header, len);
    }
#elif defined(__arm__) || defined(__aarch64__)
    if (has_neon()) {
        return wisp_simd_validate_http_header_neon(header, len);
    }
#elif defined(__riscv) && defined(__riscv_vector)
    if (has_rvv()) {
        return wisp_simd_validate_http_header_rvv(header, len);
    }
#endif
#endif /* !defined(WISP_NO_SIMD) */
    return wisp_simd_validate_http_header_scalar(header, len);
}

bool wisp_simd_validate_http_header_block(const char *headers, size_t len) {
    if (!headers || len == 0) return false;
    size_t pos = 0;

    while (pos < len) {
        size_t crlf = wisp_simd_find_crlf((const uint8_t *)(headers + pos), len - pos);
        if (crlf == (size_t)-1) {
            /* Remaining text without CRLF */
            return wisp_simd_validate_http_header(headers + pos, len - pos);
        }

        size_t line_len = crlf;
        if (line_len > 0) {
            if (!wisp_simd_validate_http_header(headers + pos, line_len)) {
                return false;
            }
        }
        pos += crlf + 2; /* Move past line and \r\n */
    }

    return true;
}
