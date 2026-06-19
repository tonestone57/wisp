# Windows Rendering Sync Status

This document tracks the parity between the Windows GDI frontend and the reference Qt frontend, as well as core rendering changes that require frontend implementation.

## Status: **Sync Complete**

All major rendering features identified in the previous audit have been implemented in the Windows GDI frontend.

### 1. Viewport Background and Absolute Borders
- **Status**: Completed.
- **Details**: Core enforces stricter viewport expansion rules and recalculates border positions for absolute boxes.
- **Reference**: `src/content/handlers/html/redraw.c`.

### 2. Transform Stack Support
- **Status**: Completed.
- **Details**: Windows plotter now implements `push_transform` and `pop_transform` using GDI `SetWorldTransform`. This enables CSS transforms and SVG coordinate systems.
- **Reference**: `frontends/windows/plot.c`.

### 3. Transform-aware Clipping
- **Status**: Completed.
- **Details**: Clipping logic now inverse-maps the clip rectangle via the current world transform to ensure consistent clipping in transformed spaces.
- **Reference**: `frontends/windows/plot.c`.

### 4. Native Linear Gradients
- **Status**: Completed.
- **Details**: Windows plotter uses GDI `GradientFill` (with triangle mesh) for linear gradients, supporting arbitrary angles.
- **Reference**: `frontends/windows/plot.c`.

### 5. Bitmap Tiling Alignment
- **Status**: Completed.
- **Details**: Bitmap tiling origin is now aligned to the clip region, preventing pattern shifts during scrolling or partial repaints.
- **Reference**: `frontends/windows/plot.c`.

### 6. Web Font (@font-face) Loading
- **Status**: Completed.
- **Details**: Windows frontend implements `html_font_face_load_data` using `AddFontMemResourceEx`.
- **Reference**: `frontends/windows/font.c`.

## Future Considerations
- **Radial Gradients**: Currently use core fallback (triangle decomposition). Native GDI implementation is possible but complex.
- **Hardware Acceleration**: Exploration of Direct2D for improved performance on modern Windows versions.
