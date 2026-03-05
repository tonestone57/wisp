import re

with open('include/wisp/layout.h', 'r') as f:
    content = f.read()

search_str = """    nserror (*load_font_data)(const struct font_variant_id *id, const uint8_t *data, size_t size);
};"""

replace_str = """    nserror (*load_font_data)(const struct font_variant_id *id, const uint8_t *data, size_t size);

    /**
     * Notify the frontend to release resources associated with a font variant.
     *
     * This is called when the core destroys a font face.
     *
     * \param[in] id   Font variant identity
     */
    void (*free_font_data)(const struct font_variant_id *id);
};"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('include/wisp/layout.h', 'w') as f:
        f.write(content)
    print("Patched layout.h successfully")
else:
    print("Search string not found")
