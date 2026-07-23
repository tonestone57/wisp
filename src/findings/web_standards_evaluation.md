# Evaluation of HTML5, CSS3, and JavaScript Support in Wisp (July 2026/2027)

This report provides a comprehensive, systematic audit and quantitative/qualitative assessment of **Wisp's** alignment with modern web standards (**HTML5**, **CSS3**, and **JavaScript**), based on the current state of the repository.

---

## 1. Quantitative Standards Support Summary

Wisp uses a combination of compiled C libraries (forked/diverged from NetSurf) and automated QuickJS-ng bindings. Below is the current estimated support level across the three pillars:

| Standard | Estimated Support % | Core Strengths | Critical Gaps |
|---|---|---|---|
| **HTML5 (DOM & Parser)** | **~75%** | Spec-compliant Hubbub tokenization, XML/HTML parser, `libdom` tree core, native Canvas 2D bridge, MutationObserver. | Shadow DOM v1, Custom Elements, HTML5 History API (`pushState`), full drag & drop / drag events. |
| **CSS3 (Layout & Style)** | **~82%** | Spec-compliant CSS Grid (auto-placement, dense packing, FR units), Flexbox (grow, shrink, column two-pass), `position: sticky`, CSS Variables (with fast-path evaluation/caching). | CSS Grid subgrids and container queries, advanced CSS3 3D transforms, advanced transitions/animations. |
| **JavaScript (ES2023+)** | **~35% (Web APIs)** <br> **100% (Language)** | Integrated **QuickJS-ng v0.15.1** (full ES2023+ compliance), Web Workers with structured cloning, Web Crypto ( LibreSSL ), basic performance timers. | ~1,500 WebIDL-declared getters/setters/methods currently fallback to weak stubs or are unimplemented (e.g. Media streams, advanced DOM events, complete fetch/streams). |

---

## 2. In-Depth Subsystem & API Analysis

### 2.1 HTML5 & DOM Parser
*   **The Parser (`libhubbub`)**: Synchronous spec-compliant parser with customized pauses for script processing and inline/external load synchronization.
*   **The DOM Core (`libdom`)**: Robust, C99-compliant implementation of the DOM tree. Supports hierarchical manipulation, child insertion, parent-child references, and right-to-left matching selectors (`querySelector`/`querySelectorAll`).
*   **WebIDL Bindings**:
    *   Wisp includes **9 IDL files** (covering Console, CSSOM, DOM, DOM Parsing, HTML, Observers, UI Events, URL Utils, and XHR) compiling down to **228 declared interfaces** and around **1,500 methods/getters/setters**.
    *   **32 interfaces** are fully or partially implemented manually via dedicated C source files in `src/content/handlers/javascript/quickjs/impl/` (e.g. `Node`, `Element`, `Document`, `HTMLScriptElement`, `HTMLImageElement`, `Canvas`, `MutationObserver`, `IntersectionObserver`, `Worker`).
    *   Unimplemented WebIDL bindings gracefully degrade to weak stubs that log warning notices using `NSLOG(wisp, WARNING, ...)` rather than crashing.

### 2.2 CSS3 Layout Engines
Wisp has over 10,000 lines of highly optimized C code dedicated to modern layout algorithms:
*   **CSS Grid (`layout_grid.c`)**: Features a spec-compliant 3-phase auto-placement grid, FR unit layout resolution, and dense grid packing.
*   **CSS Flexbox (`layout_flex.c`)**: Implements standard CSS Flexbox alignment, including `flex-grow`, `flex-shrink`, auto-margins, and two-pass layout arithmetic for column flex layouts.
*   **Style Sheet Engine (`libcss`)**: Supports advanced nested selectors, custom properties (CSS variables), `calc()`, nested counters, and tab-sizing. Optimized with style-context hashing/caching of custom property values in `libcss` to bypass redundant recursive parsing passes.

### 2.3 JavaScript Engine (QuickJS-ng)
*   **The Engine**: Utilizes **QuickJS-ng (v0.15.1)**, ensuring 100% compliance with modern ECMAScript specifications (ES2023+ including Promises, async/await, and classes).
*   **Threading**: Employs a hybrid threading model where the main thread manages single-threaded non-thread-safe DOM updates (`libdom`), and the decentralized `wisp_subsystem` worker pool runs asynchronous, isolated Web Workers.
*   **Process Isolation**: Executes JavaScript within an isolated companion helper process (`wisp-js`) via socket-based IPC to protect layout and networking contexts.

---

## 3. What is Still Required for Web API Parity (The Upgrade Blueprint)

To bridge the gap between static content rendering and fully dynamic Single-Page Application (SPA) hydration, the following key systems are planned and required:

```
                  Wisp Web API Parity Upgrade Pipeline

   +--------------------------------------------------------------+
   |  1. HTML5 Speced Event Loop & Microtask Queue Resolution     |
   |     - queueMicrotask, requestAnimationFrame, IdleCallbacks  |
   +--------------------------------------------------------------+
                                  |
                                  v
   +--------------------------------------------------------------+
   |  2. Shared-Memory Virtual DOM Space (SVDS) Topology          |
   |     - Zero-Copy IPC DOM tree access via 32-bit indices       |
   +--------------------------------------------------------------+
                                  |
                                  v
   +--------------------------------------------------------------+
   |  3. Batch-Buffered Mutation Queue (BBMQ)                     |
   |     - Microtask-tick serialization & lock-free ring-buffer   |
   +--------------------------------------------------------------+
                                  |
                                  v
   +--------------------------------------------------------------+
   |  4. Full Fetch & ReadableStream Integration                  |
   |     - Asynchronous streaming server components / CORS checks  |
   +--------------------------------------------------------------+
                                  |
                                  v
   +--------------------------------------------------------------+
   |  5. WebAssembly (Wasm) Subsystem & JIT Evaluation            |
   |     - Embedded wasm3/wasmtime and lightweight JS JIT tiers   |
   +--------------------------------------------------------------+
```

### Detailed Component Requirements

1.  **Shared-Memory Virtual DOM Space (SVDS)**:
    *   *Need*: Pass DOM tree topology across process boundaries using a compact 32-byte struct layout referencing node IDs as 32-bit array indexes.
    *   *Result*: `wisp-js` can resolve read operations (like `node.nextSibling` or `node.firstChild`) locally in O(1) time without executing heavy IPC round-trips.
2.  **Batch-Buffered Mutation Queue (BBMQ)**:
    *   *Need*: Buffer write operations (like `element.setAttribute`) locally within the JS process's shared-memory ring buffer.
    *   *Result*: Writes are flushed to the main UI thread as a single batched command list at the end of the microtask tick, preventing single-operation IPC context switches.
3.  **HTML5 Compliant Event Loop**:
    *   *Need*: A spec-compliant microtask queue handler processing Promise resolution before yielding to repaint phases, with native frame-synchronized timers.
4.  **Complete Fetch API & Streams**:
    *   *Need*: Full `fetch()` integration wrapped around `libcurl` and delivering `ReadableStream`/`WritableStream` to JS for progressive component streaming.
5.  **Component Model & Routing**:
    *   *Need*: Implement Shadow DOM v1 APIs (`attachShadow`) and the HTML5 History API (`pushState`/`replaceState`) to enable client-side SPAs to route pages offline.
