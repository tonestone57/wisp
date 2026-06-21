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
*   **Note**: Wisp-specific memory hooks and subsystem bindings are preserved.
*   **Modernization Path**: Implementing a lightweight Python-based WebIDL compiler to automate the remaining ~1,500 bindings and prioritize MutationObserver/IntersectionObserver via microtask integration (`JS_ExecutePendingJob`).

---

## 2. Rendering and Major Feature Status

### CSS Variables Support (Partial/In-Progress)
*   **Issue**: `libcss` has initial support for `CSS_PROP_CUSTOM_PROPERTY` and `var()` parsing, but full resolution during the CSS cascade and application to layout is still maturing.
*   **Impact**: High. Many modern websites use variables for core layout properties.
*   **Current State**: Basic parsing is present; selection logic handles custom properties.

### `position: sticky` (Implemented)
*   **Status**: **Fully Implemented.**
*   **Implementation**: Supported across layout, coordinate calculation (`box_coords`), and rendering. Elements remain fixed within their containing blocks using `sticky_x` and `sticky_y` offsets calculated in `layout_apply_sticky_clamping`.
*   **References**: `src/content/handlers/html/layout.c`, `src/content/handlers/html/redraw.c`.

### ISOBMFF Image Support (Implemented)
*   **Status**: **Fully Integrated.**
*   **Implementation**: Bundled `libavif` v1.4.2. Core image handling includes generalized ISOBMFF signature sniffing (`mimesniff.c`) supporting AVIF, HEIC, and HEIF brands.

### Frontend & Rendering Modernization
*   **Status**: **Active Development.**
*   **Implementation**: Abstracting the plotter engine to support diverse backends.
*   **Target Backends**:
    *   **Windows**: Windows GDI plotter is feature-complete; exploration of Direct2D for hardware acceleration is ongoing.
    *   **Linux/Cross-Platform**: **Blend2D** integration is complete for non-Qt lean frontends (Framebuffer/Haiku), leveraging JIT-compiled vector graphics.
    *   **Vector Path API**: Standardized the plotter vfunc table around a stateful path-building API (path_begin, path_move_to, path_bezier_to, path_fill, path_stroke).

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
2.  **JS Binding Completion**: Continue implementing unimplemented WebIDL bindings (approx. 1500 remaining as per `UnimplementedJavascript.md`), specifically high-value interfaces like `MutationObserver` and `IntersectionObserver`.
3.  **Canvas API**: Implement core 2D canvas drawing methods in the QuickJS subsystem and bridge them to frontend plotters.

### Performance & Caching
1.  **Incremental Layout**: Refining the incremental layout engine using the **Dual-Pass Dirty Bit Strategy** (DIRTY_INTRINSIC, CHILD_DIRTY, DIRTY_LAYOUT). Current focus is on optimizing bounding box unions in `box_mark_dirty`.
2.  **Split-Level Caching**: Productionizing the Low-Level vs High-Level cache using a zero-copy, append-only journal and `mmap` for larger assets.
3.  **Logging Refactor**: Wrap or demote high-verbosity `NSLOG` traces in layout engines to `DEEPDEBUG`.

### Security & Networking
1.  **String Safety**: Verified migration of legacy `sprintf` calls to `snprintf`.
2.  **MIME Sniffing**: Generalized ISOBMFF sniffing is implemented for AVIF, HEIC, and HEIF.
3.  **Web Crypto**: Bridging `crypto.subtle` bindings to LibreSSL for modern authentication.
4.  **Networking**: Asynchronous fetch pipeline is implemented and providing a Request/Response model with geometric buffer growth.

### Stability
1.  **Frontend Parity**: Continuous testing of GTK, Windows GDI, and Blend2D plotters. Recent tests identified regressions in CSS variable parsing and an ODR violation in `journal_test`. *(Note: Fixes for these are currently in progress in a separate branch)*
2.  **JS Engine Memory**: AddressSanitizer identified memory leaks in the QuickJS subsystem that must be resolved to ensure long-term stability.
3.  **Haiku/Framebuffer**: Re-verifying non-mainstream frontends after major layout changes.

---

## 5. Strategic Architectural Roadmap

### JS Subsystem: Automating the WebIDL Bottleneck
*   **Lightweight WebIDL Compiler**: Custom Python script (`utils/qjs_binding_generator.py`) parses WebIDL and auto-generates QuickJS-ng C bindings.

### Layout Engine: Dual-Pass Incremental Layout
*   **Dirty Bits**: Utilizing `DIRTY_INTRINSIC` (Bit 14), `CHILD_DIRTY` (Bit 15), and `DIRTY_LAYOUT` (Bit 16) in `box_flags`.
*   **Down-tree Pruning**: `layout_block_find_dimensions` tracks `last_available_width` to skip stable containers.

### Graphics & Frontend: Abstract Plotter Engine
Drawing logic in `redraw.c` utilizes an abstracted Vector Path API:

| Target Frontend | Backend | Status |
|---|---|---|
| **Windows** | GDI / Future Direct2D | GDI parity complete; supports transforms and gradients. |
| **Linux / Haiku** | Blend2D | Integration complete; high-performance JIT vector graphics. |
| **Linux / Qt** | QPainter | Reference implementation. |
