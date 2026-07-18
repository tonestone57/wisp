# Wisp

Wisp is tackling some of the hardest problems in browser development—bringing modern layout paradigms (Grid, Flexbox, CSS variables) and modern JS to a lightweight, forked codebase. While it maintains the spirit of a lean, portable browser, Wisp aspires to be a first-class citizen of the "modern web".

## Why?
Wisp has a different development vision from Netsurf. While Netsurf is a browser for the "old web", Wisp provides a bridge to modern standards without the bloat of mainstream engines.
We appreciate the philosophy of Netsurf, and intend to keep the spirit of the project alive: a lean, small, and portable browser.

![Wisp](img/wisp_home.png?raw=true "Wisp Homepage")
![GNU.org](img/wisp_gnu.png?raw=true "GNU.org")

## Development
Wisp has completed its core CSS Variables implementation and optimized the Incremental Layout engine. The project supports **Blend2D** for high-performance software rasterization and has implemented a high-performance native **Direct2D & DirectWrite** pipeline for Windows. Wisp utilizes a **Fixed-Tile Redraw** strategy to optimize performance on both retro and modern hardware.

### Core Features Status (July 2026)
*   **[Finished] Blend2D Integration**: Blend2D is available as an optional alternative rendering choice and a unified high-performance software fallback backend across all frontends, ensuring pixel-perfect software rasterization and SIMD optimization when platform-native renderers are bypassed or unavailable.
*   **[Finished] Native Direct2D & DirectWrite (Windows)**: Hardware-accelerated rendering pipeline for modern Windows systems, integrated with the core.
*   **[Finished] Fixed-Tile Redraw**: Scale-aware 256x256 or 512x512 tile strategy implemented to optimize performance and cache locality.
*   **[Finished] CSP Level 3 Trusted Types & Security Hardening**: Strict auto-sanitizing default policy safety net with cryptographic nonce parsing and validation to completely block DOM-based XSS. Enforced on internal UI and extensions.
*   **[Finished] Link Pre-connect & DNS Prefetching Pipeline**: Automated `<link rel="dns-prefetch">` and `<link rel="preconnect">` processing. IPC network thread pool offloading to bypass server connection setup latency.
*   **[Finished] QuickJS Bytecode Ahead-of-Time (AOT) Caching**: Serializes parsed scripts to binary bytecode under `/tmp/wisp-bytecode-cache` to bypass lexing/parsing phases on subsequent visits, drastically accelerating performance.
*   **[Finished] LZ4 Compressed Tile Lookaside Lists**: Out-of-viewport tiles are dynamically compressed using real-time LZ4 compression (typical 4:1 compression ratio) to minimize RAM footprint, reclaiming them instantaneously on viewport scrollback.
*   **[Finished] SIMD-Accelerated UTF-8 processing**: High-performance ASCII/UTF-8 validation, case-folding, and UTF-32 conversion utilizing vectorized pipelines (**SSE2 on X86, NEON on ARM, and RVV on RISC-V**) with safe scalar fallbacks for older CPUs (i586 compatible).
*   **[Finished] WebSocket Payload Masking SIMD Acceleration**: Upstream WebSocket client-to-proxy payloads use optimized SIMD broadcast and dynamic XOR vectorization (**SSE2 on X86, NEON on ARM, and RVV on RISC-V**) to mask up to 16 bytes per clock cycle, eliminating upstream proxy network bottleneck.
*   **[Finished] Structural JSON Parsing for QuickJS-ng SIMD Pre-parser**: High-performance two-stage SIMD JSON pre-parser utilizing vectorized scanning (**SSE2, NEON, and RVV**) to identify structural candidates and skip whitespaces and string contents, generating fast offset caches for QuickJS-ng's parser.
*   **[Finished] SIMD CSP Nonce & Security Validation**: Extremely fast security header, nonce, and origin blocklist validation using vectorized string comparison primitives (**SSE2, NEON, and RVV**) with page-safe chunk boundary checking.
*   **[Finished] CSS Variable Caching & Fast-Path Evaluation**: Implemented style-context hashing/caching of custom property values in `libcss` to skip redundant recursive resolution passes, accelerating modern variable-heavy pages.
*   **[Finished] Native Haiku/BeOS Frontend**: Fully integrated with Blend2D, fixed-tile redraw strategy, BDirectWindow for low-latency blitting, and native widget parity.
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
*   **[Finished] Canvas 2D API**: Fully bridged to Blend2D and Direct2D plotter engines with support for transformations, paths, and image drawing.
*   **[Finished] Multi-Process Isolation**: JavaScript execution and Networking subsystems isolated into separate processes via a platform-agnostic IPC layer.
*   **[Finished] Web Worker Parity**: Full support for the Web Workers API, utilizing the `wisp_subsystem` worker pool for isolated script execution.

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
* **[Incomplete] WebGPU API**: Preliminary research for GPU-accelerated compute and rendering.

## Building and Installation

Wisp uses the CMake build system. It is designed to be portable across modern desktop operating systems.

### Build Requirements

To build Wisp, you will need:

*   **Compiler**: A C99 and C++17 compliant compiler (GCC 9+, Clang 10+, or MSVC 2019+).
*   **Build Tools**:
    *   Python 3.x
    *   CMake 3.20+
    *   gperf
    *   pkg-config or pkgconf
*   **Python Modules**:
    *   `widlparser` (required for JavaScript binding generation): `pip install widlparser`
*   **Required Libraries**:
    *   libxml2
    *   libcurl
    *   OpenSSL or LibreSSL
    *   libjpeg, libpng, libwebp
    *   FFmpeg (libavformat, libavcodec, libavutil, libswscale, libswresample)
    *   libpsl
    *   libutf8proc
    *   zlib

### Platform-Specific Instructions

#### Linux (Qt Frontend)
Install dependencies via your package manager (e.g., `apt`, `pacman`, `dnf`) and then run:
```bash
cmake -B build -GNinja -DWISP_BUILD_QT_FRONTEND=ON
cmake --build build
```

#### Windows (MSVC or MinGW-w64 via MSYS2)
For MSYS2/MinGW:
```bash
cmake -B build -GNinja -DWISP_BUILD_WINDOWS_FRONTEND=ON
cmake --build build
```

#### macOS (Cocoa Frontend)
```bash
cmake -B build -GNinja -DWISP_BUILD_MACOS_FRONTEND=ON
cmake --build build
```

#### Haiku / BeOS
Wisp automatically detects Haiku and builds the native BeOS frontend:
```bash
cmake -B build -GNinja
cmake --build build
```

### Rendering Backends
Wisp compiles and runs with **platform-native rendering backends as the default**. The historical "Auto" backend selection mode has been completely removed to prioritize native platform performance, toolkit-native font rendering, and seamless compositor integration.

*   **BeOS / Haiku**: Default backend is native `BView` (AGG) rendering. Optional fallback backend is Blend2D.
*   **Linux**: Default backend is Cairo (for GTK) or QPainter (for Qt). Optional fallback backend is Blend2D.
*   **macOS**: Default backend is Cocoa native plotter. Optional fallback backend is Blend2D.
*   **Windows**: Default backend can be explicitly selected at compile time as either **Direct2D** or **GDI** (see options below). Optional fallback backend is Blend2D.

#### Direct2D vs GDI Compiles on Windows
On Windows, you can explicitly control which native rendering pipeline is compiled:
*   **Direct2D & DirectWrite (Default)**: Hardware-accelerated high-fidelity rendering pipeline.
    ```bash
    cmake -B build -DWISP_WINDOWS_USE_D2D=ON
    ```
*   **Legacy GDI**: Traditional software rendering pipeline, recommended for Windows XP or Vista targets.
    ```bash
    cmake -B build -DWISP_WINDOWS_USE_D2D=OFF
    ```

#### Optional Blend2D Backend
Blend2D is completely optional and must be explicitly enabled at compile time:
```bash
cmake -B build -DWISP_USE_BLEND2D=ON
```

#### AsmJit (JIT Support) Compile-time Safeguards
When optional Blend2D compilation is enabled (`WISP_USE_BLEND2D=ON`), the build system automatically configures the **AsmJit** JIT compiler based on the target CPU architecture:
*   **SSE2 or ARM64 targets (Standard)**: AsmJit is compiled and enabled to optimize SIMD software rendering pipelines.
*   **Legacy/non-SSE2 targets (e.g., Windows XP / i586)**: The build system automatically turns **OFF** AsmJit compilation and usage (`BLEND2D_NO_JIT=ON`) to prevent Illegal Instruction crashes on older hardware, falling back safely to scalar software rendering.
