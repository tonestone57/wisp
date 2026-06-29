# Wisp Code Audit Report - June 2026

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine, focusing on modern CSS support, incremental layout, and the QuickJS-based JavaScript subsystem. Wisp has achieved significant milestones in standard conformance while maintaining a lean architecture. The migration to QuickJS-ng is a major architectural shift, providing a robust foundation for modern web standards.

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
*   **Stateful Vector Path API**: Modernized plotter interface (MoveTo, LineTo, BezierTo) implemented across GTK (Cairo), Windows (GDI), and Blend2D.
*   **Unified Rendering (Blend2D)**: Unified rendering backbone across all frontends for pixel-perfect consistency.
*   **Fixed-Tile Redraw**: Scale-aware 256x256 tile strategy implemented to optimize performance and cache locality.
*   **Native Haiku/BeOS Frontend**: Fully integrated with Blend2D and fixed-tile redraw strategy.
*   **Incremental Layout Core**: Dual-pass reflow system using `DIRTY_INTRINSIC`, `CHILD_DIRTY`, and `DIRTY_LAYOUT` flags.
*   **A/V Master Clock Sync**: Robust synchronization between audio and video tracks in `video.c` using a centralized master clock.
*   **SIMD-Aligned Arena**: The arena allocator (`src/utils/arena.c`) enforces 64-byte alignment to support AVX-512 and other SIMD optimizations.
*   **IntersectionObserver**: Fully integrated into the layout engine via post-layout hooks in `layout.c` and `html.c`.
*   **Web Crypto (Basic)**: Bridged `crypto.getRandomValues` and `crypto.subtle.digest` to LibreSSL.
*   **Nested CSS Counters**: [Finished] Full support for nested counter scoping and inheritance in `box_construct.c`.
*   **Tab-Size Support**: [Finished] Implementation of `tab-size` property with proper tab-stop calculation in the layout engine.

### 3.2 Partial Implementation [Partial]
*   **CSS Variables**: Selection and parsing of `var()` and custom properties are complete; resolution pass during cascade is in progress.
*   **CSS Grid**: Core layout logic implemented in LibCSS fork; 3-phase auto-placement and FR unit distribution are functional. Specific edge cases in dense packing (dense flow) remain.
*   **CSS Flexbox**: Supports flex-grow, shrink, auto-margins, and two-pass resolution for column flex.
*   **Incremental Reflow**: Functional, but bounding box union logic in `box_mark_dirty` lacks optimization for elements entirely contained within parent dirty regions. Tiling child-clipping is under refinement.
*   **MutationObserver**: Integrated with LibDOM via a native mutation hook system (`dom_document_set_mutation_hook`), though handling of specific "remove" vs "add" events needs refinement.

### 3.3 Not Implemented / Planned [Incomplete]
*   **Canvas 2D API**: WebIDL stubs exist, but implementation bridging to the plotter engine is missing.
*   **Percentage Widths**: Missing resolution for IFRAMEs, text-indent, and certain max-height constraints in `layout.c`.

## 4. Subsystem Deep-Dive

### 4.1 QuickJS Binding Generator
The project uses an automated WebIDL compiler (`utils/qjs_binding_generator.py`) to bridge QuickJS-ng and LibDOM.
*   **Coverage**: Processes all `.idl` files in `src/content/handlers/javascript/WebIDL/`.
*   **Mechanism**: Generates one `.gen.c` and `.gen.h` file per interface.
*   **Stubs**: For unimplemented methods, it generates `__attribute__((weak))` C stubs that log a warning. Currently, ~1,539 missing bindings are stubbed.

### 4.2 Windows GDI Plotter
*   **Optimization**: Batches consecutive path commands within `win_plot_play_stateful_path`. Batches of 32 points or fewer use a stack-allocated buffer to reduce kernel transitions.

## 5. Bugs and Technical Debt

### 5.1 Identified Bugs
*   **[Finished] ODR Violation**: Resolved duplicate definition of `guit` symbol in test code. Tests linking against `libwisp` (`journal_test.c`, `test_quickjs.c`) now correctly use `extern`, while standalone mock tests retain local definitions.
*   **[Bug] QuickJS Leaks**: LeakSanitizer identified ~720 bytes leaked during JS runtime teardown across 27 allocations. The `qjs_bridge_cleanup` strategy avoids explicit `JS_FreeValueRT` during map destruction to prevent Use-After-Free, but this may contribute to the reported leaks if finalizers are not triggered for all bridge entries.
*   **[Bug] CSS Variable Regression**: `libcss_parse_auto` fails on certain custom property definitions involving complex fallbacks.
*   **[Finished] Binding Type Mismatch**: Verified that manual implementation signatures in `eventtarget_impl.c` and `xhr_impl.c` match the current output of the WebIDL binding generator.
*   **[Finished] Test Runner Syntax Errors**: Resolved syntax errors and format mismatches in `contrib/libcss/test/parse-auto.c` (added `min` macro, fixed `%zu` format strings).

### 5.2 Technical Debt
*   **NSLOG Verbosity**: High-verbosity layout traces in `layout_flex.c` and `layout_grid.c` should be demoted to `NSLOG_LEVEL_DEEPDEBUG`.

## 6. Future Recommendations and Advice
1.  **Binding Coverage**: Prioritize manual implementation of high-value WebIDL bindings like `Element.querySelector` and `Element.querySelectorAll` (currently missing).
2.  **Canvas 2D Bridge**: Implement the plotter bridge for the Canvas 2D API (WebIDL stubs exist but logic is pending).
3.  **Percentage Width Refinement**: Complete resolution for IFRAMEs and text-indent in the layout engine.
4.  **JS Event Loop Integration**: Tighten integration between the asynchronous fetch pipeline and the QuickJS event loop to reduce latency.
5.  **SIMD Layout**: Utilize the 64-byte aligned arena to implement SIMD-accelerated layout calculations (e.g., for Grid/Flexbox).
