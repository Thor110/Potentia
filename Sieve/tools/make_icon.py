#!/usr/bin/env python3
"""Sieve's icon: the hallway's minimap (client/hud.cpp, draw_compass), drawn as a vector.

The minimap is the library in one picture: the corridor bent into circles, binary outermost and
innermost, the six lines between in door order (pages, image, audio, video, books, models), each
in its own colour, and the needle at your angle on the line you are on, here pages at 0 degrees,
with every line's mark on it. Edward chose it as the icon, and of three drawings of it, this one
(27 September 2026): the rings on a black disc, as the minimap has them on its black panel, drawn
bold enough to hold at Explorer's sizes. The disc is what makes it read on any background; outside
it the icon is transparent, so it is round, with no square behind it.

Two jobs:

    python tools/make_icon.py                 # client/icon_pixels.h from data/icons/sieve.ico
    python tools/make_icon.py --draw DIR      # the drawing: SVGs, PNGs and an .ico, into DIR

The icon shipped is data/icons/sieve.ico, which Edward made from this drawing (design "C") by
shrinking the 256-pixel rendering with bicubic filtering: that keeps thin lines bright at 48 and 64
pixels, where drawing straight at the small size dims them. So by default this script only turns
that .ico's 64-pixel image into the window icon compiled into the programs
(client/icon_pixels.h, used by client/window_icon.hpp). --draw makes the drawing again, for a
redesign, into another folder, so it never overwrites the icon shipped.
"""

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "data" / "icons"

SIZE = 256
C = SIZE / 2
# The rings, outside in, as the minimap draws them (radii scaled from its 92-pixel outer ring), in
# each line's minimap colour (client/theme.hpp, ring_ink).
RINGS = [
    ("binary", 116.0, (30, 206, 75)),
    ("pages", 101.0, (255, 255, 255)),
    ("image", 87.0, (0, 238, 238)),
    ("audio", 73.0, (238, 165, 0)),
    ("video", 59.0, (238, 238, 0)),
    ("books", 45.0, (140, 140, 140)),
    ("models", 31.0, (0, 238, 0)),
    ("binary", 17.0, (30, 206, 75)),
]
DISC = "#050505"  # the minimap's black panel, as a disc
RIM = "#2a2a2a"   # its edge, faint, so the disc keeps its shape on a dark background
LINE = 4.5        # a ring's width, and the needle's
HERE = 8.0        # the ring you are on (pages) is drawn thicker, as in the minimap
MARK = 9.0        # a line's mark on its ring
HERE_MARK = 14.0  # yours, at the needle's end

# The small sizes (16 to 32 pixels) cannot hold eight rings: there the icon keeps the outer binary
# ring, pages (yours, white), and every other line's ring, bolder, so it still reads as the minimap.
SMALL_RINGS = [RINGS[0], RINGS[1], ("image", 78.0, RINGS[2][2]), ("video", 55.0, RINGS[4][2]), ("models", 32.0, RINGS[6][2])]
SMALL = dict(line=10.0, here=14.0, mark=0.0, here_mark=22.0, rim=False)


def rgb(c):
    return "#%02x%02x%02x" % c


def svg(rings=None, line=LINE, here=HERE, mark=MARK, here_mark=HERE_MARK, rim=True):
    rings = rings or RINGS
    parts = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{SIZE}" height="{SIZE}" viewBox="0 0 {SIZE} {SIZE}">',
             "  <title>Sieve</title>",
             f'  <circle id="disc" cx="{C}" cy="{C}" r="126" fill="{DISC}"/>']
    if rim:
        parts.append(f'  <circle id="rim" cx="{C}" cy="{C}" r="125" fill="none" stroke="{RIM}" stroke-width="2"/>')
    parts.append('  <g id="rings" fill="none">')
    for name, r, colour in rings:
        parts.append(f'    <circle cx="{C}" cy="{C}" r="{r}" stroke="{rgb(colour)}" stroke-width="{here if name == "pages" else line}"/>')
    parts.append("  </g>")
    # The needle: from the centre to the pages ring, at 0 degrees (straight up); every line's mark
    # on its ring where the needle crosses it, and yours, larger, at its end.
    top = C - rings[1][1]
    parts.append(f'  <line id="needle" x1="{C}" y1="{C}" x2="{C}" y2="{top}" stroke="#ffffff" stroke-width="{line}"/>')
    parts.append('  <g id="marks">')
    if mark > 0:
        for name, r, colour in rings[2:]:
            if name != "binary":
                parts.append(f'    <rect x="{C - mark / 2}" y="{C - r - mark / 2}" width="{mark}" height="{mark}" fill="{rgb(colour)}"/>')
    parts.append(f'    <rect x="{C - here_mark / 2}" y="{top - here_mark / 2}" width="{here_mark}" height="{here_mark}" fill="#ffffff"/>')
    parts.append("  </g>")
    parts.append("</svg>")
    return "\n".join(parts) + "\n"


def write_pixels(image64):
    """The window icon, compiled into the programs (client/window_icon.hpp): 64 by 64, RGBA."""
    px = image64.convert("RGBA").tobytes()
    rows = [", ".join(str(b) for b in px[i:i + 32]) for i in range(0, len(px), 32)]
    header = ("// Sieve's icon as a window icon: 64 by 64 pixels, RGBA, rows top to bottom. Written by\n"
              "// tools/make_icon.py from data/icons/sieve.ico; do not edit by hand.\n"
              "#pragma once\n\n"
              "inline constexpr int kSieveIconSize = 64;\n"
              "inline constexpr unsigned char kSieveIcon[64 * 64 * 4] = {\n    " + ",\n    ".join(rows) + "};\n")
    (ROOT / "client" / "icon_pixels.h").write_text(header, encoding="utf-8")
    print("wrote client/icon_pixels.h")


def main():
    global OUT
    import sys

    from PIL import Image

    if "--draw" not in sys.argv:
        ico = Image.open(OUT / "sieve.ico")
        ico.size = (64, 64)
        write_pixels(ico.copy())
        return
    OUT = Path(sys.argv[sys.argv.index("--draw") + 1]).resolve()
    OUT.mkdir(parents=True, exist_ok=True)
    text = svg()
    (OUT / "sieve.svg").write_text(text, encoding="utf-8")
    print(f"wrote {OUT / 'sieve.svg'}")
    try:
        import io

        import cairosvg
        from PIL import Image
    except ImportError:
        print("(cairosvg and Pillow make the PNGs and the .ico: pip install cairosvg pillow)")
        return
    small = svg(SMALL_RINGS, **SMALL)
    (OUT / "sieve-small.svg").write_text(small, encoding="utf-8")
    sizes = [16, 24, 32, 48, 64, 128, 256]
    images = []
    for n in sizes:
        png = cairosvg.svg2png(bytestring=(small if n <= 32 else text).encode("utf-8"), output_width=n, output_height=n)
        (OUT / f"sieve-{n}.png").write_bytes(png)
        images.append(Image.open(io.BytesIO(png)).convert("RGBA"))
    images[-1].save(OUT / "sieve.ico", sizes=[(n, n) for n in sizes], append_images=images[:-1])
    print(f"wrote sieve-<n>.png for {sizes} and sieve.ico")


if __name__ == "__main__":
    main()
