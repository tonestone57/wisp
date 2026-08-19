Build Instructions for GTK Wisp
==================================

This document provides instructions for building the GTK version of Wisp using CMake and provides guidance on obtaining Wisp's build dependencies.

Quick Start
-----------

See the QUICK-START document, which provides a simple environment with
which you can fetch, build and install Wisp and its dependencies.

The QUICK-START is the recommended way to build Wisp.


Manual building
---------------

If you can't follow the quick start instructions, you will have to build
Wisp manually.  The instructions for doing this are given below.


Obtaining the build dependencies
--------------------------------

Many of Wisp's dependencies are packaged on various operating systems.
The remainder must be installed manually.  Currently, some of the libraries
developed as part of the Wisp project have not had official releases.
Hopefully they will soon be released with downloadable tarballs and packaged
in common distros.  For now, you'll have to make do with Git checkouts.

### Package installation

Debian-like OS:

    $ sudo apt-get install cmake libgtk-3-dev libcurl4-openssl-dev
    $ apt-get install librsvg2-dev libjpeg-dev

If you want to build with gtk 3 replace libgtk2.0-dev with libgtk-3-dev

Recent OS versions might need libcurl4-dev instead of libcurl3-dev but
note that when it has not been built with OpenSSL, the SSL_CTX is not
available and results that certification details won't be presented in case
they are invalid.  But as this is currently unimplemented in the GTK
flavour of Wisp, this won't make a difference at all.

Fedora:

    $ yum install curl-devel libpng-devel
    $ yum install librsvg2-devel expat-devel

Other:

You'll need to install the development resources for libglade2, libcurl3,
libpng and librsvg.


### Preparing your workspace

Wisp has a number of libraries which must be built in-order and
installed into your workspace. Each library depends on a core build
system which Wisp projects use. This build system relies on the
presence of things like pkg-config to find libraries and also certain
environment variables in order to work correctly.

Assuming you are preparing a workspace in /home/wisp/workspace then
the following steps will set you up:

### Make the workspace directory and change to it

    $ mkdir -p ${HOME}/wisp/workspace
    $ cd ${HOME}/wisp/workspace

### Make the temporary install space

    $ mkdir inst

### Make an environment script

    $ cat > env.sh <<'EOF'
      export PKG_CONFIG_PATH=${HOME}/wisp/workspace/inst/lib/pkgconfig::
      export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${HOME}/wisp/workspace/inst/lib
      export PREFIX=${HOME}/wisp/workspace/inst
      EOF

### Change to workspace and source the environment

Whenever you wish to start development in a new shell, run the following:

    $ cd ${HOME}/wisp/workspace
    $ source env.sh

From here on, any commands in this document assume you have sourced your
shell environment.


### The Wisp project's libraries

The Wisp project has developed several libraries which are required by
the browser. These are:

| BuildSystem    | Shared build system, needed to build the other libraries |
| LibParserUtils | Parser building utility functions                        |
| LibWapcaplet   | String internment                                        |
| Hubbub         | HTML5 compliant HTML parser                              |
| LibCSS         | CSS parser and selection engine                          |
| LibNSGIF       | GIF format image decoder                                 |
| LibNSBMP       | BMP and ICO format image decoder                         |
| LibROSprite    | RISC OS Sprite format image decoder                      |

To fetch each of these libraries, run the appropriate commands from the
Docs/LIBRARIES file, from within your workspace directory.

To build and install these libraries, simply enter each of their directories
and run:

    $ make install

> Note:
>
> We advise enabling iconv() support in libparserutils, which vastly
> increases the number of supported character sets.  To do this,
> create a file called Makefile.config.override in the libparserutils
> directory, containing the following line:
>
>     CFLAGS += -DWITH_ICONV_FILTER
>
> For more information, consult the libparserutils README file.

Now you should have all the Wisp project libraries built and installed.


### Getting the Wisp source

From your workspace directory, run the following command to get the Wisp
source:

    $ git clone git://git.wisp-browser.org/wisp.git

And change to the 'wisp' directory:

    $ cd wisp

Building and executing Wisp
------------------------------

First of all, you should examine the options in Makefile.defaults
and gtk/Makefile.defaults and enable and disable relevant features
as you see fit by editing a Makefile.config file.

Some of these options can be automatically detected and used, and
where this is the case they are set to such.  Others cannot be
automatically detected from the Makefile, so you will either need to
install the dependencies, or set them to NO.

You should then obtain Wisp's dependencies, keeping in mind which options
you have enabled in the configuration file.  See the next section for
specifics.

Once done, to build GTK Wisp on a UNIX-like platform, simply run:

    $ make

If that produces errors, you probably don't have some of Wisp's
build dependencies installed. See "Obtaining Wisp's dependencies"
below. Or turn off the complaining features in a Makefile.config
file. You may need to "make clean" before attempting to build after
installing the dependencies.

Run Wisp by executing "nsgtk3":

    $ ./nsgtk3
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
