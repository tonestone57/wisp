# Wisp Browser Technical Roadmap & Architectural Summary (July 2026)

## 1. Executive Summary
Wisp is a lightweight, high-performance web engine forked from NetSurf. As of July 2026, Wisp has successfully bridged the gap between "retro" software efficiency and the modern web. The core engine is now stable, featuring a fully spec-compliant implementation of CSS Grid, Flexbox, and modern JavaScript (ES2023+ via QuickJS-ng). Wisp maintains a minimal footprint suitable for both modern and legacy operating systems including Haiku, Windows XP/7/10/11, Linux, and macOS. All major 2026 architectural goals, including Multi-Process Isolation and the Canvas 2D plotter bridge, have been achieved.

---

## 2. Graphics Architecture
Wisp utilizes a "best-of-breed" plotting architecture to ensure performance and consistency across all supported platforms.

### Primary Backends
*   **Direct2D & DirectWrite (Windows 7+)**: A hardware-accelerated pipeline providing high-performance GPU drawing and crisp native typography. Features robust device-loss recovery.
*   **Blend2D (Unified Backbone & Fallback)**: A high-performance software 2D engine using JIT-compiled SIMD (AVX-512, NEON) for rasterization. It is the primary rasterizer for Linux and macOS, and the mandatory fallback for **Windows XP/Vista**, ensuring modern CSS compatibility on legacy hardware.
*   **Native Typography Interop**: Wisp uses platform-specific handlers (`win32_plot_text_ns`, `macos_plot_text_ns`, etc.) to ensure that even when using Blend2D for content rasterization, text remains crisp and adheres to system-level subpixel rendering settings.

### Rendering Strategy
Wisp has fully transitioned to a **Fixed-Tile Redraw** strategy (256px or 512px tiles). This system optimizes cache locality, eliminates overdraw, and provides the necessary isolation for the Parallel Tile Redraw architecture.

---

## 3. Parallel Tile Redraw (PTR) Architecture
Wisp's architecture is uniquely positioned to take advantage of multi-core processors through the parallelization of the tiling loop.

### Cross-OS Implementation
1.  **Work Stealing**: The browser core pushes "Dirty Tile Tasks" to the `wisp_subsystem` worker pool, which scales based on the system's logical core count.
2.  **Thread-Local Backends**: Each worker thread utilizes a thread-local instance of the rendering backend (Blend2D or Direct2D), allowing simultaneous rasterization of different tiles without mutex locking.
3.  **Asynchronous Compositing**: Once all workers finish their assigned tiles, the main thread performs a single atomic blit (e.g., via `SetDIBitsToDevice` on Windows or `BView` blit on Haiku) to the screen.

### Platform Performance
*   **Haiku / BeOS**: Workers rasterize tiles safely into thread-confined raw memory regions, bypassing the single-threaded `BWindow` looper limit.
*   **Windows**: Background threads concurrently record independent `ID2D1CommandList` blocks, minimizing GPU pipeline stalls.
*   **Linux (GTK/Qt)**: Offloads CPU-intensive SIMD rasterization (Blend2D) away from the main event loop.

---

## 4. JavaScript & DOM Threading
Wisp implements a **hybrid threading model** to balance safety with performance.

### Single-Threaded DOM Access
All scripts that manipulate page elements run on the **Main UI Thread**. This ensures safe interaction with the underlying C-based DOM (`libdom`), which is not thread-safe, without the overhead of complex locking.

### Background Worker Pool (`wisp_subsystem`)
A decoupled worker pool subsystem scales dynamically (P = N - 1 for rasterization, P = min(4, N) for JS workers). This allows Wisp to:
*   Offload computationally expensive tasks (cryptography, data parsing, image decoding).
*   Keep the UI thread responsive during heavy site execution.
*   Priority-based scheduling ensures frame-critical tasks (viewport tiles) are processed before background scripts.

---

## 5. Core Architectural Maturity
The following high-impact structural improvements have been fully integrated into the Wisp core:

### A. Tile Memory Recycling (Fixed-Buffer Pool)
To mitigate heap fragmentation, Wisp uses a thread-safe **Lookaside List** of 1MB tile memory buffers (`src/desktop/tile_pool.c`). This is critical for stability on legacy OS allocators (XP/Vista).

### B. Viewport-Prioritized Tile Scheduling
Every dirty tile task is assigned a priority multiplier calculated based on its geometric distance from the viewport frustum. This ensures that the user always sees the most relevant content first.

### C. QuickJS-DOM Bridge Stability
The mapping of C DOM nodes to JS objects uses a **weak-reference model** and explicit cycle-breaking logic, preventing memory leaks and Use-After-Free (UAF) scenarios during complex page navigation.

---

## 6. Recent Technical Improvements (2026 Hardening Audit)
The following stability and security measures have been integrated:
*   **Hardened Parsing**: Project-wide removal of unsafe `atoi` in favor of `ns_strtoint/ns_strtouint` with overflow protection.
*   **Stable Layout Fallbacks**: Replaced browser-crashing `abort()` and `assert(0)` calls in the core layout engine with `NSLOG` warnings and geometric clamping.
*   **MutationObserver Hardening**: Implemented spec-compliant queue swapping to prevent record loss during nested mutations.
*   **CSP Hardening**: Full enforcement of modern security headers with robust port-range validation (0-65535).
*   **SIMD-Aligned Arena**: The arena allocator was hardened against integer overflows in its `ALIGN_UP` macro while maintaining 64-byte alignment for AVX-512.
*   **Timer UAF Prevention**: Implemented mandatory timer unscheduling during thread destruction.
*   **Canvas 2D Plotter Bridge**: Successfully bridged WebIDL stubs for the Canvas 2D API to the underlying Direct2D and Blend2D plotter backends.
*   **Multi-process Isolation**: JavaScript engine and network stack isolated into separate OS processes via a platform-agnostic IPC layer.
*   **Web Worker Parity**: Full spec-compliant implementation of Web Workers, utilizing an isolated `JSRuntime` and `JSContext` per worker with structured cloning for messaging.
*   **BDirectWindow Migration (Haiku)**: Granted the drawing engine direct, locked access to the frame buffer, bypassing `app_server` context loops for lower latency.
*   **Native Haiku Widget Parity**: Completed integration of native `BControl` elements into the BeOS/Haiku frontend widget map, including selects, text areas, and file pickers.

---

## 7. Remaining Tasks & Priority Backlog
These tasks are high-priority for the 2027 development cycle:

### Graphics & Performance
*   **[Planned] WebGPU API Bridge** (Complexity: **High** | Benefit: **Medium**): Implement a preliminary WebGPU bridge to modern native graphics APIs.
*   **[Planned] GPU-Accelerated Compositing** (Complexity: **High** | Benefit: **High**): Move the final tile-blitting and scrolling pass to the GPU (OpenGL/Vulkan) to ensure buttery-smooth 60FPS scrolling on modern hardware.
*   **[Planned] CSS Variable Caching & Fast-Path Evaluation** (Complexity: **Medium** | Benefit: **High**): Implement style-context hashing/caching of custom property values. If inherited style contexts have unchanged CSS custom properties, skip the recursive resolution pass entirely for that subtree to accelerate modern CSS layouts (e.g., Tailwind CSS pages).
*   **[Planned] SIMD-Accelerated UTF-8 processing** (Complexity: **Medium** | Benefit: **Medium**): Integrate SIMD vectorization (AVX2/NEON) into `libutf8proc` wrappers to accelerate both fast-path ASCII/UTF-8 validation and full-path text normalization, case-mapping, and encoding conversions. This speeds up HTML parsing, text layout, and multi-process IPC string transfers globally.

### Architecture & Security
*   **[Planned] Site Isolation** (Complexity: **High** | Benefit: **High**): Extend the multi-process model to support per-origin process isolation.
*   **[Planned] OS-Level Sandboxing** (Complexity: **High** | Benefit: **High**): Integrate Landlock (Linux), AppContainer (Windows), and Pledge (OpenBSD) for maximum protection.
*   **[Planned] Link Pre-connect & DNS Prefetching Pipeline** (Complexity: **Medium** | Benefit: **High**): Parse `<link rel="dns-prefetch">` and `<link rel="preconnect">` in the main thread and issue early async DNS/socket setup requests to `wisp-network` to bypass connection latency prior to fetching resources.
*   **[Planned] CSP Level 3 Trusted Types** (Complexity: **Medium** | Benefit: **High**): Integrate CSP Trusted Types to restrict script execution and string-based injection (e.g., `innerHTML`), completely blocking DOM-based XSS vulnerabilities.

### UI & Features
*   **[Planned] Unified C-based UI Library** (Complexity: **Medium** | Benefit: **High**): A lightweight, cross-platform UI library for consistent 'browser chrome' (tabs, address bar).

---

## 8. Future Horizons (2027-2028)
*   **Zero-Copy IPC Architecture via Shared Memory**: Optimize the multi-process boundaries so that rasterized Blend2D tile bitmaps are passed from rendering/layout worker processes using POSIX/Windows shared-memory handles (`shm_open` or native file mappings), completely bypassing serialization over IPC channels.
*   **HTTP/3 QUIC Connection Caching and 0-RTT Session Resumption**: Optimize the transport layers in `wisp-network` to leverage dynamic 0-RTT handshakes and cache transport states to minimize connection setup latency on repeated requests.
*   **Optional JIT Compilation Tier Options**: Evaluate embedding an optional JIT compilation pipeline (such as Hermes or a lightweight WebAssembly JIT) for heavy script environments while keeping QuickJS-ng as the ultra-secure, lightweight default engine.
*   **Shared-Memory GPU-Shared Textures**: In the upcoming GPU-Accelerated Compositing pass, pass GPU-shared texture buffers directly across process boundaries to be fed straight into the native window compositor loops.
*   **WebAssembly (WASM) Interpretation**: Integrate a memory-safe, lightweight WASM interpreter to expand web application compatibility without bloating the footprint.
