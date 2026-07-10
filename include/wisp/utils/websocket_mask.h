/*
 * Copyright 2026 Jules <jules@wisp-browser.org>
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#ifndef _WISP_UTILS_WEBSOCKET_MASK_H_
#define _WISP_UTILS_WEBSOCKET_MASK_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Perform WebSocket/Wisp client-to-proxy payload masking.
 * Accelerates rolling 4-byte key bitwise-XOR operation using SIMD (AVX2/NEON/RVV).
 *
 * @param data       The payload buffer to be masked/unmasked in-place.
 * @param len        The length of the payload buffer.
 * @param mask_key   The 4-byte masking key.
 * @param key_offset The initial offset within the 4-byte mask key (normally 0).
 */
void wisp_websocket_mask(uint8_t *data, size_t len, const uint8_t mask_key[4], size_t key_offset);

#ifdef __cplusplus
}
#endif

#endif /* _WISP_UTILS_WEBSOCKET_MASK_H_ */
