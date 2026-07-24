# Building Wisp for Windows

This document provides instructions for building the Windows version of Wisp using the modern CMake-based build system.

## Prerequisites

Wisp for Windows is built using the **MinGW-w64** toolchain, typically via **MSYS2**.

### 1. Install MSYS2
Download and install MSYS2 from [msys2.org](https://www.msys2.org/).

### 2. Install Build Tools and Dependencies
Open the **MSYS2 MinGW64** terminal and install the required packages:

```bash
pacman -S mingw-w64-x86_64-gcc \
          mingw-w64-x86_64-cmake \
          mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-pkg-config \
          mingw-w64-x86_64-gperf \
          mingw-w64-x86_64-python3 \
          mingw-w64-x86_64-libxml2 \
          mingw-w64-x86_64-curl \
          mingw-w64-x86_64-openssl \
          mingw-w64-x86_64-libpng \
          mingw-w64-x86_64-libjpeg-turbo \
          mingw-w64-x86_64-libwebp \
          mingw-w64-x86_64-ffmpeg
```

## Building

### 1. Configure
Create a build directory and run CMake. Wisp defaults to the Direct2D rendering pipeline on Windows.

```bash
mkdir build
cd build
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
```

### 2. Compile
```bash
ninja
```

The resulting executable `wisp.exe` will be located in the build directory.

## Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `WISP_WINDOWS_USE_D2D` | Use native Direct2D/DirectWrite rendering pipeline | `ON` |
| `WISP_ENABLE_TESTS` | Build unit/integration tests | `OFF` |

If `WISP_WINDOWS_USE_D2D` is set to `OFF`, Wisp will fall back to the legacy GDI plotter.

## Troubleshooting

### C++17 Requirement
The Windows frontend requires a C++17 compliant compiler for Direct2D and modern STL support. Ensure your GCC version is 8.0 or higher.

### Missing DLLs
When running `wisp.exe` outside the MSYS2 environment, you may need to copy required DLLs (e.g., `libcurl.dll`, `zlib1.dll`) from your MinGW64 `bin` directory to the same folder as the executable.
