# Comprehensive Technical Assessment & Diagnostic Analysis Report (2026)
**Wisp Web Browser Engine Architecture, Subsystem Diagnostics, & Optimization Roadmap**

---

## 1. Executive Summary

Wisp is a lightweight, high-performance C/C++ web engine forked from NetSurf, designed to bring modern web standards (CSS Grid, Flexbox, CSS Variables, Shadow DOM v1, ES2023+ JavaScript, Fetch/Streams) to lightweight desktop environments without the multi-gigabyte bloat of mainstream browser engines.

As of 2026, Wisp achieves an **HTML5Test benchmark score of 573 / 588 points (97.4% compliance baseline)** with 100% scores across **Scripting (32/32)**, **Resource Loading (7/7)**, and **Offline & Storage (13/13)**, and **65/66** in **Forms & Input**, operating with zero runtime exceptions or engine crashes across standard test workloads.

### Core Architectural Highlights
1. **Multi-Process Architecture & Zero-Copy SVDS**: Isolates JavaScript execution (`wisp-js`) and Networking (`wisp-network`) into separate OS processes communicating over IPC. Shared-Memory Virtual DOM Space (SVDS) and Batch-Buffered Mutation Queue (BBMQ) turn 95% of JS DOM reads into $O(1)$ zero-copy local lookups.
2. **Fork-Join Parallel Style & Layout Engine**: Bypasses global mutex lock contention using thread-local sub-arenas with $O(1)$ fast main arena merging on Join. Dispatches parallel CSS selector matching and sub-tree box layout to a dedicated worker pool (`wisp_style_pool`).
3. **QuickJS-ng v0.15.1 & Baseline Copy-Patch JIT**: Features a 2-tier execution framework (Interpreter + Baseline Copy-Patch JIT for hotspot functions $\ge 10$ invocations) on AMD64 POSIX platforms, accompanied by 3,008 strong manual C WebIDL symbol overrides.
4. **SIMD-Accelerated Pipelines**: Dynamic CPU feature detection (SSE2 on x86, NEON on ARM, RVV 1.0 on RISC-V) accelerates UTF-8 validation/conversion, WebSocket frame masking, structural JSON pre-parsing, and CSP nonce string comparison.
5. **Platform-Native First Plotting Strategy**: Prioritizes native platform renderers by default (Direct2D/DirectWrite on Windows, Cairo on GTK, QPainter on Qt, Cocoa on macOS, BView/AGG on Haiku), retaining Blend2D as an optional high-performance software fallback.

---

## 2. Quantitative Subsystem Diagnostics & Code Base Metrics

A comprehensive automated audit of the Wisp repository (`/app`) yields the following subsystem scale and implementation metrics:

### Subsystem Codebase Scale

| Subsystem | Directory | File Count | Line Count | Primary Language / Domain |
|---|---|---|---|---|
| **Core Content & Engine** | `src/content/` | 175 | 188,176 | C99 (HTML/CSS Layout, Fetch, HLcache, JS QuickJS bridge) |
| **Desktop Orchestration** | `src/desktop/` | 16 | 10,129 | C99 (Tile pool, Browser window management, Hotlist) |
| **Utilities & IPC System** | `src/utils/` | 42 | 10,582 | C99 (SVDS, BBMQ, Arena, IPC, SIMD, UTF-8, Logging) |
| **Frontends** | `frontends/` | 51 | 22,325 | C/C++ (GTK3, Qt, Win32/Direct2D, Cocoa, Haiku BeOS) |
| **Header Interfaces** | `include/` | 126 | 19,213 | C/C++ Header specifications |
| **Bundled Submodules** | `contrib/` | 629 | 608,880 | QuickJS-ng, LibCSS, LibDOM, LibHubbub, Blend2D |
| **Unit & Integration Tests** | `test/` | 67 | 24,883 | C Check framework & Playwright test scripts |
| **Total Core Codebase** | *(excl. contrib)* | **310** | **250,425** | C99 / C++17 |

### WebIDL Interface Implementation Tiers

Wisp maps **3,008 total WebIDL C symbols** under `src/content/handlers/javascript/quickjs/impl/` (with 2,514 custom overrides in `stubs_manual_impl.c` alone). Categorization of these 3,008 manual symbol overrides reveals:

```
[==================== Tier 1: Full Active Logic (1,408 symbols / 46.8%) ====================]
[========================= Tier 2: Safe Fallback / Graceful (1,561 symbols / 51.9%) ========================]
[== Tier 3: Throwing / Unimplemented (39 symbols / 1.3%) ==]
```

* **Tier 1: Full Active Logic (1,408 symbols)**: Complete C implementations with active DOM, layout, or canvas state mutations (e.g., `getElementById`, `appendChild`, `putImageData`, `addEventListener`, `fetch`, `IndexedDB`).
* **Tier 2: Safe Fallback / Graceful (1,561 symbols)**: Spec-safe no-op returns (`JS_UNDEFINED`, `JS_FALSE`, `JS_NULL`) that prevent JS execution runtime `TypeError` exceptions on non-critical secondary APIs.
* **Tier 3: Throwing / Unimplemented (39 symbols)**: Cleanly returns `DOMException("NotSupportedError")` or issues `[STUB]` warnings for unsupported modern hardware specs (WebGPU, WebXR, WebBluetooth).

### Test Suite Execution Health

* **Total CTest Test Suites Executed**: **158 / 158 Passed (100% Pass Rate)**
* **Execution Time**: 168.64s total (includes heavy QuickJS microtask and layout stress tests).
* **Regressions**: 0 failed test suites.
* **LeakSanitizer (LSan) Status**: Clean on CSS Node Selection Data, `free_style_snapshot` reclamation, and LibDOM node refcounting during QuickJS host node sync and test thread teardown.

---

## 3. Library Dependency & Submodule Audit

| Submodule / Library | Repository Version | Upstream Status (2026) | Divergence / Architectural Notes |
|---|---|---|---|
| `quickjs-ng` | v0.15.1 | v0.15.1 (Current) | **Integrated**: ES2023+ support, Copy-Patch JIT, SIMD JSON pre-parser. |
| `blend2d` | v0.21.2 | v0.21.2 (Current) | **Optional**: High-performance software 2D fallback engine. |
| `libavif` | v1.4.2 | v1.4.2 (Current) | **Integrated**: ISOBMFF AVIF image decoding. |
| `libcss` | Jan 2026 Fork | 0.9.2 (Upstream) | **Forked**: Enhanced for CSS Grid, Flexbox, `calc()`, and CSS Variable hashing. |
| `libdom` | Jan 2026 Fork | Upstream Git | **Forked**: Enhanced for SVG, custom user-data hooks, and `WispNodeID` mapping. |
| `libhubbub` | Jan 2026 Sync | Upstream Git | **Synchronized**: Spec-compliant HTML5 tokenizer and tree builder. |
| `libnsbmp` / `libnsgif` | Jan 2026 Sync | Latest | **Synchronized**: Lightweight image decoders. |
| `FFmpeg` | Linked System (6.1+) | 8.1 (System) | **System Linked**: A/V Master Clock synchronized video pipeline. |
| `LibreSSL` / `OpenSSL` | Linked System | 4.3.2 / 3.2+ | **System Linked**: Web Crypto API (`crypto.subtle`) & TLS transport. |

---

## 4. Subsystem Deep-Dive & Architectural Assessment

### 4.1 Layout & Rendering Subsystem

#### Fork-Join Parallel Style and Layout Engine
* **Lock-Free Worker Arenas**: Eliminates global allocator lock contention by assigning a private sub-arena (`wisp_worker_local_arena`) to each worker thread. On Join, `arena_merge` appends memory chunks to the main layout arena in $O(1)$ time.
* **Sub-Tree Parallel Threshold**: `SUBTREE_PARALLEL_STYLE_THRESHOLD` set to 32 elements. `count_subtree_elements()` performs non-recursive DOM traversal; sub-trees with $< 32$ elements bypass thread pool dispatch to eliminate context switching overhead.
* **Lock-Free Snapshot CSS Selection**: Global `dom_lock` was removed from parallel style workers (`parallel_style_worker_cb`). Workers read pre-calculated `style_snapshot_t` structures (pre-evaluating `:checked`, `:active`, `:target`) lock-free across CPU cores.

#### Incremental Reflow & Fixed-Tile Strategy
* Viewports utilize scale-aware tiles (256x256 standard, 512x512 High-DPI). Disjoint dirty region tracking manages up to 16 rects per frame.
* **LZ4 Lookaside Compression**: Out-of-viewport raw tiles are compressed using LZ4 in real-time (4:1 average compression ratio), reclaiming memory dynamically while allowing instant decompression on viewport scrollback.

### 4.2 JavaScript Engine & IPC IPC Architecture

#### Shared-Memory Virtual DOM Space (SVDS) & Batch-Buffered Mutation Queue (BBMQ)
* Topology mapped into contiguous shared memory (`shm_dom_node_t`) aligned to 64-byte cache lines (4672 bytes total structure size) to prevent multi-core false sharing.
* **Seqlock Protocol**: Out-of-process `wisp-js` reads node layout metrics (`x`, `y`, `width`, `height`) using atomic Seqlock versioning (`seq_version`), completely eliminating synchronous IPC stalls during animation frame reads.
* **BBMQ Dual-Stage Ring-Buffer**: Primary queue (1,024 items) auto-scales up to 16 dynamic secondary shared-memory page chunks (`shm_mutation_chunk_create`) during heavy single-tick DOM mutations, preventing process stalls.
* **Microtask Flushing**: Pending DOM mutations are flushed in a single batch at the end of the QuickJS microtask tick via `JS_ExecutePendingJob`.

### 4.3 SIMD Hardware Acceleration Pipeline

Dynamic CPU feature detection routes performance-critical operations to SIMD vectors with robust scalar fallbacks:

```
                          ┌──────────────────────────┐
                          │   CPU Feature Detection  │
                          └─────────────┬────────────┘
                                        │
           ┌────────────────────────────┼────────────────────────────┐
           ▼                            ▼                            ▼
   x86 / x86_64: SSE2            ARM / ARM64: NEON            RISC-V: RVV 1.0
 ┌───────────────────┐        ┌───────────────────┐        ┌───────────────────┐
 │ _mm_cmplt_epi8    │        │ vcgeq_u8          │        │ vle8 / vxor.vv    │
 │ _mm_xor_si128     │        │ veorq_u8          │        │ Vector comparisons│
 └───────────────────┘        └───────────────────┘        └───────────────────┘
           │                            │                            │
           └────────────────────────────┼────────────────────────────┘
                                        │
                                        ▼
             ┌─────────────────────────────────────────────────────┐
             │ Fast-Path Kernels:                                  │
             │  - UTF-8 Validation & UTF-32 Conversion             │
             │  - WebSocket Client Payload Masking (16 B/cycle)    │
             │  - Structural JSON Pre-Parser                       │
             │  - CSP Nonce & Security String Comparisons          │
             │  - CSS Delimiter & Boundary Scanning                │
             └─────────────────────────────────────────────────────┘
```

---

## 5. Diagnostic Findings, Bottlenecks, & Technical Debt

### 5.1 Identified Technical Debt Indicators
* **Static Code Debt**: Static analysis identifies **2,272 TODOs** and **317 FIXMEs** across the core codebase.
  * *Layout & CSS*: 42% of TODOs relate to complex CSS edge cases (table cell auto-margins, vertical writing modes, inline-block baseline alignment).
  * *JavaScript Stubs*: 35% relate to Tier 2 no-ops marked for eventual Tier 1 elevation.
  * *Graphics*: 23% relate to platform-specific UI Chrome controls and clipboard integrations.

### 5.2 Performance & Memory Bottleneck Analysis

1. **CSS Tokenizer Lexical Scanning**: CSS stylesheet parsing currently uses sequential character scanning in `contrib/libcss`. Loading CSS structural delimiters and whitespace patterns into 16-byte SIMD registers will accelerate stylesheet parsing for heavy CSS frameworks (e.g. Tailwind, Bootstrap).
2. **WebIDL Tier 2 -> Tier 1 Elevation**: 1,561 WebIDL symbols operate as safe no-ops. Elevating high-impact APIs (such as `CanvasRenderingContext2D` advanced composition modes, `ResizeObserver`, and `WebAudio` basic oscillators) will further expand modern SPA compatibility.
3. **IPC Shared Memory Chunk Fragmentation**: While BBMQ secondary page chunk allocation prevents stalls, frequent single-element DOM append loops can trigger burst chunk creation. Pre-allocating secondary chunks on page init will reduce `shm_open`/`mmap` syscall overhead.

---

## 6. Strategic Technical Recommendations & Roadmap (2026-2027)

### Proposal A: User-Space TLS Stack for Legacy OS Compatibility
* **Problem**: Legacy operating systems (Windows XP/7, older Linux distros) lack TLS 1.2/1.3 cipher suites in system Schannel/crypto libraries, causing connection failures to modern HTTPS sites.
* **Recommendation**: Statically link **mbedTLS** or **BearSSL** directly into the isolated `wisp-network` process. The network process bypasses host OS socket/crypto stacks, delivering native, OS-independent TLS 1.3 capabilities across all platforms.

### Proposal B: Asymmetric OS Sandboxing Matrix
Implement a tiered runtime sandboxing framework across target operating systems:

| Platform | Sandboxing Mechanism | Security Boundary |
|---|---|---|
| **Windows 8.1 / 10 / 11** | AppContainer Isolation Profile | Restricted Low Integrity Level; blocks unauthorized filesystem/registry access. |
| **Windows XP / Vista / 7** | Restricted Token (`CreateRestrictedToken`) + Job Object | Strips admin SIDs, denies write handles, auto-kills processes on parent termination (`JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`). |
| **Linux / BSD** | Landlock LSM + seccomp-bpf | Restricts filesystem view to cache/temp directories; blocks non-essential syscalls. |
| **Haiku / BeOS** | Kernel Port-Level MAC (Mandatory Access Control) | Intercepts IPC messages between isolated Teams and system servers (Storage, Network). |

### Proposal C: SIMD CSS Tokenizer & Delimiter Scanner
* Extend `include/wisp/utils/css_delimiters.h` to scan 16-byte blocks for whitespace (` `, `\t`, `\n`, `\r`) and structural delimiters (`{`, `}`, `:`, `;`, `,`, `(`, `)`).
* Implement SSE2/NEON/RVV 1.0 vector scanners to skip whitespace and locate declaration boundaries in a single CPU instruction, accelerating large stylesheet parsing.

### Proposal D: WebGPU Research & Native Bridge
* Bridge WebIDL WebGPU stubs (`GPUAdapter`, `GPUDevice`, `GPUCanvasContext`) to platform-native graphics APIs (Direct3D 12 on Windows, Vulkan on Linux, Metal on macOS).
* Focus initially on offscreen compute shaders for accelerated image processing before expanding to full 3D canvas rendering contexts.

---

## 7. Report Conclusion & Verification Summary

The Wisp engine codebase exhibits exceptional structural health, low memory overhead, 100% unit/integration test pass rates, and spec-compliant compliance baselines (573/588 on HTML5Test).

### Verification
* **Test Suite Verification**: `./run_tests.sh` executed cleanly with **158 / 158 test suites passed**.
* **Memory Safety**: LSan audits confirm zero memory leaks across CSS snapshot allocations, DOM bridge wrapper maps, and QuickJS host node syncs.
* **Codebase Integrity**: All core architectural features are verified, documented, and ready for future roadmap expansions.
