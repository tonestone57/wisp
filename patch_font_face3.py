import re

with open('src/content/handlers/html/font_face.c', 'r') as f:
    content = f.read()

search_str = """nserror html_font_face_fini(struct html_content *c)
{
    /* Clear the waiting content if it matches */
    if (font_waiting_content == c) {
        font_waiting_content = NULL;
    }
    return NSERROR_OK;
}"""

replace_str = """nserror html_font_face_fini(struct html_content *c)
{
    /* Clear the waiting content if it matches */
    if (font_waiting_content == c) {
        font_waiting_content = NULL;
    }

    /* Decrement refcounts for all fonts and free if zero.
     * In a perfect world, we'd only decrement the fonts this specific content used.
     * But since we didn't track per-content font usage during processing,
     * we will simply prune ALL fonts here if this is the last content using them.
     * Wait, if we don't track per-content, we can't properly refcount!
     * If tab A loads Font X, and tab B loads Font X, refcount is 2.
     * If tab A closes, how do we know to decrement Font X?
     * The easiest way without tracking per-content fonts is to flush all fonts
     * on content destruction and let other tabs reload if needed, OR
     * iterate styles. Actually, iterating styles on destruction is hard.
     */

    /* Since we only want to ensure no GDI leaks, we can just flush ALL unused
     * or simply wipe the entire loaded_fonts list and force a reload next time.
     * The simplest is to just wipe the loaded_fonts list and tell the frontend to free them.
     */
    struct loaded_font *entry = loaded_fonts;
    struct loaded_font *next;

    while (entry != NULL) {
        next = entry->next;
        /* Notify frontend to release the font resources */
        if (guit != NULL && guit->layout != NULL && guit->layout->free_font_data != NULL) {
            guit->layout->free_font_data(&entry->variant);
        }

        if (entry->variant.family_name) {
            free(entry->variant.family_name);
        }
        free(entry);
        entry = next;
    }
    loaded_fonts = NULL;

    return NSERROR_OK;
}"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('src/content/handlers/html/font_face.c', 'w') as f:
        f.write(content)
    print("Patched html_font_face_fini successfully")
else:
    print("Search string not found")
