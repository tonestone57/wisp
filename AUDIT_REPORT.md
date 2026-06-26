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
| `libnsbmp` | Jan 2026 Sync | Upstream Git | **[Finished]** Up-to-date |
| `libnsgif` | Jan 2026 Sync | Upstream Git | **[Finished]** Up-to-date |
| `FFmpeg` | Linked System | 7.x | **[Finished]** Compatible |
| `LibreSSL` | Linked System | 4.0.0 | **[Finished]** Compatible |

## 3. Feature Status Categorization

### 3.1 Complete Implementation [Finished]
*   **Position: Sticky**: Full support for sticky positioning, including multi-axis clamping and scroll-container constraints. Verified in `layout_apply_sticky_clamping`.
*   **ISOBMFF Support**: Native decoding for AVIF, HEIC, and HEIF formats via generalized signature sniffing in `mimesniff.c`.
*   **Stateful Vector Path API**: Modernized plotter interface (MoveTo, LineTo, BezierTo) implemented across GTK (Cairo), Windows (GDI), and Blend2D.
*   **Incremental Layout Core**: Dual-pass reflow system using `DIRTY_INTRINSIC`, `CHILD_DIRTY`, and `DIRTY_LAYOUT` flags.
*   **Web Crypto (Basic)**: Bridged `crypto.getRandomValues` and `crypto.subtle.digest` to LibreSSL.

### 3.2 Partial Implementation [Partial]
*   **CSS Variables**: Selection and parsing of `var()` and custom properties are complete; resolution pass during cascade is in progress.
*   **CSS Grid**: Core layout logic implemented in LibCSS fork; 3-phase auto-placement and FR unit distribution are functional. Specific edge cases in dense packing remain.
*   **CSS Flexbox**: Supports flex-grow, shrink, auto-margins, and two-pass resolution for column flex.
*   **Incremental Reflow**: Functional, but bounding box union logic in `box_mark_dirty` lacks optimization for elements entirely contained within parent dirty regions.
*   **CSS Counters**: Initial support for counters; nested counter scope resolution needs implementation in `box_construct.c`.

### 3.3 Not Implemented / Planned [Incomplete]
*   **MutationObserver / IntersectionObserver**: Infrastructure stubs exist; deep integration with LibDOM mutation hooks is required for full functionality.
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
*   **[Bug] ODR Violation**: `journal_test` fails due to duplicate definition of `guit` symbol in `gui_factory.c` and test code.
*   **[Bug] QuickJS Leaks**: LeakSanitizer identified ~720 bytes leaked during JS runtime teardown.
*   **[Bug] CSS Variable Regression**: `libcss_parse_auto` fails on certain custom property definitions involving complex fallbacks.

### 5.2 Technical Debt
*   **Box Construction**: `box_construct.c` lacks full support for nested CSS counters and proper tab character expansion (TODOs at lines 420, 1847).
*   **NSLOG Verbosity**: High-verbosity layout traces in `layout_flex.c` and `layout_grid.c` should be demoted to `NSLOG_LEVEL_DEEPDEBUG`.

## 6. Future Recommendations and Advice
1.  **Redraw Optimization**: Move from union-based dirty regions to a tiled redraw strategy to improve performance on large, complex pages.
2.  **LibDOM Native Observers**: Refactor LibDOM to provide a native internal notification system for mutations, which would allow a performant implementation of `MutationObserver`.
3.  **SIMD Acceleration**: Leverage the 64-byte aligned arena allocator to implement SIMD-accelerated layout calculations and color space conversions.
4.  **FFmpeg Pipeline**: Improve synchronization between audio and video tracks in `video.c` by implementing a more robust master-clock system.
5.  **Binding Coverage**: Prioritize manual implementation of high-value WebIDL bindings like `Element.querySelector` and `Element.querySelectorAll`.
