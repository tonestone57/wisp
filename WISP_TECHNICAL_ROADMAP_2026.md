# Wisp Browser Technical Roadmap & Architectural Summary (June 2026)

## 1. Executive Summary
Wisp is a lightweight, high-performance web engine forked from NetSurf. Its primary mission is to bridge the gap between "retro" software efficiency and the modern web by implementing high-priority standards (CSS Grid, Flexbox, ES2023+) while maintaining a minimal footprint suitable for both modern and legacy operating systems (Haiku, Windows XP/7, Linux, macOS).

---

## 2. Graphics Architecture
Wisp utilizes a "best-of-breed" plotting architecture to ensure performance and consistency across platforms.

### Current Backends
*   **Blend2D (Unified Backbone)**: A high-performance software 2D engine using JIT-compiled SIMD (AVX-512, NEON) for rasterization. It ensures pixel-perfect consistency across Linux, Windows, and macOS.
*   **Direct2D & DirectWrite (Windows)**: A native hardware-accelerated pipeline for Windows 7+, providing GPU-accelerated drawing and superior typography.
*   **BView / AGG (Haiku/BeOS)**: A native backend leveraging Haiku's `app_server` for logical drawing and subpixel anti-aliasing.
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
| **Haiku / BeOS** | **Maximum Scaling**: Leverages Haiku's naturally thread-safe `BView` looper to render multiple tiles simultaneously. |
| **Windows** | **GPU Latency Reduction**: Direct2D Command Lists can be recorded in parallel and submitted to the GPU in a single batch. |
| **Linux (GTK/Qt)** | **Bypass Single-Core Limits**: Offloads CPU-intensive SIMD rasterization (Blend2D) away from the main event loop. |
| **macOS** | **UI Responsiveness**: Ensures heavy "Core Text" layout tasks don't block the Cocoa event loop. |

---

## 4. JavaScript Threading Model
Wisp implements a **hybrid threading model** to balance safety with performance.

### Single-Threaded DOM Access
To ensure safe interaction with the underlying C-based DOM (`libdom`), which is not thread-safe, all scripts that manipulate page elements run on the **Main UI Thread**. This prevents race conditions and memory corruption without the overhead of complex locking mechanisms.

### Background Worker Pool (`wisp_subsystem`)
The browser includes a dedicated worker pool subsystem (`src/content/handlers/javascript/quickjs/wisp_subsystem.c`) that can spawn up to **7 background threads**. This infrastructure allows Wisp to:
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
*   **JS JIT**: Enable JIT-compilation in the QuickJS-ng engine for faster execution of compute-heavy JavaScript.

### Stability
*   **Process Isolation**: Moving the JavaScript engine and network stack into separate OS processes (multi-process architecture). This ensures that a single malicious or buggy script cannot crash the entire browser window.
*   **Memory Auditing**: Continuous resolution of memory leaks during runtime teardown (Current focus: QuickJS/DOM bridge cycles).

### Security
*   **Content Security Policy (CSP)**: Implement full CSP header support to mitigate Cross-Site Scripting (XSS) at the engine level.
*   **OS-Level Sandboxing**: Utilize features like **Landlock (Linux)** or **AppContainer (Windows)** to isolate the browser from the user's sensitive filesystem data.

---

## 7. Recent Technical Improvements (June 2026 Update)
The following stability and compatibility fixes have been integrated:
1.  **Web API Initialization**: Corrected `js_newthread` to ensure `navigator`, `location`, `storage`, and `XMLHttpRequest` are fully initialized with correct private data before script execution.
2.  **Bridge Stability**: Fixed a critical `JS_FreeRuntime` assertion failure by ensuring the DOM bridge explicitly frees JSValue references and clears the runtime opaque pointer during cleanup.
3.  **Initialization Ordering**: Reordered the JS startup sequence to ensure core bindings are registered before the bridge attempts to wrap LibDOM nodes.
