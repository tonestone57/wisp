/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#ifndef _WISP_UTILS_STREAM_CHUNK_DECODER_H_
#define _WISP_UTILS_STREAM_CHUNK_DECODER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WISP_CHUNK_STATE_SIZE,
    WISP_CHUNK_STATE_EXT,
    WISP_CHUNK_STATE_DATA,
    WISP_CHUNK_STATE_DATA_CRLF,
    WISP_CHUNK_STATE_TRAILER,
    WISP_CHUNK_STATE_DONE,
    WISP_CHUNK_STATE_ERROR
} wisp_chunk_state_t;

typedef struct wisp_stream_chunk_decoder {
    wisp_chunk_state_t state;
    size_t chunk_bytes_remaining;
    size_t hex_digits_count;
    bool is_final_chunk;
} wisp_stream_chunk_decoder_t;

/**
 * Initialize HTTP progressive chunked stream decoder state machine.
 */
void wisp_stream_chunk_decoder_init(wisp_stream_chunk_decoder_t *decoder);

/**
 * Decode progressive HTTP chunked stream buffer.
 *
 * @param decoder Context state machine.
 * @param in_buf Raw input HTTP stream buffer.
 * @param in_len Input data size in bytes.
 * @param bytes_read Returns number of bytes consumed from in_buf.
 * @param out_buf Destination buffer for decoded payload data.
 * @param out_capacity Size capacity of out_buf.
 * @param bytes_written Returns number of decoded bytes written to out_buf.
 * @return 0 on success/in-progress, 1 when terminal 0-size chunk stream complete, or negative error code.
 */
int wisp_stream_chunk_decoder_decode(wisp_stream_chunk_decoder_t *decoder,
                                      const uint8_t *in_buf, size_t in_len, size_t *bytes_read,
                                      uint8_t *out_buf, size_t out_capacity, size_t *bytes_written);

/**
 * SIMD-accelerated CRLF ("\r\n") byte scanner fastpath.
 * Scans up to len bytes using 16-byte SSE2 / NEON / RVV 1.0 vector loops with scalar fallback.
 *
 * @param data Input buffer pointer.
 * @param len Length of input buffer.
 * @return Offset of '\r' when followed by '\n', or (size_t)-1 if CRLF is not found.
 */
size_t wisp_scan_crlf_simd(const uint8_t *data, size_t len);

/* Explicit SIMD architecture scanner variants */
size_t wisp_scan_crlf_sse2(const uint8_t *data, size_t len);
size_t wisp_scan_crlf_neon(const uint8_t *data, size_t len);
size_t wisp_scan_crlf_rvv(const uint8_t *data, size_t len);
size_t wisp_scan_crlf_scalar(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* _WISP_UTILS_STREAM_CHUNK_DECODER_H_ */
