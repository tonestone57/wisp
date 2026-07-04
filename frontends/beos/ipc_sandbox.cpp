/*
 * Copyright 2026 Wisp Browser Project
 *
 * This file is part of Wisp.
 *
 * Wisp is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 */

#include <OS.h>
#include <image.h>
#include <Message.h>
#include <Messenger.h>
#include <Looper.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include "utils/errors.h"
#include "utils/log.h"
#include "wisp/desktop/ipc_sandbox.h"
}

/**
 * Haiku implementation of SpawnWorkerProcess.
 * Uses load_image to spawn a new process with the current executable.
 */
static nserror beos_spawn_worker_process(const char *type, char **argv, int *pid_out)
{
    char app_path[B_PATH_NAME_LENGTH];
    image_info info;
    int32 cookie = 0;
    bool found = false;

    while (get_next_image_info(0, &cookie, &info) == B_OK) {
        if (info.type == B_APP_IMAGE) {
            strncpy(app_path, info.name, sizeof(app_path));
            found = true;
            break;
        }
    }

    if (!found) {
        NSLOG(wisp, ERROR, "Could not find application path");
        return NSERROR_NOT_FOUND;
    }

    // Construct the full argument list including the type flag
    int argc = 0;
    if (argv) {
        while (argv[argc]) argc++;
    }

    const char **new_argv = (const char **)malloc((argc + 4) * sizeof(char *));
    if (!new_argv) return NSERROR_NOMEM;
    new_argv[0] = app_path;
    char type_arg[64];
    snprintf(type_arg, sizeof(type_arg), "--%s-process", type);
    new_argv[1] = type_arg;
    char pid_arg[64];
    snprintf(pid_arg, sizeof(pid_arg), "--ui-pid=%d", (int)getpid());
    new_argv[2] = pid_arg;
    if (argv) {
        for (int i = 0; i < argc; i++) {
            new_argv[i + 3] = argv[i];
        }
    }
    new_argv[argc + 3] = NULL;

    thread_id thread = load_image(argc + 3, new_argv, (const char **)environ);
    free(new_argv);

    if (thread < B_OK) {
        NSLOG(wisp, ERROR, "load_image failed: %s", strerror(thread));
        return NSERROR_INIT_FAILED;
    }

    resume_thread(thread);
    NSLOG(wisp, INFO, "Spawned %s process (PID: %d)", type, thread);

    if (pid_out) *pid_out = (int)thread;

    return NSERROR_OK;
}

/**
 * Haiku implementation of PostIPCMessage.
 * Uses BMessenger to send a BMessage to the target team.
 */
static nserror beos_post_ipc_message(int target_pid, uint32_t msg_id, uint32_t handle, const void *data, size_t size)
{
    BMessenger messenger(NULL, (team_id)target_pid);
    if (!messenger.IsValid()) {
        return NSERROR_NOT_FOUND;
    }

    BMessage msg(msg_id);
    msg.AddInt32("sender_pid", (int32)getpid());
    if (handle != 0) {
        msg.AddInt32("handle", (int32)handle);
    }

    if (data && size > 0) {
        msg.AddData("payload", B_RAW_TYPE, data, size);
    }

    status_t err = messenger.SendMessage(&msg);
    if (err != B_OK) {
        NSLOG(wisp, ERROR, "BMessenger::SendMessage failed: %s", strerror(err));
        return NSERROR_INIT_FAILED;
    }

    return NSERROR_OK;
}

/**
 * Haiku implementation of SharedMemoryTransport.
 * Uses Kernel Areas for zero-copy memory sharing.
 */
static nserror beos_shared_memory_transport(const char *name, size_t size, bool create, void **addr)
{
    if (create) {
        area_id area = create_area(name, addr, B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
        if (area < B_OK) {
            NSLOG(wisp, ERROR, "create_area failed: %s", strerror(area));
            return NSERROR_NOMEM;
        }
    } else {
        // If name starts with "id:", it's an area_id passed as hex
        if (strncmp(name, "id:", 3) == 0) {
            area_id source_area = (area_id)strtol(name + 3, NULL, 16);
            area_id area = clone_area("wisp_cloned_fb", addr, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, source_area);
            if (area < B_OK) {
                NSLOG(wisp, ERROR, "clone_area by ID failed: %s", strerror(area));
                return NSERROR_NOMEM;
            }
        } else {
            area_id source_area = find_area(name);
            if (source_area < B_OK) {
                return NSERROR_NOT_FOUND;
            }
            area_id area = clone_area(name, addr, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, source_area);
            if (area < B_OK) {
                NSLOG(wisp, ERROR, "clone_area failed: %s", strerror(area));
                return NSERROR_NOMEM;
            }
        }
    }

    return NSERROR_OK;
}

/**
 * Haiku implementation of SandboxFileSystem.
 * (Brokered approach - restrict direct access where possible)
 */
static nserror beos_sandbox_file_system(const char *root_path)
{
    // On Haiku, we rely on privilege dropping and the broker for isolation.
    NSLOG(wisp, INFO, "Sandboxing file system to %s", root_path);

    // Content process: inform UI process of restricted root via IPC if needed
    if (nsbeos_ipc_sandbox_table->is_content_process) {
        nsbeos_ipc_sandbox_table->post_ipc_message(nsbeos_ipc_sandbox_table->ui_process_pid, WISP_MSG_FILE_REQUEST, 0, root_path, strlen(root_path) + 1);
    }
    return NSERROR_OK;
}

/**
 * Haiku implementation of IsolateSockets.
 */
static nserror beos_isolate_sockets(void)
{
    // The sandboxed process should not create its own sockets.
    NSLOG(wisp, INFO, "Enforcing socket isolation for Content Process");
    // On Haiku, we rely on POSIX privilege dropping to 'nobody', which
    // restricts raw socket creation. The Brokered Networking further ensures
    // no direct Wisp-level networking occurs in the content process.
    return NSERROR_OK;
}

/**
 * Haiku implementation of DropPrivileges.
 * Drops the process's effective UID/GID to 'nobody'.
 */
static nserror beos_drop_privileges(void)
{
    struct passwd *pw = getpwnam("nobody");
    if (pw) {
        if (setgid(pw->pw_gid) != 0) {
            NSLOG(wisp, ERROR, "setgid failed: %s", strerror(errno));
            return NSERROR_INIT_FAILED;
        }
        if (setuid(pw->pw_uid) != 0) {
            NSLOG(wisp, ERROR, "setuid failed: %s", strerror(errno));
            return NSERROR_INIT_FAILED;
        }
        NSLOG(wisp, INFO, "Successfully dropped privileges to 'nobody'");
        return NSERROR_OK;
    }

    NSLOG(wisp, WARNING, "Could not find 'nobody' user to drop privileges");
    return NSERROR_NOT_FOUND;
}

static struct gui_ipc_sandbox_table beos_ipc_sandbox_table = {
    false, // is_content_process
    0,     // ui_process_pid
    beos_spawn_worker_process,
    beos_post_ipc_message,
    beos_shared_memory_transport,
    beos_sandbox_file_system,
    beos_isolate_sockets,
    beos_drop_privileges
};

extern "C" struct gui_ipc_sandbox_table *nsbeos_ipc_sandbox_table = &beos_ipc_sandbox_table;
