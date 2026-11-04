#!/usr/bin/env python3
"""IDEAS 19.117: the rope's kept bytes as free values (wheels), and the size they give.

The rope at P0 (no propagation): every value predicted by the straight line through the kept
points; the line's leftovers (value - line, mod 256) cost their order-0 entropy, plus the stream's
table as rope-wave-v1 counts it (entropy_bytes in core/src/rope_wave.cpp), plus one byte for each
free wheel. As built, the kept points are the start, a third, two thirds and the end (and the
centre for an odd length) and carry the file's own bytes, each coded as itself; here the inner
ones are free (any value, the file's byte there then coded as a leftover like the rest).

usage: python tools/measure_rope_wheels.py OUT_DIR NAME=FILE[@OFFSET] ...

256 KiB of each file from OFFSET. Writes NAME-one.tsv (size for each of the 256 values of the wheel
at the third), NAME-two.tsv (64 by 64, the third and two thirds, every fourth value) and report.json.
Measured with it: results/rope-wheels-2026-11-03.md.
"""
import json, math, sys
from pathlib import Path
import numpy as np

PIECE = 256 << 10
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)


def load(spec):
    name, f = spec.split("=", 1)
    at = 0
    if "@" in f:
        f, at = f.rsplit("@", 1)
        at = int(at)
    b = Path(f).read_bytes()[at:at + PIECE]
    return name, np.frombuffer(b, dtype=np.uint8).astype(np.int64)


def cost(x, pts, vals):
    """Bytes: the leftovers' order-0 entropy + table + one byte a free wheel (ends kept as themselves)."""
    n = len(x)
    line = np.empty(n, dtype=np.int64)
    for k in range(len(pts) - 1):
        a, b = pts[k], pts[k + 1]
        xa, xb = vals[k], vals[k + 1]
        i = np.arange(a, b + 1)
        d = b - a
        line[a:b + 1] = xa + np.floor_divide(2 * (xb - xa) * (i - a) + d, 2 * d)
    e = (x - line) & 255
    c = np.bincount(e, minlength=256)
    nz = c[c > 0]
    bits = float((nz * np.log2(n / nz)).sum())
    return bits / 8 + 1 + len(nz) * 4 + 3 + 8 + (len(pts) - 2)


def points(n, wheels):
    """The start, `wheels` points evenly between, the end."""
    return [0] + [round(n * (k + 1) / (wheels + 1)) for k in range(wheels)] + [n - 1]


def smoothness(curve):
    c = np.asarray(curve, dtype=float)
    d = np.diff(c)
    minima = int(((c[1:-1] < c[:-2]) & (c[1:-1] < c[2:])).sum())
    # neighbours' agreement: correlation of each step with the next (1: a steady slope, 0: noise)
    corr = float(np.corrcoef(d[:-1], d[1:])[0, 1]) if d.std() > 0 else 1.0
    return {"local_minima": minima, "step_corr": round(corr, 3), "range": round(float(c.max() - c.min()), 1)}


report = {}
for spec in sys.argv[2:]:
    name, x = load(spec)
    n = len(x)
    r = {"bytes": n}
    # As built: the thirds kept as the file's own bytes (each a byte, the ends too).
    p3 = points(n, 2)
    r["as_built_thirds"] = round(cost(x, p3, [int(x[i]) for i in p3]) + 2, 1)  # (the two inner kept as themselves: a byte each)
    flat = np.bincount(x, minlength=256); nz = flat[flat > 0]
    r["no_rope_order0"] = round(float((nz * np.log2(n / nz)).sum()) / 8 + 1 + len(nz) * 4 + 3 + 8, 1)
    # One wheel: the third free, 0..255 (two thirds as the file's own).
    vals = [int(x[i]) for i in p3]
    one = []
    for v in range(256):
        vv = list(vals); vv[1] = v
        one.append(cost(x, p3, vv))
    r["one_wheel"] = {"best": round(min(one), 1), "at": int(np.argmin(one)), "file_byte": vals[1], "at_file_byte": round(one[vals[1]], 1), **smoothness(one)}
    np.savetxt(out / f"{name}-one.tsv", np.array(one), fmt="%.2f")
    # Two wheels: the third and two thirds, on a grid of 64 by 64 (every 4th value).
    grid = np.zeros((64, 64))
    for a in range(64):
        for b in range(64):
            vv = list(vals); vv[1] = a * 4; vv[2] = b * 4
            grid[a, b] = cost(x, p3, vv)
    np.savetxt(out / f"{name}-two.tsv", grid, fmt="%.1f")
    ga, gb = np.unravel_index(np.argmin(grid), grid.shape)
    gmins = int(sum(1 for a in range(1, 63) for b in range(1, 63) if grid[a, b] < grid[a - 1:a + 2, b - 1:b + 2].flatten()[[0, 1, 2, 3, 5, 6, 7, 8]].min()))
    r["two_wheels"] = {"best": round(float(grid.min()), 1), "at": [int(ga) * 4, int(gb) * 4], "local_minima": gmins}
    # Steering: k wheels, turned one at a time by +-1 (then +-8 first, coarse to fine) while it helps,
    # from the file's own bytes and from 128 everywhere; against as many random settings.
    for k in (4, 8, 16):
        pk = points(n, k)
        def steer(start):
            v = list(start); best = cost(x, pk, v); evals = 1
            for step in (32, 8, 1):
                moved = True
                while moved:
                    moved = False
                    for w in range(1, k + 1):
                        for s in (step, -step):
                            t = list(v); t[w] = min(255, max(0, t[w] + s)); evals += 1
                            c = cost(x, pk, t)
                            if c < best - 1e-9:
                                best, v, moved = c, t, True
                                break
            return best, v, evals
        own = [int(x[i]) for i in pk]
        b1, v1, e1 = steer(own)
        b2, v2, e2 = steer([own[0]] + [128] * k + [own[-1]])
        rng = np.random.default_rng(k)
        rand = min(cost(x, pk, [own[0]] + list(rng.integers(0, 256, k)) + [own[-1]]) for _ in range(min(e1, 2000)))
        r[f"steer_{k}"] = {"start_own": round(cost(x, pk, own), 1), "steered_from_own": round(b1, 1), "steered_from_128": round(b2, 1),
                           "same_place": sum(abs(a - b) <= 2 for a, b in zip(v1[1:-1], v2[1:-1])), "of": k,
                           "evaluations": e1, "random_best_same_count": round(rand, 1)}
    report[name] = r
    print(name, json.dumps(r), flush=True)
(out / "report.json").write_text(json.dumps(report, indent=1))
