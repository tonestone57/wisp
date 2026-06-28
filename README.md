# Wisp

Wisp is tackling some of the hardest problems in browser development—bringing modern layout paradigms (Grid, Flexbox, CSS variables) and modern JS to a lightweight, forked codebase. While it maintains the spirit of a lean, portable browser, Wisp aspires to be a first-class citizen of the "modern web".

## Why?
Wisp has a different development vision from Netsurf. While Netsurf is a browser for the "old web", Wisp provides a bridge to modern standards without the bloat of mainstream engines.
We appreciate the philosophy of Netsurf, and intend to keep the spirit of the project alive: a lean, small, and portable browser.

![Wisp](img/wisp_home.png?raw=true "Wisp Homepage")
![GNU.org](img/wisp_gnu.png?raw=true "GNU.org")

## Development
Current development is focused on completing the CSS Variables implementation, and refining the Incremental Layout engine. The project has unified its rendering backbone around **Blend2D** for pixel-perfect consistency across all platforms and is transitioning to a **Fixed-Tile Redraw** strategy to optimize performance on both retro and modern hardware.

### Core Features Status (June 2026)
*   **[Finished] Unified Rendering (Blend2D)**: Blend2D is the primary rendering engine across all frontends, ensuring massive code deduplication and industry-leading software rasterization.
*   **[Finished] Fixed-Tile Redraw**: Scale-aware 256x256 tile strategy implemented to optimize performance and cache locality.
*   **[Finished] Native Haiku/BeOS Frontend**: Fully integrated with Blend2D and the fixed-tile redraw strategy.
*   **[Finished] Position: Sticky**: Full support for multi-axis sticky positioning with scroll-container constraints.
*   **[Finished] Stateful Vector Path API**: Efficient path rendering (MoveTo, LineTo, BezierTo) across all modern frontends.
*   **[Finished] ISOBMFF & AVIF**: Native support for AVIF, HEIC, and HEIF formats via linked submodules.
*   **[Finished] QuickJS-ng Integration**: Migration to QuickJS-ng (v0.15.1) for ES2023+ support.
*   **[Partial] CSS Grid**: Robust 3-phase auto-placement and FR unit distribution; dense packing refinements ongoing.
*   **[Partial] CSS Flexbox**: Support for flex-grow, shrink, auto-margins, and column-flex two-pass resolution.
*   **[Partial] Incremental Layout**: Dual-pass dirty-bit system active. Refinement of child-clipping in tiled redraw in progress.
*   **[Partial] CSS Variables**: Parsing and selection of `var()` complete; resolution pass is active with minor regressions.

## Biggest differences from Netsurf
* Removed compatibility for super old and/or obscure libraries/software/operating systems
* Dedicated LibreSSL support
* Numerous privacy improvements
* Rewritten build system (CMake-based)
* Simplified frontend development
* **Modern CSS Features**: Native support for CSS Grid, Flexbox, `calc()`, and `position: sticky`.
* **Integrated JS Engine**: Uses QuickJS-ng (v0.15.1) for modern ES2023+ JavaScript support. Automated WebIDL binding generation ensures rapid coverage of modern DOM APIs.
* **Tiled Incremental Layout**: High-performance "dirty-bit" based reflow system with a **Fixed-Tile Redraw** strategy to minimize CPU cycles and overdraw.
* **Modern Media**: Native support for AVIF, HEIC, and HEIF image formats via `libavif` v1.4.2 and FFmpeg-based media pipeline.

## Known Issues
* **[Partial] CSS Variables**: Variable resolution during cascade has known regressions in complex fallback scenarios.
* **[Partial] JS Observers**: `MutationObserver` and `IntersectionObserver` have infrastructure stubs but lack deep LibDOM integration.
* **[Incomplete] Canvas 2D API**: WebIDL stubs exist, but the bridge to the plotter engine is pending.
* **[Incomplete] Percentage Widths**: Missing resolution for IFRAMEs and certain text-indent contexts in the layout engine.
* **[Bug] QuickJS Leaks**: ~720 bytes leaked during JS runtime teardown (confirmed by LeakSanitizer).
* **[Bug] ODR Violation**: `journal_test` fails due to duplicate definition of `guit` symbol in `gui_factory.c` and test code.
* **[Bug] Binding Mismatches**: Conflict between manual implementations and generated WebIDL headers in `eventtarget_impl.c` and `xhr_impl.c`.

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
