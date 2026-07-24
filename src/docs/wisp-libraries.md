--------------------------------------------------------------------------------
  Wisp project libraries required                         2027
--------------------------------------------------------------------------------

  To build Wisp, you need the libraries required by the core, and any extra
  libraries required by the specific front end you are building.


  Wisp Core
==============

  Wisp utilizes several specialized libraries. Most are vendored or tracked as Git submodules in the `contrib/` directory.

  Primary Dependencies:
  - **QuickJS-ng (v0.15.1)**: Modern ES2023+ JavaScript engine.
  - **Blend2D (v0.21.2)**: Unified 2D vector rendering engine.
  - **libavif (v1.4.2)**: Native AVIF and HEIC image support.
  - **FFmpeg (v8.1)**: Media pipeline and video decoding.
  - **LibreSSL (v4.3.2)**: TLS and cryptography.

  Core Subsystem Libraries:
  - **LibWapcaplet**: String internment.
  - **LibParserUtils**: Parser building utility functions.
  - **Hubbub**: HTML5 compliant HTML parser.
  - **LibCSS**: CSS parser and selection engine (Diverged for Grid/Calc/Variables).
  - **LibDOM**: W3C DOM implementation (Diverged for Mutation Hooks/JS).
  - **LibNSGIF**: GIF format image decoder.
  - **LibNSBMP**: BMP and ICO format image decoder.

  Optional:
  - **Libsvgtiny**: Lightweight SVG implementation.


  RISC OS front end
===================

  Required:

      $ git clone git://git.netsurf-browser.org/libpencil
      $ git clone git://git.netsurf-browser.org/rufl


  Framebuffer front end
=======================

  Required:

      $ git clone git://git.netsurf-browser.org/libnsfb


  Non RISC OS front ends
========================

  Optional:

      $ git clone git://git.netsurf-browser.org/librosprite

