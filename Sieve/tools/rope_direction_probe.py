"""A probe for a wave direction in the rope (IDEAS §19.108): not Sieve's code, but how the idea is
measured before it is built.

A file is read as rows of W bytes. Each stretch of R rows takes one direction, the neighbour it
copies exactly: the byte to its left (W), above (N), above and to the left (NW), or above and to the
right (NE). What is written is each byte's difference from that neighbour, and the stretch's choice
(two bits). The rope's triangle (the median edge detector, P4) is measured beside it, on the same
width, as the like-for-like comparison, and so is a mix of the two (each stretch taking the triangle
or a direction, whichever leaves less).

Each remainder is counted by its order-0 entropy (what a coder of each byte alone needs, as the
rope's tables are) and by LZMA2, with the stretches' choices added.

  python tools/rope_direction_probe.py FILE [FILE ...] [--width W] [--rows R] [--pixel P]

Without --width, the width is found as the rope's search would: the W (1 to 4,096) for which the
most bytes equal the byte W before them, then the best of a few near it.
"""

import argparse
import lzma
import math
import sys

import numpy as np


def h0_bytes(r):
    r = np.asarray(r).ravel()
    c = np.bincount(r, minlength=256).astype(np.float64)
    p = c[c > 0] / r.size
    return float(-(p * np.log2(p)).sum()) * r.size / 8


def lz(r):
    return len(lzma.compress(r.tobytes(), preset=9 | lzma.PRESET_EXTREME))


def grid(b, w):
    n = len(b) // w
    return np.frombuffer(b[: n * w], dtype=np.uint8).reshape(n, w).astype(np.int16), b[n * w:]


def neighbours(g, k=1):
    """The byte k before (left), above, above-left and above-right: k is a pixel's bytes, so that in
    rows of RGB the left neighbour is the same colour of the pixel before, not the colour beside it."""
    z = np.zeros_like(g)
    L = z.copy(); L[:, k:] = g[:, :-k]
    N = z.copy(); N[1:, :] = g[:-1, :]
    NW = z.copy(); NW[1:, k:] = g[:-1, :-k]
    NE = z.copy(); NE[1:, :-k] = g[:-1, k:]
    return L, N, NW, NE


def med(g, k=1):
    L, N, NW, _ = neighbours(g, k)
    return np.where(NW >= np.maximum(L, N), np.minimum(L, N), np.where(NW <= np.minimum(L, N), np.maximum(L, N), L + N - NW))


def remainder(g, pred):
    return ((g - pred) % 256).astype(np.uint8)


def best_width(b):
    a = np.frombuffer(b[: 1 << 20], dtype=np.uint8)
    scores = [(int((a[w:] == a[:-w]).sum()), w) for w in range(1, min(4096, len(a) // 4))]
    scores.sort(reverse=True)
    return [w for _, w in scores[:4]]


def measure(b, w, rows, k=1):
    g, tail = grid(b, w)
    L, N, NW, NE = neighbours(g, k)
    dirs = {"left": L, "above": N, "above-left": NW, "above-right": NE}
    tri = remainder(g, med(g, k))
    out = {"triangle (the rope's P4)": (h0_bytes(tri), lz(tri))}
    # One direction for the whole file, the best of the four.
    whole = min(((h0_bytes(remainder(g, p)), name, remainder(g, p)) for name, p in dirs.items()), key=lambda t: t[0])
    out[f"one direction ({whole[1]})"] = (whole[0], lz(whole[2]))
    # A direction a stretch of rows, or the triangle as a fifth choice: each stretch taking what
    # leaves the fewest bytes by order-0 entropy of its own remainder (a stand-in for coding it).
    choices = list(dirs.values()) + [med(g, k)]
    stretches = math.ceil(g.shape[0] / rows)
    for label, opts in (("a direction a stretch", choices[:4]), ("a direction or the triangle a stretch", choices)):
        pieces = []
        for s in range(stretches):
            sl = slice(s * rows, min((s + 1) * rows, g.shape[0]))
            cands = [remainder(g[sl], p[sl]) for p in opts]
            pieces.append(min(cands, key=lambda r: h0_bytes(r.ravel())))
        r = np.concatenate([p.ravel() for p in pieces])
        choice_bytes = stretches * math.log2(len(opts)) / 8
        out[label] = (h0_bytes(r) + choice_bytes, lz(r) + math.ceil(choice_bytes))
    return out, len(tail)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("files", nargs="+")
    ap.add_argument("--width", type=int, default=0)
    ap.add_argument("--rows", type=int, default=16)
    ap.add_argument("--pixel", type=int, default=1, help="bytes a pixel (3 for RGB, 4 for RGBA)")
    a = ap.parse_args()
    for f in a.files:
        b = open(f, "rb").read()
        widths = [a.width] if a.width else best_width(b)
        best = None
        for w in widths:
            res, tail = measure(b, w, a.rows, a.pixel)
            score = min(v[0] for v in res.values())
            if best is None or score < best[0]:
                best = (score, w, res, tail)
        _, w, res, tail = best
        print(f"{f.split('/')[-1]}: {len(b):,} bytes, rows of {w} (of {widths}), stretches of {a.rows} rows, pixels of {a.pixel}")
        for name, (e, z) in res.items():
            print(f"  {name:40s} entropy {e:12,.0f} bytes   LZMA2 {z:10,} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
