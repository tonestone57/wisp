# Wisp Browser Audit and Modernization Report

## 1. Library Modernization Assessment

An audit of the `contrib/` directory was performed to evaluate the feasibility of upgrading bundled libraries to their latest upstream versions.

### NetSurf Libraries (`libcss`, `libdom`, `libhubbub`, `libparserutils`, `libsvgtiny`, `libnsutils`)
*   **Status**: Heavily Forked.
*   **Assessment**: **Upgrading to upstream is not recommended.**
*   **Reasoning**: Wisp has implemented major features that are not present in upstream NetSurf, including:
    *   **CSS Grid** layout support.
    *   `calc()` function support in CSS.
    *   Unified CMake build system.
    *   Native SVG DOM integration using `libdom`.
*   **Recommendation**: Modernize by manually cherry-picking security patches and critical bug fixes from upstream rather than performing a full sync.

### Image Decoders (`libnsbmp`, `libnsgif`)
*   **Status**: Lightly modified.
*   **Assessment**: High upgrade feasibility.
*   **Recommendation**: Can be re-synced with upstream while preserving Wisp's CMake integration.

### JavaScript Engine (`quickjs-ng`)
*   **Status**: Updated to v0.15.1.
*   **Assessment**: **Modernized.**
*   **Recommendation**: Continue maintaining Wisp-specific memory hooks and subsystem bindings while tracking upstream QuickJS-ng releases.

---

## 2. Critical Rendering and Major Bugs

### CSS Variables Support (Partial)
*   **Issue**: `libcss` now supports lexing and parsing of CSS Variables (`var()`) and custom properties (`--name`), but layout resolution is still work-in-progress.
*   **Impact**: High. Many modern websites use variables for core layout properties. Partial support improves compatibility but full resolution is required.
*   **Recommendation**: Complete the variable resolution pass during the CSS cascade.

### `position: sticky` (Partial)
*   **Issue**: Parsed by `libcss` but ignored by the layout engine in `src/content/handlers/html/layout.c`.
*   **Impact**: Header elements fail to remain fixed during scroll on modern sites.
*   **Recommendation**: Implement sticky positioning logic in the layout routines.

### AVIF Image Support (Implemented)
*   **Status**: Supported via `libavif`.
*   **Recommendation**: Ensure all frontends correctly handle AVIF-decoded surfaces.

---

## 3. Performance and Stability

### Logging Verbosity
*   **Issue**: Excessive debug tracing in `layout_flex.c` and `layout_grid.c` generates massive log files (hundreds of MBs for a single page load).
*   **Impact**: Disk I/O overhead and storage exhaustion.
*   **Recommendation**: Reduce log level for layout traces or wrap them in a specific debug flag.

### String and Memory Safety
*   **Issue**: Frequent use of `sprintf` in utility functions (e.g., `src/utils/filename.c`) and some unchecked `malloc` calls.
*   **Impact**: Potential buffer overflows and instability under low-memory conditions.
*   **Recommendation**: Migrate all `sprintf` usage to `snprintf` and ensure consistent NULL checking after allocations.

---

## 4. Summary Table

| Category | Item | Priority | Feasibility |
|----------|------|----------|-------------|
| Library | QuickJS-ng Update | Medium | High |
| Feature | CSS Variables | High | Medium (Complex) |
| Feature | position: sticky | Medium | Medium |
| Feature | AVIF Support | Low | Medium |
| Stability | sprintf -> snprintf | Medium | High |
| Performance| Logging Cleanup | Medium | High |
