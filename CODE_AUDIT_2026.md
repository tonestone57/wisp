# Wisp Code Audit Report - June 2026

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine as of June 2026. The project has transitioned from the legacy codebase to a modernized architecture featuring QuickJS-ng, an incremental layout engine, and advanced CSS support (Grid, Flexbox, Sticky). Significant progress has been made in media support (AVIF/ISOBMFF) and frontend parity.

## 2. Library Versions Audit

| Library | Repo Version | Latest Online (June 2026) | Status |
|---------|--------------|---------------------------|--------|
| `quickjs-ng` | v0.15.1 | v0.15.1 | **[Finished]** Up-to-date |
| `blend2d` | v0.21.2 | v0.21.2 | **[Finished]** Up-to-date |
| `libavif` | v1.4.2 | v1.4.2 | **[Finished]** Up-to-date |
| `libnsbmp` | Jan 2026 Sync | Latest | **[Finished]** Up-to-date |
| `libnsgif` | Jan 2026 Sync | Latest | **[Finished]** Up-to-date |
| `libcss` | Jan 2026 Fork | 0.9.2 (Upstream) | **[Partial]** Diverged for Grid/Calc |
| `libdom` | Jan 2026 Fork | Upstream Git | **[Partial]** Diverged for SVG/JS |
| `FFmpeg` | Linked System | 7.x | **[Finished]** Compatible |

## 3. Detailed Subsystem Analysis

### 3.1 Core Layout engine
*   **Incremental Layout [Partial]**: Utilizes `DIRTY_INTRINSIC`, `CHILD_DIRTY`, and `DIRTY_LAYOUT` flags. Correctly skips reflows for stable subtrees.
    *   *Optimization*: Dirty rectangle accumulation in `box_mark_dirty` ensures previous positions are cleared.
    *   *Fixed-Tile Redraw [Finished]*: Unified strategy using scale-aware fixed tiles (256x256 for i586/retro, 512x512 for High-DPI). Optimizes cache locality and eliminates overdraw.
*   **IntersectionObserver [Finished]**: Fully integrated into the layout engine via post-layout hooks in `layout.c` and `html.c`.
*   **CSS Grid [Partial]**: Core layout logic and 3-phase auto-placement implemented.
    *   *Optimization*: Pass 3 uses cached placement data to avoid re-parsing CSS during final stretch.
    *   *Improvement needed*: Dense packing algorithm and complex spanning edge cases require further refinement.
*   **CSS Flexbox [Partial]**: Supports flex-grow, shrink, and auto-margins.
    *   *Finished*: Two-pass resolution for column flex with indefinite heights.
*   **Position: Sticky [Finished]**: Full support for both global viewport and scrollable ancestor constraints. Verified multi-axis clamping.
*   **Percentage Widths [Incomplete]**: TODOs remain for IFRAMEs, text-indent, and max-height constraints in `layout.c`.

### 3.2 JavaScript Subsystem (QuickJS-ng)
*   **Integration [Finished]**: Migration to QuickJS-ng v0.15.1 complete.
*   **Binding Generator [Finished]**: Automated WebIDL compiler handles boilerplate and generates weak stubs to maintain linker compatibility.
*   **Observers [Partial]**: `MutationObserver` has manual implementation infrastructure.
    *   *Partial*: Integrated with LibDOM via native mutation hooks (`dom_document_set_mutation_hook`), though event refinement is ongoing.
*   **Memory Management [Partial]**: Interaction between QuickJS GC and LibDOM node mapping requires monitoring.
    *   *Bug*: ~720 bytes leaked across 27 allocations during teardown (LeakSanitizer finding).

### 3.3 Media Subsystem
*   **ISOBMFF [Finished]**: Native sniffing and brand iteration for AVIF, HEIC, and HEIF.
*   **FFmpeg [Finished]**: Asynchronous video decoding pipeline functional in `video.c`.
    *   *Optimization*: Software volume scaling and frame synchronization logic implemented.
    *   *Finished*: A/V Master Clock synchronization between audio and video tracks.

### 3.4 Frontends
*   **Windows GDI [Finished]**: Achieved parity with Qt frontend.
    *   *Optimization*: Batching of consecutive path commands reduces kernel transitions.
*   **GTK/Cairo [Finished]**: Stable reference frontend.
*   **Blend2D Backend [Finished]**: Unified high-performance vector rendering across all platforms. Serves as the single source of truth for rasterization to ensure pixel-perfect consistency.
*   **Haiku / BeOS [Finished]**: Native `libbe` frontend (BView) unified with the Blend2D rendering backend and fixed-tile redraw strategy.

## 4. Bugs and Technical Debt
*   **[Finished] ODR Violation**: Resolved duplicate definition of `guit` symbol in test code. Verified correct use of `extern` in `journal_test.c` and `test_quickjs.c`.
*   **[Bug] LibCSS Regression**: `libcss_parse_auto_custom-properties_dat` failing on complex variable fallbacks.
*   **[Finished] Binding Conflicts**: Verified that manual stubs and generated headers in `eventtarget_impl.c` and `xhr_impl.c` are in sync.
*   **[Finished] Test Runner Compilation**: Resolved syntax and format errors in `contrib/libcss/test/parse-auto.c`.
*   **[Debt] Box Construction**: `src/content/handlers/html/box_construct.c` missing full support for nested CSS counters and tab character expansion.

## 5. Future Recommendations and Optimizations
1.  **Binding Coverage**: Prioritize manual implementation of high-value WebIDL bindings like `Element.querySelector` and `Element.querySelectorAll`.
2.  **Canvas 2D API**: Implement the bridge to the plotter engine (WebIDL stubs exist).
3.  **Percentage Widths**: Resolve missing cases for IFRAMEs and text-indent in `layout.c`.
4.  **JS Event Loop Integration**: Tighten integration between the asynchronous fetch pipeline and the QuickJS event loop to reduce latency in SPAs.
5.  **Path Accumulation**: Further optimize the Windows GDI plotter by using `PolyBezier` for contiguous segments.
