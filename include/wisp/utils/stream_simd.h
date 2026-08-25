/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#ifndef WISP_UTILS_STREAM_SIMD_H
#define WISP_UTILS_STREAM_SIMD_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Result structure for progressive chunked stream parsing.
 */
typedef struct {
    size_t consumed_bytes;  /**< Number of bytes consumed from input stream. */
    size_t decoded_bytes;   /**< Number of decoded payload bytes written to output buffer. */
    bool is_final_chunk;    /**< True if terminal 0-length chunk was encountered. */
    bool is_incomplete;     /**< True if more stream data is required to complete current chunk header/payload. */
    bool is_invalid;        /**< True if chunk header syntax or CRLF delimiter is invalid. */
} wisp_chunk_decode_result;

/**
 * Locate the first CRLF ("\r\n") sequence in stream data using SIMD byte scanning fastpath with scalar fallback.
 *
 * @param data Stream input buffer.
 * @param len Length of stream input buffer.
 * @return Offset to '\r' in CRLF sequence if found, or (size_t)-1 if CRLF is not found.
 */
size_t wisp_simd_find_crlf(const uint8_t *data, size_t len);

/**
 * Parse chunk size header line from HTTP chunked stream data (e.g. "1a;extension\r\n").
 *
 * @param data Stream input buffer starting at chunk header.
 * @param len Buffer length available.
 * @param chunk_size Output pointer for parsed chunk size in bytes.
 * @param header_len Output pointer for byte length of chunk size header including CRLF.
 * @return True if header parsed successfully, false if incomplete or invalid format.
 */
bool wisp_simd_parse_chunk_header(const uint8_t *data, size_t len, size_t *chunk_size, size_t *header_len);

/**
 * Decode progressive stream data encoded with HTTP/1.1 chunked transfer-encoding using SIMD fastpath.
 *
 * @param in Input buffer containing chunked stream data.
 * @param in_len Input buffer length in bytes.
 * @param out Output buffer to receive raw decoded payload (can be same as input if in-place decoding is desired).
 * @param out_capacity Maximum capacity of output buffer.
 * @return Decoding result details.
 */
wisp_chunk_decode_result wisp_simd_decode_chunked_stream(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_capacity);

/**
 * Validate a single HTTP header line (e.g., "Content-Type: text/html\r\n") for RFC 7230 compliance
 * using SIMD vector character scanning fastpath with scalar fallback.
 *
 * Checks field name syntax, colon separator existence, control character absence, bare CR/LF, and obs-fold.
 *
 * @param header Header line buffer.
 * @param len Header line length in bytes.
 * @return True if valid HTTP header line, false otherwise.
 */
bool wisp_simd_validate_http_header(const char *header, size_t len);

/**
 * Validate a full HTTP header block (multiple header lines ending in \r\n or \r\n\r\n).
 *
 * @param headers Header block buffer.
 * @param len Header block length in bytes.
 * @return True if all header lines are valid RFC 7230 headers, false otherwise.
 */
bool wisp_simd_validate_http_header_block(const char *headers, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* WISP_UTILS_STREAM_SIMD_H */
