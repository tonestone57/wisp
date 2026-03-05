import re

with open('src/content/handlers/html/font_face.c', 'r') as f:
    content = f.read()

search_str = """/**
 * Check if a specific font variant is already loaded.
 */
static bool is_variant_loaded(const struct font_variant_id *id)
{
    struct loaded_font *entry;

    for (entry = loaded_fonts; entry != NULL; entry = entry->next) {
        if (font_variant_match(&entry->variant, id)) {
            return true;
        }
    }
    return false;
}"""

replace_str = """/**
 * Check if a specific font variant is already loaded.
 */
static bool is_variant_loaded(const struct font_variant_id *id)
{
    struct loaded_font *entry;

    for (entry = loaded_fonts; entry != NULL; entry = entry->next) {
        if (font_variant_match(&entry->variant, id)) {
            entry->last_used = ++font_use_counter;
            return true;
        }
    }
    return false;
}

/**
 * Evict the least recently used font if we exceed the limit.
 */
static void evict_lru_font_if_needed(void)
{
    if (loaded_font_count <= MAX_LOADED_WEB_FONTS) {
        return;
    }

    struct loaded_font *entry = loaded_fonts;
    struct loaded_font *prev = NULL;

    struct loaded_font *lru = NULL;
    struct loaded_font *lru_prev = NULL;
    int min_used = font_use_counter + 1;

    while (entry != NULL) {
        if (entry->last_used < min_used) {
            min_used = entry->last_used;
            lru = entry;
            lru_prev = prev;
        }
        prev = entry;
        entry = entry->next;
    }

    if (lru != NULL) {
        if (lru_prev != NULL) {
            lru_prev->next = lru->next;
        } else {
            loaded_fonts = lru->next;
        }

        NSLOG(wisp, INFO, "Evicting LRU font '%s' (weight=%d style=%d)", lru->variant.family_name, lru->variant.weight, lru->variant.style);

        if (guit != NULL && guit->layout != NULL && guit->layout->free_font_data != NULL) {
            guit->layout->free_font_data(&lru->variant);
        }

        free(lru->variant.family_name);
        free(lru);
        loaded_font_count--;
    }
}"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('src/content/handlers/html/font_face.c', 'w') as f:
        f.write(content)
    print("Patched is_variant_loaded successfully")
else:
    print("Search string not found")
