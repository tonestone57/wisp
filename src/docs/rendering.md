# Wisp Rendering Architecture

This document details the architectural decisions for Wisp's rendering subsystem, including the unification around Blend2D, the transition to a fixed-tile redraw strategy, and the native Direct2D path for Windows.

## 1. Unified Rendering with Blend2D

Wisp has unified its rendering backbone around **Blend2D** across all supported operating systems.

### The Case for Blend2D
1.  **Massive Code Deduplication**: Historically, frontends implemented their own drawing logic. By using Blend2D, Wisp utilizes a single `plotter_table` implementation (`src/desktop/plot_blend2d.c`).
2.  **Industry-Leading Performance**: Blend2D uses AsmJit to generate optimized 2D pipelines on the fly, leveraging AVX2, AVX-512, or AArch64 NEON.
3.  **Pixel-Perfect Consistency**: Unifying the rasterizer ensures that layout bugs are consistent across platforms.
4.  **Modern CSS Support**: Blend2D natively handles complex gradients, blending modes, and arbitrary path clipping.

### Recommended Architecture
To keep the core clean, Wisp utilizes an internal rendering layer called **librender_blend2d**.

```
[ Wisp Core / LibDOM ]
         │
         ▼
[ librender_blend2d ]  <-- Implements plotter_table using Blend2D
         │
         ├────────────────────────┐
         ▼                        ▼
[ Qt Linux Frontend ]   [ Windows Win32 Frontend ]
 (Blits pixel buffer)    (Blits pixel buffer)
```

---

## 2. Native Direct2D Path (Windows)

While Blend2D is the unified backbone, the Windows frontend also provides a native **Direct2D and DirectWrite** rendering pipeline (`WISP_WINDOWS_USE_D2D`).

### Advantages of Direct2D
1.  **Hardware Acceleration**: Direct2D utilizes the GPU (via Direct3D) for rendering, reducing CPU load on modern systems.
2.  **Native Typography**: DirectWrite provides high-quality subpixel text rendering and advanced OpenType feature support that integrates natively with the Windows font system.
3.  **Low Latency**: Direct hardware access can reduce input-to-render latency.

The Direct2D implementation is proxied through a C-compatible wrapper (`nsws_drawable_paint_d2d`) to ensure compatibility with Wisp's C-based core.

---

## 3. Fixed-Tile Redraw Strategy

Wisp utilizes a **Fixed-Tile Redraw** strategy, replacing the legacy union-based dirty region system.

### Why Fixed Tiles?
1.  **Cache Locality**: i586 and older processors benefit significantly from cache locality. A 256x256 tile represents a tiny, contiguous chunk of memory likely to stay in CPU cache.
2.  **Zero Fragmentation**: Every tile backing buffer requires a fixed amount of memory, allowing for a fixed-block pool inside the **Arena Allocator**.
3.  **Fast Coordinate Translation**: Finding a tile for a pixel uses bit-shifts (e.g., `x >> 8`) instead of expensive integer division.
4.  **Predictable Threading**: Fixed-size tiles provide uniform workloads for multi-threaded rendering.

### Implementation Details
*   **Scale-Aware Fixed Tiles**: 256x256 for standard DPI, **512x512** for High-DPI displays.
*   **Clipping**: The rendering loop iterates through dirty tiles and clips the plotter context to the tile bounds.
*   **Global Redraw Threshold**: If >70% of tiles are dirty, the engine falls back to a single full-screen pass.

---

## 4. Platform Status (August 2026)

| OS | Backend | Status |
|---|---|---|
| **Windows** | Blend2D or Direct2D | Finished (Dual-path supported) |
| **Linux** | Blend2D -> Qt6/GTK3 | Reference implementation |
| **Haiku** | Blend2D -> libbe (BView) | Finished (Native implementation) |
| **macOS** | Blend2D -> Cocoa | Leveraging AArch64 JIT |
| **i586** | Blend2D (Scalar) | Operational, benefits from tiling |
