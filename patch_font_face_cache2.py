import re

with open('src/content/handlers/html/font_face.c', 'r') as f:
    content = f.read()

search_str = """struct loaded_font {
    struct font_variant_id variant;
    int refcount;
    struct loaded_font *next;
};

static struct loaded_font *loaded_fonts = NULL;"""

replace_str = """#define MAX_LOADED_WEB_FONTS 64

struct loaded_font {
    struct font_variant_id variant;
    int last_used; /* simple counter for LRU */
    struct loaded_font *next;
};

static struct loaded_font *loaded_fonts = NULL;
static int font_use_counter = 0;
static int loaded_font_count = 0;"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('src/content/handlers/html/font_face.c', 'w') as f:
        f.write(content)
    print("Patched loaded_font struct successfully")
else:
    print("Search string not found")
