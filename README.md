# Wisp

Wisp is tackling some of the hardest problems in browser development—bringing modern layout paradigms (Grid, Flexbox, CSS variables) and modern JS to a lightweight, forked codebase. While it maintains the spirit of a lean, portable browser, Wisp aspires to be a first-class citizen of the "modern web".

## Why?
Wisp has a different development vision from Netsurf. While Netsurf is a browser for the "old web", Wisp provides a bridge to modern standards without the bloat of mainstream engines.
We appreciate the philosophy of Netsurf, and intend to keep the spirit of the project alive: a lean, small, and portable browser.

![Wisp](img/wisp_home.png?raw=true "Wisp Homepage")
![GNU.org](img/wisp_gnu.png?raw=true "GNU.org")

## Development
Current development is focused on completing the CSS Variables implementation and refining the Incremental Layout engine. The project supports **Blend2D** for high-performance software rasterization and has implemented a high-performance native **Direct2D & DirectWrite** pipeline for Windows. Wisp utilizes a **Fixed-Tile Redraw** strategy to optimize performance on both retro and modern hardware.

### Core Features Status (August 2026)
*   **[Finished] Blend2D Integration**: Blend2D is available as a high-performance rendering engine across frontends, ensuring pixel-perfect software rasterization and SIMD optimization.
*   **[Finished] Native Direct2D & DirectWrite (Windows)**: Hardware-accelerated rendering pipeline for modern Windows systems, integrated with the core.
*   **[Finished] Fixed-Tile Redraw**: Scale-aware 256x256 or 512x512 tile strategy implemented to optimize performance and cache locality.
*   **[Finished] Native Haiku/BeOS Frontend**: Fully integrated with Blend2D and the fixed-tile redraw strategy.
*   **[Finished] IntersectionObserver**: Fully integrated into the layout engine via post-layout hooks.
*   **[Finished] A/V Master Clock**: Synchronized audio and video tracks in the FFmpeg-based media pipeline.
*   **[Finished] SIMD-Aligned Arena**: The arena allocator enforces 64-byte alignment for AVX-512 and SIMD optimizations.
*   **[Finished] Position: Sticky**: Full support for multi-axis sticky positioning with scroll-container constraints.
*   **[Finished] Stateful Vector Path API**: Efficient path rendering (MoveTo, LineTo, BezierTo) across all modern frontends (GDI, Direct2D, Cairo, Blend2D).
*   **[Finished] ISOBMFF & AVIF**: Native support for AVIF, HEIC, and HEIF formats via linked submodules.
*   **[Finished] QuickJS-ng Integration**: Migration to QuickJS-ng (v0.15.1) for ES2023+ support.
*   **[Finished] Nested CSS Counters**: Full support for nested counter scoping and inheritance in `box_construct.c`.
*   **[Finished] Tab-Size Support**: Implementation of `tab-size` property with proper tab-stop calculation in the layout engine.
*   **[Finished] LibCSS Test Runner Fixes**: Resolved long-standing syntax and format issues in the `parse-auto` runner.
*   **[Finished] CSS Grid**: Spec-compliant 3-phase auto-placement, FR unit distribution, and dense packing.
*   **[Finished] CSS Flexbox**: Full support for flex-grow, shrink, auto-margins, and two-pass resolution for column flex.
*   **[Finished] Incremental Layout**: Dual-strategy using a dirty-bit reflow system and scale-aware fixed-tile redraw for maximum efficiency.
*   **[Finished] CSS Variables**: Full parsing, selection, and recursive resolution pass with fallback support.
*   **[Finished] MutationObserver**: Native integration with LibDOM and optimized QuickJS callback delivery.
*   **[Finished] Percentage Widths**: Comprehensive resolution for nested percentage constraints and definite-height containing blocks.
*   **[Finished] DOM Selectors**: `querySelector` and `querySelectorAll` support with complex combinators and selector groups.
*   **[Finished] Content Security Policy (CSP)**: Enforcement of modern security headers (default-src, script-src, img-src, etc.) at both network and engine levels.
*   **[Finished] Tile Memory Recycling**: Thread-safe lookaside list of fixed-size 1MB tile buffers implemented to mitigate heap fragmentation.

## Biggest differences from Netsurf
* Removed compatibility for super old and/or obscure libraries/software/operating systems
* Dedicated LibreSSL support
* Numerous privacy improvements
* Rewritten build system (CMake-based)
* Simplified frontend development
* **Modern CSS Features**: Native support for CSS Grid, Flexbox, `calc()`, and `position: sticky`.
* **Integrated JS Engine**: Uses QuickJS-ng (v0.15.1) for modern ES2023+ JavaScript support. Automated WebIDL binding generation ensures rapid coverage of modern DOM APIs.
* **Windows Frontend Migration**: Core window and bitmap management migrated to C++ to leverage COM and modern STL containers.
* **Tiled Incremental Layout**: High-performance "dirty-bit" based reflow system with a **Fixed-Tile Redraw** strategy to minimize CPU cycles and overdraw.
* **Modern Media**: Native support for AVIF, HEIC, and HEIF image formats via `libavif` v1.4.2 and FFmpeg-based media pipeline.

## Known Issues
* **[Incomplete] Canvas 2D API**: WebIDL stubs exist, but the bridge to the plotter engine is pending.
* **[Bug] QuickJS Leaks**: ~720 bytes leaked during JS runtime teardown (confirmed by LeakSanitizer).

## Building and installation
Wisp can be built:
* On Windows (for the Windows frontend) using MSYS2 and the MinGW-w64 toolchain.
* On Linux (for the Qt frontend) using CMake and ninja or make.

Wisp intends to be portable, keeping a lean C99 codebase and minimal dependencies.

At build-time, Wisp requires the following programs:
* python3
* cmake
* any CMake-compatible build utility (typically make or ninja)
* gperf
* pkg-config or pkgconf
