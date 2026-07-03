/*
 * Copyright 2026 Wisp Browser Project
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

/**
 * \file
 * IPC and Sandboxing Abstraction Layer interface.
 */

#ifndef _WISP_DESKTOP_IPC_SANDBOX_H_
#define _WISP_DESKTOP_IPC_SANDBOX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "utils/errors.h"

struct gui_ipc_sandbox_table {
    /**
     * True if this is a sandboxed content process.
     */
    bool is_content_process;

    /**
     * The PID/TeamID of the UI Process.
     */
    int ui_process_pid;

    /**
     * Spawn a new worker process.
     *
     * @param type The type of process (e.g., "content")
     * @param argv Null-terminated array of arguments.
     * @return NSERROR_OK on success.
     */
    nserror (*spawn_worker_process)(const char *type, char **argv);

    /**
     * Post a message to another process.
     *
     * @param target_pid The PID of the target process.
     * @param msg_id The message identifier.
     * @param handle An optional handle (e.g., fetch_id or window_id).
     * @param data Pointer to the message data.
     * @param size Size of the message data.
     * @return NSERROR_OK on success.
     */
    nserror (*post_ipc_message)(int target_pid, uint32_t msg_id, uint32_t handle, const void *data, size_t size);

    /**
     * Create or attach to a shared memory block for frame buffer transport.
     *
     * @param name Name or identifier of the shared memory.
     * @param size Size of the memory block.
     * @param create True to create, false to attach.
     * @param addr Pointer to store the mapped address.
     * @return NSERROR_OK on success.
     */
    nserror (*shared_memory_transport)(const char *name, size_t size, bool create, void **addr);

    /**
     * Apply file system sandbox restrictions.
     *
     * @param root_path The restricted root path.
     * @return NSERROR_OK on success.
     */
    nserror (*sandbox_file_system)(const char *root_path);

    /**
     * Isolate the process from direct network access.
     *
     * @return NSERROR_OK on success.
     */
    nserror (*isolate_sockets)(void);

    /**
     * Drop process privileges to an unprivileged user.
     *
     * @return NSERROR_OK on success.
     */
    nserror (*drop_privileges)(void);
};

#endif
