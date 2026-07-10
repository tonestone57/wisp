/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#ifndef _WISP_UTILS_UTF8PROC_WRAPPER_H_
#define _WISP_UTILS_UTF8PROC_WRAPPER_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <utf8proc.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Detect if ASCII and validate UTF-8 strings.
 */
bool wisp_is_ascii(const char *str, size_t len);
bool wisp_validate_utf8(const char *str, size_t len);

/*
 * SIMD-accelerated string comparison primitives.
 */
int wisp_simd_strcmp(const char *s1, const char *s2);
bool wisp_simd_streq(const char *s1, const char *s2);

/*
 * Security and blocklist checks.
 */
bool wisp_security_is_origin_blocked(const char *origin);

/*
 * Fast-path ASCII case conversions.
 * Requires that inputs are verified to be ASCII.
 */
void wisp_ascii_tolower(const char *src, char *dst, size_t len);
void wisp_ascii_toupper(const char *src, char *dst, size_t len);

/*
 * Fast-path ASCII <-> UTF-32 (UCS-4) conversions.
 */
void wisp_ascii_to_utf32(const char *src, int32_t *dst, size_t len);
void wisp_utf32_to_ascii(const int32_t *src, char *dst, size_t len);

/*
 * Optimized wrappers for utf8proc standard functions.
 */
utf8proc_ssize_t wisp_utf8proc_decompose(
    const utf8proc_uint8_t *str, utf8proc_ssize_t len,
    utf8proc_int32_t *buffer, utf8proc_ssize_t bufsize, utf8proc_option_t options);

utf8proc_ssize_t wisp_utf8proc_normalize_utf32(
    utf8proc_int32_t *buffer, utf8proc_ssize_t length, utf8proc_option_t options);

utf8proc_ssize_t wisp_utf8proc_reencode(
    utf8proc_int32_t *buffer, utf8proc_ssize_t length, utf8proc_option_t options);

/* Normalization shortcuts */
utf8proc_uint8_t *wisp_utf8proc_NFD(const utf8proc_uint8_t *str);
utf8proc_uint8_t *wisp_utf8proc_NFC(const utf8proc_uint8_t *str);
utf8proc_uint8_t *wisp_utf8proc_NFKD(const utf8proc_uint8_t *str);
utf8proc_uint8_t *wisp_utf8proc_NFKC(const utf8proc_uint8_t *str);

#ifdef __cplusplus
}
#endif

#endif /* _WISP_UTILS_UTF8PROC_WRAPPER_H_ */
