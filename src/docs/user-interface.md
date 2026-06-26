User Interface
==============

[TOC]

Wisp is divided into a series of frontends which provide a user
interface around common core functionality. Each frontend is a
distinct implementation for a specific GUI toolkit.

Because of this the user interface has different features in
each frontend allowing the browser to be a native application.

# Frontends

As GUI toolkits are often applicable to a single Operating
System (OS) some frontends are named for their OS instead of the
toolkit e.g. RISC OS WIMP frontend is named riscos and the Windows
win32 frontend is named windows.

## amiga

Frontend specific to the amiga

## atari

Frontend specific to the atari

## beos

The native Haiku/BeOS frontend utilizing `libbe` (BView). It has been updated to use the unified **Blend2D** rendering backend, ensuring pixel-perfect consistency and supporting Wisp's modern layout features while remaining a native application.

## framebuffer

There is a basic user guide for the [framebuffer](docs/using-framebuffer.md)

## gtk

Frontend that uses the GTK+2 or GTK+3 toolkit

## monkey

This is the internal unit test frontend.

There is a basic user guide [monkey](docs/using-monkey.md)

## riscos

Frontend for the RISC OS WIMP toolkit.

## windows

Frontend which uses the Microsodt win32 GDI toolkit.

# Rendering Backend

Wisp has unified its rendering architecture around **Blend2D** across all platforms, including Windows, Linux, and Haiku. This provides a high-performance software rasterization pipeline and supports the **Fixed-Tile Redraw** strategy to optimize performance on both retro and modern hardware.

# User configuration

The behaviour of the browser can be changed from the defaults with a
configuration file. The [core user options](docs/wisp-options.md)
of the browser are common to all versions and are augmented by each
frontend in a specific manner.


