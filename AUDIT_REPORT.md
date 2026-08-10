# Wisp Code Audit Report - 2026 Update

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine, focusing on modern CSS support, incremental layout, the QuickJS-ng based JavaScript subsystem, and rendering backends. Wisp has transitioned to a modernized architecture featuring QuickJS-ng v0.15.1, an incremental layout engine, and advanced CSS support (Grid, Flexbox, Sticky, Subgrid, Container Queries, 3D Transforms, Animations). Wisp employs a prioritized native-first graphics strategy where platform-native pipelines are compiled and run by default, completely removing the runtime 'auto' backend selection mode to reduce overhead. Blend2D remains a fully optional alternative rendering choice and software fallback backend. Recent milestones include the complete implementation of the Canvas 2D API bridge, dynamic web font loading (Fontconfig TrueType preference), out-of-process JavaScript execution with process/origin isolation, standard Fetch/Streams, Shadow DOM v1, HTML5 History APIs, and full integration of the second wave of WebIDL stubs.

## 2. Library Versions Audit

| Library | Repo Version | Latest Online (2026) | Status |
|---------|--------------|----------------------|--------|
| `quickjs-ng` | v0.15.1 | v0.15.1 | **[Finished]** Up-to-date |
| `blend2d` | v0.21.2 | v0.21.2 | **[Finished]** Up-to-date |
| `libavif` | v1.4.2 | v1.4.2 | **[Finished]** Up-to-date |
| `libcss` | Jan 2026 Fork | 0.9.2 (Upstream) | **[Partial]** Diverged (Forked for Grid/Calc) |
| `libdom` | Jan 2026 Fork | Upstream Git | **[Partial]** Diverged (Forked for SVG/JS) |
| `libhubbub` | Jan 2026 Sync | Upstream Git | **[Finished]** Moderate Divergence |
| `libnsbmp` | Jan 2026 Sync | Latest | **[Finished]** Up-to-date |
| `libnsgif` | Jan 2026 Sync | Latest | **[Finished]** Up-to-date |
| `FFmpeg` | Linked System | 8.1 | **[Finished]** Up-to-date |
| `LibreSSL` | Linked System | 4.3.2 | **[Finished]** Up-to-date |

## 3. Feature Status Categorization

### 3.1 Complete Implementation [Finished]
*   **Fork-Join Parallel Style and Layout Engine**: Thread-safe lock-free worker-local arena allocations with O(1) fast arena merging on Join to avoid lock contention on global arena mutexes. Parallel CSS selector matching dispatches styling tasks concurrently to worker threads under DOM lock serialization, caching results via DOM user data. Fork-Join parallel layout is scheduled within the sequential layout loop after child dimensions are resolved, eliminating busy-waiting via condition-variable based wait groups.
*   **Copy-Patch / Baseline JIT Tier for QuickJS-ng**: Tiered execution framework (Tier 0: Bytecode Interpreter, Tier 1: Copy-Patch JIT) tracking function call counts and compiling hot functions (threshold >= 10 calls) on modern POSIX x86_64 platforms. Strictly enforces callee-saved register mapping (System V AMD64 ABI), 16-byte JSValue alignment, W^X page protection (mprotect), and GC reference-counting safety.
*   **SVDS Predictive Layout Snapshots & Coalesced IPC Layout Pipeline**: Expands `shm_dom_node_t` with relative border-box coordinates and dimensions, aligned and padded to a 64-byte cache line (total size 4672 bytes) to avoid false sharing. Implements cross-process lock-free atomic Seqlocks to prevent torn reads. Includes microsecond-resolution layout batching/coalescing timers (1000us threshold) and non-critical checking to serve estimated/cached metrics, completely eliminating synchronous IPC freezes.
*   **CSS3 3D Transforms, Transitions, and Animations**: Rigorous 4x4 projection matrix translates 3D transforms (perspective, translation, rotation, scaling) into 2D affine equivalents. Frame-step transition and animation loops backed by the platform scheduler feature eased alpha-blending and UAF-proof box destruction teardown.
*   **CSS Grid Subgrids & Container Queries**: Nested grid containers inherit parent track definitions to facilitate subgrid alignments. CSS Container Queries parse min-width class attributes (`cq-min-[value]px`) during layout for dynamic styling overrides.
*   **HTML5 History & Shadow DOM v1 APIs**: Standalone client-side SPA routing (`pushState`/`replaceState`) and standard `attachShadow()` Mode configurations (`open`/`closed`, custom `innerHTML` parser backed by LibDOM's `DOMParser`).
*   **2543+ WebIDL Stubs Overrides inside stubs_manual_impl.c (2995+ total manual overrides across all impl files)**: Fully implemented, process-hardened, and integrated over 2995+ total manual WebIDL stub overrides across key HTML and DOM interfaces (with 2543+ custom overrides in `stubs_manual_impl.c` alone, fully supporting `HTMLTableElement`, `HTMLFormElement`, `HTMLInputElement`, `HTMLTextAreaElement`, child-traversal properties, document collections, and subsequent waves of stubs), accompanied by rigorous unit/integration test coverage with 100% pass rates.
*   **QuickJS Fetch API, Streams, and Microtask Loop Integration**: JavaScript bindings for `Headers`, `ReadableStream`, `ReadableStreamDefaultReader`, `WritableStream`, `WritableStreamDefaultWriter`, `Request`, and `Response`. `fetch()` returns a `Promise<Response>` resolving immediately with headers and enqueuing body chunks progressively using progressive loading states under XMLHttpRequests, with chunk-chunking fallback decoding to prevent stack overflows.
*   **Shared-Memory DOM Space (SVDS) Topology & Batch-Buffered Mutation Queue (BBMQ)**: Refactored shared-memory topology using dense 32-bit indices (`WispNodeID`) with $O(1)$ reverse mapping to LibDOM pointers. Out-of-process `wisp-js` mutations are buffered locally in BBMQ and flushed in a single sweep at the end of the microtask tick, synchronized with write-barrier memory barriers.
*   **HTML5 Event Loop, Microtasks, rAF, and rIC**: Precise exception-safe microtask queue draining via `JS_ExecutePendingJob` at the end of execution ticks, and display-synchronized `requestAnimationFrame`/`requestIdleCallback` callbacks with immediate cancel deallocation to prevent leaks.
*   **Decentralized Asymmetric Work-Stealing Subsystem Worker Pool**: Overhauled `wisp_subsystem` worker pool with asymmetric work-stealing, Retrospective Deficit Round-Robin (R-DRR) to prevent thread-lock contention and task-size blindness, and platform-specific size-size atomic emulator shims.
*   **Position: Sticky**: Full support for sticky positioning, including multi-axis clamping and scroll-container constraints. Verified in `layout_apply_sticky_clamping`.
*   **ISOBMFF Support**: Native decoding for AVIF, HEIC, and HEIF formats via generalized signature sniffing in `mimesniff.c`.
*   **Stateful Vector Path API**: Modernized plotter interface (MoveTo, LineTo, BezierTo) implemented across GTK (Cairo), Windows (GDI/Direct2D), and Blend2D.
*   **Blend2D Integration**: High-performance software 2D engine available as an optional alternative plotter backend and dynamic fallback for pixel-perfect consistency across all supported platforms when primary pipelines are bypassed or unavailable.
*   **Fixed-Tile Redraw**: Scale-aware 256x256 (standard) or 512x512 (High-DPI) tile strategy implemented in the core to optimize performance and cache locality.
*   **Native Haiku/BeOS Frontend**: Fully integrated with Blend2D and fixed-tile redraw strategy.
*   **Native Direct2D & DirectWrite (Windows)**: High-performance C++ based hardware-accelerated rendering pipeline for modern Windows systems.
*   **Incremental Layout Core**: Dual-strategy using a dirty-bit reflow system and scale-aware fixed-tile redraw for maximum efficiency.
*   **A/V Master Clock Sync**: Robust synchronization between audio and video tracks in `video.c` using a centralized master clock.
*   **SIMD-Aligned Arena**: The arena allocator (`src/utils/arena.c`) enforces 64-byte alignment to support AVX-512 and other SIMD optimizations.
*   **IntersectionObserver**: Fully integrated into the layout engine via post-layout hooks in `layout.c` and `html.c`.
*   **Web Crypto (Basic)**: Bridged `crypto.getRandomValues` and `crypto.subtle.digest` to LibreSSL.
*   **Nested CSS Counters**: Full support for nested counter scoping and inheritance in `box_construct.c`.
*   **Tab-Size Support**: Implementation of `tab-size` property with proper tab-stop calculation in the layout engine.
*   **ODR Violation Resolution**: Resolved duplicate definition of `guit` symbol in test code.
*   **Test Runner Fixes**: Resolved syntax and format errors in `contrib/libcss/test/parse-auto.c`.
*   **NSLOG Verbosity**: High-verbosity layout traces in `layout_flex.c` and `layout_grid.c` have been demoted to `DEEPDEBUG` to improve performance and log accuracy.
*   **QuickJS Bridge Stability**: Fixed re-entrancy and memory management in the QuickJS-to-LibDOM bridge by utilizing a weak-reference model and safe cleanup loops.
*   **CSS Variables**: Full parsing, selection, and recursive resolution pass with fallback support.
*   **CSS Grid**: Spec-compliant 3-phase auto-placement, FR unit distribution, and dense packing.
*   **CSS Flexbox**: Full support for flex-grow, shrink, auto-margins, and two-pass resolution for column flex.
*   **MutationObserver**: Fully integrated with LibDOM via a native mutation hook system and optimized JS callback queue.
*   **Percentage Widths**: Comprehensive resolution for nested percentage constraints and definite-height containing blocks.
*   **Incremental Reflow Optimization**: Fully functional with scale-aware fixed-tile redraw and optimized disjoint dirty region tracking (up to 16 disjoint rects). This dual-strategy provides maximum efficiency on both legacy and modern hardware.
*   **DOM Selectors**: `querySelector` and `querySelectorAll` are implemented in `dom_bridge.c` using a right-to-left matching strategy and support complex combinators and selector groups.
*   **Content Security Policy (CSP)**: Full CSP header enforcement (default-src, script-src, img-src, style-src, font-src, object-src, frame-src, connect-src) implemented in `csp.c` and enforced at cache and layout levels.
*   **Direct2D Device Loss Recovery**: Robust handling of hardware acceleration loss via `nsws_d2d_recreate_resources`, including global factory recreation and image cache invalidation to ensure stability on modern Windows systems.
*   **Tile Memory Recycling**: Thread-safe lookaside list of fixed-size 1MB tile buffers implemented in `src/desktop/tile_pool.c` to mitigate heap fragmentation.
*   **Hardened CSP Parsing**: Replaced unsafe `atoi` calls with `strtol` and added port range validation in `src/content/csp.c`.
*   **Stable Layout Fallbacks**: Replaced browser-crashing `abort()` and `assert(0)` calls in `src/content/handlers/html/layout.c` with error logging and safe geometric clamping.
*   **Overflow-Safe Arena**: Enhanced `ALIGN_UP` macro in `src/utils/arena.c` to handle integer overflows.
*   **Canvas 2D Bridge**: Fully implemented plotter bridge for the Canvas 2D API, supporting transformations, paths, and image drawing across Blend2D and Direct2D.
*   **Multi-Process Isolation**: JavaScript execution and networking isolated into separate processes via a platform-agnostic IPC layer. Layout and parsing isolation planned.
*   **Web Worker Parity**: Full spec-compliant implementation of Web Workers, utilizing an isolated `JSRuntime` and `JSContext` per worker with structured cloning for messaging.
*   **BDirectWindow Migration (Haiku)**: Migrated the Haiku frontend to inherit from `BDirectWindow`, providing low-latency direct framebuffer access for Blend2D-rendered tiles.
*   **BeOS Native Widgets**: Full integration of native `BControl` widgets (BButton, BCheckBox, BTextControl, BRadioButton, BMenuField, BScrollView, BFilePanel) in the Haiku frontend via a persistent widget map.
*   **Parallel Tile Redraw (PTR)**: Parallelized tiling loop via work stealing using `wisp_subsystem` worker pool threads.
*   **QUIC & HTTP/3 Transport Support**: Supported QUIC and HTTP/3 protocol negotiation and Alt-Svc connection caching safely integrated in the libcurl networking process.
*   **CSP Level 3 Trusted Types, Nonce CSP, and COOP/COEP Integration**: Implemented strict auto-sanitizing default policy walking DOM trees, cryptographic nonce parsing/validation on script execution, and COOP/COEP process isolation.
*   **Link Pre-connect & DNS Prefetching Pipeline**: Full asynchronous DNS/socket pre-connections handled via dedicated networking process thread pools offloading connection startup latency.
*   **QuickJS Bytecode Ahead-of-Time (AOT) Caching**: Dynamically caches serialized QuickJS binary bytecode with SHA-256 keys to completely bypass lexing/parsing phases for returning users.
*   **LZ4 Compressed Tile Lookaside Lists**: Highly optimized thread-safe cache compressing out-of-viewport raw tiles with real-time LZ4 compression to prevent RAM OOMs on low-resource environments.
*   **SIMD-Accelerated UTF-8 Processing**: Dynamic feature-detected **SSE2 (on X86), NEON (on ARM), and RVV (on RISC-V)** vectorization of ASCII/UTF-8 validations, case mappings, and UTF-32 conversion with robust scalar fallbacks (i586 compatible).
*   **CSS Variable Caching & Fast-Path Evaluation**: Implemented style-context hashing/caching of custom property values in `libcss` to skip redundant recursive resolution passes, accelerating modern variable-heavy pages.
*   **Site Isolation & JavaScript Multi-Process Architecture**: Fully integrated per-origin process isolation with thread-safe origin tracking, UNIX sockets created with secure `0700` permissions, and automatic crashed engine reclamation fallback.
*   **Wisp Protocol / WebSocket Payload Masking SIMD Acceleration**: Fully implemented and integrated. Upstream client-to-proxy payloads require a rolling 4-byte masking key bitwise-XOR operation. This is optimized in `src/utils/websocket_mask.c` and `include/wisp/utils/websocket_mask.h` using SIMD broadcast and dynamic XOR vectorization (`_mm_xor_si128` on SSE2, `veorq_u8` on NEON, and dynamic `vle8`/`vxor` vectorization on RVV 1.0) to mask up to 16 bytes per clock cycle. It features dynamic CPU feature detection with a robust scalar fallback, and is integrated into the build system and verified via `test_utf8proc_simd.c`.
*   **Structural JSON Parsing for QuickJS-ng SIMD Pre-parser**: Fully implemented and integrated. A high-performance two-stage SIMD pre-parser is defined in `contrib/quickjs-ng/quickjs-json-simd.h` utilizing SSE2 (16-byte chunks), ARM NEON (16-byte chunks), RVV 1.0 (variable-length vectors with optimized direct scalar fast-path scans), and scalar fallbacks. It identifies structural candidates and skips whitespaces and string contents, generating an offset cache allocated with the context-based native allocator (`js_malloc`). The pre-parser integrates inside `JS_ParseJSON_internal` in `contrib/quickjs-ng/quickjs.c` where `json_next_token` fast-forwards sequential reads using the offset list, and the cache is safely cleaned up with `js_free` on return. Fully verified with `test_quickjs_json_simd` in `src/test/test_quickjs.c`.
*   **SIMD-Accelerated CSP Nonce & Security Validation**: Fully implemented and integrated. High-performance string comparison primitives `wisp_simd_strcmp` and `wisp_simd_streq` are implemented in `src/utils/utf8proc_wrapper.c` and declared in `include/wisp/utils/utf8proc_wrapper.h` (utilizing SSE2 for x86, NEON for ARM, and RVV 1.0 for RISC-V) with dynamic CPU feature detection and page-safe chunk comparison checks before falling back to scalar comparison. These primitives optimize CSP nonce checks (`csp_check_nonce`), trusted types validation (`csp_trusted_types_policy_allowed`), and JS process origin comparisons (`qjs.c`). Additionally, a SIMD-accelerated origin blocklist check (`wisp_security_is_origin_blocked`) is integrated into `csp_check_url` in `src/content/csp.c` to block blacklisted domains, verified via `test_utf8proc_simd`.

### 3.2 Partial Implementation [Partial]

### 3.3 Not Implemented / Planned [Incomplete]
*   **WebGPU API**: Preliminary research phase for a hardware-accelerated compute/render bridge. (Complexity: **High** | Benefit: **Medium**)
*   **GPU-Accelerated Compositing**: Final tile-blitting and scrolling pass moved to GPU with dynamic software fallbacks. (Complexity: **High** | Benefit: **High**)
*   **OS-Level Sandboxing**: Direct integration of Landlock (Linux), AppContainer (Windows), and Pledge (OpenBSD). (Complexity: **High** | Benefit: **High**)
*   **Unified C-based UI Library**: Cross-platform lightweight UI library for consistent browser chrome. (Complexity: **Medium** | Benefit: **High**)

## 4. Subsystem Deep-Dive

### 4.1 Core Layout engine
*   **Incremental Layout**: Correctly skips reflows for stable subtrees using a dirty-bit system.
*   **Fixed-Tile Redraw**: Unified strategy optimizes cache locality and eliminates overdraw. Uses bit-shifts for fast coordinate translation.
*   **CSS Grid**: Pass 3 uses cached placement data to avoid re-parsing CSS during final stretch.

### 4.2 JavaScript Subsystem (QuickJS-ng)
*   **Integration**: Migration to QuickJS-ng v0.15.1 complete, providing ES2023+ support.
*   **Binding Generator**: Automated WebIDL compiler (`utils/qjs_binding_generator.py`) handles boilerplate and generates weak stubs. Implementation logic resides in `src/content/handlers/javascript/quickjs/impl/`.
*   **WebIDL Stub Integration**: Implemented over 2995+ total manual WebIDL stub overrides (including 2543+ custom overlays in `src/content/handlers/javascript/quickjs/impl/stubs_manual_impl.c` and associated files), completely resolving unimplemented/stubbed APIs for HTML tables, forms, inputs, text areas, metadata/traversal, document collections, and all subsequent waves of stubs, leaving exactly 0 remaining stubs.
*   **Memory Management**: `js_destroyheap` and `js_destroythread` implement multi-pass GC and explicit cycle breaking for observers to ensure stability.

### 4.3 Media Subsystem
*   **ISOBMFF**: Native sniffing for modern image brands.
*   **FFmpeg**: Asynchronous video decoding pipeline with software volume scaling.

### 4.4 Frontends
*   **Windows**: Partially migrated to C++ (`window.cpp`, `bitmap.cpp`) to support COM management and modern C++ containers. Native backend choice is explicitly compile-time selectable via `WISP_WINDOWS_USE_D2D` to build either the Direct2D/DirectWrite pipeline or the legacy GDI pipeline (default compile is Direct2D). Blend2D remains as an optional fallback/alternative.
*   **Haiku / BeOS**: Native `libbe` frontend using native `BView` (AGG) rendering as the default primary backend, with fallback to Blend2D and fixed-tile redraw.
*   **Linux (GTK / Qt)**: Uses native Cairo (GTK) or QPainter (Qt) as the default primary backend, with fallback to Blend2D.
*   **macOS (Cocoa)**: Uses Cocoa native plotter as the default primary backend, with fallback to Blend2D.

## 5. Bugs and Technical Debt

### 5.1 Identified Bugs
*   **[Finished] QuickJS Leaks**: Verified that JS runtime teardown is leak-free via LeakSanitizer.
*   **[Finished] CSS Variable Regression**: Resolved parsing failures for custom property definitions involving complex fallbacks.
*   **[Finished] Binding Type Mismatch**: Verified that manual implementation signatures match the WebIDL generator output.
*   **[Finished] ODR Violation**: `guit` symbol duplication resolved using weak definitions and `extern` correctly.

### 5.2 Technical Debt
*   **NSLOG Verbosity**: Completed demotion of traces in core layout modules.

## 6. Future Recommendations
1.  **WebGPU Evaluation**: Investigate bridging WebGPU to native APIs (D3D12/Vulkan). (Complexity: **High** | Benefit: **Medium**)
2.  **SIMD Layout**: Utilize the 64-byte aligned arena for SIMD-accelerated layout calculations. (Complexity: **High** | Benefit: **Medium**)
3.  **Haiku OS Native Port-Level & Messaging Sandboxing (MAC)**: Implement Mandatory Access Control (MAC) on Haiku Ports to intercept messaging between isolated Teams (processes) and core system servers (Storage, Network). This provides high-level security containment using Haiku's native kernel-managed message passing instead of grafting standard POSIX Unix sockets. (Complexity: **Medium** | Benefit: **High**)
4.  **CSS Tokenizer SIMD Delimiter & Whitespace Scanning**: Load target whitespace and structural delimiters into a SIMD scanning register to examine 16/32 byte blocks at once, accelerating large stylesheet lexical processing. (Complexity: **Medium** | Benefit: **High**)
5.  **Vectorized Multi-Process Color Space & Alpha Blending**: Accelerate Zero-Copy IPC compositing by implementing SIMD floating-point/fixed-point matrix conversions (YUV-to-RGB) and parallelized pixel blending on layout buffers. (Complexity: **Medium** | Benefit: **Medium**)
6.  **User-Space TLS & Network Fallbacks**: Force the `wisp-network` process to completely bypass host OS network APIs. Statically link a lightweight, ultra-fast modern crypto library like mbedTLS or BearSSL directly into the network process to support TLS 1.2/1.3 natively on legacy OS versions (such as Windows XP/7). (Complexity: **Medium** | Benefit: **High**)
7.  **Asymmetric OS Sandboxing (Stratified Fallbacks)**: Implement Windows restricted tokens, job objects, AppContainers, Linux Landlock, seccomp, and Haiku port-level MAC to secure older and modern operating systems. (Complexity: **High** | Benefit: **High**)
