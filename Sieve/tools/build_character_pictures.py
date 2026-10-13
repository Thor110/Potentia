#!/usr/bin/env python3
"""Writes the training pictures of the shipped characters model (data/training/type-2x32).

Every printable ASCII character but the space (0x21 to 0x7E, 94 of them), drawn from Sieve's own
8 x 8 font (data/fonts/sieve8x8.hex: font8x8, public domain) onto a 10 x 10 picture of the image
line, at each of the 9 places an 8 x 8 glyph fits (0, 1 or 2 pixels from the left and the top):
846 pictures, one a line, 100 digits each, top row first, in the figures model's digits (1 the
background, 0 the ink). The order is place by place, and within a place, by code point.

  python tools/build_character_pictures.py characters.txt     (or with no file: to the console)
  sieve diffuse --record data/training/type-2x32 --pictures characters.txt --ai-layers 2 --ai-width 32 --steps 15000 --checkpoint-every 1500
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(HERE, "..", "data", "fonts", "sieve8x8.hex")
SIDE = 10                 # the image line's default picture: 10 x 10
GLYPH = 8                 # the font's glyphs: 8 x 8
FIRST, LAST = 0x21, 0x7E  # printable ASCII, the space left out
BACKGROUND, INK = "1", "0"


def glyphs():
    out = {}
    with open(FONT, encoding="ascii") as f:
        for line in f:
            if line.startswith("#") or ":" not in line:
                continue
            cp, rows = line.strip().split(":")
            out[int(cp, 16)] = [int(rows[i:i + 2], 16) for i in range(0, 2 * GLYPH, 2)]
    return out


def main():
    g = glyphs()
    lines = []
    for top in range(SIDE - GLYPH + 1):
        for left in range(SIDE - GLYPH + 1):
            for cp in range(FIRST, LAST + 1):
                cells = [[BACKGROUND] * SIDE for _ in range(SIDE)]
                for r, bits in enumerate(g[cp]):
                    for c in range(GLYPH):
                        if bits & (0x80 >> c):  # the leftmost pixel is the high bit
                            cells[top + r][left + c] = INK
                lines.append("".join("".join(row) for row in cells))
    data = ("\n".join(lines) + "\n").encode("ascii")
    if len(sys.argv) > 1:
        with open(sys.argv[1], "wb") as f:
            f.write(data)
    else:
        # Bytes, so a Windows console writes \n and not \r\n: the same everywhere.
        sys.stdout.buffer.write(data)


if __name__ == "__main__":
    main()
