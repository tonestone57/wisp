import re

with open('src/content/handlers/html/font_face.c', 'r') as f:
    content = f.read()

# Instead of true reference counting per document (which requires tracking inside libcss's arena or deep inside layout.c),
# a simpler Least Recently Used / Cap on the number of loaded fonts is more robust.
# The user's feedback suggests:
# "If the core doesn't currently prune loaded_fonts, you might need to implement a simple Least Recently Used (LRU) Cache"
# Let's cap the cache at 64 fonts. When we hit 64, we evict the oldest.
