#!/usr/bin/env python3
r"""Gliders through a file (Edward, 1 November 2026: "what chaos a few gliders might cause through a
file from the game of life ... pick some random bytes, fire them off, have them alter values as
they go through the bytes with a +1/-1 type schema ... making sure to wrap the value around";
IDEAS §19.111). A prototype of the shortcut seed's, measured.

The file as a grid of rows of W bytes, its edges wrapped (a torus). A recipe names a few gliders
(each a place and one of four directions) and a step; Conway's rules run for as many generations as
the grid has rows, and each generation every live cell's byte has the step added, wrapping round.
What the gliders change depends on the gliders alone, never on the file, so installing makes the
same board and takes the same changes away. The search: a glider at every place and in every
direction, each scored by what the changed file would cost (its bytes counted by how often each
value comes, the shortcut seed's literals), the best kept, then the next, up to four; then the
changed file given to the shortcut seed (tools/shortcut_seed.py) and the gliders' own numbers paid.

  python tools/shortcut_gliders.py [--real FILE:WIDTH ...]

First a designed grid: 64 by 64 zeros through which two gliders were fired (it should be cancelled);
then the real files given (their first 32 KiB).
"""

import argparse
import math
import random
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import shortcut_seed as seed  # noqa: E402

# A glider, and its four directions (by reflecting it).
GLIDER = np.array([[0, 1, 0], [0, 0, 1], [1, 1, 1]], dtype=np.uint8)
SHAPES = [GLIDER, GLIDER[:, ::-1], GLIDER[::-1, :], GLIDER[::-1, ::-1]]


def life(board):
    n = sum(np.roll(np.roll(board, dy, 0), dx, 1) for dy in (-1, 0, 1) for dx in (-1, 0, 1) if (dy, dx) != (0, 0))
    return ((n == 3) | ((board == 1) & (n == 2))).astype(np.uint8)


def trails(gliders, rows, w):
    """How many generations each cell was alive, the gliders started as given, for rows generations."""
    board = np.zeros((rows, w), dtype=np.uint8)
    for (r, c, d) in gliders:
        for dy in range(3):
            for dx in range(3):
                if SHAPES[d][dy, dx]:
                    board[(r + dy) % rows, (c + dx) % w] = 1
    count = np.zeros((rows, w), dtype=np.int64)
    for _ in range(rows):
        count += board
        board = life(board)
    return count


def apply(x, gliders, step, w, back=False):
    rows = len(x) // w
    body = np.frombuffer(bytes(x[: rows * w]), dtype=np.uint8).astype(np.int64).reshape(rows, w)
    change = trails(gliders, rows, w) * step
    out = (body + change) if back else (body - change)
    return (out & 255).astype(np.uint8).tobytes() + bytes(x[rows * w:])


def order0_bits(b):
    c = np.bincount(np.frombuffer(b, dtype=np.uint8), minlength=256).astype(np.float64)
    c = c[c > 0]
    return float(-(c * np.log2(c / c.sum())).sum())


def glider_bits(gliders, rows, w):
    """The gliders' own numbers: how many (2 bits), each its row, column and direction, the step."""
    return 2 + 8 + len(gliders) * (math.ceil(math.log2(rows)) + math.ceil(math.log2(w)) + 2)


def search(x, w, most=4, steps=(1, 255)):
    rows = len(x) // w
    best_g, best_step, best_bits = [], 1, order0_bits(bytes(x))
    for step in steps:
        chosen = []
        cur = best_bits if step == steps[0] else order0_bits(bytes(x))
        for _ in range(most):
            found = None
            for r in range(rows):
                for c in range(w):
                    for d in range(4):
                        g = chosen + [(r, c, d)]
                        bits = order0_bits(apply(x, g, step, w)) + glider_bits(g, rows, w)
                        if found is None or bits < found[0]:
                            found = (bits, (r, c, d))
            if found is None or found[0] >= cur:
                break
            cur = found[0]
            chosen.append(found[1])
        if chosen and cur < best_bits:
            best_g, best_step, best_bits = chosen, step, cur
    return best_g, best_step


def measure(name, x, w):
    rows = len(x) // w
    g, step = search(x, w)
    y = apply(x, g, step, w)
    assert apply(y, g, step, w, back=True) == bytes(x), "the gliders do not undo"
    plain = min(r["bytes"] for r in seed.search(x, w))
    with_g = min(r["bytes"] for r in seed.search(y, w)) + math.ceil(glider_bits(g, rows, w) / 8) if g else plain
    return {"name": name, "n": len(x), "w": w, "gliders": g, "step": step, "plain": plain, "with": with_g, "xz": len(seed.xz_of(x))}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--real", nargs="*", default=[])
    a = ap.parse_args()
    rnd = random.Random(28)
    # Two designs: gliders from random places and directions (here they meet, and leave a still-life
    # behind, so what they made is not the sum of each one's trail); and two flying the same way,
    # apart, which never meet.
    met = apply(bytes(64 * 64), [(rnd.randrange(64), rnd.randrange(64), rnd.randrange(4)) for _ in range(2)], 1, 64, back=True)
    apart = apply(bytes(64 * 64), [(5, 5, 0), (5, 37, 0)], 1, 64, back=True)
    rows = []
    for name, x in (("two gliders that meet, through zeros (64 by 64)", met), ("two gliders flying apart, through zeros (64 by 64)", apart)):
        rows.append(measure(name, x, 64))
        print(rows[-1], flush=True)
    for spec in a.real:
        f, w = spec.rsplit(":", 1)
        x = Path(f).read_bytes()[:4096]
        rows.append(measure(Path(f).name + " (first 4 KiB)", x, int(w)))
        print(rows[-1], flush=True)
    print("\n| File | Bytes | W | Gliders found | Seed without | Seed with gliders | xz |\n| :--- | ---: | ---: | :--- | ---: | ---: | ---: |")
    for r in rows:
        print(f"| {r['name']} | {r['n']:,} | {r['w']} | {len(r['gliders'])}, step {r['step']} | {r['plain']:,} | {r['with']:,} | {r['xz']:,} |")


if __name__ == "__main__":
    main()
