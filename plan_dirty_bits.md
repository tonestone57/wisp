# Incremental Layout Strategy (Dirty Bits)

Wisp implements an incremental layout engine to avoid redundant full-page reflows. To solve the problem of unnecessary sub-tree reflows when container dimensions change, we utilize a **Dual-Pass Dirty Bit Strategy**.

## 1. Dual-Pass Dirty Bits
The `box_flags` in `include/wisp/content/handlers/html/box.h` use three distinct bits for tracking changes:

1.  **DIRTY_INTRINSIC (Bit 14)**: Indicates that the element’s internal contents, text, or styles have changed. This requires a recalculation of intrinsic traits (like min/max widths).
2.  **CHILD_DIRTY (Bit 15)**: Indicates that a descendant in the box tree is dirty and requires a pass.
3.  **DIRTY_LAYOUT (Bit 16)**: Indicates that the element's bounds or layout environment changed (e.g., parent size changed), but its children's intrinsic traits (min/max) remain intact.
4.  **DIRTY (Bit 14 | Bit 16)**: A convenience alias for any type of layout invalidation.

## 2. Propagation and Execution Rules
*   **DOM/Style Mutation**: When a DOM node or style changes, the `DIRTY_INTRINSIC` bit is set on the associated box via `box_mark_dirty`, and `CHILD_DIRTY` is propagated up to the root.
*   **Dirty Rect Accumulation**: `box_mark_dirty` captures the *old* bounding box of the element and unions it with the document's `dirty_rect` to ensure the previous position is cleared.
*   **Dirty List**: Boxes marked dirty are added to a `dirty_list` (Bit 17 `BOX_IN_DIRTY_LIST`) for post-layout processing to capture the *new* bounding box.
*   **Layout Pass**: If a node has `CHILD_DIRTY` but its calculated constraints (width/height allocated by the parent) match its previous layout run, down-tree processing can be skipped for any child that lacks its own `DIRTY` bits.
*   **Width Tracking**: `layout_block_find_dimensions` tracks `last_available_width` in each `struct box` to skip dimension calculations for clean subtrees in stable containers.

## 3. Implementation Status
*   Core `DIRTY_INTRINSIC`, `CHILD_DIRTY`, and `DIRTY_LAYOUT` flags are implemented in `box.h`.
*   `box_mark_dirty` in `box_manipulate.c` handles upward propagation and `dirty_rect` unioning.
*   `layout_document` and `layout_block_context` utilize these flags to skip reflows.
*   Redraw is triggered at the end of `layout_document` in `src/content/handlers/html/layout.c` using the accumulated `dirty_rect`.

## 4. Tiled Redraw Strategy
Wisp has successfully transitioned from a union-based dirty region system to a **Fixed-Tile Redraw** strategy to eliminate "overdraw hell" and improve performance on cache-constrained hardware (like i586). This is now the core architectural standard for all modern frontends.

### Key Architectural Decisions:
1.  **Platform-Defined Fixed Tile Size**:
    - **Retro Systems (i586)**: Locked to **256x256** for optimal cache locality and bit-shift math (`x >> 8`).
    - **High-DPI / 4K / Mobile**: Locked to **512x512** to minimize management overhead on high-density displays.
    - **Zero Fragmentation**: Enables a fixed-block memory pool within the arena allocator, resulting in zero runtime heap fragmentation.
    - **Cache Locality**: Small tiles fit optimally within L2/L3 caches, serving as a "surrogate SIMD" for older processors.
2.  **Redraw Loop**:
    - The engine iterates through dirty tiles, clips the Blend2D context to the tile boundary, and executes the standard display list.
    - **Global Redraw Threshold**: If more than 70% of tiles are dirty, the engine falls back to a single full-screen redraw to avoid tile-management overhead.
3.  **Parallel Rendering**: Tiled independence allows for future multi-threaded rendering where separate threads rasterize different tiles simultaneously.
