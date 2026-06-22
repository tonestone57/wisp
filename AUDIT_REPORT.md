# Wisp Code Audit Report - June 2026

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine, focusing on modern CSS support, incremental layout, and the QuickJS-based JavaScript subsystem. Wisp has achieved significant milestones in standard conformance while maintaining a lean architecture. The migration to QuickJS-ng is a major architectural shift, providing a robust foundation for modern web standards.

## 2. Library Versions Audit

| Library | Repo Version | Latest Online (June 2026) | Status |
|---------|--------------|---------------------------|--------|
| `quickjs-ng` | v0.15.1 | v0.15.1 | Up-to-date |
| `blend2d` | v0.21.2 | v0.21.2 | Up-to-date |
| `libavif` | v1.4.2 | v1.4.2 | Up-to-date |
| `libcss` | Jan 2026 Fork | 0.9.2 (Upstream) | Diverged (Forked for Grid/Calc) |
| `libdom` | Jan 2026 Fork | Upstream Git | Diverged (Forked for SVG/JS) |
| `libhubbub` | Jan 2026 Sync | Upstream Git | Moderate Divergence |
| `libnsbmp` | Jan 2026 Sync | Upstream Git | Up-to-date |
| `libnsgif` | Jan 2026 Sync | Upstream Git | Up-to-date |
| `FFmpeg` | Linked System | 7.1 | Compatible |
| `LibreSSL` | Linked System | 4.0.0 | Compatible |

## 3. Feature Status Categorization

### 3.1 Complete Implementation
*   **Position: Sticky**: Full support for sticky positioning, including multi-axis clamping and scroll-container constraints. Verified in `layout_apply_sticky_clamping`.
*   **ISOBMFF Support**: Native decoding for AVIF, HEIC, and HEIF formats via generalized signature sniffing in `mimesniff.c`.
*   **Stateful Vector Path API**: Modernized plotter interface (MoveTo, LineTo, BezierTo) implemented across GTK (Cairo), Windows (GDI), and Blend2D.
*   **Incremental Layout Core**: Dual-pass reflow system using `DIRTY_INTRINSIC`, `CHILD_DIRTY`, and `DIRTY_LAYOUT` flags.
*   **Web Crypto (Basic)**: Bridged `crypto.getRandomValues` and `crypto.subtle.digest` to LibreSSL.

### 3.2 Partial Implementation
*   **CSS Variables**: Selection and parsing of `var()` and custom properties are complete; resolution pass during cascade is in progress.
*   **CSS Grid**: Core layout logic implemented in LibCSS fork; specific edge cases in `fr` unit distribution and auto-placement remain.
*   **Percentage Widths**: Basic support exists, but complex contexts (IFRAMEs, nested flexbox) still have `TODO` markers in `layout.c`.
*   **CSS Counters**: Initial support for counters; nested counter scope resolution needs implementation in `box_construct.c`.
*   **Incremental Reflow**: Functional, but bounding box union logic in `box_mark_dirty` lacks optimization for elements entirely contained within parent dirty regions.

### 3.3 Not Implemented / Planned
*   **MutationObserver / IntersectionObserver**: Manual bindings are pending; required for many modern single-page applications.
*   **Canvas 2D API**: WebIDL stubs exist, but implementation bridging to the plotter engine is missing.
*   **Advanced JS Bindings**: Approximately 1,500 WebIDL bindings are auto-generated as weak stubs by `utils/qjs_binding_generator.py`. These allow code to run without crashing but currently only log warnings.

## 4. QuickJS Binding Generator Deep-Dive
The project uses an automated WebIDL compiler (`utils/qjs_binding_generator.py`) to bridge QuickJS-ng and LibDOM.
*   **Coverage**: The generator processes all `.idl` files in `src/content/handlers/javascript/WebIDL/`.
*   **Mechanism**: It generates one `.gen.c` and `.gen.h` file per interface.
*   **Stubs**: For unimplemented methods, it generates `__attribute__((weak))` C stubs that log a warning. This prevents "undefined symbol" errors during linking and allows incremental implementation of the 1,539+ missing bindings.
*   **Optimization**: The generator handles inheritance via `JS_SetPrototype` and avoids variable name collisions with C keywords by appending `_val`.

## 5. Bugs and Technical Debt

### 5.1 Identified Bugs
*   **ODR Violation**: `journal_test` fails due to duplicate definition of `guit` symbol in `gui_factory.c` and test code.
*   **QuickJS Leaks**: LeakSanitizer identified ~720 bytes leaked during JS runtime teardown, specifically in `emalloc` and `erealloc` wrappers.
*   **CSS Variable Regression**: `libcss_parse_auto` fails on certain custom property definitions involving complex fallbacks.
*   **Percentage Width TODOs**: `src/content/handlers/html/layout.c:801` and `layout_internal.h:547` identify missing percentage resolution for IFRAMEs and max-height constraints.
*   **Iframe Static Positioning**: `browser_window.c:3571` has a TODO about whether to return URLs for loading content, which might affect address bar updates during navigation.

### 5.2 Potential Improvements & Optimizations
*   **Redraw Tiling**: Currently, Wisp unions all dirty rectangles into a single bounding box. Implementing a tiled or list-based redraw would prevent redundant painting of clean areas between distant updates.
*   **Path Accumulation**: In the Windows GDI plotter, stateful paths could be further optimized by using `PolyBezier` for contiguous segments.
*   **Arena Alignment**: Ensure `arena_alloc` maintains 16-byte alignment strictly to satisfy modern SIMD requirements on all platforms.
*   **JS Registration**: Centralize all manual and automated binding registrations to ensure deterministic initialization order.

## 6. Conclusion
Wisp has made significant strides in modernization. The immediate focus should be:
1.  Resolving percentage width issues in the layout engine.
2.  Completing the high-value JS bindings (MutationObserver).
3.  Resolving identified memory leaks in the QuickJS subsystem.
4.  Ensuring all frontend backends (Haiku, Framebuffer) are re-verified against the new incremental layout engine.
