# Wisp

Wisp is tackling some of the hardest problems in browser development—bringing modern layout paradigms (Grid, Flexbox, CSS variables) and modern JS to a lightweight, forked codebase. While it maintains the spirit of a lean, portable browser, Wisp aspires to be a first-class citizen of the "modern web".

## Why?
Wisp has a different development vision from Netsurf. While Netsurf is a browser for the "old web", Wisp provides a bridge to modern standards without the bloat of mainstream engines.
We appreciate the philosophy of Netsurf, and intend to keep the spirit of the project alive: a lean, small, and portable browser.

![Wisp](img/wisp_home.png?raw=true "Wisp Homepage")
![GNU.org](img/wisp_gnu.png?raw=true "GNU.org")

## Development
Current development is focused on completing the CSS Variables implementation, and refining the Incremental Layout engine. The project has recently achieved parity between the Windows GDI frontend and the reference Qt frontend, and integrated high-performance vector graphics via Blend2D.

### Recently Completed
*   **Position: Sticky**: Full support for sticky positioning across layout and rendering.
*   **Stateful Path API**: A modernized plotter interface for efficient vector shape rendering.
*   **ISOBMFF Support**: Native AVIF, HEIC, and HEIF image decoding.
*   **Incremental Layout Core**: Implemented a dual-pass dirty-bit system (`DIRTY_INTRINSIC`, `DIRTY_LAYOUT`) to optimize reflows.

## Biggest differences from Netsurf
* Removed compatibility for super old and/or obscure libraries/software/operating systems
* Dedicated LibreSSL support
* Numerous privacy improvements
* Rewritten build system (CMake-based)
* Simplified frontend development
* **Modern CSS Features**: Native support for CSS Grid, Flexbox, `calc()`, and `position: sticky`.
* **Integrated JS Engine**: Uses QuickJS-ng (v0.15.1) for modern ES2023+ JavaScript support. Automated WebIDL binding generation ensures rapid coverage of modern DOM APIs.
* **Incremental Layout**: High-performance "dirty-bit" based reflow system designed to minimize CPU cycles on dynamic modern pages.
* **Modern Media**: Native support for AVIF, HEIC, and HEIF image formats via `libavif` v1.4.2 and FFmpeg-based media pipeline.

## Known Issues
* CSS Variables support is partially implemented (variable resolution pass is in progress).
* Advanced JS APIs like `MutationObserver` and `IntersectionObserver` are pending manual implementation.
* Haiku and Framebuffer frontends require re-testing after recent core layout updates.

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
