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
    *   **Transform Stack**: Support for `push_transform` and `pop_transform` using GDI `SetWorldTransform`.
    *   **Linear Gradients**: Native GDI `GradientFill` implementation for linear gradients.
    *   **Bitmap Tiling**: Corrected tile origin alignment to match Qt/CSS expectations.
    *   **Transform-aware Clipping**: Inverse-mapping of clip regions when transforms are active.

---

## 3. Performance and Stability

### Logging Verbosity
*   **Issue**: Excessive debug tracing in `layout_flex.c` and `layout_grid.c` can generate large log files.
*   **Recommendation**: Continue wrapping layout traces in `NSLOG(..., DEEPDEBUG, ...)` or specific debug flags.

### String and Memory Safety
*   **Issue**: Legacy use of `sprintf` in some utility functions.
*   **Impact**: Potential buffer overflows.
*   **Status**: Ongoing migration to `snprintf`.

---

## 4. Library Detail Table (Merged from CONTRIB_AUDIT)

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

## 5. Remaining Outstanding Tasks

Based on the audit, the following tasks remain for project completion/modernization:

1.  **CSS Variables Resolution**: Complete the implementation of the variable resolution pass during the CSS cascade in `libcss` and ensure layout correctly handles resolved values.
2.  **Incremental Layout**: Implement dirty-bit based incremental layout to avoid full reflows on every change (see `plan_dirty_bits.md`).
3.  **Logging Refactor**: Wrap remaining layout traces in `layout_flex.c` and `layout_grid.c` with appropriate debug flags to reduce log file sizes.
4.  **Security Hardening**: Complete the migration from `sprintf` to `snprintf` across all utility and core functions.
5.  **Windows Direct2D**: Investigate and implement a Direct2D-based plotter for improved performance on modern Windows systems.
6.  **JS Binding Completion**: Continue implementing unimplemented WebIDL bindings (approx. 1500 remaining as per `UnimplementedJavascript.md`).
