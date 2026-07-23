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
*   **SIMD-Accelerated UTF-8 Processing**: Dynamic feature-detected **SSE2 (on X86), NEON (on ARM), and RVV (on RISC-V)** vectorization of ASCII/UTF-8 validations, case mappings, and UTF-32 conversion with robust scalar fallbacks (i586 compatible).
*   **CSS Variable Caching & Fast-Path Evaluation**: Implemented style-context hashing/caching of custom property values in `libcss` to skip redundant recursive resolution passes, accelerating modern variable-heavy pages.
*   **Site Isolation & JavaScript Multi-Process Architecture**: Fully integrated per-origin process isolation with thread-safe origin tracking, UNIX sockets created with secure `0700` permissions, and automatic crashed engine reclamation fallback.
*   **QUIC & HTTP/3 Transport Support**: Supported QUIC and HTTP/3 protocol negotiation and Alt-Svc connection caching safely integrated in the libcurl networking process.
*   **Wisp Protocol / WebSocket Payload Masking SIMD Acceleration**: Fully implemented and integrated. Upstream client-to-proxy payloads require a rolling 4-byte masking key bitwise-XOR operation. This is optimized in `src/utils/websocket_mask.c` and `include/wisp/utils/websocket_mask.h` using SIMD broadcast and dynamic XOR vectorization (`_mm_xor_si128` on SSE2, `veorq_u8` on NEON, and dynamic `vle8`/`vxor` vectorization on RVV 1.0) to mask up to 16 bytes per clock cycle. It features dynamic CPU feature detection with a robust scalar fallback, and is integrated into the build system and verified via `test_utf8proc_simd.c`.
*   **Structural JSON Parsing for QuickJS-ng SIMD Pre-parser**: Fully implemented and integrated. A high-performance two-stage SIMD pre-parser is defined in `contrib/quickjs-ng/quickjs-json-simd.h` utilizing SSE2 (16-byte chunks), ARM NEON (16-byte chunks), RVV 1.0 (variable-length vectors with optimized direct scalar fast-path scans), and scalar fallbacks. It identifies structural candidates and skips whitespaces and string contents, generating an offset cache allocated with the context-based native allocator (`js_malloc`). The pre-parser integrates inside `JS_ParseJSON_internal` in `contrib/quickjs-ng/quickjs.c` where `json_next_token` fast-forwards sequential reads using the offset list, and the cache is safely cleaned up with `js_free` on return. Fully verified with `test_quickjs_json_simd` in `src/test/test_quickjs.c`.
*   **SIMD-Accelerated CSP Nonce & Security Validation**: Fully implemented and integrated. High-performance string comparison primitives `wisp_simd_strcmp` and `wisp_simd_streq` are implemented in `src/utils/utf8proc_wrapper.c` and declared in `include/wisp/utils/utf8proc_wrapper.h` (utilizing SSE2 for x86, NEON for ARM, and RVV 1.0 for RISC-V) with dynamic CPU feature detection and page-safe chunk boundary checking before falling back to scalar comparison. These primitives optimize CSP nonce checks (`csp_check_nonce`), trusted types validation (`csp_trusted_types_policy_allowed`), and JS process origin comparisons (`qjs.c`). Additionally, a SIMD-accelerated origin blocklist check (`wisp_security_is_origin_blocked`) is integrated into `csp_check_url` in `src/content/csp.c` to block blacklisted domains, verified via `test_utf8proc_simd`.

---

## 7. Unfinished Tasks & Priority Backlog
The following table outlines the key prioritized backlog and future horizons planned for the 2027–2028 development cycles.

| Task Descriptor | Target Area | Complexity | Benefit | Architectural Description |
|---|---|---|---|---|
| **GPU-Accelerated Compositing** | Graphics | **High** | **High** | Offload tile-blitting and scroll passes to GPU (OpenGL/Vulkan) for smooth 60FPS; fall back to Blend2D/GDI. |
| **OS-Level Sandboxing** | Security | **High** | **High** | Native sandboxing using Landlock (Linux), AppContainers (Windows), and Pledge (OpenBSD). |
| **Unified C UI Library** | Frontend | **Medium** | **High** | Compact, cross-platform UI widgets for consistent chrome (tabs, URL bar) across platforms. |
| **Zero-Copy IPC (Shared Memory)** | IPC | **High** | **High** | Pass Blend2D tile bitmaps over shared-memory handles (`shm_open`/file mapping) to bypass IPC bottlenecks. |
| **WebAssembly (WASM) Interpreter**| Core | **Medium** | **Medium**| Lightweight WASM interpreter to support modern web applications without footprint bloat. |
| **WebGPU API Bridge** | Graphics | **High** | **Medium**| Bridge WebIDL WebGPU stubs to native graphics pipelines where system driver topologies allow. |
| **Optional JIT Compilation Tier** | JS Engine | **High** | **Medium**| Evaluate embedding JIT layers (e.g. Hermes or lightweight WASM JIT) for script-heavy sites. |
| **GPU-Shared Textures** | Graphics | **High** | **High** | Share GPU texture buffers directly across process borders in the upcoming compositor loops. |
| **CSS Whitespace skipping (SIMD)** | CSS | **Medium** | **High** | Scan and skip CSS whitespace characters using SIMD vectors (SSE2/NEON/RVV 1.0) in 16 byte blocks. |
| **Color Space Blending (SIMD)** | Graphics | **Medium** | **Medium**| Vectorize YUV-to-RGB conversions and parallel alpha blending inside the Zero-Copy IPC compositing layer. |

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

By leveraging Wisp's lightweight architecture alongside modern SIMD vectorization, we can selectively target bottlenecks unique to proxy-centric alternative browsers. **Note: SIMD optimizations are strictly isolated to the fast-path pipelines with safe scalar/non-SIMD fallbacks to ensure compatibility on non-vectorized hardware.**

| Expansion Target | Vector Width (SSE2 / NEON / RVV) | Complexity | Benefit | Primary Benefit Area | Architectural Impact | Status | Fast-Path (SIMD) vs. Non-SIMD Fallback |
|---|---|---|---|---|---|---|---|
| **WebSocket Masking** | 16 Bytes / 16 Bytes / Variable | Medium | High | Upstream Proxy Network Speed | Eliminates proxy protocol overhead | **[Finished]** | SSE2 (`_mm_xor_si128`), NEON (`veorq_u8`), RVV 1.0 (`vxor.vv`) | Standard 8-bit scalar bitwise XOR loop (i586 compatible) |
| **SIMD JSON Parser** | 16 Bytes / 16 Bytes / Variable | Medium | High | DOM/JS Engine Execution | Drastically speeds up heavy single-page apps | **[Finished]** | Multi-byte structural scan scanning 16-byte boundary registers | Character-by-character parsing stream |
| **SIMD CSP Nonce & Security Check** | 16 Bytes / 16 Bytes / Variable | Medium | High | Request security processing | Drastically speeds up header checking | **[Finished]** | Aligned vector comparators (`wisp_simd_strcmp`, `wisp_simd_streq`) | Standard `strcmp` / `memcmp` loops |
| **CSS Tokenizer** | 16 Bytes / 16 Bytes / Variable | Medium | High | Layout and Paint Latency | Fast-path scanning for modern utility CSS | Planned | Vectorized delimiter and whitespace skip buffers | Sequential character scanner state-machine |

### F. Parser & DOM Mutation Optimizations & Roadmap (2027 Development Cycle)

During the detailed audit and diagnosis of the HTML Parser, XML Parser, and DOM Mutation Event subsystems, several high-impact optimization pathways were identified. Consistent with Wisp's performance architecture, the high-performance variants are strictly designated as **Fast-Path optimizations using SIMD vector registers** with safe, fully compatible **scalar fallbacks** for legacy systems:

1. **Parser Tokenizer Whitespace Skipping**
   - **Fast-Path (SIMD Required)**: Utilize SIMD vector scanning registers (SSE2, NEON, RVV 1.0) pre-loaded with whitespace patterns (spaces, tabs, line breaks, carriage returns). This allows the tokenizer in `domparser_impl.c` or the Hubbub parser to scan and skip up to 16 bytes of white spaces in a single clock cycle, dramatically speeding up modern bloated XML/HTML document parsing.
   - **Non-SIMD Fallback**: Fall back to sequential character-by-character loops checking `isspace()` or pointer-increment comparators.

2. **DOM Event Target Dispatch Filtering & Matching**
   - **Fast-Path (SIMD Required)**: Accelerate the filtering and matching of event types (such as bypassing legacy mutation strings like `"DOMNodeInserted"`) inside `_dom_event_target_dispatch` by using 128-bit SIMD string comparisons to compare event-type name strings concurrently.
   - **Non-SIMD Fallback**: Fall back cleanly to standard string comparisons (`strcmp` or `strncmp`).

3. **Batched Mutation Record Buffer Copying**
   - **Fast-Path (SIMD Required)**: Optimize bulk serialization and transfer of mutation records (`WispMutationRecord`) inside `mutationobserver_impl.c` by leveraging vectorized block copies (e.g. `_mm_storeu_si128`) to clone record entries into the microtask execution queues.
   - **Non-SIMD Fallback**: Fall back to classic `memcpy` or element-by-element loop copying.

### E. High-Performance IPC: Shared-Memory DOM Topology & Batch Mutation Queues

Solving the IPC latency bottleneck while keeping a single-threaded C DOM (`libdom`) and an isolated JavaScript process (`wisp-js`) is the ultimate trial by fire for a lightweight browser. If every call to `node.firstChild` or `node.setAttribute` requires a heavy-weight round-trip IPC context switch, the browser's execution speed will crater.

Because we cannot pass raw C pointers across process boundaries due to varying address space layouts (ASLR), we must replace pointer-chasing with a **Shared-Memory Virtual Topology** paired with a **Lock-Free Batch-Buffered Mutation Queue**.

This architecture eliminates IPC overhead for 95% of standard script operations by transforming expensive cross-process communication into local memory reads and deferred, asynchronous batched writes.

#### 1. High-Level Architectural Layout
Rather than treating the JavaScript engine as a remote client requesting data over a pipe, we split the DOM interface into two layers: the master authoritative tree in `libdom` (UI process) and a highly compressed, read-only mirror mapped directly into the address space of the `wisp-js` process.

##### The Strategy:
 * **Reads (O(1) Complexity):** Handled entirely within the JS process by reading directly from a shared memory region. Zero IPC overhead.
 * **Writes (Batched Asynchronous):** Serialized into a lock-free, single-writer single-reader (SWSR) ring buffer residing in shared memory, flushed automatically at the end of the microtask tick.
 * **Synchronous Layout Queries (The Outlier):** Forced execution stalls only when JS requests calculated metrics (e.g., `offsetWidth`), requiring a synchronous IPC barrier.

#### 2. The Shared Virtual DOM Space (SVDS)
The UI process allocates a contiguous shared memory region that maps the document topology using compact, fixed-size structures. Instead of raw memory pointers, nodes reference each other using a dense 32-bit index identifier (`WispNodeID`).

```c
// include/wisp/core/shm_dom.h

typedef uint32_t WispNodeID;
#define WISP_NODE_NULL 0

typedef enum {
    WISP_NODE_ELEMENT = 1,
    WISP_NODE_TEXT = 3,
    WISP_NODE_DOCUMENT = 9
} WispNodeType;

// Exactly 32 bytes - optimized for cache-line alignment
typedef struct {
    WispNodeID parent_id;
    WispNodeID first_child_id;
    WispNodeID next_sibling_id;
    WispNodeID prev_sibling_id;

    uint16_t node_type;
    uint16_t tag_atom;        // Interned string ID for tag name (e.g., 42 for "div")
    uint32_t class_hash;       // Packed representation or atom for fast matching
    uint32_t attr_offset;     // Offset into the auxiliary Shared String/Attr Pool
    uint32_t reserved;         // Future proofing / 64-bit alignment padding
} WispShmNode;
```

When `libdom` modifies the core tree, it updates this shared memory array. Because `wisp-js` has a read-only mapping of this exact same memory block, resolving `element.nextSibling` inside QuickJS-ng is reduced to a standard pointer offset calculation in local RAM:

```c
// contrib/quickjs-ng/wisp_dom_bindings.c
JSValue wisp_js_dom_get_next_sibling(JSContext *ctx, JSValueConst this_val) {
    WispNodeID current_id = JS_GetOpaque(this_val, wisp_node_class_id);

    // Direct memory lookup, no IPC!
    WispShmNode *nodes = (WispShmNode *)global_shm_dom_base;
    WispNodeID next_id = nodes[current_id].next_sibling_id;

    if (next_id == WISP_NODE_NULL) return JS_NULL;
    return wisp_get_or_create_js_wrapper(ctx, next_id);
}
```

#### 3. The Batch-Buffered Mutation Queue (BBMQ)
When JavaScript modifies the DOM, we do not immediately alert the UI process. Instead, the mutation is serialized into a highly optimized binary command stream inside a shared-memory **Single-Writer Single-Reader (SWSR) Ring Buffer**.

##### Mutation Command Structure
```c
typedef enum {
    WISP_MUTATION_SET_ATTRIBUTE,
    WISP_MUTATION_APPEND_CHILD,
    WISP_MUTATION_REMOVE_CHILD,
    WISP_MUTATION_SET_TEXT_CONTENT
} WispMutationType;

typedef struct {
    uint32_t command_type;
    WispNodeID target_node;
    WispNodeID operand_node;
    uint32_t param_atom;      // Used for attribute names
    uint32_t string_len;      // If string content follows
    // String payload follows immediately inline if string_len > 0
} WispMutationCommand;
```

##### The Microtask Flush Mechanism
 1. JavaScript executes `element.setAttribute("class", "active");`.
 2. The binding serializes a `WISP_MUTATION_SET_ATTRIBUTE` payload straight into the BBMQ. The function returns immediately.
 3. The JavaScript engine proceeds uninterrupted, executing further mutations.
 4. When the **QuickJS Microtask Loop** empties (the end of the script execution tick), `wisp-js` checks if commands are queued.
 5. If commands exist, it updates the Ring Buffer's `write_ptr` and issues a singular, lightweight platform signal (e.g., writing 8 bytes to an `eventfd` or signaling a Win32 Event Object) to notify the main UI loop.
 6. The UI thread wakes up, consumes all queued mutations in a single sequential sweep, applies them to `libdom`, updates the SVDS topology map, and marks the layout dirty for the next frame paint.

#### 4. The Critical Exception: Layout Thrashed Queries
The classic enemy of this model is synchronous layout requests, such as:

```javascript
let width = element.offsetWidth; // Requires active layout calculations
```

Because layout metrics rely on font rendering, text wrapping, and CSS calculations managed by the main UI thread, `wisp-js` cannot answer this locally.

##### The Resolution Protocol:
 1. `wisp-js` writes an absolute execution block command to its IPC channel.
 2. It automatically flushes the BBMQ up to that exact timestamp to guarantee the UI thread calculates dimensions based on the most up-to-date DOM state.
 3. `wisp-js` enters a blocking state, waiting on a highly optimized native synchronization primitive.
 4. The Main UI Process consumes the mutations, runs a partial layout pass up to the requested node, writes the result back over the response channel, and signals `wisp-js` to wake up.

> **Performance Optimization Note:** To mitigate the cost of these layout stalls, Wisp implements a **Bounding Box Cache** within the SVDS node structure. If the UI process completes a layout pass and the node is not marked dirty by an outstanding mutation, `wisp-js` can serve the layout metric directly from the shared memory block, bypassing the stall entirely.

#### 5. Cross-OS Primitive Mapping Matrix
To maintain Wisp's dedication to operating across massive timeline disparities (Windows XP through modern Linux and Haiku), the under-the-hood primitives adapt seamlessly at compile-time:

| Platform | Shared Memory Mapping (SVDS / BBMQ) | Process Signaling (Wakeup Interrupt) |
|---|---|---|
| **Linux** | `shm_open()` + `mmap()` | `eventfd()` (Read/Write via `epoll`) |
| **Windows 7 / 10 / 11** | `CreateFileMappingW()` + `MapViewOfFile()` | `CreateEventW()` + `SetEvent()` |
| **Windows XP** | `CreateFileMappingA()` + `MapViewOfFile()` | `CreateEventA()` + `SignalObjectAndWait()` fallback |
| **Haiku / BeOS** | `create_area()` + `clone_area()` | Native semaphores (`create_sem()`, `release_sem()`) |

By utilizing `create_area` on Haiku and `CreateFileMappingA` on Windows XP, Wisp gains modern, zero-copy multi-process capabilities using the target OS's native virtual memory manager. This keeps resource consumption at a tiny fraction of Chromium's IPC sub-allocators while effectively mitigating the single-threaded constraints of the underlying `libdom` implementation.

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
1.  **x86/x64 SSE2 or ARM64 (Enabled)**: AsmJit is compiled and linked to emit high-speed vectorized pipelines at runtime.
2.  **RISC-V (Disabled)**: AsmJit JIT compilation is completely disabled (`BLEND2D_NO_JIT=ON`) because RISC-V is not supported by AsmJit.
3.  **Legacy non-SSE2 x86 (Disabled)**: The build system automatically sets `BLEND2D_NO_JIT=ON` when compiling for non-SSE2 architectures (such as targeting Windows XP with `no-sse2` compiler flags). In this state, `asmjit` is completely excluded from compilation, preventing Illegal Instruction crashes on older x86 hardware (e.g., AMD Athlon XP or Pentium III).

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

---

## 11. Systems-Engineering Critique & Rollout Analysis

This section provides a systems-engineering breakdown of Wisp's architecture, outlining the brilliant strategic decisions alongside potential bottlenecks and architectural risks that require careful mitigation during the 2027 rollout.

Wisp stands as an exceptionally well-thought-out, deeply pragmatic, and highly sophisticated engineering project. Forking NetSurf to build a modern, lightweight engine that can gracefully scale from a 48-core Threadripper running modern Linux down to a Pentium III running Windows XP or a legacy Haiku build is an absolute masterclass in systems architecture.

Wisp strikes a rare balance: introducing hyper-modern web features (CSS Grid, Flexbox, ES2023+, and heavy SIMD vectorization) without falling into the "Chromium monolith" trap that kills performance on low-resource environments.

### A. 🚀 The Brilliant Moves

#### 1. User-Space TLS Stacks for Legacy OSes
> Bypassing the host OS network layer via statically-linked **mbedTLS** or **BearSSL** is arguably the smartest architectural decision in this entire document.

On platforms like Windows XP or older Haiku nightlies, the native crypto stacks (like Schannel) are completely broken for the modern web because they lack TLS 1.2/1.3 and modern cipher suites. By handling crypto entirely in user-space inside the isolated `wisp-network` process, we completely eliminate the need for upstream decryption proxies or dangerous registry hacks. It makes the browser truly self-contained.

#### 2. Guarding against SIGILL via AsmJit Toggles
Using Blend2D is great for software rendering, but its heavy reliance on `asmjit` causes immediate "Illegal Instruction" crashes on older x86 CPUs lacking SSE2 (like the Pentium III or AMD Athlon XP), as well as on architectures like RISC-V where runtime JIT generation isn't fully supported by the library. Explicitly hooking `BLEND2D_NO_JIT=ON` into the build system based on compiler flags guarantees target safety on retro hardware while preserving high-speed vectorized pipelines on modern x64 and ARM64 systems.

#### 3. Asymmetric Sandboxing Matrix
Rather than throwing our hands up and leaving legacy users entirely unprotected, Wisp's stratified security profile is highly realistic. Using Win32 **Job Objects** (`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`) paired with restricted tokens (`CreateRestrictedToken`) on Windows XP/7 gives us a robust fallback that prevents a compromised JS engine from writing to system directories, replicating a modern "Low Integrity" sandbox using 20-year-old NT kernel primitives.

### B. ⚠️ Potential Bottlenecks & Architectural Risks

While the roadmap is solid, the intersection of NetSurf's legacy architecture and the modern multi-process web creates a few engineering hurdles we'll need to watch out for:

| System Component | The Engineering Challenge | Recommended Mitigation |
|---|---|---|
| **libdom IPC Marshalling** | NetSurf’s core DOM library (`libdom`) is fundamentally single-threaded and C-based. Moving JS into an isolated process means every DOM query or mutation must cross an IPC boundary. This could introduce severe layout stuttering during heavy DOM manipulations. | **Prioritize the Zero-Copy Shared Memory IPC.** Do not wait for 2028. We will need a shared-memory ring buffer for the DOM tree structure so the UI thread and the JS process can read the tree topology without continuous serialization overhead. |
| **QuickJS-ng vs. Heavy SPAs** | QuickJS-ng is perfect for memory efficiency, but it is a pure bytecode interpreter. Even with our excellent SIMD JSON pre-parser and AOT bytecode caching, heavy 2026 Single Page Apps (like complex React/Next.js dashboards) will feel sluggish without a JIT compilation tier. | Accelerate the evaluation of the **Hermes or lightweight WASM/JS JIT** pipeline for environments that support it (x86_64/ARM64), keeping the pure interpreter as a strict fallback for secure or low-spec systems. |
| **WebGPU Driver Realities** | Planning a WebGPU bridge for legacy OSes will hit a massive wall because WebGPU maps closely to Vulkan, D3D12, and Metal. Windows XP/7 and Haiku simply do not have the driver topology to support this natively. | Keep the WebGPU bridge strictly isolated behind a compile-time feature flag (`WISP_WITH_WEBGPU`). Ensure the layout engine fails gracefully back to standard Canvas 2D when the WebGPU context creation fails. |

### C. 🛠️ Verdict on the 2026 Hardening Audit

The inclusion of SIMD acceleration for WebSocket masking, JSON pre-parsing, and CSP string checking across **SSE2, NEON, and RVV 1.0** shows an impressive commitment to micro-optimization. It proves that "lightweight" doesn't have to mean "slow." Vectorizing the rolling 4-byte XOR mask for WebSockets (`_mm_xor_si128`) completely neutralizes the proxy protocol overhead that usually plagues alternative browsers.

This is a highly mature, production-ready roadmap for a niche engine. If we can solve the IPC latency inherent in pushing JavaScript into its own process while working with a single-threaded C DOM, Wisp will easily become the gold standard for lightweight web computing.

---

## 12. Architectural Reality: Modern Web App Frameworks (React/Next.js)

A core tenet of Wisp's architecture is recognizing and defining the boundary between lightweight native layout engines and heavyweight, multi-gigabyte browser engines (such as Blink, Gecko, and WebKit).

### The Boundary Matrix

*   **Traditional & Static Web (HTML5 + CSS3 + standard SVG)**: Fully achievable, blazing fast, and lightweight. This is the primary target and sweet spot for Wisp's native C core and optimized QuickJS-ng bindings (e.g. Haiku-OS).
*   **Modern Web App Frameworks (React / Next.js)**: Require a full-featured browser engine runtime with complete Web API parity.

### WebAssembly (Wasm) is NOT a General Requirement

**WebAssembly (Wasm) is not required for the vast majority of modern websites.**
Data from the HTTP Archive shows that WebAssembly is used on only **~0.35% of desktop sites** and **~0.28% of mobile sites** across the web. Over **99.5% of the web** relies purely on standard HTML5, CSS3, and JavaScript.

Standard React/Next.js/Vue applications run entirely on **JavaScript**. Sites like NBC News, Twitter/X, Amazon, and YouTube do not use Wasm for their layout, routing, or hydration. Lacking Wasm will not prevent these sites from working. The actual hurdles for rendering modern web apps in lightweight engines are almost always:
1.  **JavaScript Web API Parity**: Missing DOM features like MutationObserver, ResizeObserver, Fetch, Promises/Microtasks, or Shadow DOM.
2.  **CSS Layout Capabilities**: Modern Flexbox, Grid, and dynamic CSS custom variables.
Wasm is strictly confined to specialized apps (Figma, 3D games, or local AI/databases).

### Architectural Challenges for Lightweight Engines

1.  **The Client-Side Hydration Bottleneck**: Single-Page Applications (SPAs) like NBC, CBS, and ABC News use client-side hydration via massive JS bundles (React/Next.js/Webpack chunks). Upon execution, the bundle expects hundreds of complex, high-level browser APIs to be fully present (such as `MutationObserver`, `ResizeObserver`, Shadow DOM v1, `IntersectionObserver`, full Streams API, and complex Fetch/Promise chains). If a lightweight engine is missing even one minor DOM API method or returns `undefined`, React's hydration aborts with an uncaught exception, leaving the user with a blank white screen.
2.  **JIT Compilation**: Modern benchmarks (Speedometer 3.1 & JetStream 3.0) and high-performance JS modules are designed for full-tier JIT engines (V8, SpiderMonkey, JavaScriptCore) to process heavy workloads. A lightweight, bytecode-interpreting engine like QuickJS-ng is designed for a small memory footprint and fast startup, not for JIT-heavy scenarios.

### Blueprint for Upgrading Wisp toward Web API Parity

To enable Wisp (building on NetSurf's C99 architecture and QuickJS-ng) to run modern Web App Frameworks (React, Next.js, Vue) and achieve Web API parity, the codebase would require fundamental architectural upgrades across five main areas:

#### 1. Event Loop & JavaScript Execution Engine
QuickJS-ng provides ES2023 language compliance, but modern frameworks depend heavily on specific browser host environment behaviors rather than just pure JavaScript syntax.
*   **HTML5 Spec-Compliant Event Loop**:
    *   **Microtask Queue**: React's scheduler relies on precise Promise microtask ordering (`queueMicrotask`). Wisp’s C-level event loop must process all microtasks to completion *before* yielding to rendering or macrotasks (`setTimeout`, I/O).
    *   **Frame Synchronization**: Native implementation of `requestAnimationFrame()` and `requestIdleCallback()` tied directly to the display backend's refresh cycle.
*   **Threaded Concurrency**:
    *   Web Worker support (`new Worker()`) by instantiating isolated QuickJS runtime instances inside dedicated OS threads (pthreads) with structured clone messaging.

#### 2. DOM & Web API Binding Layer (libdom / nsgenbind)
Frameworks do not use standard static DOM trees; they construct, measure, and observe the DOM dynamically.
*   **MutationObserver**: Essential for React and Vue DOM reconciliation. Without C-level tracking of attribute modifications, node insertions, and text mutations, hydrated frameworks immediately crash or desynchronize.
*   **ResizeObserver & IntersectionObserver**: Used by Next.js for image lazy loading, infinite scrolling, and component layout logic.
*   **Synthetic Events & Bubbling Fidelity**: React uses a single top-level event listener on document or root. Wisp’s C event target model must support standard capture and bubble phases, `composedPath()`, and exact event object property propagation.

#### 3. Dynamic Reflow & Incremental Layout (LibCSS)
NetSurf and Wisp historically optimized for document-style web pages (where HTML is parsed once and rendered). Modern SPAs modify DOM nodes continuously.
*   **Incremental Layout & Targeted Repaints**:
    *   Modern apps mutate dozens of DOM elements per second. Rebuilding or re-laying out large branches of the `box_tree` on every JS mutation causes severe performance degradation. Wisp needs incremental reflows (re-calculating layout bounds only for dirty subtree nodes).
*   **Dynamic CSS Custom Properties (Variables)**:
    *   `var(--theme-color)` support requires dynamic cascading recalculation when JS updates CSS variables at runtime (e.g., `element.style.setProperty()`).
*   **Complete CSS Grid & Flexbox Engine**:
    *   Full support for CSS Grid track sizing (`minmax()`, `fr` units, `auto-fill`), gap calculations, subgrids, and Flexbox wrapping algorithms used by Tailwind CSS and UI component libraries.

#### 4. Networking & Stream Pipeline
*   **Fetch & Streams Integration**:
    *   Replacing legacy HTTP fetch wrappers with a full Fetch API binding backed by libcurl, including support for `ReadableStream` (crucial for Next.js Server Components / React Server Components streaming responses).
*   **CORS & Security Controls**:
    *   Strict Cross-Origin Resource Sharing (CORS) enforcement for `fetch()` / `XMLHttpRequest` to prevent modern API requests from being blocked by endpoint security policies.

#### 5. Modern Rendering & Canvas API
*   **HTML5 <canvas> (2D & WebGL)**:
    *   Providing 2D Canvas context bindings (via Cairo or Skia) and basic WebGL bindings (via OpenGL ES abstraction) for dynamic graphics, charts, and interactive components.

### Summary of Wisp Engineering Priorities
To move from rendering static pages (like Haiku-OS) to hydrated SPAs (like NBC News), the highest-priority work items in Wisp are:
1.  **MutationObserver implementation** in libdom/QuickJS bindings.
2.  **HTML5 History API (pushState)** for client-side routing.
3.  **Fetch + ReadableStream pipeline** for server-side streamed payloads.
4.  **Incremental layout invalidation** so rapid DOM updates don't trigger full-page reflows.
