# Wisp Browser Audit and Modernization Report

## 1. Library Modernization Assessment

An audit of the `contrib/` directory was performed to evaluate the state of bundled libraries and the feasibility of upgrading them to their latest upstream versions.

### NetSurf Libraries (`libcss`, `libdom`, `libhubbub`, `libparserutils`, `libsvgtiny`, `libnsutils`)
*   **Status**: Heavily Forked.
*   **Assessment**: **Upgrading to upstream is not recommended.**
*   **Reasoning**: Wisp has implemented major features that are not present in upstream NetSurf, including:
    *   **CSS Grid** layout support.
    *   `calc()` function support in CSS.
    *   Unified CMake build system.
    *   Native SVG DOM integration using `libdom`.
    *   Merged `libwapcaplet` into `libnsutils`.
*   **Recommendation**: Modernize by manually cherry-picking security patches and critical bug fixes from upstream rather than performing a full sync.

### Image Decoders (`libnsbmp`, `libnsgif`)
*   **Status**: Lightly modified.
*   **Assessment**: High upgrade feasibility.
*   **Recommendation**: Can be re-synced with upstream while preserving Wisp's CMake integration.

### JavaScript Engine (`quickjs-ng`)
*   **Status**: Updated to **v0.15.1**.
*   **Assessment**: **Up-to-date.**
*   **Note**: Wisp-specific memory hooks and subsystem bindings are preserved. Previous versions used v0.11.0; the current version provides significant ES2023+ improvements and bug fixes.

---

## 2. Rendering and Major Feature Status

### CSS Variables Support (Partial/In-Progress)
*   **Issue**: `libcss` has initial support for `CSS_PROP_CUSTOM_PROPERTY` and `var()` parsing, but full resolution during the CSS cascade and application to layout is still maturing.
*   **Impact**: High. Many modern websites use variables for core layout properties.
*   **Current State**: Basic parsing is present; selection logic handles custom properties.

### `position: sticky` (Implemented)
*   **Status**: **Fully Implemented.**
*   **Implementation**: Supported across layout, coordinate calculation (`box_coords`), and rendering. Elements remain fixed within their containing blocks using `sticky_x` and `sticky_y` offsets.
*   **References**: `src/content/handlers/html/layout.c`, `src/content/handlers/html/redraw.c`.

### AVIF Image Support (Implemented)
*   **Status**: **Fully Integrated.**
*   **Implementation**: Bundled `libavif` v1.4.2. Core image handling includes ISOBMFF signature sniffing and static image decoding.

### Windows Rendering Modernization (Implemented)
*   **Status**: **Fully Integrated.**
*   **Features**:
    *   **Transform Stack**: Support for `push_transform` and `pop_transform` using GDI `SetWorldTransform`. This enables CSS transforms and SVG coordinate systems.
    *   **Linear Gradients**: Native GDI `GradientFill` implementation for linear gradients.
    *   **Bitmap Tiling**: Corrected tile origin alignment to match Qt/CSS expectations.
    *   **Transform-aware Clipping**: Inverse-mapping of clip regions when transforms are active.

---

## 3. Library Detail Table (Merged from CONTRIB_AUDIT)

| Library | Current Version | Divergence | Upgrade Feasibility | Recommendation |
|---------|-----------------|------------|---------------------|----------------|
| `libcss` | ~Jan 2026 Sync | Extremely Heavy (Grid, calc) | Low | Manual Patching |
| `libdom` | ~Jan 2026 Sync | Extremely Heavy | Low | Manual Patching |
| `libhubbub` | ~Jan 2026 Sync | Moderate (Wisp-specific IDs) | Low/Medium | Manual Patching |
| `libavif` | v1.4.2 | Light | High | Stay Synced |
| `libnsbmp` | ~Jan 2026 Sync | Light | High | Periodic Sync |
| `libnsgif` | ~Jan 2026 Sync | Light | High | Periodic Sync |
| `libnsutils` | ~Jan 2026 Sync | Significant (Merged wapcaplet) | Medium | Manual Patching |
| `libsvgtiny` | ~Jan 2026 Sync | Heavy (DOM integration) | Low | Manual Patching |
| `quickjs-ng` | v0.15.1 | Moderate (Wisp Subsystem) | High (Applied) | Keep Synced |

---

## 4. Remaining Outstanding Tasks

### Browser Completion
1.  **CSS Variables Resolution**: Complete the implementation of the variable resolution pass during the CSS cascade in `libcss` and ensure layout correctly handles resolved values.
2.  **JS Binding Completion**: Continue implementing unimplemented WebIDL bindings (approx. 1500 remaining as per `UnimplementedJavascript.md`), specifically high-value interfaces like `URLSearchParams`, `MutationObserver`, and `IntersectionObserver`.
3.  **Canvas API**: Implement core 2D canvas drawing methods in the QuickJS subsystem and bridge them to frontend plotters.

### Performance
1.  **Incremental Layout**: Implement dirty-bit based incremental layout to avoid full reflows on every change. Ensure proper `CHILD_DIRTY` propagation up the box tree (see `plan_dirty_bits.md`).
2.  **Logging Refactor**: Wrap or demote approximately 80 high-verbosity `NSLOG` traces (currently at `WARNING` or `INFO` levels) in `layout_flex.c` and `layout_grid.c` to `DEEPDEBUG` to reduce log file sizes during normal operation.
3.  **Arena Alignment**: Audit all arena-based allocations to ensure 16-byte alignment, preventing AddressSanitizer misaligned access errors.

### Security
1.  **String Safety**: Complete the migration of the remaining 38 `sprintf` calls to `snprintf`. Priority targets:
    *   `src/content/urldb.c`: 13 calls in cookie and URL handling.
    *   `src/desktop/searchweb.c`: 1 call in search URL generation.
    *   `frontends/windows/download.c`: 4 calls in UI formatting.
    *   `frontends/gtk/download.c`: 2 calls in path and speed formatting.
2.  **MIME Sniffing**: Extend AVIF sniffing to support all common ISOBMFF-based image types to prevent MIME-type spoofing.

### Stability
1.  **Build System Fix**: Resolve a critical compilation error in `src/content/handlers/html/box_manipulate.c` where `box_mark_dirty` is defined twice, currently breaking the build on some platforms.
2.  **Windows Direct2D**: Implement a Direct2D-based plotter for the Windows frontend to improve rendering performance and support for advanced features like subpixel antialiasing and blur effects.
3.  **Frontend Parity**: Improve regular testing coverage for non-Qt/GDI frontends (Haiku, Framebuffer, Monkey) to ensure they haven't regressed after core layout changes.
4.  **Error Handling**: Replace remaining `assert()` calls in layout coordinate calculations with graceful error recovery and `NSLOG` reporting.
