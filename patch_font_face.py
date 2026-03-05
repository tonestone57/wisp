import re

with open('src/content/handlers/html/font_face.c', 'r') as f:
    content = f.read()

search_str = """struct loaded_font {
    struct font_variant_id variant;
    struct loaded_font *next;
};"""

replace_str = """struct loaded_font {
    struct font_variant_id variant;
    int refcount;
    struct loaded_font *next;
};"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('src/content/handlers/html/font_face.c', 'w') as f:
        f.write(content)
    print("Patched loaded_font struct successfully")
else:
    print("Search string not found")
