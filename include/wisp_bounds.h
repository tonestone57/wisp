#ifndef WISP_BOUNDS_H
#define WISP_BOUNDS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define WISP_MAX_IPC_PACKET_SIZE 65536

static inline bool wisp_validate_packet_length(size_t payload_len) {
    return (payload_len > 0 && payload_len <= WISP_MAX_IPC_PACKET_SIZE);
}

#endif /* WISP_BOUNDS_H */
