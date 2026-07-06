# Wisp Browser Technical Roadmap & Architectural Summary (August 2026)

## 1. Executive Summary
Wisp is a lightweight, high-performance web engine forked from NetSurf. Its primary mission is to bridge the gap between "retro" software efficiency and the modern web by implementing high-priority standards (CSS Grid, Flexbox, ES2023+) while maintaining a minimal footprint suitable for both modern and legacy operating systems (Haiku, Windows XP/7, Linux, macOS).

---

## 2. Graphics Architecture
Wisp utilizes a "best-of-breed" plotting architecture to ensure performance and consistency across platforms.

### Current Backends
*   **Direct2D & DirectWrite (Windows 7+ / 10 / 11)**: A hardware-accelerated pipeline providing GPU drawing and native typography.
*   **Blend2D (Unified Backbone & Fallback)**: A high-performance software 2D engine using JIT-compiled SIMD (AVX-512, NEON) for rasterization. It serves as the primary rasterizer for Linux and macOS, and the mandatory fallback for **Windows XP/Vista**, ensuring modern CSS compatibility on legacy hardware via GDI/GDI+ blitting.
*   **BView / AGG (Haiku/BeOS)**: A native backend leveraging Haiku's `app_server` for subpixel anti-aliasing and native OS integration.
*   **Core Graphics / Core Text (macOS)**: Native Cocoa-based rendering for the macOS frontend.
*   **Cairo / QPainter**: Standard fallbacks for the GTK and Qt frontends.

### Rendering Strategy
Wisp utilizes a **Fixed-Tile Redraw** strategy (256px or 512px tiles) to optimize cache locality and performance. This replaces the legacy union-based dirty region system, significantly reducing overdraw and providing a foundation for parallel painting.

---

## 3. Parallel Tile Redraw (PTR) Strategy
Wisp's architecture is uniquely positioned to take advantage of multi-core processors through parallelization of the tiling loop.

### Cross-OS Parallelization Strategy
1.  **Work Stealing**: Instead of the UI thread painting every tile sequentially, the browser core pushes "Dirty Tile Tasks" to the `wisp_subsystem` worker pool (spawning up to 7 background threads).
2.  **Thread-Local Backends**: Each worker thread utilizes a thread-local instance of the rendering backend (Blend2D or Direct2D), allowing simultaneous rasterization of different tiles without mutex locking.
3.  **Asynchronous Compositing**: Once all workers finish their assigned tiles, the main thread performs a single atomic blit to the screen.

### Platform-Specific Benefits
| Platform | Benefit |
| :--- | :--- |
| **Haiku / BeOS** | **Isolated Offscreen Paint**: Workers rasterize tiles safely into thread-confined raw memory regions, bypassing the single-threaded `BWindow` looper limit before a final main-thread synchronized blit. |
| **Windows** | **Command List Parallelism**: Background worker threads concurrently record independent `ID2D1CommandList` blocks via deferred contexts, minimizing GPU pipeline stalls and avoiding D2D factory serialization. |
| **Linux (GTK/Qt)** | **Bypass Single-Core Limits**: Offloads CPU-intensive SIMD rasterization (Blend2D) away from the main event loop. |
| **macOS** | **UI Responsiveness**: Ensures heavy "Core Text" layout tasks don't block the Cocoa event loop. |

---

## 4. JavaScript Threading Model
Wisp implements a **hybrid threading model** to balance safety with performance.

### Single-Threaded DOM Access
To ensure safe interaction with the underlying C-based DOM (`libdom`), which is not thread-safe, all scripts that manipulate page elements run on the **Main UI Thread**. This prevents race conditions and memory corruption without the overhead of complex locking mechanisms.

### Background Worker Pool (`wisp_subsystem`)
The browser includes a dedicated worker pool subsystem (`src/content/handlers/javascript/quickjs/wisp_subsystem.c`) that scales based on the available CPU cores. This infrastructure allows Wisp to:
*   Offload computationally expensive tasks (cryptography, large data parsing, image decoding).
*   Keep the UI thread responsive during heavy site execution.
*   Provide a foundation for a future full `Web Workers` implementation.

---

## 5. Platform-Specific Roadmap: Haiku/BeOS
While functional, the Haiku frontend has several paths for significant advancement:
*   **Native Widget Migration**: Replacing custom-drawn interactive elements with native `BControl` widgets (e.g., `BTextControl`). This allows Wisp to automatically adopt Haiku system themes and accessibility features.
*   **Parallel Tile Rendering**: Leveraging Haiku’s thread-safe `BView` looper to render the fixed-tile grid in parallel across multiple CPU cores.
*   **Replicant Support**: Implementing `BArchivable` so Wisp views can be embedded as live, interactive tiles directly on the Haiku Desktop (Deskbar/Workspaces).

---

## 6. Global Performance, Stability, and Security Goals

### Performance
*   **GPU Compositing**: Move the final "tile blitting" and scrolling pass to the **GPU (OpenGL/Vulkan)** to reduce CPU overhead and provide smoother 60FPS scrolling.
*   **Script Optimization**: Prioritize bytecode execution and interpreter loop enhancements for QuickJS-ng. For high-performance JS requirements exceeding interpreter capabilities, evaluate engines with native JIT tiers such as **Hermes** or **V8 (Lite mode)**.

### Stability
*   **Process Isolation**: Moving the JavaScript engine and network stack into separate OS processes (multi-process architecture). This ensures that a single malicious or buggy script cannot crash the entire browser window.
*   **Ownership Proxy Model**: Transition to a strict proxy model for the JS/DOM bridge where JS wrappers point to a tracked reference map rather than extending C node lifecycles directly, mitigating complex reference cycles.

### Security
*   **Content Security Policy (CSP)**: Implement full CSP header support to mitigate Cross-Site Scripting (XSS) at the engine level.
*   **OS-Level Sandboxing**: Utilize features like **Landlock (Linux)** or **AppContainer (Windows)** to isolate the browser from the user's sensitive filesystem data.

---

## 7. Recent Technical Improvements (June 2026 Update)
The following stability and compatibility fixes have been integrated:
1.  **Web API Initialization**: Corrected `js_newthread` to ensure `navigator`, `location`, `storage`, and `XMLHttpRequest` are fully initialized with correct private data before script execution.
2.  **Bridge Stability**: Fixed a critical `JS_FreeRuntime` assertion failure by ensuring the DOM bridge explicitly frees JSValue references and clears the runtime opaque pointer during cleanup.
3.  **Initialization Ordering**: Reordered the JS startup sequence to ensure core bindings are registered before the bridge attempts to wrap LibDOM nodes.

---

## 8. Remaining Tasks & Priority Backlog
The following tasks are identified as high-priority for the next development cycle:

### Graphics & Rendering
*   **[Incomplete] Canvas 2D Plotter Bridge** (Complexity: **Medium** | Benefit: **High**): Connect the WebIDL stubs for the Canvas 2D API to the underlying plotter engine (Direct2D/Blend2D).
    *   *Benefit*: Enables high-performance interactive graphics, charts, and games, reaching parity with modern web standards.
*   **[Planned] GPU-Accelerated Compositing** (Complexity: **High** | Benefit: **High**): Move the final tile-blitting and scrolling pass to the GPU (OpenGL/Vulkan).
    *   *Benefit*: Offloads expensive pixel transfers from the CPU, ensuring buttery-smooth 60FPS scrolling and lower power consumption on modern hardware.
*   **[Finished] Parallel Tile Redraw** (Complexity: **Medium** | Benefit: **Medium**): Parallelize the Fixed-Tile Redraw strategy across multiple CPU cores via the `wisp_subsystem` worker pool.
    *   *Benefit*: Dramatically reduces latency on complex pages by utilizing all available CPU cores for concurrent tile rasterization.

### Performance & Stability
*   **[Bug] QuickJS Leak Resolution** (Complexity: **Low** | Benefit: **Low**): Investigate and resolve the remaining heap leaks (~720 bytes) identified during runtime teardown in `qjs.c`.
    *   *Benefit*: Ensures a "perfect" leak-free baseline for embedding Wisp as a library in other applications.
*   **[Planned] Multi-process Architecture** (Complexity: **High** | Benefit: **High**): Isolate the JavaScript engine and network stack into separate OS processes.
    *   *Benefit*: Improves system-wide stability by ensuring a crash in a script or network component does not affect the main browser process.

### UI & Features
*   **[Planned] Unified C-based UI Library** (Complexity: **Medium** | Benefit: **High**): Implement a cross-platform, lightweight UI component library for consistent 'browser chrome' (tabs, address bar).
    *   *Benefit*: Simplifies maintenance and ensures a professional, consistent user experience across Linux, Windows, Haiku, and macOS.
*   **[Planned] Web Worker Parity** (Complexity: **Medium** | Benefit: **Medium**): Extend the `wisp_subsystem` worker pool to support a full, spec-compliant `Web Workers` API.
    *   *Benefit*: Unlocks the ability to run heavy computations (like image processing) in the background without freezing the UI.
*   **[Partial] Native Haiku Widget Parity** (Complexity: **Low** | Benefit: **Medium**): Integrate native `BControl` elements (buttons, inputs) into the BeOS/Haiku frontend via a persistent widget map in `gui_window`.
    *   *Benefit*: Provides perfect system theme integration and accessibility support for Haiku users.

### Security
*   **[Finished] Content Security Policy (CSP)** (Complexity: **Medium** | Benefit: **High**): Full CSP header enforcement (default-src, script-src, img-src, style-src, font-src, object-src, frame-src, connect-src).
    *   *Benefit*: Provides a critical layer of defense against Cross-Site Scripting (XSS) and data injection attacks.
*   **[Planned] OS-Level Sandboxing** (Complexity: **High** | Benefit: **High**): Integrate Landlock (Linux), AppContainer (Windows), and Pledge (OpenBSD).
    *   *Benefit*: Rigorously isolates the browser from sensitive user data, providing maximum protection against zero-day exploits.

---

## 9. Architectural Refinement: Optimal Worker Pool Size
The structural layout of the Wisp architecture shows a clever adaptation of NetSurf’s ultra-light base. Bridging modern CSS layout rules with high-performance software rasterization like Blend2D is a great approach for low-spec hardware. However, the threading and graphics pipeline model requires refinement.

### The Core Problem with a Unified Fixed Pool
Hardcoding or capping the background worker pool at **7 threads** introduces performance issues:
1.  **Low-End Hardware Thrashing**: Spawning 7 threads on a legacy dual-core (e.g., Core 2 Duo) causes severe context-switching overhead and cache thrashing.
2.  **High-End Hardware Starvation**: On modern 8-core or 16-core machines, capping at 7 leaves performance on the table.
3.  **Resource Contention**: Since the `wisp_subsystem` handles both Parallel Tile Redraw (PTR) and background JavaScript tasks, a heavy script can stall rendering, causing UI stutter.

### Implemented Sizing Architecture
The subsystem decouples tasks into a dedicated Rasterization Pool and a separate JS Worker Pool, scaling dynamically based on logical core count (N).

| Pool Type | Target System Power | Implemented Formula | Behavior |
|---|---|---|---|
| **Rasterization Pool** | Single-Core (N=1) | 0 (Synchronous) | Avoids threading overhead completely. |
| | Multi-Core (N > 1) | P = N - 1 | Leaves 1 core for the Main UI thread and OS event loops. |
| **JavaScript Worker Pool** | All Systems | P = min(4, N) | Bounded to ensure scripts never starve rasterization. |

---

## 10. High-Impact Structural Improvements

### A. [Finished] Tile Memory Recycling (Fixed-Buffer Pool)
Dynamic allocation/freeing of tile backing stores triggers **heap fragmentation**, especially on legacy OS allocators.
*   **The Fix**: Implemented a thread-safe **Lookaside List** of fixed-size 1MB tile memory buffers in `src/desktop/tile_pool.c`. Worker threads checkout buffers, rasterize, and return them after the main thread executes the atomic blit.

### B. Viewport-Prioritized Tile Scheduling
*   **[Finished] Viewport-Prioritized Tile Scheduling**: A simple FIFO task queue can hurt perceived performance during heavy reflows if tiles at the bottom of the page are rendered before visible ones.
*   **The Fix**: Implemented a **Spatially Weighted Task Queue**. Every dirty tile task is assigned a priority multiplier calculated via `browser_calculate_tile_priority` based on its geometric distance from the viewport frustum.

### C. Direct Render Passes for Haiku (BDirectWindow)
Copying large memory blocks back to the main UI thread creates a bottleneck on older Haiku rigs.
*   **The Fix**: Migrate the final compositor step to a `BDirectWindow`. This grants the drawing engine direct, locked access to the frame buffer, bypassing `app_server` context loops.

### D. IPC & Sandboxing Abstraction Layer
Building a sandboxing model across modern and legacy architectures (Haiku, XP) requires a clean separation.
*   **The Fix**: Isolate multi-process messaging behind a platform-agnostic IPC interface wrapper using native primitives:
    *   **Windows XP/7**: Named Pipes with restricted SIDs.
    *   **Linux / macOS**: Unix Domain Sockets (`socketpair`).
    *   **Haiku**: Native OS `BMessage` ports.
