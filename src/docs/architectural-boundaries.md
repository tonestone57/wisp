# Architectural Boundaries: Modern Web App Frameworks vs. Lightweight Native Engines

This document outlines the architectural boundaries and realities of Wisp (a lightweight C-based browser engine fork of NetSurf with QuickJS-ng) when dealing with modern Single-Page Applications (SPAs), WebAssembly (Wasm), and traditional static web content. It also details the theoretical and practical upgrade path required to reach Web API parity.

## Architectural Reality

**Modern Web App Frameworks (React/Next.js) require a full-featured browser engine runtime (such as Blink or Gecko) with complete Web API parity.**

While Wisp successfully bridges the gap between retro software efficiency and modern CSS/JS standards, there is a fundamental boundary between a lightweight native web engine and multi-gigabyte browser engines (Blink/Gecko/WebKit).

---

## WebAssembly (Wasm) is NOT a General Requirement

**WebAssembly (Wasm) is not required for the vast majority of modern websites.**

Data from the HTTP Archive shows that WebAssembly is used on only **~0.35% of desktop sites** and **~0.28% of mobile sites** across the entire web. Over **99.5% of the web** relies purely on standard HTML5, CSS3, and JavaScript.

### What Standard Modern Websites Use
Even heavy, dynamic Single-Page Applications (SPAs) built with modern frameworks—such as React, Next.js, Vue, Angular, or Svelte—run entirely on **JavaScript**. Sites like NBC News, Twitter/X, Amazon, and YouTube do not use Wasm for their layout, routing, or hydration.

### Where WebAssembly *Is* Used
Wasm is heavily concentrated in the top tier of complex, desktop-grade web applications (where it accounts for 1–2% of usage):
*   **Browser-based Software**: Figma (C++ compiled to Wasm), Adobe Photoshop Web, AutoCAD Web.
*   **3D Games & Engines**: Games exported from Unity, Unreal Engine, or Godot.
*   **Media Processing**: Video streaming decoders (like Amazon IVS), audio processing, and client-side image manipulation (`FFmpeg.wasm`).
*   **In-Browser Databases & AI**: In-memory databases (`SQLite Wasm`), data science environments (`Pyodide/Python`), and local machine learning models (ONNX / TensorFlow.js Wasm backends).

### What This Means for Browser Engines (like Wisp)
If your goal is to render standard web pages, news portals, e-commerce, or typical React/Next.js sites, **lacking Wasm will not prevent those sites from working.**

The actual hurdles for rendering modern web apps in lightweight engines are almost always:
1.  **JavaScript Web API Parity**: Missing DOM features like `MutationObserver`, `ResizeObserver`, Fetch, Promises/Microtasks, or Shadow DOM.
2.  **CSS Layout Capabilities**: Modern Flexbox, Grid, and dynamic CSS custom variables (`var(--...)`).

Unless a user attempts to run a full desktop port (like Figma) or an intensive 3D game in the browser, a lack of Wasm support will generally go entirely unnoticed.

---

## Why This Boundary Exists

### 1. Modern SPAs & React/Next.js Client-Side Hydration

Single-Page Applications (SPAs) like NBC, CBS, or ABC News rely heavily on **client-side hydration**. Instead of sending pre-rendered HTML layout structures, the server sends a minimal HTML shell and large JavaScript bundles (React, Next.js, Webpack chunks).

*   **The Hydration Bottleneck**: When the JavaScript bundle executes on the client, it expects hundreds of complex, high-level browser APIs to be present. This includes `MutationObserver`, `ResizeObserver`, Shadow DOM v1, `IntersectionObserver`, the full Streams API, and complex Fetch/Promise chains.
*   **Cascading Failures**: If a lightweight engine or its JS wrapper is missing even one minor DOM API method or returns an unhandled `undefined`, React's hydration process will throw an uncaught exception and abort. This leaves the user with a blank white screen, broken layouts, or raw, unhydrated skeletons—not because the C layout engine failed, but because the DOM tree was never built by JavaScript.

### 2. High-Performance Benchmarks (Speedometer 3.1 & JetStream 3.0)

Modern benchmarks are explicitly designed to test full-tier browser engines (Chromium/V8, Firefox/SpiderMonkey, Safari/JavaScriptCore) and push them to their limits:

*   **Speedometer 3.1**: Tests DOM rendering and JS execution responsiveness across dozens of modern framework workloads (React, Vue, Angular, Svelte, Lit). It relies on complex event loops, web components, and precise microtask timing that lightweight embedded JS integrations cannot fully cover without massive bloat.
*   **JetStream 3.0**: Focuses heavily on advanced ES2022+ features and JIT (Just-In-Time) compilation performance (e.g., matrix math, regex, 3D physics compiled from C++/Rust). A non-JIT or lightweight JS engine like QuickJS-ng is designed for small memory footprints and fast startup times, not for heavy JIT workloads.

### 3. Static & Semantic Pages (e.g., Haiku-OS)

Sites like **Haiku-OS** represent the best-case scenario and target audience for lightweight layout engines:

*   **No Heavy JS Reliance**: They rely on **server-rendered, semantic HTML5 and standard CSS** rather than JavaScript DOM generation.
*   **Direct Core Rendering**: Once layout features (such as CSS Grid `css_fixed_or_calc` handling, Flexbox calculations, font loading, and LibRSVG image decoding) are in place, the core C engine parses the DOM tree directly and calculates exact layout boxes.
*   **Performance**: The result is fast, crisp, pixel-accurate rendering without needing to run heavy, battery-draining JavaScript runtime loops.

---

## Required Architectural Upgrades for Web API Parity

To enable a lightweight, C99-based engine like **Wisp** to run modern Web App Frameworks (React, Next.js, Vue) and reach full Web API parity, the codebase would require fundamental architectural upgrades across five main areas:

### 1. Event Loop & JavaScript Execution Engine
QuickJS-ng provides ES2023 language compliance, but modern frameworks depend heavily on specific browser host environment behaviors rather than just pure JavaScript syntax.

*   **HTML5 Spec-Compliant Event Loop**:
    *   **Microtask Queue**: React's scheduler relies on precise Promise microtask ordering (`queueMicrotask`). Wisp’s C-level event loop must process all microtasks to completion *before* yielding to rendering or macrotasks (`setTimeout`, I/O).
    *   **Frame Synchronization**: Native implementation of `requestAnimationFrame()` and `requestIdleCallback()` tied directly to the display backend's refresh cycle.
*   **Threaded Concurrency**:
    *   Web Worker support (`new Worker()`) by instantiating isolated QuickJS runtime instances inside dedicated OS threads (pthreads) with structured clone messaging.

### 2. DOM & Web API Binding Layer (libdom / nsgenbind)
Frameworks do not use standard static DOM trees; they construct, measure, and observe the DOM dynamically.

| Required Modern DOM APIs | Description / Usage | Implementation Status |
|---|---|---|
| **Observers** | `MutationObserver`, `ResizeObserver`, `IntersectionObserver` | **[Finished]** `MutationObserver` & `IntersectionObserver` fully integrated |
| **Component Model** | Shadow DOM v1 (`attachShadow`), Custom Elements (`customElements.define`) | **[Finished]** Shadow DOM v1 & Web Components integrated |
| **Routing & State** | History API (`pushState`, `replaceState`), `localStorage`, `sessionStorage`, `IndexedDB` | **[Finished]** SPA History API & complete IndexedDB/Storage polyfills |
| **Networking** | Fetch API, `ReadableStream`/`WritableStream`, WebSockets, CORS headers enforcement | **[Finished]** Fetch, Streams, WebSockets, & IPC security isolation |

*   **MutationObserver**: Essential for React and Vue DOM reconciliation. Without C-level tracking of attribute modifications, node insertions, and text mutations, hydrated frameworks immediately crash or desynchronize.
*   **ResizeObserver & IntersectionObserver**: Used by Next.js for image lazy loading, infinite scrolling, and component layout logic.
*   **Synthetic Events & Bubbling Fidelity**: React uses a single top-level event listener on document or root. Wisp’s C event target model must support standard capture and bubble phases, `composedPath()`, and exact event object property propagation.

### 3. Dynamic Reflow & Incremental Layout (LibCSS)
NetSurf and Wisp historically optimized for document-style web pages (where HTML is parsed once and rendered). Modern SPAs modify DOM nodes continuously.

*   **Incremental Layout & Targeted Repaints**:
    *   Modern apps mutate dozens of DOM elements per second. Rebuilding or re-laying out large branches of the `box_tree` on every JS mutation causes severe performance degradation. Wisp needs incremental reflows (re-calculating layout bounds only for dirty subtree nodes).
*   **Dynamic CSS Custom Properties (Variables)**:
    *   `var(--theme-color)` support requires dynamic cascading recalculation when JS updates CSS variables at runtime (e.g., `element.style.setProperty()`).
*   **Complete CSS Grid & Flexbox Engine**:
    *   Full support for CSS Grid track sizing (`minmax()`, `fr` units, `auto-fill`), gap calculations, subgrids, and Flexbox wrapping algorithms used by Tailwind CSS and UI component libraries.

### 4. Networking & Stream Pipeline
*   **Fetch & Streams Integration**:
    *   Replacing legacy HTTP fetch wrappers with a full Fetch API binding backed by libcurl, including support for `ReadableStream` (crucial for Next.js Server Components / React Server Components streaming responses).
*   **CORS & Security Controls**:
    *   Strict Cross-Origin Resource Sharing (CORS) enforcement for `fetch()` / `XMLHttpRequest` to prevent modern API requests from being blocked by endpoint security policies.

### 5. Modern Rendering & Canvas API
*   **HTML5 `<canvas>` (2D & WebGL)**:
    *   Providing 2D Canvas context bindings (via Cairo or Skia) and basic WebGL bindings (via OpenGL ES abstraction) for dynamic graphics, charts, and interactive components.

---

### Summary of Engineering Priority
To move from rendering static pages (like Haiku-OS) to hydrated SPAs (like NBC News), the highest-priority work items in Wisp would be:
1.  **MutationObserver implementation** in libdom/QuickJS bindings.
2.  **HTML5 History API (`pushState`)** for client-side routing.
3.  **Fetch + ReadableStream pipeline** for server-side streamed payloads.
4.  **Incremental layout invalidation** so rapid DOM updates don't trigger full-page reflows.

---

## Summary of the Engine Boundary

This distinction defines the boundary between **lightweight, native web engines** and **multi-gigabyte browser engines (Blink/Gecko)**:

*   **Traditional / Static Web (HTML5 + CSS3 + Standard SVG)**: Fully achievable, blazing fast, and lightweight. This is the sweet spot for Wisp.
*   **Modern Web App Frameworks (React / Next.js)**: Require a full-featured browser engine runtime with complete Web API parity.
