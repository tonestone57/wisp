# Incremental Layout Strategy (Dirty Bits)

Wisp implements an incremental layout engine to avoid redundant full-page reflows. To solve the problem of unnecessary sub-tree reflows when container dimensions change, we utilize a **Dual-Pass Dirty Bit Strategy**.

## 1. Dual-Pass Dirty Bits
The `box_flags` in `include/wisp/content/handlers/html/box.h` use two distinct bits for tracking changes:

1.  **DIRTY (Bit 14)**: Indicates that the element’s internal contents, text, or styles have changed. This requires a recalculation of intrinsic traits (like min/max widths).
2.  **CHILD_DIRTY (Bit 15)**: Indicates that a descendant in the box tree is dirty and requires a pass.

### Refinement: Intrinsic vs. Layout Dirty
To further optimize, we distinguish between:
*   **DIRTY_INTRINSIC**: (Current `DIRTY`) The element itself changed.
*   **DIRTY_LAYOUT**: The element's bounds or layout environment changed (e.g., parent size changed), but its children's intrinsic traits (min/max) remain intact.

## 2. Propagation and Execution Rules
*   **DOM/Style Mutation**: When a DOM node or style changes, the `DIRTY` bit is set on the associated box, and `CHILD_DIRTY` is propagated up to the root.
*   **Layout Pass**: If a node has `CHILD_DIRTY` but its calculated constraints (width/height allocated by the parent) match its previous layout run, down-tree processing can be skipped for any child that lacks its own `DIRTY` bit.
*   **Width Tracking**: `layout_block_find_dimensions` tracks `last_available_width` in each `struct box` to skip dimension calculations for clean subtrees in stable containers.

## 3. Implementation Status
*   Core `DIRTY` and `CHILD_DIRTY` flags are implemented.
*   `box_mark_dirty` in `box_manipulate.c` handles upward propagation of `CHILD_DIRTY`.
*   `layout_document` and `layout_block_context` utilize these flags to skip reflows when possible.
