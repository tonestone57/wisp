import re

with open('src/content/handlers/html/font_face.c', 'r') as f:
    content = f.read()

search_str = """    /* Check if already in list */
    for (entry = loaded_fonts; entry != NULL; entry = entry->next) {
        if (font_variant_match(&entry->variant, id)) {
            entry->refcount++;
            return;
        }
    }

    /* Add to list */
    entry = malloc(sizeof(struct loaded_font));
    if (entry != NULL) {
        entry->variant.family_name = strdup(id->family_name);
        entry->variant.weight = id->weight;
        entry->variant.style = id->style;
        entry->refcount = 1;
        entry->next = loaded_fonts;
        loaded_fonts = entry;
        NSLOG(wisp, INFO, "Marked font '%s' (weight=%d style=%d) as loaded (refcount=1)", id->family_name, id->weight, id->style);
    }"""

replace_str = """    /* Check if already in list */
    for (entry = loaded_fonts; entry != NULL; entry = entry->next) {
        if (font_variant_match(&entry->variant, id)) {
            entry->last_used = ++font_use_counter;
            return;
        }
    }

    /* Add to list */
    entry = malloc(sizeof(struct loaded_font));
    if (entry != NULL) {
        entry->variant.family_name = strdup(id->family_name);
        entry->variant.weight = id->weight;
        entry->variant.style = id->style;
        entry->last_used = ++font_use_counter;
        entry->next = loaded_fonts;
        loaded_fonts = entry;
        loaded_font_count++;

        NSLOG(wisp, INFO, "Marked font '%s' (weight=%d style=%d) as loaded", id->family_name, id->weight, id->style);

        evict_lru_font_if_needed();
    }"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('src/content/handlers/html/font_face.c', 'w') as f:
        f.write(content)
    print("Patched mark_font_loaded successfully")
else:
    print("Search string not found")
