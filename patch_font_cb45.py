import re

with open('frontends/windows/font.c', 'r') as f:
    content = f.read()

search_str = """    if (loaded_font_count < MAX_LOADED_FONTS) {
        loaded_font_handles[loaded_font_count++] = font_handle;
    } else {
        NSLOG(wisp, WARNING, "Max loaded fonts reached. Handle will leak on shutdown.");
    }

    /* We no longer manually schedule a global repaint here. The core engine
     * correctly queues a redraw sequence if the html_font_face_set_done_callback
     * is registered and active via win32_font_repaint_callback.
     */

    return NSERROR_OK;
}

void win32_font_fini(void)
{
    win32_font_caches_flush();

    for (int i = 0; i < loaded_font_count; i++) {
        if (loaded_font_handles[i] != NULL) {
            RemoveFontMemResourceEx(loaded_font_handles[i]);
            loaded_font_handles[i] = NULL;
        }
    }
    loaded_font_count = 0;
}"""

replace_str = """    struct win32_loaded_font *lf = malloc(sizeof(struct win32_loaded_font));
    if (lf != NULL) {
        lf->variant.family_name = strdup(id->family_name);
        lf->variant.weight = id->weight;
        lf->variant.style = id->style;
        lf->handle = font_handle;
        lf->internal_name = internal_name ? strdup(internal_name) : NULL;
        lf->next = win32_loaded_fonts;
        win32_loaded_fonts = lf;
    }

    /* We no longer manually schedule a global repaint here. The core engine
     * correctly queues a redraw sequence if the html_font_face_set_done_callback
     * is registered and active via win32_font_repaint_callback.
     */

    return NSERROR_OK;
}

static void win32_free_font_data(const struct font_variant_id *id)
{
    struct win32_loaded_font *entry = win32_loaded_fonts;
    struct win32_loaded_font *prev = NULL;

    while (entry != NULL) {
        if (entry->variant.weight == id->weight && entry->variant.style == id->style &&
            strcasecmp(entry->variant.family_name, id->family_name) == 0) {

            NSLOG(wisp, INFO, "Freeing GDI resources for web font '%s'", id->family_name);

            if (entry->handle != NULL) {
                RemoveFontMemResourceEx(entry->handle);
            }

            /* Remove mapping */
            if (entry->internal_name != NULL) {
                /* Actually we can't easily remove a single font map entry right now,
                 * but it will just be ignored if the font is gone. */
                free(entry->internal_name);
            }

            free(entry->variant.family_name);

            if (prev != NULL) {
                prev->next = entry->next;
            } else {
                win32_loaded_fonts = entry->next;
            }

            free(entry);
            break;
        }
        prev = entry;
        entry = entry->next;
    }

    win32_font_caches_flush();
}

void win32_font_fini(void)
{
    win32_font_caches_flush();

    struct win32_loaded_font *entry = win32_loaded_fonts;
    while (entry != NULL) {
        struct win32_loaded_font *next = entry->next;
        if (entry->handle != NULL) {
            RemoveFontMemResourceEx(entry->handle);
        }
        if (entry->internal_name != NULL) {
            free(entry->internal_name);
        }
        free(entry->variant.family_name);
        free(entry);
        entry = next;
    }
    win32_loaded_fonts = NULL;
}"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('frontends/windows/font.c', 'w') as f:
        f.write(content)
    print("Patched font.c replace successfully")
else:
    print("Search string not found")
