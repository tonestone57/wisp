Quick Build Steps for Wisp
=============================

Last Updated: August 2026

This document provides steps for building Wisp using CMake.

Build Requirements
==================

To build Wisp, you will need:

*   **Compiler**: A C99 and C++17 compliant compiler (GCC 9+, Clang 10+, or MSVC 2019+).
*   **Build Tools**: Python 3.x, CMake 3.20+, Ninja, gperf, pkg-config
*   **Python Modules**: `widlparser` (`pip install widlparser`)
*   **Libraries**: libxml2, libcurl, OpenSSL/LibreSSL, libjpeg, libpng, libwebp, FFmpeg, libpsl, libutf8proc, zlib

Building Wisp
=============

Wisp uses **CMake** as its primary build system across all platforms.

### 1. Clone the Repository

```bash
git clone https://github.com/wisp-browser/wisp.git
cd wisp
```

### 2. Configure and Build

#### Linux (GTK Frontend)
```bash
cmake -B build -GNinja -DWISP_BUILD_GTK_FRONTEND=ON
cmake --build build
```
Run executable:
```bash
./build/frontends/gtk/wisp-gtk
```

#### Linux / Embedded (Framebuffer Frontend)
```bash
cmake -B build -GNinja -DWISP_BUILD_FRAMEBUFFER_FRONTEND=ON
cmake --build build
```
Run executable:
```bash
./build/frontends/framebuffer/wisp-fb
```
More detailed documentation on using the [framebuffer](using-framebuffer.md) frontend is available.

#### Windows (Direct2D or GDI Frontend)
Using MSYS2 / MinGW-w64 or MSVC:
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
Native BeOS/Haiku frontend auto-detected:
```bash
cmake -B build -GNinja
cmake --build build
```

Documentation
=============

For detailed build instructions per platform, see:
- [GTK Build Guide](building-GTK.md)
- [Framebuffer Build Guide](building-Framebuffer.md)
- [Haiku Build Guide](building-Haiku.md)
- [Windows Build Guide](building-Windows.md)
