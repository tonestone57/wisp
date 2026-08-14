#include <stdlib.h>
#include <string.h>
#include <wisp/utils/core_buffer.h>

nserror core_buffer_init(core_buffer *buffer)
{
    if (buffer == NULL)
        return NSERROR_BAD_PARAMETER;

    buffer->data = NULL;
    buffer->length = 0;
    buffer->allocated = 0;

    return NSERROR_OK;
}

nserror core_buffer_append(core_buffer *buffer, const uint8_t *data, size_t length)
{
    if (buffer == NULL || (data == NULL && length > 0))
        return NSERROR_BAD_PARAMETER;

    if (length == 0)
        return NSERROR_OK;

    nserror err = core_buffer_reserve(buffer, buffer->length + length);
    if (err != NSERROR_OK)
        return err;

    memcpy(buffer->data + buffer->length, data, length);
    buffer->length += length;

    return NSERROR_OK;
}

nserror core_buffer_reserve(core_buffer *buffer, size_t length)
{
    if (buffer == NULL)
        return NSERROR_BAD_PARAMETER;

    if (length <= buffer->allocated)
        return NSERROR_OK;

    size_t new_alloc = buffer->allocated == 0 ? 64 : buffer->allocated * 2;
    while (new_alloc < length) {
        new_alloc *= 2;
    }

    uint8_t *new_data = realloc(buffer->data, new_alloc);
    if (new_data == NULL)
        return NSERROR_NOMEM;

    buffer->data = new_data;
    buffer->allocated = new_alloc;

    return NSERROR_OK;
}

void core_buffer_destroy(core_buffer *buffer)
{
    if (buffer == NULL)
        return;

    free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
    buffer->allocated = 0;
}

nserror core_buffer_shrink(core_buffer *buffer)
{
    if (buffer == NULL) return NSERROR_BAD_PARAMETER;

    if (buffer->length == 0) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->allocated = 0;
        return NSERROR_OK;
    }

    if (buffer->allocated > buffer->length) {
        uint8_t *new_data = realloc(buffer->data, buffer->length);
        if (new_data != NULL) {
            buffer->data = new_data;
            buffer->allocated = buffer->length;
        }
    }

    return NSERROR_OK;
}
