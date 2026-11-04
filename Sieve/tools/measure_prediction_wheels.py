#!/usr/bin/env python3
"""IDEAS 19.117, the second car: wheels that touch every value's prediction.

Each value is predicted from its neighbours by weights (wheels): p = (w1*n1 + w2*n2 + w3*n3) / 32
+ bias, held within 0..255; for a picture the neighbours are the value to its left, above it and
above-left; for a file of one dimension, the three values before it. Each weight is a byte (-128 to
127, in 32nds: -4 to +4), the bias a byte. Then a bias for each region of 4,096 values (64 more
wheels on 256 KiB). What the prediction leaves (value - p, mod 256) costs its order-0 entropy plus a
table, as tools/measure_rope_wheels.py counts it, plus a byte a wheel. The rope's own fixed
predictions (the value to the left, the gradient left + above - above-left, the triangle) are
counted the same way, so all are compared on one footing.

Steering: each wheel in turn moved by 32, 8, then 1, kept where it helps, until nothing does.

usage: python tools/measure_prediction_wheels.py OUT_DIR NAME=FILE[@OFFSET][:STEP,WIDTH] ...
  STEP,WIDTH: a picture's bytes a pixel and bytes a row (none: a file of one dimension).
Measured with it: results/rope-wheels-2026-11-03.md (its second part).
"""
import json, sys
from pathlib import Path
import numpy as np

PIECE = 256 << 10
REGION = 4096
out = Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)


def load(spec):
    name, f = spec.split("=", 1)
    shape = None
    if ":" in f:
        f, sh = f.rsplit(":", 1)
        shape = tuple(int(v) for v in sh.split(","))
    at = 0
    if "@" in f:
        f, at = f.rsplit("@", 1)
        at = int(at)
    b = Path(f).read_bytes()[at:at + PIECE]
    return name, np.frombuffer(b, dtype=np.uint8).astype(np.int64), shape


def neighbours(x, shape):
    """Three neighbour arrays (0 where there is none)."""
    n = len(x)
    if shape:
        s, w = shape
        offs = [s, w, w + s]
    else:
        offs = [1, 2, 3]
    out = []
    for o in offs:
        a = np.zeros(n, dtype=np.int64)
        a[o:] = x[:-o]
        out.append(a)
    return out


def entropy_cost(e):
    n = len(e)
    c = np.bincount(e, minlength=256)
    nz = c[c > 0]
    return float((nz * np.log2(n / nz)).sum()) / 8 + 1 + len(nz) * 4 + 3 + 8


def cost(x, nb, wheels, regions=None):
    w1, w2, w3, bias = wheels
    p = (w1 * nb[0] + w2 * nb[1] + w3 * nb[2]) // 32 + bias
    nwheels = 4
    if regions is not None:
        p = p + np.repeat(np.asarray(regions, dtype=np.int64), REGION)[: len(x)]
        nwheels += len(regions)
    p = np.clip(p, 0, 255)
    return entropy_cost((x - p) & 255) + nwheels


def steer(f, start, lo, hi):
    v = list(start); best = f(v); evals = 1
    for step in (32, 8, 1):
        moved = True
        while moved:
            moved = False
            for k in range(len(v)):
                for s in (step, -step):
                    t = list(v); t[k] = min(hi[k], max(lo[k], t[k] + s)); evals += 1
                    c = f(t)
                    if c < best - 1e-9:
                        best, v, moved = c, t, True
                        break
    return best, v, evals


def smooth(curve):
    c = np.asarray(curve, dtype=float); d = np.diff(c)
    minima = int(((c[1:-1] < c[:-2]) & (c[1:-1] < c[2:])).sum())
    corr = float(np.corrcoef(d[:-1], d[1:])[0, 1]) if d.std() > 0 else 1.0
    return minima, round(corr, 3)


report = {}
for spec in sys.argv[2:]:
    name, x, shape = load(spec)
    nb = neighbours(x, shape)
    r = {"bytes": len(x), "shape": shape}
    r["plain"] = round(entropy_cost(x), 1)
    # The rope's fixed predictions, at the same cost (no wheels paid for).
    left = nb[0]
    grad = np.clip(nb[0] + nb[1] - nb[2], 0, 255)
    mx, mn = np.maximum(nb[0], nb[1]), np.minimum(nb[0], nb[1])
    tri = np.where(nb[2] >= mx, mn, np.where(nb[2] <= mn, mx, nb[0] + nb[1] - nb[2]))
    r["fixed_left"] = round(entropy_cost((x - left) & 255), 1)
    r["fixed_gradient"] = round(entropy_cost((x - grad) & 255), 1)
    r["fixed_triangle"] = round(entropy_cost((x - tri) & 255), 1)
    lo, hi = [-128, -128, -128, -128], [127, 127, 127, 127]
    f = lambda v: cost(x, nb, v)
    # One wheel: the first weight through all its values, the others as the gradient's (32, 32, -32).
    one = [cost(x, nb, [w, 32, -32, 0]) for w in range(-128, 128)]
    np.savetxt(out / f"{name}-weight1.tsv", np.array(one), fmt="%.1f")
    m, c = smooth(one)
    r["one_weight"] = {"best": round(min(one), 1), "at": int(np.argmin(one)) - 128, "local_minima": m, "step_corr": c}
    # Steering the four from nothing (all 0), from the value to the left (32, 0, 0), from the gradient.
    starts = {"nothing": [0, 0, 0, 0], "left": [32, 0, 0, 0], "gradient": [32, 32, -32, 0]}
    best_overall = None
    for sname, st in starts.items():
        b, v, e = steer(f, st, lo, hi)
        r[f"steer_from_{sname}"] = {"bytes": round(b, 1), "wheels": v, "evaluations": e}
        if best_overall is None or b < best_overall[0]:
            best_overall = (b, v, e)
    rng = np.random.default_rng(7)
    e = best_overall[2]
    r["random_same_count"] = round(min(f(list(rng.integers(-128, 128, 4))) for _ in range(min(e, 3000))), 1)
    # Then a bias for each region of 4,096 values, steered with the best weights held.
    nreg = (len(x) + REGION - 1) // REGION
    wv = best_overall[1]
    g = lambda reg: cost(x, nb, wv, reg)
    b2, reg, e2 = steer(g, [0] * nreg, [-128] * nreg, [127] * nreg)
    r["with_region_bias"] = {"bytes": round(b2, 1), "regions": nreg, "evaluations": e2, "nonzero": sum(1 for q in reg if q)}
    report[name] = r
    print(name, json.dumps(r), flush=True)
(out / "report.json").write_text(json.dumps(report, indent=1))
