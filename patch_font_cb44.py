import re

with open('frontends/windows/font.c', 'r') as f:
    content = f.read()

search_str = """#define MAX_LOADED_FONTS 128
static HANDLE loaded_font_handles[MAX_LOADED_FONTS];
static int loaded_font_count = 0;"""

replace_str = """struct win32_loaded_font {
    struct font_variant_id variant;
    HANDLE handle;
    char *internal_name;
    struct win32_loaded_font *next;
};

static struct win32_loaded_font *win32_loaded_fonts = NULL;"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('frontends/windows/font.c', 'w') as f:
        f.write(content)
    print("Patched font.c replace successfully")
else:
    print("Search string not found")
