/*
 * Copyright 2026 Wisp Browser Project
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#ifndef _WISP_CONTENT_FETCHERS_BROKER_H_
#define _WISP_CONTENT_FETCHERS_BROKER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "utils/errors.h"

/**
 * Register the broker fetcher.
 *
 * @return NSERROR_OK on success.
 */
nserror fetch_broker_register(void);

/**
 * Deliver fetch header from broker.
 */
void fetch_broker_deliver_header(int fetch_id, const uint8_t *data, size_t len);

/**
 * Deliver fetch data from broker.
 */
void fetch_broker_deliver_data(int fetch_id, const uint8_t *data, size_t len);

/**
 * Signal fetch completion from broker.
 */
void fetch_broker_deliver_done(int fetch_id);

/**
 * Signal fetch error from broker.
 */
void fetch_broker_deliver_error(int fetch_id);

#ifdef __cplusplus
}
#endif

#endif
