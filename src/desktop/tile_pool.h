#ifndef WISP_DESKTOP_TILE_POOL_H
#define WISP_DESKTOP_TILE_POOL_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Initialize the tile memory pool.
 *
 * \param pool_size  Number of buffers to pre-allocate.
 * \return true on success, false on failure.
 */
bool tile_pool_init(size_t pool_size);

/**
 * Finalize the tile memory pool and free all buffers.
 */
void tile_pool_fini(void);

/**
 * Checkout a fixed-size tile buffer from the pool.
 *
 * \return Pointer to a 1MB buffer, or NULL if none available.
 */
void *tile_pool_checkout(void);

/**
 * Return a tile buffer to the pool.
 *
 * \param buffer  The buffer to return.
 */
void tile_pool_return(void *buffer);

/**
 * Get the fixed size of buffers in the pool.
 */
size_t tile_pool_get_buffer_size(void);

#endif /* WISP_DESKTOP_TILE_POOL_H */
