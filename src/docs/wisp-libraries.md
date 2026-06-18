--------------------------------------------------------------------------------
  Wisp project libraries required                               1 July 2012
--------------------------------------------------------------------------------

  To build Wisp, you need the libraries required by the core, and any extra
  libraries required by the specific front end you are building.


  Wisp Core
==============

  Most core libraries are now bundled in the `contrib/` directory of the
  Wisp repository.

  Bundled Libraries:

      - libparserutils
      - libnsutils (includes libwapcaplet)
      - libhubbub
      - libcss
      - libdom
      - libnsbmp
      - libnsgif
      - libsvgtiny
      - quickjs-ng

  External Dependencies:

      - libcurl
      - OpenSSL / LibreSSL
      - libxml2
      - FFmpeg (libavformat, libavcodec, libswscale, libswresample, libavutil)
      - libpng, libjpeg, libwebp, libavif
      - libutf8proc
      - libpsl


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

