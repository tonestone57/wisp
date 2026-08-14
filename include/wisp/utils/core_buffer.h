#ifndef WISP_UTILS_CORE_BUFFER_H_
#define WISP_UTILS_CORE_BUFFER_H_

#include <stddef.h>
#include <stdint.h>
#include <wisp/utils/errors.h>

/**
 * A generic dynamic buffer object.
 */
typedef struct core_buffer {
    uint8_t *data;    /**< Pointer to the data buffer */
    size_t length;    /**< Current length of the data */
    size_t allocated; /**< Total allocated size of the buffer */
} core_buffer;

/**
 * Initialize a core_buffer.
 *
 * \param buffer The buffer to initialize.
 * \return NSERROR_OK on success, or appropriate error.
 */
nserror core_buffer_init(core_buffer *buffer);

/**
 * Append data to a core_buffer.
 *
 * \param buffer The buffer to append to.
 * \param data The data to append.
 * \param length The length of the data to append.
 * \return NSERROR_OK on success, or appropriate error.
 */
nserror core_buffer_append(core_buffer *buffer, const uint8_t *data, size_t length);

/**
 * Pre-allocate space in a core_buffer.
 *
 * \param buffer The buffer to pre-allocate.
 * \param length The total size to allocate.
 * \return NSERROR_OK on success, or appropriate error.
 */
nserror core_buffer_reserve(core_buffer *buffer, size_t length);

/**
 * Destroy a core_buffer, freeing its data.
 *
 * \param buffer The buffer to destroy.
 */
void core_buffer_destroy(core_buffer *buffer);

/**
 * Trim buffer space if possible.
 */
nserror core_buffer_shrink(core_buffer *buffer);


/**
 * Wrap an external unowned buffer.
 * Note: core_buffer_destroy will not free data if allocated is 0.
 *
 * \param buffer The buffer to setup
 * \param data The external data pointer
 * \param length The length of the external data
 */
nserror core_buffer_wrap_external(core_buffer *buffer, uint8_t *data, size_t length);

/**
 * Get pointer to the buffer data.
 *
 * \param buffer The buffer
 * \return pointer to the data or NULL
 */
const uint8_t *core_buffer_data(const core_buffer *buffer);

/**
 * Get current length of the buffer data.
 *
 * \param buffer The buffer
 * \return the length of the data
 */
size_t core_buffer_length(const core_buffer *buffer);


/**
 * Truncate the buffer length to 0 without freeing memory.
 *
 * \param buffer The buffer
 */
void core_buffer_clear(core_buffer *buffer);

#endif /* WISP_UTILS_CORE_BUFFER_H_ */
