import re
import sys

files = ["main/ui/ui.c"]
korean = set()
for fname in files:
    with open(fname, encoding="utf-8") as f:
        text = f.read()
    strings = re.findall(r'"((?:[^"\\]|\\.)*)"', text)
    for s in strings:
        for ch in s:
            if 0xAC00 <= ord(ch) <= 0xD7A3:
                korean.add(ch)

chars = "".join(sorted(korean))
print(len(korean), "unique syllables")
print(chars)
with open("scripts/korean_chars.txt", "w", encoding="utf-8") as f:
    f.write(chars)
