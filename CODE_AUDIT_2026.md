# Wisp Code Audit Report - June 2026

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine as of June 2026. The project has transitioned from the legacy codebase to a modernized architecture featuring QuickJS-ng, an incremental layout engine, and advanced CSS support (Grid, Flexbox, Sticky). Significant progress has been made in media support (AVIF/ISOBMFF) and frontend parity.

## 2. Library Versions Audit

| Library | Repo Version | Latest Online (June 2026) | Status |
|---------|--------------|---------------------------|--------|
| `quickjs-ng` | v0.15.1 | v0.15.1 | **[Finished]** Up-to-date |
| `blend2d` | v0.21.2 | v0.21.2 | **[Finished]** Up-to-date |
| `libavif` | v1.4.2 | v1.4.2 | **[Finished]** Up-to-date |
| `libcss` | Jan 2026 Fork | 0.9.2 (Upstream) | **[Partial]** Diverged for Grid/Calc |
| `libdom` | Jan 2026 Fork | Upstream Git | **[Partial]** Diverged for SVG/JS |
| `FFmpeg` | Linked System | 7.x | **[Finished]** Compatible |

## 3. Detailed Subsystem Analysis

### 3.1 Core Layout engine
*   **Incremental Layout [Partial]**: Utilizes `DIRTY_INTRINSIC`, `CHILD_DIRTY`, and `DIRTY_LAYOUT` flags. Correctly skips reflows for stable subtrees.
    *   *Optimization*: Dirty rectangle accumulation in `box_mark_dirty` ensures previous positions are cleared.
    *   *Improvement needed*: Bounding box union logic could be optimized for elements entirely contained within parent dirty regions.
*   **CSS Grid [Partial]**: Core layout logic and 3-phase auto-placement implemented.
    *   *Optimization*: Pass 3 uses cached placement data to avoid re-parsing CSS during final stretch.
    *   *Improvement needed*: Dense packing algorithm and complex spanning edge cases require further refinement.
*   **CSS Flexbox [Partial]**: Supports flex-grow, shrink, and auto-margins.
    *   *Finished*: Two-pass resolution for column flex with indefinite heights.
*   **Position: Sticky [Finished]**: Full support for both global viewport and scrollable ancestor constraints. Verified multi-axis clamping.
*   **Percentage Widths [Incomplete]**: TODOs remain for IFRAMEs, text-indent, and max-height constraints in `layout.c`.

### 3.2 JavaScript Subsystem (QuickJS-ng)
*   **Integration [Finished]**: Migration to QuickJS-ng v0.15.1 complete.
*   **Binding Generator [Finished]**: Automated WebIDL compiler handles boilerplate and generates ~1,500 weak stubs to maintain linker compatibility.
*   **Observers [Partial]**: `MutationObserver` and `IntersectionObserver` have manual implementation infrastructure.
    *   *Incomplete*: Deep integration with LibDOM's internal mutation hooks is pending.
*   **Memory Management [Partial]**: Interaction between QuickJS GC and LibDOM node mapping requires monitoring.
    *   *Bug*: ~720 bytes leaked across 27 allocations during teardown (LeakSanitizer finding).

### 3.3 Media Subsystem
*   **ISOBMFF [Finished]**: Native sniffing and brand iteration for AVIF, HEIC, and HEIF.
*   **FFmpeg [Finished]**: Asynchronous video decoding pipeline functional in `video.c`.
    *   *Optimization*: Software volume scaling and frame synchronization logic implemented.

### 3.4 Frontends
*   **Windows GDI [Finished]**: Achieved parity with Qt frontend.
    *   *Optimization*: Batching of consecutive path commands reduces kernel transitions.
*   **GTK/Cairo [Finished]**: Stable reference frontend.
*   **Blend2D Backend [Finished]**: High-performance vector rendering.
*   **Haiku / BeOS [Partial]**: Requires re-verification against the new incremental layout engine.

## 4. Bugs and Technical Debt
*   **[Bug] ODR Violation**: `journal_test` fails due to duplicate definition of `guit` symbol.
*   **[Bug] LibCSS Regression**: `libcss_parse_auto_custom-properties_dat` failing on complex variable fallbacks.
*   **[Debt] Box Construction**: Missing support for nested CSS counters and tab character expansion in `box_construct.c`.

## 5. Future Recommendations and Optimizations
1.  **Redraw Tiling**: Implement a tiled redraw strategy in `html_content` to avoid redundant painting of clean areas when multiple disjoint regions are dirty.
2.  **LibDOM Native Observers**: Implement a native notification system within LibDOM to allow `MutationObserver` to respond to DOM changes without relying on legacy MutationEvents.
3.  **SIMD Alignment**: Strictly enforce 64-byte alignment in the arena allocator (`src/utils/arena.c`) to better support AVX-512 operations in layout and rendering.
4.  **JS Event Loop Integration**: Tighten integration between the asynchronous fetch pipeline and the QuickJS event loop to reduce latency in SPAs.
5.  **Path Accumulation**: Further optimize the Windows GDI plotter by using `PolyBezier` for contiguous segments.
