# Edward's wave paths (8 November 2026; results/wave-paths-2026-11-08.md). A wave is a path that
# curves through the file laid out as a grid; where a value on the path is predicted exactly, it is
# cut, and decoding puts it back from its prediction. Every value keeps its place in the grid (cutting
# it out of the grid itself would shift every later value into the wrong column), so each pass must
# be a different wave: another predictor, or another path. Passes repeat up to a limit; the paths are
# chosen from a fixed set of shapes or from seeds, and the choice is paid for in bits.
#
# Also measured, as variants: every value cut where its wave is exact (no path); the base predictor
# itself chosen among wider waves (a wave continued from its last two or three values, as FLAC's fixed
# predictors do); and the reversible 5/3 wavelet of lossless JPEG 2000.
#
# Every variant is costed by one adaptive coder (each symbol at its count so far, plus one for every
# symbol, so no table is free). Use: wave_paths.py NAME=PATH ... (names below).
import sys, os, math, numpy as np

def adaptive(sym, alphabet, ctx=None):
    """Bits to code `sym` adaptively, each given its context `ctx` (None: one context)."""
    sym = np.asarray(sym, np.int64)
    if len(sym) == 0: return 0.0
    ctx = np.zeros(len(sym), np.int64) if ctx is None else np.asarray(ctx, np.int64)
    def rank(keys):
        order = np.argsort(keys, kind="stable"); k = keys[order]
        start = np.r_[0, np.flatnonzero(np.diff(k)) + 1]
        r = np.arange(len(k)) - np.repeat(start, np.diff(np.r_[start, len(k)]))
        out = np.empty(len(k), np.int64); out[order] = r; return out
    return float(np.sum(np.log2(rank(ctx) + alphabet) - np.log2(rank(ctx * alphabet + sym) + 1)))

def static_bits(sym, alphabet, ctx, nctx):
    """A fast estimate for the search: each symbol at its context's final share (no learning cost)."""
    joint = np.bincount(ctx * alphabet + sym, minlength=nctx * alphabet).astype(float)
    c = joint.reshape(nctx, alphabet).sum(1)
    j = joint[joint > 0]; cc = c[c > 0]
    return float(-(j * np.log2(j)).sum() + (cc * np.log2(cc)).sum())

def load(path, skip, cut):
    raw = np.fromfile(path, dtype=np.uint8)[skip: skip + cut if cut else None]
    raw = raw[: len(raw) // 2 * 2]
    return raw.view("<u2").astype(np.int64)

def neighbours(x, W, S=1):
    n = len(x); i = np.arange(n); col = i % W
    def at(d, ok):
        v = np.full(n, -1, np.int64); m = ok & (i >= d); v[m] = x[i[m] - d]; return v
    a = at(S, col >= S); b = at(W, np.ones(n, bool)); c = at(W + S, col >= S)
    aa = at(2 * S, col >= 2 * S); bb = at(2 * W, np.ones(n, bool)); bbb = at(3 * W, np.ones(n, bool))
    cc = at(2 * W + S, col >= S)
    return a, b, c, aa, bb, bbb, cc

def predictors(x, W):
    a, b, c, aa, bb, bbb, cc = neighbours(x, W)
    clamp = lambda v: np.clip(v, 0, 65535)
    fall = np.where(a >= 0, a, np.where(b >= 0, b, 0))  # as the rope: the value before, else above, else 0
    full = (a >= 0) & (b >= 0) & (c >= 0)
    P = {}
    P["P1"] = fall
    P["P3"] = np.where(full, clamp(a + b - c), fall)
    med = np.where(c >= np.maximum(a, b), np.minimum(a, b), np.where(c <= np.minimum(a, b), np.maximum(a, b), a + b - c))
    P["P4"] = np.where(full, med, fall)
    # a wave continued: the line above's value moved on by its last change (order 2) or its last two (order 3)
    t2 = (b >= 0) & (bb >= 0); t3 = t2 & (bbb >= 0)
    base_t = np.where(b >= 0, b, fall)
    P["O2"] = np.where(t2, clamp(2 * b - bb), base_t)
    P["O3"] = np.where(t3, clamp(3 * b - 3 * bb + bbb), P["O2"])
    # the value before, moved on by how the line above's difference to its own before is changing
    full2 = full & (bb >= 0) & (cc >= 0)
    P["O2X"] = np.where(full2, clamp(a + 2 * (b - c) - (bb - cc)), P["P3"])
    return P, (a, b, c)

def bands(a, b, c):
    ok = (a >= 0) & (b >= 0) & (c >= 0)
    g = np.abs(a - c) + np.abs(b - c)
    return np.where(ok, np.searchsorted(np.array([2, 4, 8, 16, 32, 64, 128]), g, side="right"), 8)

def planes(e, band):
    return adaptive(e & 0xFF, 256, band) + adaptive(e >> 8, 256, band)

def flag_ctx(f, k=8):
    ctx = np.zeros(len(f), np.int64)
    for j in range(1, k + 1): ctx = ctx * 2 + np.r_[np.zeros(j, np.int64), f[:-j].astype(np.int64)]
    return ctx

def flags_bits(f, band):
    return adaptive(f.astype(np.int64), 2, flag_ctx(f) * 9 + band) if len(f) else 0.0

# --- paths -------------------------------------------------------------------------------------------
def shape_cells(n_vals, G, P, phi, k):
    """Cells on a wave of k phases across a grid of G columns: in each row r, phase j's column is
    G/2 * (1 + sin(2 pi r / P + phi + 2 pi j / k)), held within the grid."""
    rows = np.arange((n_vals + G - 1) // G)
    on = np.zeros(n_vals, bool)
    for j in range(k):
        col = np.minimum((G * (0.5 + 0.5 * np.sin(2 * np.pi * rows / P + phi + 2 * np.pi * j / k))).astype(np.int64), G - 1)
        idx = rows * G + col; on[idx[idx < n_vals]] = True
    return on

SHAPES = [(G, P, phi, k) for G in (2, 8, 32, 128, 512) for P in (2, 8, 32, 128, 512)
          for phi in np.arange(8) * np.pi / 4 for k in (1, 2, 4)]

def seed_cells(n_vals, seed, G, k):
    rows = (n_vals + G - 1) // G
    rng = np.random.default_rng(seed); on = np.zeros(n_vals, bool)
    for _ in range(k):
        idx = np.arange(rows) * G + rng.integers(0, G, rows); on[idx[idx < n_vals]] = True
    return on

def symbol_cost(e, band):
    """Bits each value's leftover costs among the base leftovers, by its band (both planes, the final
    shares plus one): what cutting that value saves."""
    cost = np.zeros(len(e))
    for plane in (e & 0xFF, e >> 8):
        joint = np.bincount(band * 256 + plane, minlength=9 * 256).reshape(9, 256) + 1.0
        p = joint / joint.sum(1, keepdims=True)
        cost += -np.log2(p[band, plane])
    return cost

def run_passes(x, W, P, band, base, waves, cell_sets, max_passes, choice_bits):
    """Greedy passes: each pass picks the (wave, cells) whose estimated net is best, among waves not yet
    used; stops when no pass would pay. Returns the adaptive total and what each pass took."""
    e_base = (x - P[base]) % 65536
    save = symbol_cost(e_base, band)
    kept = np.ones(len(x), bool); total = 0.0; taken = []; used = set()
    for _ in range(max_passes):
        best = None
        for w in waves:
            if w in used: continue
            exact = (x == P[w])
            for name, make in cell_sets:
                on = make() & kept
                if not on.any(): continue
                f = exact[on]
                fb = static_bits(f.astype(np.int64), 2, band[on], 9)
                net = fb - save[on][f].sum() + choice_bits
                if best is None or net < best[0]: best = (net, w, name, on, f)
        if best is None or best[0] >= 0: break
        net, w, name, on, f = best
        total += flags_bits(f, band[on]) + choice_bits
        cut = np.zeros(len(x), bool); cut[np.flatnonzero(on)[f]] = True
        kept &= ~cut; used.add(w); taken.append(f"{w} on {name}: {int(f.sum()):,} cut")
    total += planes(e_base[kept], band[kept])
    return total, taken

# --- the 5/3 wavelet ---------------------------------------------------------------------------------
def lift53(v, axis=0):
    """One level of the reversible 5/3 lifting (JPEG 2000) along an axis: the low and high halves,
    ends mirrored."""
    v = np.moveaxis(v, axis, 0)
    even, odd = v[0::2], v[1::2]; m = len(odd)
    right = np.concatenate([even[1:], even[-1:]])[:m]
    d = odd - ((even[:m] + right) >> 1)
    dl = np.concatenate([d[:1], d])[: len(even)]; dr = np.concatenate([d, d[-1:]])[: len(even)]
    s = even + ((dl + dr + 2) >> 2)
    return np.moveaxis(s, 0, axis), np.moveaxis(d, 0, axis)

def unlift53(s, d, axis=0):
    s, d = np.moveaxis(s, axis, 0), np.moveaxis(d, axis, 0); m = len(d)
    dl = np.concatenate([d[:1], d])[: len(s)]; dr = np.concatenate([d, d[-1:]])[: len(s)]
    even = s - ((dl + dr + 2) >> 2)
    right = np.concatenate([even[1:], even[-1:]])[:m]
    odd = d + ((even[:m] + right) >> 1)
    out = np.empty((len(even) + m,) + even.shape[1:], np.int64); out[0::2] = even; out[1::2] = odd
    return np.moveaxis(out, 0, axis)

def bucket_bits(v, ctx):
    """Signed values coded as their bit length (adaptive, by context) and then the bits below the top one."""
    z = np.where(v >= 0, 2 * v, -2 * v - 1)
    k = np.zeros(len(z), np.int64); nz = z > 0
    k[nz] = np.floor(np.log2(z[nz])).astype(np.int64) + 1
    return adaptive(k, 40, ctx) + float(np.maximum(k - 1, 0).sum())

def wavelet_bits(x, W, levels):
    """Audio (W = 2): each channel along time. Pictures: rows then columns, the low quarter again."""
    parts, ctxs = [], []
    def add(arr, i): parts.append(arr.ravel()); ctxs.append(np.full(arr.size, i))
    if W == 2:
        for ch in range(2):
            s = x[ch::2]
            for lv in range(levels):
                s, d = lift53(s); add(d, lv)
            add(s, levels)
    else:
        rows = len(x) // W; A = x[: rows * W].reshape(rows, W)
        for lv in range(levels):
            L, H = lift53(A, 1); LL, LH = lift53(L, 0); HL, HH = lift53(H, 0)
            add(LH, 3 * lv); add(HL, 3 * lv + 1); add(HH, 3 * lv + 2); A = LL
        add(A, 3 * levels)
    return bucket_bits(np.concatenate(parts), np.concatenate(ctxs))

def signed(e):
    return np.where(e >= 32768, e - 65536, e)

# --- the files -----------------------------------------------------------------------------------------
FILES = [("Music-Martian/03", 2, 44, 8_000_000), ("Music-Martian/04", 2, 44, 8_000_000), ("Music-Martian/05", 2, 44, 8_000_000),
         ("mr", 512, 0, None), ("x-ray", 1901, 0, None), ("noise", 2, 0, 8_000_000)]
paths = dict(a.split("=", 1) for a in sys.argv[1:])
for name, W, skip, cut in FILES:
    if name not in paths: continue
    x = load(paths[name], skip, cut)
    P, (a, b, c) = predictors(x, W)
    band = bands(a, b, c)
    out = {}
    for w in P: out[f"base {w}, T1"] = planes((x - P[w]) % 65536, band)
    base_bits = out["base P3, T1"]
    best_w = min(P, key=lambda w: out[f"base {w}, T1"])
    waves = list(P)
    allcells = [("every value", lambda: np.ones(len(x), bool))]
    out["cut, 1 pass, every value"], t1 = run_passes(x, W, P, band, "P3", ["P3"], allcells, 1, 0)
    out["cut, up to 3 waves, every value"], t3 = run_passes(x, W, P, band, "P3", waves, allcells, 3, math.log2(len(waves)))
    shapes = [(f"G{G} P{Pp} phase {int(round(phi * 4 / np.pi))} k{k}", (lambda G=G, Pp=Pp, phi=phi, k=k: shape_cells(len(x), G, Pp, phi, k))) for G, Pp, phi, k in SHAPES]
    out["cut, paths, up to 3 waves"], tp = run_passes(x, W, P, band, "P3", waves, shapes, 3, math.log2(len(waves) * len(shapes)))
    seeds = [(f"seed {sd} G32 k{k}", (lambda sd=sd, k=k: seed_cells(len(x), sd, 32, k))) for sd in range(64) for k in (1, 4)]
    out["cut, seeded paths, up to 3 waves"], ts = run_passes(x, W, P, band, "P3", waves, seeds, 3, math.log2(len(waves) * len(seeds)))
    out[f"best base ({best_w}), cut up to 3 waves"], tb = run_passes(x, W, P, band, best_w, waves, allcells, 3, math.log2(len(waves)))
    # the wavelet against the same predictors under the same coder (signed values by bit length)
    out["P3 leftovers, bit-length coder, by band"] = bucket_bits(signed((x - P["P3"]) % 65536), band)
    out[f"{best_w} leftovers, bit-length coder, by band"] = bucket_bits(signed((x - P[best_w]) % 65536), band)
    out["5/3 wavelet, 4 levels, bit-length coder"] = wavelet_bits(x, W, 4)
    print(f"{name}: {len(x):,} values; base P3 with T1 {base_bits/8:,.0f} bytes", flush=True)
    for k, v in out.items(): print(f"  {k:44} {v/8:>13,.0f} bytes  {100*(v/base_bits-1):+7.2f}%")
    for label, t in (("1 pass", t1), ("3 waves", t3), ("paths", tp), ("seeded paths", ts), ("best base", tb)):
        print(f"    {label}: " + ("; ".join(t) if t else "no pass paid"))
    print(flush=True)
