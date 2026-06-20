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
*   **Implementation**: Supported across layout, coordinate calculation (`box_coords`), and rendering. Elements remain fixed within their containing blocks using `sticky_x` and `sticky_y` offsets.
*   **References**: `src/content/handlers/html/layout.c`, `src/content/handlers/html/redraw.c`.

### ISOBMFF Image Support (Implemented)
*   **Status**: **Fully Integrated.**
*   **Implementation**: Bundled `libavif` v1.4.2. Core image handling includes generalized ISOBMFF signature sniffing (`mimesniff.c`) supporting AVIF, HEIC, and HEIF brands.

### Frontend & Rendering Modernization
*   **Status**: **Active Development.**
*   **Implementation**: Abstracting the plotter engine to support diverse backends.
*   **Target Backends**:
    *   **Windows**: Moving from GDI to Direct2D/DirectWrite for hardware acceleration and superior text antialiasing.
    *   **Linux/Cross-Platform**: Evaluating **Blend2D** for non-Qt lean frontends (Framebuffer/Haiku) to leverage JIT-compiled vector graphics.
    *   **Vector Path API**: Standardizing the plotter vfunc table around path-building syntax (MoveTo, BezierTo) for cleaner SVG rendering.

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

### Performance & Caching
1.  **Incremental Layout**: Refining the incremental layout engine using a **Dual-Pass Dirty Bit Strategy** (DIRTY_INTRINSIC vs DIRTY_LAYOUT) to skip down-tree processing when parent-allocated constraints remain stable.
2.  **Split-Level Caching**: Productionizing the Low-Level vs High-Level cache. Utilizing a zero-copy, append-only journal for low-level storage and `mmap` for larger assets (AVIF/Scripts) to minimize physical memory footprint.
3.  **Logging Refactor**: Wrap or demote approximately 80 high-verbosity `NSLOG` traces in `layout_flex.c` and `layout_grid.c` to `DEEPDEBUG`.

### Security & Networking
1.  **String Safety**: All legacy `sprintf` calls in `src/` and frontends have been migrated to `snprintf`.
2.  **MIME Sniffing**: Generalized ISOBMFF sniffing is implemented, protecting against spoofing of modern image types.
3.  **Web Crypto**: Bridging `crypto.subtle` bindings to LibreSSL to satisfy modern authentication and encryption requirements.
4.  **Networking**: Refactoring the fetch pipeline into an asynchronous, Fetch-API-aligned architecture to move away from legacy blocking loops.

### Stability
1.  **Windows Direct2D**: Implement a Direct2D-based plotter for the Windows frontend to improve rendering performance and support for advanced features like subpixel antialiasing and blur effects.
2.  **Frontend Parity**: Improve regular testing coverage for non-Qt/GDI frontends (Haiku, Framebuffer, Monkey) to ensure they haven't regressed after core layout changes.

---

## 5. Strategic Architectural Roadmap

To accelerate Wisp's transition into a competitive modern browser, the following strategic directions are prioritized:

### JS Subsystem: Automating the WebIDL Bottleneck
*   **Lightweight WebIDL Compiler**: Develop a custom Python script to parse standard WebIDL files and auto-generate QuickJS-ng C bindings, JSClassID registrations, and prototype boilerplate.
*   **Async/Event Loop Integration**: Ensure the top-level event loop correctly drains the QuickJS microtask queue (`JS_ExecutePendingJob`) after layout/paint cycles to support `MutationObserver` and `IntersectionObserver`.

### Layout Engine: Dual-Pass Incremental Layout
*   **Refined Dirty Bits**: Implement a **Dual-Pass Dirty Bit Strategy** distinguishing between `DIRTY_INTRINSIC` (content/style changes) and `DIRTY_LAYOUT` (parent-driven constraint changes).
*   **Down-tree Pruning**: Skip down-tree processing during the layout pass if a node's parent-allocated constraints match its previous run and it lacks its own `DIRTY` bit.

### Caching: Production-Grade Cache
*   **Zero-Copy Low-Level Storage**: Use an append-only journal file with an in-memory hash map index for the low-level cache to avoid heavy database dependencies.
*   **Memory-Mapped Files (mmap)**: Utilize `mmap` (POSIX) or `CreateFileMapping` (Windows) for large assets like AVIF images and scripts to minimize physical memory footprint.

### Graphics & Frontend: Abstract Plotter Engine
Drawing logic in `redraw.c` is being abstracted to support modern hardware-accelerated backends:

| Target Frontend | Suggested Backend | Strategy |
|---|---|---|
| **Windows** | Direct2D / DirectWrite | Provides hardware-accelerated rendering and vastly superior subpixel text antialiasing over GDI. |
| **Linux / Qt** | QPainter / Embedded Blend2D | **Blend2D** is prioritized for non-Qt lean frontends (Framebuffer/Haiku). It utilizes JIT compilation for high-performance vector graphics. |
| **Cross-Platform** | Abstracted Vector Path API | Ensuring the plotter vfunc table supports native path-building (MoveTo, LineTo, BezierTo) to improve SVG support via `libsvgtiny`. |

### Security & Networking Foundations
*   **LibreSSL for Web Crypto**: Bridge JavaScript `crypto.subtle` bindings directly to LibreSSL's crypto library for modern authentication support.
*   **Asynchronous Fetch Pipeline**: Refactor network operations into a modern asynchronous pipeline aligned with the Fetch API paradigm.
