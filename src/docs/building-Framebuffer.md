--------------------------------------------------------------------------------
  Build Instructions for Framebuffer Wisp
--------------------------------------------------------------------------------

  This document provides instructions for building the Framebuffer version of 
  Wisp using CMake.

  Framebuffer Wisp is primarily intended for kiosk and embedded applications.


Dependencies
============

  Debian / Ubuntu:
  ```bash
  sudo apt-get install build-essential cmake ninja-build pkg-config gperf \
      python3 python3-pip libcurl4-openssl-dev libxml2-dev libpng-dev \
      libjpeg-dev libwebp-dev libssl-dev libpsl-dev libutf8proc-dev \
      ffmpeg libavcodec-dev libavformat-dev libswscale-dev zlib1g-dev
  pip3 install widlparser
  ```


Building with CMake
===================

  To build the Framebuffer frontend with CMake:

  ```bash
  cmake -B build -GNinja -DWISP_BUILD_FRAMEBUFFER_FRONTEND=ON -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  ```

  Run the binary:
  ```bash
  ./build/frontends/framebuffer/wisp-fb
  ```

  For usage details, refer to the [usage guide](using-framebuffer.md).
