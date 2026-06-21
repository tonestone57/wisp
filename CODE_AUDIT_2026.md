# Wisp Code Audit Report - June 2026

## 1. Executive Summary
This audit evaluates the current state of the Wisp browser engine, focusing on the recent transitions to incremental layout, modern CSS support, and the QuickJS-based JavaScript subsystem.

## 2. Categorized Findings

### 2.1 Errors and Potential Bugs
*   **Test Failures (CI/CD Regression)**: *(Note: Fixes for these are currently in progress in a separate branch)*
    *   `libcss_parse_auto_custom-properties_dat`: CSS Variable parsing tests are failing, indicating regressions or incomplete implementation of the custom property parser.
    *   `test_quickjs`: AddressSanitizer/LeakSanitizer detected 720 bytes leaked across 27 allocations in the QuickJS subsystem, primarily in `emalloc` and `erealloc` wrappers.
    *   `journal_test`: Failed with an **ODR violation** for the global symbol `guit`, which is defined in both `src/test/journal_test.c` and `src/desktop/gui_factory.c`.
*   **Layout Logic (Percentage Widths)**: `src/content/handlers/html/layout.c` contains multiple TODOs regarding proper handling of percentage widths in complex layout contexts (e.g., lines 801, 547). This may lead to incorrect scaling in nested containers.
*   **Box Construction (Tabs and Counters)**: `box_construct.c` lacks full support for nested CSS counters and proper tab character expansion (TODOs at lines 420, 1847).
*   **Memory Management (JS Finalizers)**: While the `hashmap_destroy` order in `js_destroyheap` was recently corrected, the interaction between `JS_FreeRuntime` and manual `dom_node` mapping requires continuous monitoring for edge-case use-after-free during asynchronous GC cycles.

### 2.2 Performance Optimizations
*   **Incremental Layout Refinement**: The engine now correctly utilizes `DIRTY_INTRINSIC` (Bit 14) and `DIRTY_LAYOUT` (Bit 16). However, the dirty list accumulation in `box_mark_dirty` could be further optimized by skipping redundant bounding box unions for elements entirely contained within their parent's already-dirty region.
*   **Redraw Optimization**: Redraws in `html_content` currently union all dirty regions. A future optimization could involve a tiled redraw strategy to avoid painting large "clean" gaps between two distant dirty elements.
*   **Stateful Path API**: The new stateful Path API (MoveTo, LineTo, BezierTo) in the plotter interface reduces the overhead of passing large float arrays for complex SVG shapes, particularly in the Blend2D and Windows backends.

### 2.3 Architectural Improvements
*   **Position: Sticky**: The implementation in `layout_apply_sticky_clamping` correctly handles both global viewport and scrollable ancestor constraints. The logic for multi-axis (top + left) sticky positioning is functional.
*   **JS Subsystem**: The migration to QuickJS-ng v0.15.1 is complete. The automated WebIDL generator handles the bulk of binding boilerplate, though manual implementation of `MutationObserver` and `IntersectionObserver` remains a high-priority task.
*   **Network Pipeline**: The asynchronous fetch pipeline is functional but needs tighter integration with the QuickJS event loop to ensure Promises resolve in a timely manner without blocking the UI thread.

### 2.4 Documentation and Cleanliness
*   **Technical Debt**: Approximately 50+ `TODO` and `FIXME` comments remain in core `src/` files. A systematic effort should be made to address these, starting with those in the layout engine.
*   **String Safety**: The project has successfully migrated most legacy `sprintf` calls to `snprintf`. Remaining `strcpy` usage is largely confined to compatibility layers for older platforms or static string assignments.

## 3. Conclusion and Recommendations
Wisp has made significant strides in modernizing its rendering and scripting capabilities. The immediate focus should be:
1.  Resolving percentage width issues in the layout engine.
2.  Completing the high-value JS bindings (MutationObserver).
3.  Productionizing the Blend2D plotter for Linux/Haiku environments.
