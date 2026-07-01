# Windows Rendering Sync Status

This document tracks the parity between the Windows GDI/Direct2D frontend and the reference Qt frontend, as well as core rendering changes that require frontend implementation.

## Status: **Sync Complete**

All major rendering features identified in the previous audits have been implemented in the Windows frontend, which now supports both a legacy GDI path and a modern Direct2D/DirectWrite pipeline.

### 1. Viewport Background and Absolute Borders
- **Status**: Completed.
- **Details**: Core enforces stricter viewport expansion rules and recalculates border positions for absolute boxes.
- **Reference**: `src/content/handlers/html/redraw.c`.

### 2. Transform Stack Support
- **Status**: Completed.
- **Details**: GDI plotter implements `push_transform` and `pop_transform` using `SetWorldTransform`. Direct2D plotter utilizes `ID2D1RenderTarget::SetTransform` with a dedicated transform stack.
- **Reference**: `frontends/windows/plot.c`, `frontends/windows/plot_d2d.cpp`.

### 3. Transform-aware Clipping
- **Status**: Completed.
- **Details**: Clipping logic handles coordinate mapping in transformed spaces. Direct2D path uses `PushAxisAlignedClip`.
- **Reference**: `frontends/windows/plot.c`, `frontends/windows/plot_d2d.cpp`.

### 4. Native Linear Gradients
- **Status**: Completed.
- **Details**: GDI plotter uses `GradientFill`. Direct2D path uses `ID2D1LinearGradientBrush`.
- **Reference**: `frontends/windows/plot.c`, `frontends/windows/plot_d2d.cpp`.

### 5. Bitmap Tiling Alignment
- **Status**: Completed.
- **Details**: Bitmap tiling origin is aligned to the clip region.
- **Reference**: `frontends/windows/plot.c`, `frontends/windows/plot_d2d.cpp`.

### 6. Web Font (@font-face) Loading
- **Status**: Completed.
- **Details**: GDI uses `AddFontMemResourceEx`. Direct2D uses `IDWriteFontCollectionLoader` and custom memory-based font loading.
- **Reference**: `frontends/windows/font.c`, `frontends/windows/font_dwrite.cpp`.

### 7. Vector Path API
- **Status**: Completed.
- **Details**: Implements the stateful Path API (`path_begin`, `path_move_to`, etc.). Direct2D implementation uses `ID2D1PathGeometry`.
- **Reference**: `frontends/windows/plot.c`, `frontends/windows/plot_d2d.cpp`.

### 8. C++ Migration and Resource Management
- **Status**: Completed.
- **Details**: Core window (`window.cpp`) and bitmap (`bitmap.cpp`) management migrated to C++ to safely handle COM objects and Direct2D resources.
- **Reference**: `frontends/windows/window.cpp`, `frontends/windows/bitmap.cpp`.

### 9. Native Radial Gradients
- **Status**: Completed.
- **Details**: Direct2D path uses `ID2D1RadialGradientBrush` for hardware-accelerated radial gradients. Integration enabled via `DEFAULT_NATIVE_RADIAL=ON`.
- **Reference**: `frontends/windows/plot_d2d.cpp`.

## Future Considerations
- **Hardware Acceleration Tuning**: Further optimization of Direct2D device-loss recovery scenarios.
