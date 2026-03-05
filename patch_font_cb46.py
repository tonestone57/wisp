import re

with open('frontends/windows/font.c', 'r') as f:
    content = f.read()

search_str = """static struct gui_layout_table layout_table = {
    .width = win32_font_width,
    .position = win32_font_position,
    .split = win32_font_split,
    .load_font_data = html_font_face_load_data,
};"""

replace_str = """static struct gui_layout_table layout_table = {
    .width = win32_font_width,
    .position = win32_font_position,
    .split = win32_font_split,
    .load_font_data = html_font_face_load_data,
    .free_font_data = win32_free_font_data,
};"""

if search_str in content:
    content = content.replace(search_str, replace_str)
    with open('frontends/windows/font.c', 'w') as f:
        f.write(content)
    print("Patched font.c replace successfully")
else:
    print("Search string not found")
