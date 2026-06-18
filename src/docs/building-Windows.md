--------------------------------------------------------------------------------
  Build Instructions for Windows Wisp                      13 February 2010
--------------------------------------------------------------------------------

  This document provides instructions for building the Windows version
  of Wisp and provides guidance on obtaining Wisp's build
  dependencies.

  Windows Wisp has been tested on Wine and Vista.


  Building and executing Wisp
================================

  Wisp uses CMake for building. The Windows port is typically cross-compiled
  from Linux using MinGW-w64 or built natively on Windows using MSYS2.

  To cross-compile for Windows from Linux:

      $ cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake
      $ make -C build-win -j$(nproc)

  Run Wisp using Wine or on a Windows machine:

      $ wine build-win/frontends/windows/wisp-windows.exe

  The staticaly linked binary which is generated may be several
  megabytes in size, this can be reduced by stripping the binary.

      $ i586-mingw32msvc-strip Wisp.exe 


  Obtaining Wisp's build dependencies
========================================

  Package installation
----------------------

  Debian-based OS:

  The mingw cross compilation tools are required. These can be
  installed as pakages on Debian/Ubuntu systems:

      $ sudo apt-get install mingw32 mingw32-binutils mingw32-runtime

  These provide a suitable set of compilers and headers including the win32 API.

  The includes and associated libraries are installed in
  /usr/i586-mingw32msvc/ Which is where the build system will include
  files from by default. The packages at time of writing only target
  32bit windows builds.

  Other:

  For other OS the apropriate packages and environment must be installed.

  pkg-config
------------

  A pkg-config wrapper script is required to make things easier

cat > /usr/i586-mingw32msvc/bin/pkg-config <<EOF
#!/bin/bash
export PKG_CONFIG_LIBDIR=/usr/i586-mingw32msvc/lib/pkgconfig
/usr/bin/pkg-config $*
EOF


  Base libraries
----------------

  Unlike other OS the base libraries and their dependancies need to be
  built and installed.

  The instructions given here assume you will be installing on a
  Debian derived OS using the mingw32 packages. The libraries should
  be unpacked and built from a suitable temporary directory.

  zlib:
  
      $ apt-get source zlib1g
      $ cd zlib-1.2.3.3.dfsg
      $ CC=i586-mingw32msvc-gcc AR=i586-mingw32msvc-ar RANLIB=i586-mingw32msvc-ranlib CFLAGS="-DNO_FSEEKO" ./configure --prefix=/usr/i586-mingw32msvc/
      $ make
      $ sudo make install

  libiconv:

      $ wget http://ftp.gnu.org/pub/gnu/libiconv/libiconv-1.13.1.tar.gz
      $ tar -zxf libiconv-1.13.1.tar.gz
      $ cd libiconv-1.13.1
      $ ./configure --prefix=/usr/i586-mingw32msvc/ --host=i586-mingw32msvc --disable-shared
      $ make
      $ sudo make install

  regex:

      $ wget http://kent.dl.sourceforge.net/project/mingw/Other/UserContributed/regex/mingw-regex-2.5.1/mingw-libgnurx-2.5.1-src.tar.gz
      $ tar -zxf mingw-libgnurx-2.5.1-src.tar.gz
      $ cd mingw-libgnurx-2.5.1
      $ ./configure --prefix=/usr/i586-mingw32msvc/ --host=i586-mingw32msvc
      $ make
      $ sudo make install

  openssl:

      $ wget http://www.openssl.org/source/openssl-1.0.0a.tar.gz
      $ tar -zxf openssl-1.0.0a.tar.gz
      $ cd openssl-1.0.0a
      $ PATH=/usr/i586-mingw32msvc/bin/:$PATH ./Configure no-shared disable-capieng --prefix=/usr/i586-mingw32msvc/ mingw
      $ PATH=/usr/i586-mingw32msvc/bin/:$PATH make CC=i586-mingw32msvc-gcc RANLIB=i586-mingw32msvc-ranlib
      $ sudo make install

  libcurl:

      $ wget http://curl.haxx.se/download/curl-7.26.0.tar.gz
      $ tar -zxf curl-7.26.0.tar.gz
      $ cd curl-7.26.0
      $ LDFLAGS=-mwindows ./configure --prefix=/usr/i586-mingw32msvc/ --host=i586-mingw32msvc --disable-shared --disable-ldap --without-random
      $ make
      $ sudo make install

  libpng:

      $ wget http://kent.dl.sourceforge.net/project/libpng/libpng14/1.4.12/libpng-1.4.12.tar.gz
      $ tar -zxf libpng-1.4.12.tar.gz
      $ cd libpng-1.4.12
      $ ./configure --prefix=/usr/i586-mingw32msvc/ --host=i586-mingw32msvc
      $ make
      $ sudo make install

  libjpeg:

      $ wget http://www.ijg.org/files/jpegsrc.v8d.tar.gz
      $ tar -zxf jpegsrc.v8d.tar.gz
      $ cd jpeg-8d
      $ ./configure --prefix=/usr/i586-mingw32msvc/ --host=i586-mingw32msvc --disable-shared
      $ make
      $ sudo make install

  The Wisp project's libraries
---------------------------------

  The Wisp project has developed several libraries which are required by
  the browser. These are:

  LibParserUtils  --  Parser building utility functions
  LibWapcaplet    --  String internment
  Hubbub          --  HTML5 compliant HTML parser
  LibCSS          --  CSS parser and selection engine
  LibNSGIF        --  GIF format image decoder
  LibNSBMP        --  BMP and ICO format image decoder
  LibROSprite     --  RISC OS Sprite format image decoder

  To fetch each of these libraries, run the appropriate commands from the
  Docs/LIBRARIES file.

  To build and install these libraries.

  Ensure the MINGW_INSTALL_ENV variable is correctly set.

      $ export MINGW_INSTALL_ENV=/usr/i586-mingw32msvc/

 Then simply enter each of their directories and run:
  
      $ make TARGET=windows PREFIX=/usr/i586-mingw32msvc/
      $ sudo make TARGET=windows PREFIX=/usr/i586-mingw32msvc/ install

  Resources
-----------

  The windows resources may be rebuilt. Currently there is 1 object
  file included in the Git distribution of Wisp that could be
  manually compiled

      $ cd windows/res
      $ i586-mingw32msvc-windres resource.rc -O coff -o resource.o
