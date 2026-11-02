#!/usr/bin/env python3
r"""The shortcut seed, stage 1 (Edward, 1 November 2026; IDEAS §19.111): a small language of
shortcuts to a file, a search for the shortest recipe in it, and every recipe measured.

"Every filter/method is a shortcut": a recipe is the file written as a run of steps, each a
literal byte or a shortcut, after one transform of the whole file (or none). The search finds the
recipe of fewest bits, and the recipe is decoded again and checked against the file byte for byte
before its size is kept. Any file is in scope; what comes back is exactly the file that went in.

The shortcuts (each named by a tag, then its numbers):
  copy    an earlier stretch: its distance back and its length (a distance shorter than the length
          repeats, so this is also "repeat the last p bytes n times")
  again   the same distance as the last copy, a length only
  run     bytes counting from a start by a step (mod 256), a length: a run of one value is step 0
  grid    with the file seen as rows of W bytes (W said once): copy the byte above, above-left or
          above-right, for a length
The transforms (the recipe is then of what the transform leaves, and the installer undoes it):
  none; diff S (each byte less the one S before: S of 1, 2, 3 or 4); and with rows of W, the
  rope's waves: left, up, average of the two, and the triangle (the median edge detector).

What it costs, in bits: the file's length and the transform as fixed codes; every number as an
Elias gamma code; the literal bytes, the literal-or-shortcut choices and the tags each by an
adaptive count (what an adaptive arithmetic coder pays, by the Krichevsky-Trofimov estimate), so
no table is carried. The search: candidates found at each place (the longest earlier matches by a
hash of three bytes, the run and the grid copies that hold), the cheapest path through them by
dynamic programming, then the costs of literals and tags learned from that path and the path found
again (three rounds).

  python tools/shortcut_seed.py SIEVE FILE... [--cut BYTES] [--out FOLDER] [--rope-seconds S]

Each file (its first --cut bytes, 32 KiB by default, so every measurement is of the same kind) is
measured beside zlib -9 and xz -9e (raw LZMA2, no container), the rope's search (`sieve rope`) and
mixing (`sieve mix`), all on the same bytes. Writes results.md and results.tsv into FOLDER.
"""

import argparse
import lzma
import math
import os
import re
import subprocess
import sys
import tempfile
import time
import zlib
from pathlib import Path

import numpy as np

KINDS = ("copy", "again", "run", "grid", "skiprun")
GRID_DIRS = ((0, "above"), (-1, "above-left"), (1, "above-right"))
TRANSFORMS = ["none", "diff1", "diff2", "diff3", "diff4", "left", "up", "average", "triangle", "columns", "palette"]
COLUMNS = TRANSFORMS + ["stored"]


def gamma(n):
    """Bits of the Elias gamma code of n >= 1."""
    return 2 * (n.bit_length() - 1) + 1


def kt_bits(counts, alphabet):
    """Bits an adaptive coder pays for a sequence with these counts (the KT estimate)."""
    n = sum(counts)
    if n == 0:
        return 0.0
    lg = math.lgamma
    total = lg(n + alphabet / 2) - lg(alphabet / 2)
    for c in counts:
        if c:
            total -= lg(c + 0.5) - lg(0.5)
    return total / math.log(2)


# ---- transforms -----------------------------------------------------------------------------

def med(a, b, c):
    if c >= max(a, b):
        return min(a, b)
    if c <= min(a, b):
        return max(a, b)
    return a + b - c


def predict(t, x, i, w):
    """The transform's prediction of byte i from the bytes before it (the installer has them)."""
    if t.startswith("diff"):
        s = int(t[4:])
        return x[i - s] if i >= s else 0
    col = i % w
    left = x[i - 1] if col >= 1 else None
    up = x[i - w] if i >= w else None
    if t == "left":
        return left if left is not None else (up or 0)
    if t == "up":
        return up if up is not None else (left or 0)
    if left is None or up is None:
        return left if left is not None else (up if up is not None else 0)
    if t == "average":
        return (left + up) // 2
    return med(left, up, x[i - w - 1])  # triangle


def columns(x, w, back=False):
    """The file read a column at a time (rows of w; the bytes past the last whole row as they are),
    or that undone."""
    x = bytes(x)
    rows = len(x) // w if w else 0
    if rows < 2:
        return x
    body = np.frombuffer(x[: rows * w], dtype=np.uint8)
    body = body.reshape(w, rows).T if back else body.reshape(rows, w).T
    return body.tobytes() + x[rows * w:]


def forward(t, x, w):
    if t == "none":
        return bytes(x)
    if t == "columns":
        return columns(x, w)
    return bytes((x[i] - predict(t, x, i, w)) & 255 for i in range(len(x)))


def backward(t, y, w):
    if t == "none":
        return bytes(y)
    if t == "columns":
        return columns(y, w, back=True)
    x = bytearray(len(y))
    for i in range(len(y)):
        x[i] = (y[i] + predict(t, x, i, w)) & 255
    return bytes(x)


# ---- the search -------------------------------------------------------------------------------

def same_from(ya, d):
    """For each place i, how many bytes from i on equal those d before (0 where i < d)."""
    n = len(ya)
    m = np.zeros(n + 1, dtype=np.int64)
    if d >= n:
        return m
    eq = ya[d:] == ya[:-d]                  # eq[k]: byte d + k equals byte k
    breaks = np.flatnonzero(~eq)             # where a run of equal bytes stops
    k = np.arange(len(eq))
    nxt = np.searchsorted(breaks, k)         # the first break at or after k
    breaks = np.append(breaks, len(eq))      # (and the end, past which nothing is equal)
    stop = breaks[nxt]
    m[d:n] = stop - k
    return m


def candidates(y, w):
    """For each place, the shortcuts that hold there: (kind, numbers, length)."""
    n = len(y)
    cand = [[] for _ in range(n)]
    # Runs counting by a step: the length from each place, one backward pass.
    steps = [(y[i + 1] - y[i]) & 255 for i in range(n - 1)]
    run = [0] * (n + 1)
    for i in range(n - 2, -1, -1):
        run[i] = run[i + 1] + 1 if i + 1 < n - 1 and steps[i] == steps[i + 1] else 1
    for i in range(n - 3):
        if run[i] + 1 >= 4:
            cand[i].append(("run", (y[i], steps[i]), run[i] + 1))
    # Edward's skip-and-insert (1 November 2026: "L3 W1 S1 0F"): bytes counting from a start by a
    # step, every e of them followed by one fixed byte, the count going on after it; e of 2 to 15.
    for i in range(n - 5):
        v, stp = y[i], (y[i + 1] - y[i]) & 255
        for e in range(2, 16):
            if i + e >= n:
                break
            if any(((y[i + k] - v - k * stp) & 255) for k in range(e)):
                break
            X = y[i + e]
            m, k = 0, 0
            while i + m < n:
                pos = m % (e + 1)
                want = X if pos == e else (v + (k * stp)) & 255
                if y[i + m] != want:
                    break
                if pos != e:
                    k += 1
                m += 1
            if m >= 2 * (e + 1):
                cand[i].append(("skiprun", (v, stp, e, X), m))
    # The grid's copies: the byte above, above-left or above-right, kept within the row.
    if w:
        for dx, _ in GRID_DIRS:
            g = [0] * (n + 1)
            for i in range(n - 1, w, -1):
                col = i % w + dx
                g[i] = g[i + 1] + 1 if 0 <= col < w and y[i] == y[i - w + dx] else 0
            for i in range(w + 1, n):
                if g[i] >= 2:
                    cand[i].append(("grid", dx, g[i]))
    # Copies: at each place the longest earlier matches by a hash of three bytes (the 48 latest
    # places with the same three). A match's length is carried on from the place before where the
    # same distance matched there (one shorter), else measured by comparing the two stretches a block
    # at a time.
    heads = {}
    ya = np.frombuffer(bytes(y), dtype=np.uint8)
    carried = {}  # distance -> its match's length at this place, known from the place before

    def measure(i, j):
        m, blk = 0, 64
        while i + m < n:
            k = min(blk, n - i - m)
            diff = np.flatnonzero(ya[j + m:j + m + k] != ya[i + m:i + m + k])
            if len(diff):
                return m + int(diff[0])
            m += k
            blk = min(blk * 2, 1 << 14)
        return m

    for i in range(n):
        if i + 3 > n:
            break
        key = y[i:i + 3]
        chain = heads.get(key)
        now = {}
        if chain:
            found = []
            for j in reversed(chain[-48:]):
                d = i - j
                m = carried.get(d) or measure(i, j)
                now[d] = m
                if m >= 3:
                    found.append((m, -d))
            found.sort(reverse=True)
            for m, nd in found[:4]:
                cand[i].append(("copy", -nd, m))
        # (each match is one shorter at the next place, by the same distance, whether looked at here
        # or carried from before)
        carried.update(now)
        carried = {d: m - 1 for d, m in carried.items() if m >= 2}
        heads.setdefault(key, []).append(i)
    return cand


def lengths_to_try(m, low):
    """Lengths of a shortcut that holds for m bytes: all short ones, then the longest."""
    out = list(range(low, min(m, low + 6) + 1))
    if m not in out:
        out.append(m)
    return out


NUM_CATS = ("distance", "copy length", "again length", "run length", "grid length", "skiprun length")
BUCKETS = 40


def step_numbers(kind, num, L):
    """A shortcut's numbers: (category, value >= 1) each, and its bits written as they are."""
    if kind == "copy":
        return [("distance", num), ("copy length", L - 2)], 0
    if kind == "again":
        return [("again length", L - 1)], 0
    if kind == "run":
        return [("run length", L - 3)], 16  # (its start and step, a byte each)
    if kind == "skiprun":
        return [("skiprun length", L - 3)], 28  # (its start, step and inserted byte, a byte each; how often, 4 bits)
    return [("grid length", L - 1)], 2      # (its direction)


def number_bits(bc, cat, v):
    """A number as its size (how many bits it has, by an adaptive count) and then its bits below
    the top one, as they are (as LZMA writes its distances)."""
    bl = v.bit_length()
    return bc[cat][bl] + (bl - 1)


def gamma_costs():
    """The size costs before anything is learned: the Elias gamma code's."""
    return {c: [float(bl) for bl in range(BUCKETS)] for c in NUM_CATS}


def parse(y, w, cand, lit_cost, flag_cost, tag_cost, bc):
    """The cheapest path through the candidates under these costs: a list of steps."""
    n = len(y)
    INF = float("inf")
    best = [INF] * (n + 1)
    step = [None] * (n + 1)
    last_d = [0] * (n + 1)
    best[0] = 0.0
    for i in range(n):
        if best[i] == INF:
            continue
        b = best[i]
        c = b + flag_cost[0] + lit_cost[y[i - 1] if i else 0][y[i]]
        if c < best[i + 1]:
            best[i + 1], step[i + 1], last_d[i + 1] = c, ("lit", y[i], 1, i, y[i - 1] if i else 0), last_d[i]
        for kind, num, m in cand[i]:
            if kind == "copy":
                d = num
                again = d == last_d[i]
                for L in lengths_to_try(m, 3 if not again else 2):
                    if again:
                        c = b + flag_cost[1] + tag_cost["again"] + number_bits(bc, "again length", L - 1)
                        k = "again"
                    else:
                        c = b + flag_cost[1] + tag_cost["copy"] + number_bits(bc, "distance", d) + number_bits(bc, "copy length", L - 2)
                        k = "copy"
                    if c < best[i + L]:
                        best[i + L], step[i + L], last_d[i + L] = c, (k, d, L, i), d
            elif kind == "run":
                for L in lengths_to_try(m, 4):
                    c = b + flag_cost[1] + tag_cost["run"] + 16 + number_bits(bc, "run length", L - 3)
                    if c < best[i + L]:
                        best[i + L], step[i + L], last_d[i + L] = c, ("run", num, L, i), last_d[i]
            elif kind == "skiprun":
                for L in lengths_to_try(m, 4):
                    c = b + flag_cost[1] + tag_cost["skiprun"] + 28 + number_bits(bc, "skiprun length", L - 3)
                    if c < best[i + L]:
                        best[i + L], step[i + L], last_d[i + L] = c, ("skiprun", num, L, i), last_d[i]
            else:
                for L in lengths_to_try(m, 2):
                    c = b + flag_cost[1] + tag_cost["grid"] + 2 + number_bits(bc, "grid length", L - 1)
                    if c < best[i + L]:
                        best[i + L], step[i + L], last_d[i + L] = c, ("grid", num, L, i), last_d[i]
        # (a copy found at i that holds from i + 1 on is also tried from there: its own candidate)
    path = []
    at = n
    while at > 0:
        s = step[at]
        path.append(s)
        at = s[3]
    path.reverse()
    return path


def cost_of(path, n, t, w, alphabet=256):
    """The recipe's bits, as it would be written: header, numbers, and the adaptive counts."""
    lits = [0] * 256
    lits1 = {}
    flags = [0, 0]
    tags = {k: 0 for k in KINDS}
    numbers = 0.0
    sizes = {c: [0] * BUCKETS for c in NUM_CATS}
    for s in path:
        kind, num, L = s[0], s[1], s[2]
        if kind == "lit":
            lits[num] += 1
            row = lits1.setdefault(s[4], [0] * 256)
            row[num] += 1
            flags[0] += 1
            continue
        flags[1] += 1
        tags[kind] += 1
        nums, raw = step_numbers(kind, num, L)
        numbers += raw
        for cat, v in nums:
            sizes[cat][v.bit_length()] += 1
            numbers += v.bit_length() - 1
    header = gamma(n + 1) + 4 + (gamma(w + 1) if w else 1)
    if alphabet < 256:
        header += min(256, 8 + 8 * alphabet)  # (the values the file uses: a mask of 256 bits, or how many and each)
    # The literal bytes counted alone (order 0) or by the byte before each (order 1), whichever is
    # smaller, and a bit to say which.
    o0 = kt_bits(lits, alphabet)
    o1 = sum(kt_bits(r, alphabet) for r in lits1.values())
    numbers += sum(kt_bits(v, BUCKETS) for v in sizes.values())
    body = 1 + min(o0, o1) + kt_bits(flags, 2) + kt_bits(list(tags.values()), len(KINDS)) + numbers
    return header + body, {"literals": flags[0], "shortcuts": flags[1], "order": 0 if o0 <= o1 else 1, "sizes": sizes, **tags}


def decode(path, n, w):
    y = bytearray()
    for s in path:
        kind, num, L = s[0], s[1], s[2]
        if kind == "lit":
            y.append(num)
        elif kind in ("copy", "again"):
            for _ in range(L):
                y.append(y[-num])
        elif kind == "run":
            v, stp = num
            for k in range(L):
                y.append((v + k * stp) & 255)
        elif kind == "skiprun":
            v, stp, e, X = num
            k = 0
            for m in range(L):
                if m % (e + 1) == e:
                    y.append(X)
                else:
                    y.append((v + k * stp) & 255)
                    k += 1
        else:
            for _ in range(L):
                y.append(y[len(y) - w + num])
    assert len(y) == n
    return bytes(y)


def search(x, w):
    """Every transform, each with the cheapest recipe; the best kept. Returns a list of results."""
    out = []
    for t in TRANSFORMS:
        if t in ("left", "up", "average", "triangle", "columns") and not w:
            continue
        t0 = time.time()
        if t == "palette":
            # The values the file uses, said once; its bytes then their places in that list.
            values = sorted(set(x))
            if len(values) >= 256 or not values:
                continue
            place = {v: k for k, v in enumerate(values)}
            y = bytes(place[v] for v in x)
            alphabet = len(values)
        else:
            y = forward(t, x, w)
            alphabet = 256
        cand = candidates(y, w)
        lit_cost, flag_cost, tag_cost, bc = [[8.0] * 256] * 256, [1.0, 1.0], {k: 2.0 for k in KINDS}, gamma_costs()
        best = None
        for _ in range(3):
            path = parse(y, w, cand, lit_cost, flag_cost, tag_cost, bc)
            bits, counts = cost_of(path, len(y), t, w, alphabet)
            if best is None or bits < best[0]:
                best = (bits, counts, path)
            if counts["order"] == 0:
                lits = [1] * 256
                for s in path:
                    if s[0] == "lit":
                        lits[s[1]] += 1
                tot = sum(lits)
                lit_cost = [[-math.log2(c / tot) for c in lits]] * 256
            else:
                rows = [[0.5] * 256 for _ in range(256)]
                for s in path:
                    if s[0] == "lit":
                        rows[s[4]][s[1]] += 1
                lit_cost = []
                for r in rows:
                    tot = sum(r)
                    lit_cost.append([-math.log2(c / tot) for c in r])
            nf = [counts["literals"] + 1, counts["shortcuts"] + 1]
            flag_cost = [-math.log2(c / sum(nf)) for c in nf]
            nt = {k: counts[k] + 1 for k in KINDS}
            tag_cost = {k: -math.log2(nt[k] / sum(nt.values())) for k in KINDS}
            bc = {}
            for cat, v in counts["sizes"].items():
                tot = sum(v) + BUCKETS * 0.5
                bc[cat] = [-math.log2((c + 0.5) / tot) for c in v]
        bits, counts, path = best
        back = bytes(values[v] for v in decode(path, len(y), w)) if t == "palette" else backward(t, decode(path, len(y), w), w)
        assert back == bytes(x), f"{t}: the recipe does not give the file back"
        out.append({"transform": t, "bytes": math.ceil(bits / 8), "bits": bits, "counts": counts, "seconds": time.time() - t0})
    # Carried as it is: its length, the transform's code, and its bytes (the floor every file has).
    zero = {"literals": len(x), "shortcuts": 0, "order": 0, **{k: 0 for k in KINDS}}
    out.append({"transform": "stored", "bytes": math.ceil((gamma(len(x) + 1) + 4) / 8) + len(x), "bits": gamma(len(x) + 1) + 4 + 8 * len(x), "counts": zero, "seconds": 0.0})
    return out


# ---- the others, on the same bytes --------------------------------------------------------------

def guess_width(x):
    """The row width at which the bytes best match themselves a row later (as the rope's search
    finds its widths), from 2 to 4,096; 0 where none stands out."""
    n = len(x)
    if n < 64:
        return 0
    best, bw = None, 0
    base = sum(1 for i in range(1, n) if x[i] == x[i - 1]) / (n - 1)
    for w in list(range(2, 65)) + [96, 128, 192, 256, 320, 384, 512, 640, 768, 1024, 1536, 2048, 3072, 4096]:
        if w >= n // 2:
            break
        # (the byte above, above-left or above-right: a file that moves along a row is a grid too)
        same = max(sum(1 for i in range(w + 1, n - 1, 7) if x[i] == x[i - w + dx]) for dx in (-1, 0, 1)) / len(range(w + 1, n - 1, 7))
        if best is None or same > best:
            best, bw = same, w
    return bw if best > base + 0.02 else 0


def xz_of(x):
    """xz -9e as the File Locator gives it: raw LZMA2, no container."""
    return lzma.compress(bytes(x), format=lzma.FORMAT_RAW, filters=[{"id": lzma.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}])


def sieve_bytes(sieve, args, pattern):
    p = subprocess.run([sieve] + args, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    m = re.search(pattern, p.stdout.decode("utf-8", "replace"))
    return int(m.group(1)) if m else None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("sieve")
    ap.add_argument("files", nargs="+")
    ap.add_argument("--cut", type=int, default=32768)
    ap.add_argument("--out", default="shortcut-seed")
    ap.add_argument("--rope-seconds", type=float, default=5)
    ap.add_argument("--width", type=int, default=-1, help="rows of this many bytes (default: guessed)")
    a = ap.parse_args()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    rows = []
    with tempfile.TemporaryDirectory() as tmp:
        for f in a.files:
            x = Path(f).read_bytes()[: a.cut]
            name = Path(f).name
            shape = re.search(r"-(\d+)x(\d+)\.rgb$", name) or re.search(r"-(\d+)\.gray$", name)
            if a.width >= 0:
                w = a.width
            elif shape:  # (a picture's shape from its name: rows of width x 3 bytes, or of width bytes)
                w = int(shape.group(1)) * (3 if name.endswith(".rgb") else 1)
            else:
                w = guess_width(x)
            sample = Path(tmp) / ("s-" + name)
            sample.write_bytes(x)
            zl = len(zlib.compress(x, 9))
            xz_out = xz_of(x)
            xz = len(xz_out)
            rope = sieve_bytes(a.sieve, ["rope", str(sample), "--seconds", str(a.rope_seconds)], r"\ncode\s+(\d+) bytes")
            mix = sieve_bytes(a.sieve, ["mix", str(sample), "--out", str(Path(tmp) / "m.mix")], r"\ncode\s+(\d+) bytes")
            res = search(x, w)
            best = min(res, key=lambda r: r["bytes"])
            # The two prongs. Forward: a shortcut into what beat it (the seed searched on xz's output
            # and on mixing's). Back: a shortcut in front of a compressor (each transform's output
            # through xz and through mixing, a byte added to say which transform).
            seed_xz = min(r["bytes"] for r in search(xz_out, 0))
            mix_file = Path(tmp) / "m.mix"
            seed_mix = min(r["bytes"] for r in search(mix_file.read_bytes(), 0)) if mix is not None and mix_file.exists() else None
            front = []
            for t in TRANSFORMS[1:]:
                if (t in ("left", "up", "average", "triangle", "columns") and not w) or t == "palette":
                    continue
                y = forward(t, x, w)
                ty = Path(tmp) / "t.bin"
                ty.write_bytes(y)
                my = sieve_bytes(a.sieve, ["mix", str(ty), "--out", str(Path(tmp) / "t.mix")], r"\ncode\s+(\d+) bytes")
                front.append((t, len(xz_of(y)) + 1, (my + 1) if my is not None else None))
            front_xz = min(front, key=lambda f: f[1]) if front else None
            front_mix = min((f for f in front if f[2] is not None), key=lambda f: f[2], default=None)
            print(f"{name}: {len(x)} bytes, W {w}: shortcut seed {best['bytes']} ({best['transform']}), xz {xz}, mixing {mix}, rope {rope}, zlib {zl}; "
                  f"seed of xz's output {seed_xz}, of mixing's {seed_mix}; transform then xz {front_xz}, then mixing {front_mix}", flush=True)
            rows.append({"file": name, "n": len(x), "w": w, "zlib": zl, "xz": xz, "rope": rope, "mix": mix, "res": res, "best": best,
                         "seed_xz": seed_xz, "seed_mix": seed_mix, "front": front, "front_xz": front_xz, "front_mix": front_mix})
            write(out, rows, a)


def write(out, rows, a):
    with open(out / "results.tsv", "w", encoding="utf-8") as f:
        f.write("file\tbytes\twidth\ttransform\tshortcut seed\tliterals\tshortcuts\tcopy\tagain\trun\tgrid\tseconds\tzlib\txz\trope\tmixing\n")
        for r in rows:
            for t in r["res"]:
                c = t["counts"]
                f.write("\t".join(str(v) for v in (r["file"], r["n"], r["w"], t["transform"], t["bytes"], c["literals"], c["shortcuts"], c["copy"], c["again"], c["run"], c["grid"],
                                                   f"{t['seconds']:.1f}", r["zlib"], r["xz"], r["rope"], r["mix"])) + "\n")
    with open(out / "results.md", "w", encoding="utf-8") as f:
        f.write(f"# The shortcut seed, stage 1\n\nEach file's first {a.cut:,} bytes (the whole file where smaller). The shortcut seed's best recipe "
                "(its transform, then how many steps were literal bytes and how many shortcuts of each kind), every recipe decoded again and checked "
                "byte for byte; beside it zlib -9, xz -9e (raw LZMA2), the rope's search "
                f"({a.rope_seconds:g} s) and mixing (mix-v4), all on the same bytes. W: the row width guessed for the grid and the waves (0: none).\n\n")
        f.write("| File | Bytes | W | Shortcut seed | Its transform | Literals / shortcuts (copy, again, run, grid) | xz | Mixing | Rope | zlib |\n")
        f.write("| :--- | ---: | ---: | ---: | :--- | :--- | ---: | ---: | ---: | ---: |\n")
        for r in rows:
            b, c = r["best"], r["best"]["counts"]
            f.write(f"| {r['file']} | {r['n']:,} | {r['w']} | {b['bytes']:,} | {b['transform']} | {c['literals']:,} / {c['shortcuts']:,} "
                    f"({c['copy']}, {c['again']}, {c['run']}, {c['grid']}) | {r['xz']:,} | {r['mix'] if r['mix'] is not None else ''} | "
                    f"{r['rope'] if r['rope'] is not None else ''} | {r['zlib']:,} |\n")
        f.write("\n## The two prongs\n\nForward: the shortcut seed searched on what beat it (xz's output, mixing's code): smaller than that "
                "output only if a shortcut into it exists. Back: each transform's output put through xz and through mixing, a byte added to say "
                "which transform (the best transform shown): smaller than xz or mixing alone only if the shortcut in front helps it.\n\n")
        f.write("| File | xz | Seed of xz's output | Mixing | Seed of mixing's code | Transform, then xz | Transform, then mixing |\n")
        f.write("| :--- | ---: | ---: | ---: | ---: | :--- | :--- |\n")
        for r in rows:
            fx, fm = r["front_xz"], r["front_mix"]
            f.write(f"| {r['file']} | {r['xz']:,} | {r['seed_xz']:,} | {r['mix'] if r['mix'] is not None else ''} | {r['seed_mix'] if r['seed_mix'] is not None else ''} | "
                    f"{f'{fx[1]:,} ({fx[0]})' if fx else ''} | {f'{fm[2]:,} ({fm[0]})' if fm else ''} |\n")
        f.write("\n## Every transform\n\n| File | " + " | ".join(COLUMNS) + " |\n| :--- |" + " ---: |" * len(COLUMNS) + "\n")
        for r in rows:
            by = {t["transform"]: t["bytes"] for t in r["res"]}
            f.write(f"| {r['file']} | " + " | ".join(f"{by[t]:,}" if t in by else "" for t in COLUMNS) + " |\n")


if __name__ == "__main__":
    main()
