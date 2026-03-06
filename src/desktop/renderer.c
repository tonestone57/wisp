#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
    #include <unistd.h>
#else
    #include <windows.h>
#endif
#include "wisp/ipc/ipc.h"
#include "wisp/utils/log.h"
#include "wisp/utils/errors.h"
#include "wisp/browser_window.h"
#include "wisp/types.h"
#include "desktop/browser_private.h"
#include "wisp/ipc/task_queue.h"

void wisp_renderer_main(const char *ipc_name) {
    NSLOG(wisp, INFO, "Renderer process started, attempting to connect to '%s'", ipc_name);

    struct ipc_connection *conn = NULL;
    nserror err;

    // Wait slightly to ensure the parent has bound the socket
    for (int i = 0; i < 10; i++) {
        err = ipc_connect(ipc_name, &conn);
        if (err == NSERROR_OK) break;
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
        usleep(100000);
#else
        Sleep(100);
#endif
    }

    if (err != NSERROR_OK) {
        NSLOG(wisp, ERROR, "Renderer failed to connect to IPC channel '%s'", ipc_name);
        return;
    }

    NSLOG(wisp, INFO, "Renderer successfully connected to Browser UI process");


    struct task_queue *queue = NULL;
    if (task_queue_init(&queue) != NSERROR_OK || !queue) {
        NSLOG(wisp, ERROR, "Failed to initialize renderer task queue.");
        ipc_close(conn);
        return;
    }

    extern void ipc_plotter_set_queue(struct task_queue *queue);
    ipc_plotter_set_queue(queue); // Hook up the serialization queue

    // Main Renderer Event Loop
    while (!queue->stop) {
        // Because UI bypasses accept for Phase 0.5, we don't block on socket read.
        // Instead we only read from the shared task_queue.

        task_func func = NULL;
        void *ctx = NULL;
        if (task_queue_pop(queue, &func, &ctx) == NSERROR_OK) {
            if (func) func(ctx);
            // Simulate handling navigation internally
        } else {
#if defined(__unix__) || defined(__linux__) || defined(__APPLE__)
            usleep(10000); // 10ms idle
#else
            Sleep(10);
#endif
        }
    }

    ipc_close(conn);
    task_queue_destroy(queue);
}
