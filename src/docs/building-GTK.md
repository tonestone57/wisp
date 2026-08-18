Build Instructions for GTK Wisp
==================================

This document provides instructions for building the GTK version of Wisp using CMake and provides guidance on obtaining Wisp's build dependencies.

Quick Start
-----------

Wisp uses CMake as its primary build system.

### Build Dependencies

#### Debian / Ubuntu
```bash
sudo apt-get install build-essential cmake ninja-build pkg-config gperf \
    python3 python3-pip libgtk-3-dev libcurl4-openssl-dev libxml2-dev \
    libpng-dev libjpeg-dev libwebp-dev libssl-dev libpsl-dev libutf8proc-dev \
    ffmpeg libavcodec-dev libavformat-dev libswscale-dev zlib1g-dev
pip3 install widlparser
```

#### Fedora
```bash
sudo dnf install gcc gcc-c++ cmake ninja-build pkgconf gperf python3 python3-pip \
    gtk3-devel libcurl-devel libxml2-devel libpng-devel libjpeg-turbo-devel \
    libwebp-devel openssl-devel libpsl-devel utf8proc-devel ffmpeg-devel zlib-devel
pip3 install widlparser
```

Building
--------

### 1. Configure
Create build directory and configure with CMake enabling the GTK frontend:

```bash
cmake -B build -GNinja -DWISP_BUILD_GTK_FRONTEND=ON -DCMAKE_BUILD_TYPE=Release
```

### 2. Compile
```bash
cmake --build build
```

### 3. Execution
Run the compiled GTK binary:

```bash
./build/frontends/gtk/wisp-gtk
```

Note for packagers
------------------

If you are packaging Wisp, see the PACKAGING-GTK document.
