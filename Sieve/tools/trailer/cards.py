"""Title cards and captions for the trailer, drawn in Sieve's own font (data/fonts/sieve8x8.hex,
font8x8, public domain), each glyph's pixels scaled up whole so they stay sharp, as the hallway
draws them."""
import os

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(HERE, "..", "..", "data", "fonts", "sieve8x8.hex")
W, H = 1920, 1080

# The lines' colours (client/dimensions.hpp, as the hallway paints them): background, then accent.
COLOURS = {
    "pages": ((0, 0, 0), (255, 255, 255)),
    "image": ((0, 0, 160), (0, 255, 255)),
    "audio": ((0, 100, 0), (255, 191, 0)),
    "video": ((150, 0, 0), (255, 255, 0)),
    "models": ((140, 120, 0), (0, 255, 0)),
    "ai": ((60, 0, 100), (220, 180, 255)),
    "fractals": ((40, 30, 70), (200, 180, 255)),
    "binary": ((0, 10, 0), (0, 255, 90)),
}

_glyphs = None


def glyphs():
    global _glyphs
    if _glyphs is None:
        _glyphs = {}
        for line in open(FONT, encoding="utf-8"):
            if ":" in line and not line.startswith("#"):
                cp, bits = line.strip().split(":")
                _glyphs[int(cp, 16)] = bytes.fromhex(bits)
    return _glyphs


def text_width(s, scale):
    return len(s) * 8 * scale


def draw_text(img, x, y, s, scale, colour):
    d = ImageDraw.Draw(img)
    g = glyphs()
    for i, ch in enumerate(s):
        rows = g.get(ord(ch), g.get(ord("?")))
        for r, b in enumerate(rows):
            for c in range(8):
                if b & (0x80 >> c):
                    x0, y0 = x + (i * 8 + c) * scale, y + r * scale
                    d.rectangle([x0, y0, x0 + scale - 1, y0 + scale - 1], fill=colour)


def wrap(s, scale, width):
    words, lines, cur = s.split(" "), [], ""
    for w in words:
        t = (cur + " " + w).strip()
        if text_width(t, scale) > width and cur:
            lines.append(cur)
            cur = w
        else:
            cur = t
    if cur:
        lines.append(cur)
    return lines


def card(path, title=None, lines=(), line="pages", kicker=None, title_scale=7, body_scale=3, foot=None, table=False):
    """A card: an optional small kicker above, a title, and lines of text below, centred."""
    bg, accent = COLOURS[line]
    bg = tuple(int(c * 0.35) for c in bg)
    img = Image.new("RGB", (W, H), bg)
    blocks = []
    if kicker:
        blocks.append((kicker, 3, accent, 28))
    if title:
        for t in wrap(title, title_scale, W - 240):
            blocks.append((t, title_scale, (255, 255, 255), 18))
        blocks.append(("", 0, None, 30))
    body_start = len(blocks)
    for l in lines:
        if not l:
            blocks.append(("", 0, None, 16))
            continue
        for t in ([l] if table else wrap(l, body_scale, W - 360)):
            blocks.append((t, body_scale, (225, 225, 225), 14))
    # a table's lines share one left edge, the block centred as a whole, so its columns stay in line
    left = None
    if table:
        widest = max(text_width(t, s) for t, s, _, _ in blocks[body_start:] if t)
        left = (W - widest) // 2
    height = sum(s * 8 + gap for _, s, _, gap in blocks)
    y = (H - height) // 2
    for i, (t, s, colour, gap) in enumerate(blocks):
        if t:
            x = left if left is not None and i >= body_start else (W - text_width(t, s)) // 2
            draw_text(img, x, y, t, s, colour)
        y += s * 8 + gap
    # a rule in the line's accent above and below, as the menus underline their titles
    d = ImageDraw.Draw(img)
    d.rectangle([W // 2 - 300, 120, W // 2 + 300, 123], fill=accent)
    d.rectangle([W // 2 - 300, H - 124, W // 2 + 300, H - 121], fill=accent)
    if foot:
        draw_text(img, (W - text_width(foot, 2)) // 2, H - 90, foot, 2, (150, 150, 150))
    img.save(path)


def caption(path, text, line="pages", head=None):
    """A caption over footage: a dark box in the lower middle, its edge in the line's accent."""
    _, accent = COLOURS[line]
    scale = 3
    lines = []
    for para in text.split("\n"):
        lines += wrap(para, scale, 1400)
    if head:
        lines = [None] + lines
    lh = 8 * scale + 12
    h = len(lines) * lh + 40
    w = max(text_width(l, scale) for l in lines if l) + 80
    if head:
        w = max(w, text_width(head, 4) + 80)
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    x0, y0 = (W - w) // 2, 770 - h // 2
    d = ImageDraw.Draw(img)
    d.rectangle([x0, y0, x0 + w, y0 + h], fill=(0, 0, 0, 215), outline=accent + (255,), width=3)
    y = y0 + 22
    for l in lines:
        if l is None:
            draw_text(img, x0 + 40, y, head, 4, accent + (255,))
            y += 32 + 14
            continue
        draw_text(img, x0 + 40, y, l, scale, (255, 255, 255, 255))
        y += lh
    img.save(path)
