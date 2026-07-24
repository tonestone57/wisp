#ifndef wisp_dnd_bridge_h_
#define wisp_dnd_bridge_h_

#include <stddef.h>
#include <stdint.h>

struct browser_window;

typedef enum {
    WISP_DND_DRAGSTART,
    WISP_DND_DRAGENTER,
    WISP_DND_DRAGOVER,
    WISP_DND_DRAGLEAVE,
    WISP_DND_DROP,
    WISP_DND_DRAGEND
} wisp_dnd_event_type_t;

typedef struct wisp_dnd_payload {
    char **mime_types;
    size_t type_count;
    void *raw_data;
    size_t data_len;
    uint32_t allowed_effects; /* 1 = COPY, 2 = MOVE, 4 = LINK */
} wisp_dnd_payload_t;

/* Dispatches a native DND event to the active document/element at (screen_x, screen_y) */
void wisp_dnd_dispatch_native_event(
    void *thread_ptr, /* jsthread pointer */
    wisp_dnd_event_type_t type,
    wisp_dnd_payload_t *payload,
    int screen_x,
    int screen_y);

/* Decoupled dispatch: window.c doesn't need to know about jsthread */
void wisp_dnd_dispatch_native_event_bw(
    struct browser_window *bw,
    wisp_dnd_event_type_t type,
    wisp_dnd_payload_t *payload,
    int screen_x,
    int screen_y);

#endif
