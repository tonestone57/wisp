We need to implement incremental layout using dirty bits in layout.c and related functions.

1. `box.h`: Add a `DIRTY = 1 << 14` flag to `box_flags`. (Already done, just need to review).
2. When layout is done on a box, clear its `DIRTY` flag.
3. In functions like `layout_block_context`, `layout_minmax_block`, etc., we can check if the tree is dirty or skip reflow if it's not.

Wait, the instructions say: "The layout engine needs to ensure it only recalculates subtrees that have changed (via dirty bit flags). Currently, redraw.c and layout.c suggest full reflows happen too frequently."

But how do we know when a subtree has changed? We probably need to set the `DIRTY` flag when the DOM is mutated or style is changed.
And then in `layout_document`, we don't need to recalculate `minmax` or `layout` if the box isn't dirty? No, if the container width changes, we need to relayout even if the contents haven't changed. Incremental layout is tricky.

Let's look at `layout_document`:
```c
    layout_minmax_block(doc, font_func, content);
    layout_block_find_dimensions(&content->unit_len_ctx, width, height, 0, 0, doc);
    ret = layout_block_context(doc, height, content);
```

Let's find out how the problem is described: "Currently, redraw.c and layout.c suggest full reflows happen too frequently."

Wait! Are there any `dirty` flags already that I missed?
"The layout engine needs to ensure it only recalculates subtrees that have changed (via dirty bit flags). Currently, redraw.c and layout.c suggest full reflows happen too frequently."

Let's look at `layout.c`.
Is there a way to skip recalculation if `DIRTY` is not set?
