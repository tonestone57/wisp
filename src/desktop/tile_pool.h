#ifndef WISP_DESKTOP_TILE_POOL_H
#define WISP_DESKTOP_TILE_POOL_H

#include <stddef.h>
#include <stdbool.h>

#define TILE_WIDTH 512
#define TILE_HEIGHT 512
#define TILE_BUFFER_SIZE (TILE_WIDTH * TILE_HEIGHT * 4) // 1MB

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

/**
 * Try to retrieve a cached tile from the pool's in-memory tile cache.
 * If found, it will decompress it (if compressed) and return the uncompressed buffer.
 * If decompressed/raw, it returns the buffer directly.
 *
 * \param owner             The owner/window of this tile.
 * \param doc_x             The document space x coordinate of the tile.
 * \param doc_y             The document space y coordinate of the tile.
 * \param tile_size         The size of the tile.
 * \param out_from_cache    Set to true if a cache hit occurred, false otherwise.
 * \return Pointer to the uncompressed raw 1MB buffer on hit, or NULL on miss.
 */
void *tile_pool_get_cached(void *owner, int doc_x, int doc_y, int tile_size, bool *out_from_cache);

/**
 * Save a rendered tile buffer to the cache.
 *
 * \param owner     The owner/window of this tile.
 * \param doc_x     The document space x coordinate of the tile.
 * \param doc_y     The document space y coordinate of the tile.
 * \param tile_size The size of the tile.
 * \param buffer    The uncompressed raw 1MB buffer containing pixels.
 * \param priority  The priority of this tile (calculated from viewport distance).
 */
void tile_pool_put_cached(void *owner, int doc_x, int doc_y, int tile_size, void *buffer, float priority);

/**
 * Recalculate priorities of all cached tiles for the owner and compress/evict accordingly.
 *
 * \param owner           The owner/window of the tiles.
 * \param viewport_x      The current viewport scroll x coordinate.
 * \param viewport_y      The current viewport scroll y coordinate.
 * \param viewport_width  The viewport width in pixels.
 * \param viewport_height The viewport height in pixels.
 */
void tile_pool_manage_cache(void *owner, int viewport_x, int viewport_y, int viewport_width, int viewport_height);

#endif /* WISP_DESKTOP_TILE_POOL_H */
