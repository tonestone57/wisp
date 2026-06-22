# Wisp Code Audit Report - June 2026

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine, focusing on the recent transitions to incremental layout, modern CSS support, and the QuickJS-based JavaScript subsystem. Wisp has achieved significant milestones in standard conformance while maintaining a lean architecture.

## 2. Library Versions Audit

| Library | Repo Version | Latest Online (June 2026) | Status |
|---------|--------------|---------------------------|--------|
| `quickjs-ng` | v0.15.1 | v0.15.1 | Up-to-date |
| `blend2d` | v0.21.2 | v0.21.2 | Up-to-date |
| `libavif` | v1.4.2 | v1.4.2 | Up-to-date |
| `libcss` | Jan 2026 Fork | 0.9.2 (Upstream) | Diverged (Forked for Grid/Calc) |
| `libdom` | Jan 2026 Fork | Upstream Git | Diverged (Forked for SVG/JS) |
| `FFmpeg` | Linked System | 7.x | Compatible |

## 3. Categorized Findings

### 3.1 Errors and Potential Bugs
*   **Test Failures (CI/CD Regression)**:
    *   `libcss_parse_auto_custom-properties_dat`: CSS Variable parsing tests are failing, indicating regressions or incomplete implementation of the custom property parser.
    *   `test_quickjs`: AddressSanitizer/LeakSanitizer detected 720 bytes leaked across 27 allocations in the QuickJS subsystem, primarily in `emalloc` and `erealloc` wrappers.
    *   `journal_test`: Failed with an **ODR violation** for the global symbol `guit`, which is defined in both `src/test/journal_test.c` and `src/desktop/gui_factory.c`.
*   **Layout Logic (Percentage Widths)**: `src/content/handlers/html/layout.c` contains multiple TODOs regarding proper handling of percentage widths in complex layout contexts (e.g., IFRAMEs, nested flexbox).
*   **Box Construction (Tabs and Counters)**: `box_construct.c` lacks full support for nested CSS counters and proper tab character expansion (TODOs at lines 420, 1847).
*   **Memory Management (JS Finalizers)**: The interaction between `JS_FreeRuntime` and manual `dom_node` mapping requires continuous monitoring for edge-case use-after-free during asynchronous GC cycles.

### 3.2 Performance Optimizations
*   **Incremental Layout Refinement**: The engine correctly utilizes `DIRTY_INTRINSIC` and `DIRTY_LAYOUT`. Bounding box union logic in `box_mark_dirty` could be further optimized for contained elements.
*   **Redraw Optimization**: Redraws in `html_content` currently union all dirty regions. A future optimization could involve a tiled redraw strategy.
*   **Stateful Path API**: The new stateful Path API reduces overhead for complex SVG shapes, particularly in the Blend2D and Windows backends.

### 3.3 Architectural Improvements
*   **Position: Sticky**: Correctly handles both global viewport and scrollable ancestor constraints. Multi-axis sticky positioning is functional.
*   **JS Subsystem**: Migration to QuickJS-ng v0.15.1 is complete. Automated WebIDL generator handles binding boilerplate, but `MutationObserver` remains a high priority.
*   **Network Pipeline**: Asynchronous fetch pipeline is functional but needs tighter integration with the QuickJS event loop.

### 3.4 Documentation and Cleanliness
*   **Technical Debt**: Approximately 50+ `TODO` and `FIXME` comments remain in core `src/` files.
*   **String Safety**: Migration of legacy `sprintf` calls to `snprintf` is largely complete across core and frontends.

## 4. Conclusion and Recommendations
Wisp has made significant strides in modernization. The immediate focus should be:
1.  Resolving percentage width issues in the layout engine.
2.  Completing the high-value JS bindings (MutationObserver).
3.  Resolving identified memory leaks in the QuickJS subsystem.
