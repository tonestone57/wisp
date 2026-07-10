# Wisp Browser Technical Roadmap & Architectural Summary (July 2026)

## 1. Executive Summary
Wisp is a lightweight, high-performance web engine forked from NetSurf. As of July 2026, Wisp has successfully bridged the gap between "retro" software efficiency and the modern web. The core engine is now stable, featuring a fully spec-compliant implementation of CSS Grid, Flexbox, and modern JavaScript (ES2023+ via QuickJS-ng). Wisp maintains a minimal footprint suitable for both modern and legacy operating systems including Haiku, Windows XP/7/10/11, Linux, and macOS. All major 2026 architectural goals, including Multi-Process Isolation and the Canvas 2D plotter bridge, have been achieved.

---

## 2. Graphics Architecture
Wisp utilizes a prioritized platform-native-first plotting architecture by default, removing the historical runtime "Auto" mode to prevent performance overhead and toolkit drift. Platform-native backends serve as the default compiling and runtime rendering engines, while **Blend2D** remains fully optional and available as an alternative choice and high-performance software fallback backend.

### Primary Backends
*   **BeOS / Haiku**: Native `BView` (AGG) rendering (Default).
*   **Linux (GTK / Qt)**: Cairo (GTK) or QPainter (Qt) (Default).
*   **macOS**: Cocoa native plotter (Default).
*   **Windows**: Can be explicitly chosen at compile time via `WISP_WINDOWS_USE_D2D` to build either the **Direct2D** (Default) or legacy **GDI** pipeline.

### Optional Fallback Backend
*   **Blend2D**: A software 2D engine compiled optionally with `WISP_USE_BLEND2D=ON`. It acts as an optional alternative and software fallback across all supported platforms when native pipelines are bypassed or unavailable.

### Typography Interop
*   **Native Typography**: Wisp uses platform-specific handlers (`win32_plot_text_ns`, `macos_plot_text_ns`, etc.) to ensure text remains crisp, adhering to system-level subpixel rendering settings across all plotting configurations.

### Rendering Strategy
Wisp has fully transitioned to a **Fixed-Tile Redraw** strategy (256px or 512px tiles). This system optimizes cache locality, eliminates overdraw, and provides the necessary isolation for the Parallel Tile Redraw architecture.

---

## 3. Parallel Tile Redraw (PTR) Architecture
Wisp's architecture is uniquely positioned to take advantage of multi-core processors through the parallelization of the tiling loop.

### Cross-OS Implementation
1.  **Work Stealing**: The browser core pushes "Dirty Tile Tasks" to the `wisp_subsystem` worker pool, which scales based on the system's logical core count.
2.  **Thread-Local Backends**: Each worker thread utilizes a thread-local instance of the active rendering backend (such as platform-native backends or fallback Blend2D), allowing simultaneous rasterization of different tiles without mutex locking.
3.  **Asynchronous Compositing**: Once all workers finish their assigned tiles, the main thread performs a single atomic blit (e.g., via `SetDIBitsToDevice` on Windows or native `BView` blit on Haiku) to the screen.

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
*   **BDirectWindow Migration (Haiku)**: Granted the drawing engine direct, locked access to the frame buffer, bypassing `app_server` context loops for lower latency on both native BView and Blend2D fallback renderings.
*   **Native Haiku Widget Parity**: Completed integration of native `BControl` elements into the BeOS/Haiku frontend widget map, including selects, text areas, and file pickers.
*   **CSP Level 3 Trusted Types, Nonce CSP, and COOP/COEP Integration**: Implemented strict auto-sanitizing default policy walking DOM trees, cryptographic nonce parsing/validation on script execution, and COOP/COEP process isolation.
*   **Link Pre-connect & DNS Prefetching Pipeline**: Full asynchronous DNS/socket pre-connections handled via dedicated networking process thread pools offloading connection startup latency.
*   **QuickJS Bytecode Ahead-of-Time (AOT) Caching**: Dynamically caches serialized QuickJS binary bytecode with SHA-256 keys to completely bypass lexing/parsing phases for returning users.
*   **LZ4 Compressed Tile Lookaside Lists**: Highly optimized thread-safe cache compressing out-of-viewport raw tiles with real-time LZ4 compression to prevent RAM OOMs on low-resource environments.
*   **SIMD-Accelerated UTF-8 Processing**: Dynamic feature-detected **AVX2 (on X86), NEON (on ARM), and RVV (on RISC-V)** vectorization of ASCII/UTF-8 validations, case mappings, and UTF-32 conversion with robust scalar fallbacks (i586 compatible).
*   **CSS Variable Caching & Fast-Path Evaluation**: Implemented style-context hashing/caching of custom property values in `libcss` to skip redundant recursive resolution passes, accelerating modern variable-heavy pages.
*   **Site Isolation & JavaScript Multi-Process Architecture**: Fully integrated per-origin process isolation with thread-safe origin tracking, UNIX sockets created with secure `0700` permissions, and automatic crashed engine reclamation fallback.
*   **QUIC & HTTP/3 Transport Support**: Supported QUIC and HTTP/3 protocol negotiation and Alt-Svc connection caching safely integrated in the libcurl networking process.

---

## 7. Remaining Tasks & Priority Backlog
These tasks are high-priority for the 2027 development cycle:

### Graphics & Performance
*   **[Planned] WebGPU API Bridge** (Complexity: **High** | Benefit: **Medium**): Implement a preliminary WebGPU bridge to modern native graphics APIs.
*   **[Planned] GPU-Accelerated Compositing** (Complexity: **High** | Benefit: **High**): Move the final tile-blitting and scrolling pass to the GPU (OpenGL/Vulkan) to ensure buttery-smooth 60FPS scrolling on modern hardware. This compositor dynamically pivots back to the high-performance Blend2D/GDI pipelines on non-DX11 or legacy hardware to maintain perfect backward compatibility.

### Architecture & Security
*   **[Planned] OS-Level Sandboxing** (Complexity: **High** | Benefit: **High**): Integrate Landlock (Linux), AppContainer (Windows), and Pledge (OpenBSD) for maximum protection.

### UI & Features
*   **[Planned] Unified C-based UI Library** (Complexity: **Medium** | Benefit: **High**): A lightweight, cross-platform UI library for consistent 'browser chrome' (tabs, address bar).

---

## 8. Future Horizons (2027-2028)
*   **Zero-Copy IPC Architecture via Shared Memory** (Complexity: **High** | Benefit: **High**): Optimize the multi-process boundaries so that rasterized Blend2D tile bitmaps are passed from rendering/layout worker processes using POSIX/Windows shared-memory handles (`shm_open` or native file mappings), completely bypassing serialization over IPC channels.
*   **Optional JIT Compilation Tier Options** (Complexity: **High** | Benefit: **Medium**): Evaluate embedding an optional JIT compilation pipeline (such as Hermes or a lightweight WebAssembly JIT) for heavy script environments while keeping QuickJS-ng as the ultra-secure, lightweight default engine.
*   **Shared-Memory GPU-Shared Textures** (Complexity: **High** | Benefit: **High**): In the upcoming GPU-Accelerated Compositing pass, pass GPU-shared texture buffers directly across process boundaries to be fed straight into the native window compositor loops.
*   **WebAssembly (WASM) Interpretation** (Complexity: **Medium** | Benefit: **Medium**): Integrate a memory-safe, lightweight WASM interpreter to expand web application compatibility without bloating the footprint.
*   **Wisp Protocol / WebSocket Payload Masking SIMD Acceleration** (Complexity: **Medium** | Benefit: **High**): Upstream WebSocket client-to-proxy payloads require a rolling 4-byte masking key bitwise-XOR operation. Implement SIMD broadcast and dynamic XOR vectorization (`_mm256_xor_si256` on AVX2 / `veorq_u8` on NEON / `vxor.vv` on RVV) to mask up to 32 bytes per clock cycle, eliminating upstream proxy network bottleneck.
*   **Structural JSON Parsing for QuickJS-ng SIMD Pre-parser** (Complexity: **Medium** | Benefit: **High**): Leverage a two-stage SIMD pre-parser (similar to `simdjson`) using **AVX2 (on X86), NEON (on ARM), and RVV (on RISC-V)** vector comparisons to scan raw incoming JSON strings 16/32 bytes at a time, generating structural token masks and fast bitwise jumps (`popcnt` or trailing zero counts) to completely skip raw data blocks and whitespaces.
*   **CSS Lexical and Layout Whitespace Skipping SIMD** (Complexity: **Medium** | Benefit: **High**): Incorporate a vector scanning register in `libcss` lexical scanners pre-loaded with target whitespace characters (spaces, carriage returns, newlines, tabs) to compare blocks of 16/32 bytes at once, advancing unstyled text pointers instantly.
*   **Multi-Process Shared Memory Color Space & Alpha Blending SIMD** (Complexity: **Medium** | Benefit: **Medium**): Accelerate Zero-Copy IPC compositing by offloading YUV-to-RGB floating-point/fixed-point matrix conversions and parallel alpha blending/composition to vectorized SIMD lanes to process 8 to 16 pixels simultaneously.
*   **SIMD CSP Nonce & Security Validation** (Complexity: **Medium** | Benefit: **High**): Speed up incoming request packet header verification (nonce checks, origin blocklists) by utilizing SIMD string comparison primitives rather than sequential `strcmp` loops.

---

## 9. Next-Generation Roadmap Proposals (2027 Development Cycle)

To take Wisp to the next level for its 2027 development cycle, several highly specialized architectural additions will address the hidden "tax" of supporting such a vast timeline of hardware and software.

### A. Compatibility & Performance: User-Space TLS & Network Fallbacks
Because Wisp aims to run natively on Windows XP/7 alongside modern OSes, relying on the host operating system's network stack creates a massive compatibility bottleneck.
*   **The Problem**: Windows XP and Vista's native crypto stacks (Schannel) do not support TLS 1.2 or TLS 1.3, making the modern web completely inaccessible without a proxy handling the decryption.
*   **The Fix (Statically-Linked User-Space Crypto Stack)** (Complexity: **Medium** | Benefit: **High**): Force the `wisp-network` process to completely bypass host OS network APIs. Statically link a lightweight, ultra-fast modern crypto library like **mbedTLS** or **BearSSL** directly into the network process.
*   **The Benefit**: Wisp achieves 100% independent HTTPS capability. A user on Windows XP or an older Haiku nightly build can connect directly to modern, strictly secured websites without requiring a middleman proxy or OS-level registry hacks.

### B. Security: Asymmetric OS Sandboxing
Leaving legacy OS users entirely unsandboxed is highly dangerous, but legacy environments do not support modern sandboxing mechanisms like AppContainers.
*   **The Fix (Stratified Execution Sandboxes)** (Complexity: **High** | Benefit: **High**): Implement an explicit architectural fallback matrix based on runtime OS detection:

| Target OS | Primary Sandbox Mechanism | Security Profile |
|---|---|---|
| **Windows 8.1 / 10 / 11** | AppContainer Isolation Profile | **Maximum** (Restricted Low Integrity) |
| **Windows XP / 7** | Token De-elevation (`CreateRestrictedToken`) + Job Objects | **Moderate** (Blocks Admin/Registry writes, auto-kills processes on close) |
| **Linux** | Landlock + seccomp-bpf | **Maximum** (Restricted filesystem view and syscall surface) |
| **Haiku** | Thread-Confined Memory Domains | **Basic** (Isolated address spaces) |

> **Note on Legacy Windows Security**: By creating a restricted token, stripping away SIDs, and placing the JS/Network processes into a Win32 JobObject with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, you prevent a compromised process from writing to the system directories or surviving a browser crash, even on Windows XP.

### C. Architectural Trade-offs Matrix
Implementing these additions alongside your current 2027 backlog balances out the engineering effort:

| Improvement | Complexity | Benefit | Target Area | Key Beneficiary |
|---|---|---|---|---|
| **User-Space TLS Stack** | Medium | High | Compatibility | Legacy Windows / Alternative OS |
| **Asymmetric Sandboxing** | High | High | Security | Windows XP / 7 Legacy Users |

### D. Strategic Optimization Impact (Vectorized Bottlenecks)

By leveraging Wisp's lightweight architecture alongside modern SIMD vectorization, we can selectively target bottlenecks unique to proxy-centric alternative browsers:

| Expansion Target | Vector Width (AVX2 / NEON / RVV) | Complexity | Benefit | Primary Benefit Area | Architectural Impact |
|---|---|---|---|---|---|
| **WebSocket Masking** | 32 Bytes / 16 Bytes / Variable | Medium | High | Upstream Proxy Network Speed | Eliminates proxy protocol overhead |
| **SIMD JSON Parser** | 32 Bytes / 16 Bytes / Variable | Medium | High | DOM/JS Engine Execution | Drastically speeds up heavy single-page apps |
| **CSS Tokenizer** | 32 Bytes / 16 Bytes / Variable | Medium | High | Layout and Paint Latency | Fast-path scanning for modern utility CSS |

---

## 10. Frontend Implementation Nuances & Dynamic Fallbacks
To ensure the backend rollout is reliable and performant across all hardware/software tiers, several architectural and platform-specific design constraints must be observed:

### A. Windows: Explicit Compile-time & Runtime Selection
Windows compiles allow explicit pipeline choices, coupled with dynamic runtime validation:
1.  **Explicit Compile-time selection**: Controlled by `WISP_WINDOWS_USE_D2D` in CMake to choose either Direct2D (`ON`) or GDI (`OFF`) as the native backend.
2.  **Direct2D Loader**: For Direct2D compiles, dynamically attempt to load the Direct2D library (`d2d1.dll`) via `LoadLibrary` to gracefully handle older systems like Windows XP/Vista.
3.  **Graceful Degeneration**: If initialization fails, fall back to **Blend2D** software rendering (if compiled) or the **GDI** plotter.

### B. Compile-time AsmJit (JIT) Toggle based on SSE2, ARM64 and RISC-V
To guarantee absolute safety on retro hardware, `asmjit` is optionally compiled based on target CPU capabilities:
1.  **x86/x64 SSE2 or ARM64 / RISC-V (Enabled)**: AsmJit or native vector compilers are compiled and linked to emit high-speed vectorized pipelines at runtime.
2.  **Legacy non-SSE2 x86 (Disabled)**: The build system automatically sets `BLEND2D_NO_JIT=ON` when compiling for non-SSE2 architectures (such as targeting Windows XP with `no-sse2` compiler flags). In this state, `asmjit` is completely excluded from compilation, preventing Illegal Instruction crashes on older x86 hardware (e.g., AMD Athlon XP or Pentium III).

### C. Consistent Internal Configuration Mapping
To prevent configuration drift and code bloat, mapped user options bind cleanly to a unified internal rendering enum definition, with the Auto option removed:
```c
typedef enum {
    WISP_RENDER_BACKEND_NATIVE = 0,
    WISP_RENDER_BACKEND_BLEND2D = 2
} WispRenderBackend;
```

### D. Tracking BeOS/Haiku View Implementation File
The primary rendering and event dispatch logic for BeOS/Haiku is centralized under **`frontends/beos/window.cpp`**:
*   Under the default `NATIVE` option, prioritize native **`BView` (AGG)** rendering.
*   Under the optional fallback `OPTION_RENDER_BACKEND_BLEND2D`, use the thread-local **Blend2D** rasterizer inside the `BDirectWindow` frame buffer lock loop.
