# Multi-Process Architecture: Remaining Upgrade Outline

This document outlines the remaining engineering steps required to transition Wisp from the current **Phase 0.5 (Interim Threaded Mode)** to a fully secured, Chromium-style **Multi-Process Architecture**.

---

## Current State (Phase 0.5: Completed)
We have successfully decoupled the layout engine's drawing intent from the frontend UI.
*   **Thread Isolation:** `browser_window_create` spawns a dedicated background `pthread` for the rendering engine (`wisp_renderer_main`).
*   **Asynchronous Drawing:** Instead of synchronous UI blocking, the `ipc_plotter` interface intercepts drawing commands (e.g., `ipc_plot_rectangle`) and dispatches them across thread boundaries via a resilient `task_queue`.
*   **IPC Infrastructure:** A secure IPC Domain Socket system (`ipc.c`) using `$XDG_RUNTIME_DIR` and `0600` umasks is scaffolded and ready to be activated.
*   **Stability:** Cross-platform compatibility (Windows vs POSIX) is stabilized, and thread teardown memory leaks are plugged.

---

## Phase 1.0: Full Process Isolation
To protect the browser from single-tab crashes, we must transition from `pthreads` to full OS-level process isolation.

1.  **Swap Threads for Processes:**
    *   Replace `pthread_create` with `fork()` and `exec()` (on POSIX) or `CreateProcess()` (on Windows) inside `browser_window_create`.
    *   Each tab must run in its own distinct virtual memory space.
2.  **Activate IPC Sockets:**
    *   Switch the primary communication channel from the shared memory `task_queue` to the hardened Unix Domain Sockets / Windows Named Pipes using the existing `ipc_send` / `ipc_receive` stubs.
3.  **Serialize Input Events:**
    *   Currently, the UI directly invokes `browser_window_mouse_action` and `browser_window_key_press`. These synchronous calls must be serialized into `IPC_MSG_MOUSE_EVENT` / `IPC_MSG_KEY_EVENT` packets and sent over the socket to the renderer process.
4.  **Process Monitor (Handling "Zombies"):**
    *   The Browser UI must actively listen for `SIGCHLD` (POSIX) or use `WaitForSingleObject` (Windows).
    *   If a renderer crashes, the UI must intercept the signal, cleanly close the dead socket, and present a "Sad Tab / Crash" error screen with an option to reload the domain.

---

## Phase 2.0: Performance & Back-pressure
Process isolation introduces IPC latency. This phase addresses the bottlenecks to ensure 60fps scrolling and layout speeds.

1.  **Font Metrics Mirroring (Crucial):**
    *   *The Problem:* The layout engine synchronously asks the frontend for text dimensions (e.g., "How wide is 'Hello' in 12pt Arial?"). Over a socket, this IPC round-trip per word is catastrophically slow.
    *   *The Solution:* The UI process must construct a "Font Metric Table" upon startup and pass it down to the renderer via Shared Memory (SHM). The renderer can then calculate word widths entirely locally.
2.  **Shared Memory (SHM) for Bitmaps:**
    *   *The Problem:* Serializing large 4K uncompressed images over a Domain Socket will choke the pipe and freeze the UI.
    *   *The Solution:* Implement a handle-based SHM system (POSIX `shm_open` or Windows `CreateFileMapping`). The renderer decodes pixels directly into SHM and passes the lightweight handle over IPC (`IPC_MSG_BITMAP_READY`).
3.  **Command Batching & Back-pressure:**
    *   Group drawing commands into single IPC packets per layer/tile to reduce syscall context switches.
    *   Implement a circular buffer or double-buffering logic. If the UI process falls behind the renderer's frame generation rate, it must exert back-pressure to pause the renderer rather than flooding the socket.

---

## Phase 3.0: Security & Sandboxing
With processes successfully isolated, the final step is locking them down to prevent exploited renderers from damaging the host OS.

1.  **OS-Level Sandboxing:**
    *   Apply `seccomp-bpf` (Linux), `AppContainer` (Windows), or `seatbelt` (macOS) to the renderer executable.
    *   A compromised QuickJS instance or buffer overflow in the layout engine must be mathematically prevented from reading or writing directly to the user's filesystem or spawning child shells.
