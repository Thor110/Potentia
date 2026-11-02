#!/usr/bin/env python3
r"""Designed files for the shortcut seed's geometric filters (IDEAS §19.111), and each measured.

Each file is a grid of 64 rows of 64 bytes built by one shape, from a fixed seed, so it is made
again here rather than kept: a count along a spiral, rings of equal values round the centre, a
plane, a slope from the centre, stripes along the diagonals, a snake, a count along the Morton
curve, tiles of one value each. Each is searched by the shortcut seed with every transform, and
measured beside xz -9e; the table says which transform won and what each geometric filter came to.
Then the same for any real files given (each with its grid's width).

  python tools/shortcut_designs.py [--out FOLDER] [--real FILE:WIDTH ...]
"""

import argparse
import math
import random
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import shortcut_seed as seed  # noqa: E402

N = 64


def designs():
    rnd = random.Random(111)
    r, c = np.divmod(np.arange(N * N), N)
    d = {}
    spiral = seed.order_of("spiral", N, N)
    g = np.zeros(N * N, dtype=np.int64)
    g[spiral] = np.arange(N * N) % 256
    d["spiral count"] = g
    d["rings"] = np.round(np.hypot(r - (N - 1) / 2, c - (N - 1) / 2) * 2).astype(np.int64)
    d["plane"] = c + 2 * r
    d["slope from the centre"] = np.round(3 * np.hypot(r - (N - 1) / 2, c - (N - 1) / 2)).astype(np.int64)
    d["diagonal stripes"] = ((r + c) % 7) * 30
    snake = np.where(r % 2 == 1, N - 1 - c, c) + r * N
    d["snake count"] = snake % 256
    morton = seed.order_of("morton", N, N)
    g = np.zeros(N * N, dtype=np.int64)
    g[morton] = np.arange(N * N) % 256
    d["Morton count"] = g
    tiles = np.array([rnd.getrandbits(8) for _ in range((N // 8) ** 2)])
    d["tiles of one value"] = tiles[(r // 8) * (N // 8) + c // 8]
    return {k: bytes((v & 255).astype(np.uint8)) for k, v in d.items()}


def measure(name, x, w):
    res = seed.search(x, w)
    best = min(res, key=lambda q: q["bits"])
    by = {q["transform"]: q["bytes"] for q in res}
    return {"name": name, "n": len(x), "w": w, "xz": len(seed.xz_of(x)), "best": best, "by": by}


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--out", default="shortcut-designs")
    ap.add_argument("--real", nargs="*", default=[], help="FILE:WIDTH, the file's first 32 KiB")
    a = ap.parse_args()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    for name, x in designs().items():
        rows.append(measure(name, x, N))
        print(name, rows[-1]["best"]["transform"], rows[-1]["best"]["bytes"], "xz", rows[-1]["xz"], flush=True)
    for spec in a.real:
        f, w = spec.rsplit(":", 1)
        x = Path(f).read_bytes()[:32768]
        rows.append(measure(Path(f).name, x, int(w)))
        print(Path(f).name, rows[-1]["best"]["transform"], rows[-1]["best"]["bytes"], "xz", rows[-1]["xz"], flush=True)
    cols = ["none"] + seed.GEOMETRIC
    with open(out / "results.md", "w", encoding="utf-8") as fh:
        fh.write("# The geometric filters\n\nEach designed file is 64 rows of 64 bytes built by one shape; each real file its first 32 KiB in rows of the width given. "
                 "The best is over every transform; the columns are what no transform and each geometric filter came to (bytes, its numbers paid).\n\n")
        fh.write("| File | Bytes | W | Best | Its transform | xz | " + " | ".join(cols) + " |\n")
        fh.write("| :--- | ---: | ---: | ---: | :--- | ---: |" + " ---: |" * len(cols) + "\n")
        for r_ in rows:
            fh.write(f"| {r_['name']} | {r_['n']:,} | {r_['w']} | {r_['best']['bytes']:,} | {r_['best']['transform']} | {r_['xz']:,} | "
                     + " | ".join(f"{r_['by'][t]:,}" if t in r_['by'] else "" for t in cols) + " |\n")


if __name__ == "__main__":
    main()
