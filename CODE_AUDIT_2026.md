# Wisp Code Audit Report - June 2026

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine, focusing on modern CSS support, incremental layout, the QuickJS-ng based JavaScript subsystem, and unified rendering. Wisp has transitioned to a modernized architecture featuring QuickJS-ng v0.15.1, an incremental layout engine, and advanced CSS support (Grid, Flexbox, Sticky). The project has successfully unified its rendering backbone around Blend2D while providing a high-performance native Direct2D/DirectWrite path for Windows.

## 2. Library Versions Audit

| Library | Repo Version | Latest Online (June 2026) | Status |
|---------|--------------|---------------------------|--------|
| `quickjs-ng` | v0.15.1 | v0.15.1 | **[Finished]** Up-to-date |
| `blend2d` | v0.21.2 | v0.21.2 | **[Finished]** Up-to-date |
| `libavif` | v1.4.2 | v1.4.2 | **[Finished]** Up-to-date |
| `libcss` | Jan 2026 Fork | 0.9.2 (Upstream) | **[Partial]** Diverged (Forked for Grid/Calc) |
| `libdom` | Jan 2026 Fork | Upstream Git | **[Partial]** Diverged (Forked for SVG/JS) |
| `libhubbub` | Jan 2026 Sync | Upstream Git | **[Finished]** Moderate Divergence |
| `libnsbmp` | Jan 2026 Sync | Latest | **[Finished]** Up-to-date |
| `libnsgif` | Jan 2026 Sync | Latest | **[Finished]** Up-to-date |
| `FFmpeg` | Linked System | 7.x | **[Finished]** Compatible |
| `LibreSSL` | Linked System | 4.0.0 | **[Finished]** Compatible |

## 3. Feature Status Categorization

### 3.1 Complete Implementation [Finished]
*   **Position: Sticky**: Full support for sticky positioning, including multi-axis clamping and scroll-container constraints. Verified in `layout_apply_sticky_clamping`.
*   **ISOBMFF Support**: Native decoding for AVIF, HEIC, and HEIF formats via generalized signature sniffing in `mimesniff.c`.
*   **Stateful Vector Path API**: Modernized plotter interface (MoveTo, LineTo, BezierTo) implemented across GTK (Cairo), Windows (GDI/Direct2D), and Blend2D.
*   **Unified Rendering (Blend2D)**: Unified rendering backbone across all frontends for pixel-perfect consistency.
*   **Fixed-Tile Redraw**: Scale-aware 256x256 (standard) or 512x512 (High-DPI) tile strategy implemented to optimize performance and cache locality.
*   **Native Haiku/BeOS Frontend**: Fully integrated with Blend2D and fixed-tile redraw strategy.
*   **Native Direct2D & DirectWrite (Windows)**: High-performance C++ based hardware-accelerated rendering pipeline for modern Windows systems.
*   **Incremental Layout Core**: Dual-pass reflow system using `DIRTY_INTRINSIC`, `CHILD_DIRTY`, and `DIRTY_LAYOUT` flags.
*   **A/V Master Clock Sync**: Robust synchronization between audio and video tracks in `video.c` using a centralized master clock.
*   **SIMD-Aligned Arena**: The arena allocator (`src/utils/arena.c`) enforces 64-byte alignment to support AVX-512 and other SIMD optimizations.
*   **IntersectionObserver**: Fully integrated into the layout engine via post-layout hooks in `layout.c` and `html.c`.
*   **Web Crypto (Basic)**: Bridged `crypto.getRandomValues` and `crypto.subtle.digest` to LibreSSL.
*   **Nested CSS Counters**: Full support for nested counter scoping and inheritance in `box_construct.c`.
*   **Tab-Size Support**: Implementation of `tab-size` property with proper tab-stop calculation in the layout engine.
*   **ODR Violation Resolution**: Resolved duplicate definition of `guit` symbol in test code.
*   **Test Runner Fixes**: Resolved syntax and format errors in `contrib/libcss/test/parse-auto.c`.

### 3.2 Partial Implementation [Partial]
*   **CSS Variables**: Selection and parsing of `var()` and custom properties are complete; resolution pass during cascade is in progress with some regressions in complex fallback scenarios.
*   **CSS Grid**: Core layout logic implemented in LibCSS fork; 3-phase auto-placement and FR unit distribution are functional. Dense packing refinements are ongoing.
*   **CSS Flexbox**: Supports flex-grow, shrink, auto-margins, and two-pass resolution for column flex with indefinite heights.
*   **Incremental Reflow Optimization**: Functional, but bounding box union logic in `box_mark_dirty` is being refined for elements entirely contained within parent dirty regions.
*   **MutationObserver**: Integrated with LibDOM via a native mutation hook system, though event refinement is ongoing.

### 3.3 Not Implemented / Planned [Incomplete]
*   **Canvas 2D API**: WebIDL stubs exist, but implementation bridging to the plotter engine is missing.
*   **Percentage Widths**: Missing resolution for IFRAMEs and certain text-indent contexts in `layout.c`.

## 4. Subsystem Deep-Dive

### 4.1 Core Layout engine
*   **Incremental Layout**: Correctly skips reflows for stable subtrees using a dirty-bit system.
*   **Fixed-Tile Redraw**: Unified strategy optimizes cache locality and eliminates overdraw. Uses bit-shifts for fast coordinate translation.
*   **CSS Grid**: Pass 3 uses cached placement data to avoid re-parsing CSS during final stretch.

### 4.2 JavaScript Subsystem (QuickJS-ng)
*   **Integration**: Migration to QuickJS-ng v0.15.1 complete, providing ES2023+ support.
*   **Binding Generator**: Automated WebIDL compiler (`utils/qjs_binding_generator.py`) handles boilerplate and generates weak stubs. Implementation logic resides in `src/content/handlers/javascript/quickjs/impl/`.
*   **Memory Management**: `js_destroyheap` and `js_destroythread` implement multi-pass GC and explicit cycle breaking for observers to ensure stability.

### 4.3 Media Subsystem
*   **ISOBMFF**: Native sniffing for modern image brands.
*   **FFmpeg**: Asynchronous video decoding pipeline with software volume scaling.

### 4.4 Frontends
*   **Windows**: Partially migrated to C++ (`window.cpp`, `bitmap.cpp`) to support COM management and modern C++ containers. Supports both GDI and Direct2D/DirectWrite paths.
*   **Haiku / BeOS**: Native `libbe` frontend unified with Blend2D and fixed-tile redraw.

## 5. Bugs and Technical Debt

### 5.1 Identified Bugs
*   **[Bug] QuickJS Leaks**: ~720 bytes leaked across 27 allocations during teardown (LeakSanitizer finding).
*   **[Bug] CSS Variable Regression**: `libcss_parse_auto` fails on certain custom property definitions involving complex fallbacks.
*   **[Finished] Binding Type Mismatch**: Verified that manual implementation signatures match the WebIDL generator output.
*   **[Finished] ODR Violation**: `guit` symbol duplication resolved using weak definitions and `extern` correctly.

### 5.2 Technical Debt
*   **NSLOG Verbosity**: High-verbosity layout traces in `layout_flex.c` and `layout_grid.c` should be demoted.

## 6. Future Recommendations
1.  **Binding Coverage**: Prioritize manual implementation of `Element.querySelector` and `Element.querySelectorAll`.
2.  **Canvas 2D Bridge**: Implement the plotter bridge for the Canvas 2D API.
3.  **Percentage Width Refinement**: Complete resolution for IFRAMEs and text-indent.
4.  **SIMD Layout**: Utilize the 64-byte aligned arena for SIMD-accelerated layout calculations.
