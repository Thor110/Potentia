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

KINDS = ("copy", "again", "run", "grid", "skiprun", "again2")

# How the recipe's steps are coded (2 November 2026, each measured on its own; IDEAS §19.111):
#   flag_ctx    the choice of literal or shortcut counted by the step before it
#   rep2        a second recent distance to copy from again ("again2")
#   lit_model   "o01": literal bytes counted alone or by the byte before (the first coder);
#               "blend": by the two bytes before, falling back to one and to none (PPM's way)
#   lit_all     with "blend", the counts learn from every byte the installer has made, not only
#               the literal ones (it has them all)
#   matched     a literal just after a copy counted by the byte the copy would have given next
#               (LZMA's matched literal), falling back to the literal's own counts: True, as whole
#               bytes; "bits", bit by bit as LZMA does (while the literal's bits are the copy's
#               byte's, each bit counted by its place and the copy's bit; from the first that
#               differs, the literal's own counts); "bits+own", the same with each bit while they
#               agree counted by both, the copy's counts and the literal's own, together
CODER = {"flag_ctx": True, "rep2": True, "lit_model": "blend", "lit_all": False, "beta": 2.0, "matched": "bits"}
GRID_DIRS = ((0, "above"), (-1, "above-left"), (1, "above-right"))
GEOMETRIC = ["zigzag", "diagonals", "spiral", "rings", "morton", "tiles", "plane", "radial"]
TRANSFORMS = ["none", "diff1", "diff2", "diff3", "diff4", "left", "up", "average", "triangle", "columns", "palette"] + GEOMETRIC
GRID_ONLY = ("left", "up", "average", "triangle", "columns") + tuple(GEOMETRIC)
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


# ---- geometric filters (Edward, 1 November 2026: "circles, lines, squares, grids, geometric
# principles used to shuffle up a file"; IDEAS §19.111). The file's whole rows as a grid of w
# columns; the bytes past the last whole row as they are.

def order_of(t, rows, w):
    """The grid's places in the order a geometric filter reads them (a permutation of rows * w)."""
    r, c = np.divmod(np.arange(rows * w), w)
    if t == "zigzag":  # every other row backwards
        return np.where(r % 2 == 1, r * w + (w - 1 - c), r * w + c)
    if t == "diagonals":  # by r + c, then by row
        return np.lexsort((r, r + c))
    if t == "rings":  # by distance from the centre, then by angle
        dy, dx = r - (rows - 1) / 2, c - (w - 1) / 2
        return np.lexsort((np.arctan2(dy, dx), np.round(np.hypot(dy, dx) * 2)))
    if t == "morton":  # the Z-order curve: the bits of row and column interleaved
        key = np.zeros(rows * w, dtype=np.int64)
        for bit in range(16):
            key |= ((c >> bit) & 1) << (2 * bit)
            key |= ((r >> bit) & 1) << (2 * bit + 1)
        return np.argsort(key, kind="stable")
    if t == "tiles":  # 8 by 8 tiles, each read row by row, the tiles in rows
        return np.lexsort((c % 8, r % 8, c // 8, r // 8))
    if t == "spiral":  # from the outside in, clockwise from the top left
        out, top, bottom, left, right = [], 0, rows - 1, 0, w - 1
        while top <= bottom and left <= right:
            out += [top * w + k for k in range(left, right + 1)]
            out += [k * w + right for k in range(top + 1, bottom + 1)]
            if top < bottom:
                out += [bottom * w + k for k in range(right - 1, left - 1, -1)]
            if left < right:
                out += [k * w + left for k in range(bottom - 1, top, -1)]
            top, bottom, left, right = top + 1, bottom - 1, left + 1, right - 1
        return np.array(out, dtype=np.int64)
    raise ValueError(t)


def shape_of(t, x, rows, w):
    """A shape laid over the grid, fitted to it: its numbers (as written) and its values."""
    v = np.frombuffer(bytes(x[: rows * w]), dtype=np.uint8).astype(np.float64)
    r, c = np.divmod(np.arange(rows * w), w)
    if t == "plane":
        cols = np.stack([np.ones_like(v), c, r], axis=1)
    else:  # radial: a + b * distance from the centre
        cols = np.stack([np.ones_like(v), np.hypot(r - (rows - 1) / 2, c - (w - 1) / 2)], axis=1)
    coef = np.linalg.lstsq(cols, v, rcond=None)[0]
    q = np.round(coef * 256).astype(np.int64)  # (each a 16-bit number of 256ths, as the recipe says it)
    q = np.clip(q, -32768, 32767)
    return q, np.floor((cols @ q) / 256).astype(np.int64)


def geometric(t, x, w, back=False, numbers=None):
    x = bytes(x)
    rows = len(x) // w if w else 0
    if rows < 2:
        return x, None
    body = np.frombuffer(x[: rows * w], dtype=np.uint8)
    if t in ("plane", "radial"):
        if back:
            q = numbers
            r, c = np.divmod(np.arange(rows * w), w)
            cols = np.stack([np.ones(rows * w), c, r], axis=1) if t == "plane" else np.stack([np.ones(rows * w), np.hypot(r - (rows - 1) / 2, c - (w - 1) / 2)], axis=1)
            shape = np.floor((cols @ q) / 256).astype(np.int64)
            return ((body.astype(np.int64) + shape) & 255).astype(np.uint8).tobytes() + x[rows * w:], q
        q, shape = shape_of(t, x, rows, w)
        return ((body.astype(np.int64) - shape) & 255).astype(np.uint8).tobytes() + x[rows * w:], q
    perm = order_of(t, rows, w)
    if back:
        out = np.empty_like(body)
        out[perm] = body
        return out.tobytes() + x[rows * w:], None
    return body[perm].tobytes() + x[rows * w:], None


def geometric_bits(t):
    """What a geometric filter's own numbers cost in the recipe."""
    return {"plane": 48, "radial": 32}.get(t, 0)


def forward(t, x, w):
    if t == "none":
        return bytes(x)
    if t == "columns":
        return columns(x, w)
    if t in GEOMETRIC:
        return geometric(t, x, w)[0]
    return bytes((x[i] - predict(t, x, i, w)) & 255 for i in range(len(x)))


def backward(t, y, w, numbers=None):
    if t == "none":
        return bytes(y)
    if t == "columns":
        return columns(y, w, back=True)
    if t in GEOMETRIC:
        return geometric(t, y, w, back=True, numbers=numbers)[0]
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
            if X == (v + e * stp) & 255:
                continue  # (the byte put in is the count's own next value: a plain run, which `run` says)
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
        # (each match looked at here is one shorter at the next place, by the same distance; only
        # these are carried, so a repetitive file's thousands of distances are not walked at every
        # place)
        carried = {d: m - 1 for d, m in now.items() if m >= 2}
        heads.setdefault(key, []).append(i)
    return cand


def lengths_to_try(m, low):
    """Lengths of a shortcut that holds for m bytes: all short ones, then the longest."""
    out = list(range(low, min(m, low + 6) + 1))
    if m not in out:
        out.append(m)
    return out


NUM_CATS = ("distance", "copy length", "again length", "run length", "grid length", "skiprun length", "again2 length")
BUCKETS = 40


def step_numbers(kind, num, L):
    """A shortcut's numbers: (category, value >= 1) each, and its bits written as they are."""
    if kind == "copy":
        return [("distance", num), ("copy length", L - 2)], 0
    if kind == "again":
        return [("again length", L - 1)], 0
    if kind == "again2":
        return [("again2 length", L - 1)], 0
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


def parse(y, w, cand, litc, flag_cost, tag_cost, bc, matched=None):
    """The cheapest path through the candidates under these costs: a list of steps. litc[i] is the
    cost of byte i as a literal (its context is the file's own bytes before it, so known here);
    flag_cost[state][0 or 1] the choice's, the state the step before (0 a literal, 1 a shortcut).
    Each place keeps a path for each state, with its two most recent copy distances."""
    n = len(y)
    INF = float("inf")
    best = [[INF, INF, INF] for _ in range(n + 1)]
    back = [[None, None, None] for _ in range(n + 1)]
    dist = [[(0, 0), (0, 0), (0, 0)] for _ in range(n + 1)]
    best[0][2] = 0.0  # (the start counts as after a shortcut; state 1 is after a copy)

    def offer(j, st, cost, stepv, prev_state, d):
        if cost < best[j][st]:
            best[j][st], back[j][st], dist[j][st] = cost, (stepv, prev_state), d

    for i in range(n):
        for ps in (0, 1, 2):
            b = best[i][ps]
            if b == INF:
                continue
            d1, d2 = dist[i][ps]
            fc = flag_cost[ps]
            lc = litc[i]
            if ps == 1 and matched is not None and i >= d1 > 0:
                lc = matched(i, y[i - d1], lc)
            offer(i + 1, 0, b + fc[0] + lc, ("lit", y[i], 1, i, y[i - 1] if i else 0), ps, (d1, d2))
            for kind, num, m in cand[i]:
                if kind == "copy":
                    d = num
                    if d == d1:
                        for L in lengths_to_try(m, 2):
                            offer(i + L, 1, b + fc[1] + tag_cost["again"] + number_bits(bc, "again length", L - 1), ("again", d, L, i), ps, (d1, d2))
                    elif CODER["rep2"] and d == d2:
                        for L in lengths_to_try(m, 2):
                            offer(i + L, 1, b + fc[1] + tag_cost["again2"] + number_bits(bc, "again2 length", L - 1), ("again2", d, L, i), ps, (d2, d1))
                    else:
                        for L in lengths_to_try(m, 3):
                            offer(i + L, 1, b + fc[1] + tag_cost["copy"] + number_bits(bc, "distance", d) + number_bits(bc, "copy length", L - 2), ("copy", d, L, i), ps, (d, d1))
                elif kind == "run":
                    for L in lengths_to_try(m, 4):
                        offer(i + L, 2, b + fc[1] + tag_cost["run"] + 16 + number_bits(bc, "run length", L - 3), ("run", num, L, i), ps, (d1, d2))
                elif kind == "skiprun":
                    for L in lengths_to_try(m, 4):
                        offer(i + L, 2, b + fc[1] + tag_cost["skiprun"] + 28 + number_bits(bc, "skiprun length", L - 3), ("skiprun", num, L, i), ps, (d1, d2))
                else:
                    for L in lengths_to_try(m, 2):
                        offer(i + L, 2, b + fc[1] + tag_cost["grid"] + 2 + number_bits(bc, "grid length", L - 1), ("grid", num, L, i), ps, (d1, d2))
    path = []
    at, st = n, min(range(3), key=lambda k: best[n][k])
    while at > 0:
        stepv, ps = back[at][st]
        path.append(stepv)
        at, st = stepv[3], ps
    path.reverse()
    return path


class Literals:
    """The literal bytes' coder: by the two bytes before, falling back to one and to none, each
    count adaptive (what an arithmetic coder pays, step by step), learning from every byte made
    or from the literals alone."""

    def __init__(self, alphabet, beta, order0=False):
        self.A, self.beta, self.order0 = alphabet, beta, order0
        self.c0, self.n0 = [0] * 256, 0
        self.c1, self.c2 = {}, {}
        self.cm = {}  # by the byte a copy would have given next

    def prob(self, a2, a1, s):
        p = (self.c0[s] + 0.5) / (self.n0 + 0.5 * self.A)
        if self.order0:
            return p
        r = self.c1.get(a1)
        if r:
            p = (r[0].get(s, 0) + self.beta * p) / (r[1] + self.beta)
        r = self.c2.get((a2, a1))
        if r:
            p = (r[0].get(s, 0) + self.beta * p) / (r[1] + self.beta)
        return p

    def prob_matched(self, a2, a1, s, mb):
        p = self.prob(a2, a1, s)
        r = self.cm.get(mb)
        return (r[0].get(s, 0) + self.beta * p) / (r[1] + self.beta) if r else p

    def learn_matched(self, mb, s):
        r = self.cm.setdefault(mb, [{}, 0])
        r[0][s] = r[0].get(s, 0) + 1
        r[1] += 1

    def learn(self, a2, a1, s):
        self.c0[s] += 1
        self.n0 += 1
        for table, key in ((self.c1, a1), (self.c2, (a2, a1))):
            r = table.setdefault(key, [{}, 0])
            r[0][s] = r[0].get(s, 0) + 1
            r[1] += 1


def literal_costs(y, path, alphabet):
    """Each place's cost as a literal, from counts made over the last path's file (two passes:
    what the next round of the parse uses)."""
    lit = Literals(alphabet, CODER["beta"])
    is_lit = bytearray(len(y))
    for st in path:
        if st[0] == "lit":
            is_lit[st[3]] = 1
    for i in range(len(y)):
        if CODER["lit_all"] or is_lit[i]:
            lit.learn(y[i - 2] if i > 1 else 0, y[i - 1] if i else 0, y[i])
    return [-math.log2(lit.prob(y[i - 2] if i > 1 else 0, y[i - 1] if i else 0, y[i])) for i in range(len(y))]


def after_copy(path):
    """For each literal just after a copy, the copy's distance: {place: distance}."""
    out, last = {}, None
    for st in path:
        if st[0] == "lit" and last is not None:
            out[st[3]] = last
        last = st[1] if st[0] in ("copy", "again", "again2") else None
    return out


class MatchedBits:
    """LZMA's matched literal, bit by bit: while the bits so far are the copy's byte's, bit k is
    counted by the bits so far and the copy's bit k (0x300 counts, shared by every byte); from the
    first that differs, the rest by the literal's own counts, the byte's chances among those that
    begin with the bits so far."""

    def __init__(self):
        self.c = {}

    def bits(self, own, s, mb, both):
        """The cost in bits of literal s, the copy's byte mb, the literal's own chances `own` (256)."""
        cost, pre, agree = 0.0, 1, True
        for k in range(7, -1, -1):
            bit, mbit = (s >> k) & 1, (mb >> k) & 1
            # (the literal's own chance of this bit, given the bits so far)
            base = (pre - (1 << (7 - k))) << (k + 1)
            span = 1 << k
            p0 = sum(own[base:base + span])
            p1 = sum(own[base + span:base + 2 * span])
            q1 = p1 / (p0 + p1) if p0 + p1 > 0 else 0.5
            if agree:
                n = self.c.get((pre, mbit), [0.4, 0.4])
                m1 = n[1] / (n[0] + n[1])
                if both:
                    st = math.log(m1 / (1 - m1)) + math.log(max(q1, 1e-9) / max(1 - q1, 1e-9))
                    q = 1 / (1 + math.exp(-st))
                else:
                    q = m1
                n[bit] += 1
                self.c[(pre, mbit)] = n
            else:
                q = q1
            q = min(max(q, 1e-6), 1 - 1e-6)
            cost -= math.log2(q if bit else 1 - q)
            if bit != mbit:
                agree = False
            pre = (pre << 1) | bit
        return cost


def literal_bits(y, path, alphabet, order0=False):
    """What the literals cost as an adaptive coder codes them, byte by byte in order (by the two
    bytes before, or with order0 counted alone, as the static order-0 count but learned as it goes,
    which costs the same; the matched literal, where on, with either)."""
    lit = Literals(alphabet, CODER["beta"], order0)
    total = 0.0
    is_lit = bytearray(len(y))
    for st in path:
        if st[0] == "lit":
            is_lit[st[3]] = 1
    ac = after_copy(path) if CODER["matched"] else {}
    mbits = MatchedBits() if str(CODER["matched"]).startswith("bits") else None
    for i in range(len(y)):
        a2, a1 = (y[i - 2] if i > 1 else 0), (y[i - 1] if i else 0)
        if is_lit[i]:
            d = ac.get(i)
            if d is not None and i >= d and mbits is not None:
                own = [lit.prob(a2, a1, t) for t in range(256)]
                total += mbits.bits(own, y[i], y[i - d], CODER["matched"] == "bits+own")
            elif d is not None and i >= d:
                total -= math.log2(lit.prob_matched(a2, a1, y[i], y[i - d]))
                lit.learn_matched(y[i - d], y[i])
            else:
                total -= math.log2(lit.prob(a2, a1, y[i]))
        if CODER["lit_all"] or is_lit[i]:
            lit.learn(a2, a1, y[i])
    return total


def matched_costs(y, path):
    """The next round's cost of a literal just after a copy, from this path's counts: a function
    of the place, the byte the copy would give and the literal's own cost."""
    ac = after_copy(path)
    cm = {}
    for i, d in ac.items():
        if i >= d:
            r = cm.setdefault(y[i - d], [{}, 0])
            r[0][y[i]] = r[0].get(y[i], 0) + 1
            r[1] += 1
    beta = CODER["beta"]

    def cost(i, mb, own):
        r = cm.get(mb)
        if not r:
            return own
        return -math.log2((r[0].get(y[i], 0) + beta * 2.0 ** -own) / (r[1] + beta))
    return cost


def cost_of(path, n, t, w, alphabet=256, y=None):
    """The recipe's bits, as it would be written: header, numbers, and the adaptive counts."""
    lits = [0] * 256
    lits1 = {}
    flags = [0, 0]
    flags_by = [[0, 0], [0, 0], [0, 0]]
    state = 2
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
            flags_by[state][0] += 1
            state = 0
            continue
        flags[1] += 1
        flags_by[state][1] += 1
        state = 1 if kind in ("copy", "again", "again2") else 2
        tags[kind] += 1
        nums, raw = step_numbers(kind, num, L)
        numbers += raw
        for cat, v in nums:
            sizes[cat][v.bit_length()] += 1
            numbers += v.bit_length() - 1
    header = gamma(n + 1) + 5 + (gamma(w + 1) if w else 1)  # (the transform: one of up to 32)
    if alphabet < 256:
        header += min(256, 8 + 8 * alphabet)  # (the values the file uses: a mask of 256 bits, or how many and each)
    # The literal bytes counted alone (order 0) or by the byte before each (order 1), whichever is
    # smaller, and a bit to say which.
    o0 = kt_bits(lits, alphabet)
    o1 = sum(kt_bits(r, alphabet) for r in lits1.values())
    ob = literal_bits(y, path, alphabet) if (CODER["lit_model"] == "blend" and y is not None) else float("inf")
    # (With the matched literal bit by bit, order 0 with it too: where the bytes alone are the
    # cheapest count, as in a program, the matched literal is still had.)
    om = literal_bits(y, path, alphabet, order0=True) if (str(CODER["matched"]).startswith("bits") and y is not None) else float("inf")
    numbers += sum(kt_bits(v, BUCKETS) for v in sizes.values())
    flag_bits = sum(kt_bits(r, 2) for r in flags_by) if CODER["flag_ctx"] else kt_bits(flags, 2)
    body = 2 + min(o0, o1, ob, om) + flag_bits + kt_bits(list(tags.values()), len(KINDS)) + numbers
    order = min((o0, 0), (o1, 1), (ob, 2), (om, 3))[1]
    # (the costs the next round's parse is steered by: the order-0 matched coder's, or where the
    # literals by two bytes beat order 0 alone, theirs, the better guide to what a literal costs)
    steer = 2 if order == 3 and ob < o0 else order
    return header + body, {"literals": flags[0], "shortcuts": flags[1], "order": order, "steer": steer, "sizes": sizes, "flags_by": flags_by, **tags}


def decode(path, n, w):
    y = bytearray()
    for s in path:
        kind, num, L = s[0], s[1], s[2]
        if kind == "lit":
            y.append(num)
        elif kind in ("copy", "again", "again2"):
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
        if t in GRID_ONLY and not w:
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
        elif t in GEOMETRIC:
            y, numbers = geometric(t, x, w)
            alphabet = 256
        else:
            y = forward(t, x, w)
            alphabet = 256
        if t not in GEOMETRIC:
            numbers = None
        cand = candidates(y, w)
        litc, flag_cost, tag_cost, bc = [8.0] * len(y), [[1.0, 1.0]] * 3, {k: 2.0 for k in KINDS}, gamma_costs()
        mcost = None
        best = None
        for _ in range(3):
            path = parse(y, w, cand, litc, flag_cost, tag_cost, bc, mcost)
            bits, counts = cost_of(path, len(y), t, w, alphabet, y)
            if best is None or bits < best[0]:
                best = (bits, counts, path)
            # The costs the next round's parse uses, learned from this path.
            if counts["steer"] == 2:
                litc = literal_costs(y, path, alphabet)
                mcost = matched_costs(y, path) if CODER["matched"] is True else None
            elif counts["steer"] in (0, 3):
                lits = [1] * 256
                for st in path:
                    if st[0] == "lit":
                        lits[st[1]] += 1
                tot = sum(lits)
                litc = [-math.log2(lits[v] / tot) for v in y]
            else:
                rows = {}
                for st in path:
                    if st[0] == "lit":
                        r = rows.setdefault(st[4], [0.5] * 256)
                        r[st[1]] += 1
                litc = []
                for i, v in enumerate(y):
                    r = rows.get(y[i - 1] if i else 0)
                    litc.append(-math.log2(r[v] / sum(r)) if r else 8.0)
            fb = counts["flags_by"] if CODER["flag_ctx"] else [[counts["literals"], counts["shortcuts"]]] * 3
            flag_cost = [[-math.log2((row[f] + 0.5) / (sum(row) + 1)) for f in (0, 1)] for row in fb]
            nt = {k: counts[k] + 1 for k in KINDS}
            tag_cost = {k: -math.log2(nt[k] / sum(nt.values())) for k in KINDS}
            bc = {}
            for cat, v in counts["sizes"].items():
                tot = sum(v) + BUCKETS * 0.5
                bc[cat] = [-math.log2((c + 0.5) / tot) for c in v]
        bits, counts, path = best
        bits += geometric_bits(t)
        back = bytes(values[v] for v in decode(path, len(y), w)) if t == "palette" else backward(t, decode(path, len(y), w), w, numbers)
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
                if (t in GRID_ONLY and not w) or t == "palette":
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
        f.write("file\tbytes\twidth\ttransform\tshortcut seed\tliterals\tshortcuts\tcopy\tagain\trun\tgrid\tskiprun\tseconds\tzlib\txz\trope\tmixing\n")
        for r in rows:
            for t in r["res"]:
                c = t["counts"]
                f.write("\t".join(str(v) for v in (r["file"], r["n"], r["w"], t["transform"], t["bytes"], c["literals"], c["shortcuts"], c["copy"], c["again"], c["run"], c["grid"], c.get("skiprun", 0),
                                                   f"{t['seconds']:.1f}", r["zlib"], r["xz"], r["rope"], r["mix"])) + "\n")
    with open(out / "results.md", "w", encoding="utf-8") as f:
        f.write(f"# The shortcut seed, stage 1\n\nEach file's first {a.cut:,} bytes (the whole file where smaller). The shortcut seed's best recipe "
                "(its transform, then how many steps were literal bytes and how many shortcuts of each kind), every recipe decoded again and checked "
                "byte for byte; beside it zlib -9, xz -9e (raw LZMA2), the rope's search "
                f"({a.rope_seconds:g} s) and mixing (mix-v4), all on the same bytes. W: the row width guessed for the grid and the waves (0: none).\n\n")
        f.write("| File | Bytes | W | Shortcut seed | Its transform | Literals / shortcuts (copy, again, run, grid, skip-and-insert) | xz | Mixing | Rope | zlib |\n")
        f.write("| :--- | ---: | ---: | ---: | :--- | :--- | ---: | ---: | ---: | ---: |\n")
        for r in rows:
            b, c = r["best"], r["best"]["counts"]
            f.write(f"| {r['file']} | {r['n']:,} | {r['w']} | {b['bytes']:,} | {b['transform']} | {c['literals']:,} / {c['shortcuts']:,} "
                    f"({c['copy']}, {c['again']}, {c['run']}, {c['grid']}, {c.get('skiprun', 0)}) | {r['xz']:,} | {r['mix'] if r['mix'] is not None else ''} | "
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
