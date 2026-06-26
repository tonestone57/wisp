# Wisp Rendering Architecture

This document details the architectural decisions for Wisp's rendering subsystem, specifically the unification around Blend2D and the transition to a fixed-tile redraw strategy.

## 1. Unified Rendering with Blend2D

Wisp has unified its rendering backbone around **Blend2D** across all supported operating systems.

### The Case for Blend2D
1.  **Massive Code Deduplication**: Historically, frontends (GDI, Qt, GTK) implemented their own drawing logic. By using Blend2D, Wisp utilizes a single `plotter_table` implementation (`src/desktop/plot_blend2d.c`). Frontends now primarily handle windowing and blitting the raw pixel buffer.
2.  **Industry-Leading Performance**: Blend2D uses AsmJit to generate optimized 2D pipelines on the fly, leveraging AVX2, AVX-512, or AArch64 NEON. It routinely outperforms Cairo, Skia (CPU), and native platform APIs.
3.  **Pixel-Perfect Consistency**: Unifying the rasterizer ensures that layout bugs are consistent across platforms, eliminating "it works on my machine" issues caused by different font engines or clipping rules.
4.  **Modern CSS Support**: Blend2D natively handles complex gradients, blending modes, and arbitrary path clipping required by modern web specifications.

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

### Constraints and Considerations
*   **CPU-Based**: Blend2D is a software-based CPU renderer. It does not utilize the GPU.
*   **Architecture Limits**: Peak performance requires JIT support (x86/ARM64). On other architectures (like i586), it falls back to a slower portable C++ pipeline.
*   **Blitting Overhead**: Frontends still require minimal platform-specific code to hand the Blend2D buffer to the OS (e.g., `StretchDIBits` on Windows, `QImage` on Qt).

---

## 2. Fixed-Tile Redraw Strategy

Wisp is transitioning from a union-based dirty region system (which suffers from "overdraw hell") to a **Fixed-Tile Redraw** strategy.

### Why Fixed Tiles?
1.  **Cache Locality**: i586 and older processors benefit significantly from cache locality. A 256x256 tile represents a tiny, contiguous chunk of memory that is more likely to stay in the CPU's L1/L2 cache. **Cache locality is the surrogate SIMD for older hardware.**
2.  **Zero Fragmentation**: Every tile backing buffer requires a fixed amount of memory (e.g., 256KB for 256x256). This allows for a fixed-block pool inside the **Arena Allocator**. Tiles can be recycled without `malloc`/`free` overhead.
3.  **Fast Coordinate Translation**: Finding a tile for a pixel uses bit-shifts (e.g., `x >> 8` for 256x256) instead of expensive integer division. On an i586, this makes tile lookup practically free.
4.  **Predictable Threading**: Fixed-size tiles provide uniform workloads for multi-threaded rendering, avoiding the "one giant tile" bottleneck common in dynamic tiling systems.

### Implementation Details
*   **Scale-Aware Fixed Tiles**: While 256x256 is the "sweet spot" for retro systems, modern High-DPI, 4K, and mobile screens utilize **512x512** tiles to reduce tile-management overhead on high-density displays. The size is locked once during initialization based on platform DPI.
*   **Clipping**: The rendering loop iterates through dirty tiles and clips the `B2Context` to the tile bounds. Blend2D's rasterizer then rejects geometry outside that boundary.
*   **Global Redraw Threshold**: If >70% of tiles are dirty, the engine drops the tiling loop for a single full-screen pass to reduce management overhead.
*   **Bleeding Margins**: Dirty tile calculations include a slight "padding" (1-2 pixels) to prevent clipping artifacts for glyphs or borders sitting exactly on a boundary.

---

## 3. Platform Status (June 2026)

| OS | Backend | Status |
|---|---|---|
| **Windows** | Blend2D -> GDI | Finished (Parity with Qt) |
| **Linux** | Blend2D -> Qt6/GTK3 | Reference implementation |
| **Haiku** | Blend2D -> libbe (BView) | Native implementation |
| **macOS** | Blend2D -> Cocoa | Leveraging AArch64 JIT |
| **i586** | Blend2D (Scalar) | Operational, benefits from tiling |
