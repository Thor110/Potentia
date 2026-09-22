#!/usr/bin/env python3
"""Builds data/fonts/sieve8x8.hex, the menus' default font, from Daniel Hepper's font8x8
(public domain, https://github.com/dhepper/font8x8, itself based on Marcel Sondaar's and IBM's
public domain VGA fonts).

    python3 tools/build_font.py FONT8X8_DIR > data/fonts/sieve8x8.hex

FONT8X8_DIR holds font8x8_basic.h, font8x8_ext_latin.h, font8x8_greek.h, font8x8_box.h,
font8x8_block.h and font8x8_hiragana.h. Output is the Unifont .hex format with a height line:

    # height: 8
    0041:0C1E33333F333300       (code point : one byte per row, top row first, leftmost pixel = high bit)

Any font in this format (Unifont's own .hex files, 16 rows, 8 or 16 pixels wide) can be put in
data/fonts and named by a language file's `font =` line.
"""
import re
import sys

# file, first code point (font8x8 files are dense tables starting there)
SOURCES = [("font8x8_basic.h", 0x0000), ("font8x8_ext_latin.h", 0x00A0), ("font8x8_greek.h", 0x0390),
           ("font8x8_box.h", 0x2500), ("font8x8_block.h", 0x2580), ("font8x8_hiragana.h", 0x3040)]


def rows_of(path):
    text = open(path, encoding="utf-8", errors="replace").read()
    body = text[text.index("= {") + 3:]
    for m in re.finditer(r"\{\s*((?:0x[0-9A-Fa-f]{2}\s*,?\s*){8})\}", body):
        yield [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]{2})", m.group(1))]


def main():
    sys.stdout.reconfigure(newline="\n", encoding="utf-8")  # the same bytes on every platform
    folder = sys.argv[1] if len(sys.argv) > 1 else "."
    glyphs = {}
    for name, first in SOURCES:
        for i, rows in enumerate(rows_of(f"{folder}/{name}")):
            cp = first + i
            if cp < 0x20 or (0x7F <= cp < 0xA0):
                continue  # control characters
            # font8x8 puts the leftmost pixel in bit 0; .hex puts it in the high bit.
            flipped = [int(f"{r:08b}"[::-1], 2) for r in rows]
            glyphs.setdefault(cp, flipped)
    out = sys.stdout
    out.write("# sieve8x8: font8x8 by Daniel Hepper (public domain), from Marcel Sondaar's and IBM's\n")
    out.write("# public domain VGA fonts. Built by tools/build_font.py.\n")
    out.write("# height: 8\n")
    for cp in sorted(glyphs):
        out.write(f"{cp:04X}:{''.join(f'{r:02X}' for r in glyphs[cp])}\n")


if __name__ == "__main__":
    main()
