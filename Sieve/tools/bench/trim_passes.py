# Edward's trimming (8 November 2026; results/rope-trim-2026-11-08.md): a wave run through the file;
# the values it predicts exactly are trimmed out (the file shrinks), and decoding runs the wave again
# in order, putting each back where it came from. Passes may repeat over what is left (a second wave,
# or noise from a seed). Every variant is costed by one adaptive coder (each symbol at its count so
# far, +1 for every symbol, so no table is free), including where the trims were: a yes/no a value,
# coded given the eight yes/nos before it (and, against T1, the value's band).
# Use: trim_passes.py NAME=PATH ... for the names below; big files are cut to their first 8 MB.
import sys, os, tempfile, numpy as np
sys.argv, extra = sys.argv[:1], sys.argv[1:]
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "residual_repeats.py")).read().split("train = [")[0])

def adaptive(sym, alphabet, ctx=None, nctx=1):
    """Bits to code `sym` adaptively, each given its context `ctx` (None: one context)."""
    sym = sym.astype(np.int64)
    ctx = np.zeros(len(sym), np.int64) if ctx is None else ctx.astype(np.int64)
    pair = ctx * alphabet + sym
    def rank(keys):  # how many of the same key came before each
        order = np.argsort(keys, kind="stable")
        k = keys[order]
        start = np.r_[0, np.flatnonzero(np.diff(k)) + 1]
        r = np.arange(len(k)) - np.repeat(start, np.diff(np.r_[start, len(k)]))
        out = np.empty(len(k), np.int64); out[order] = r
        return out
    return float(np.sum(np.log2(rank(ctx) + alphabet) - np.log2(rank(pair) + 1)))

def planes_cost(e, order1=False, band=None):
    lo, hi = (e & 0xFF), (e >> 8)
    if band is not None: return adaptive(lo, 256, band, 9) + adaptive(hi, 256, band, 9)
    if not order1: return adaptive(lo, 256) + adaptive(hi, 256)
    phi = np.r_[0, hi[:-1]]
    return adaptive(lo, 256, phi, 256) + adaptive(hi, 256, phi, 256)  # each plane given the high byte before

def flag_ctx(f, k=8):
    ctx = np.zeros(len(f), np.int64)
    for j in range(1, k + 1): ctx = ctx * 2 + np.r_[np.zeros(j, np.int64), f[:-j].astype(np.int64)]
    return ctx

def flags_cost(f, band=None):
    ctx = flag_ctx(f)
    if band is None: return adaptive(f.astype(np.int64), 2, ctx, 256)
    return adaptive(f.astype(np.int64), 2, ctx * 9 + band, 256 * 9)

def trim(e):
    f = (e == 0)
    return f, e[~f]

def bands(path, W, skip, S=1):
    """T1's bands: |a - c| + |b - c| split at 2, 4, ..., 128; a ninth where there are no such neighbours."""
    raw = np.fromfile(path, dtype=np.uint8)[skip:]; raw = raw[: len(raw) // 2 * 2]
    x = raw.view("<u2").astype(np.int64); n = len(x); i = np.arange(n); col = i % W
    ok = (i >= W + S) & (col >= S)
    g = np.abs(np.roll(x, S) - np.roll(x, W + S)) + np.abs(np.roll(x, W) - np.roll(x, W + S))
    return np.where(ok, np.searchsorted(np.array([2, 4, 8, 16, 32, 64, 128]), g, side="right"), 8)

def rows(path, W, skip):
    pl = residual(path, W, skip=skip)
    e = (pl[0].astype(np.int64) | (pl[1].astype(np.int64) << 8))
    r = {"values": len(e), "zero share": float(np.mean(e == 0)), "now (order 0)": planes_cost(e),
         "order 1": planes_cost(e, True)}
    f, kept = trim(e)
    r["trim 1 pass"] = flags_cost(f) + planes_cost(kept)
    r["trim + order 1"] = flags_cost(f) + planes_cost(kept, True)
    # second pass: the kept leftovers predicted again from the kept one before (a wave over what is left)
    f2, kept2 = trim((kept - np.r_[0, kept[:-1]]) % 65536)
    r["trim 2 passes"] = flags_cost(f) + flags_cost(f2) + planes_cost(kept2)
    # noise pass: the kept leftovers less noise from the best of up to 256 seeds, its matches trimmed
    best = None
    for seed in range(256):
        fz, keptz = trim((kept - np.random.default_rng(seed).integers(0, 65536, len(kept))) % 65536)
        c = flags_cost(f) + flags_cost(fz) + planes_cost(keptz) + 8
        best = c if best is None or c < best else best
        if seed >= 15 and best >= r["trim 1 pass"]: break  # (it is not catching up)
    r["trim + noise pass"] = best
    bd = bands(path, W, skip)[: len(e)]
    r["T1"] = planes_cost(e, band=bd)
    r["trim + T1"] = flags_cost(f, bd) + planes_cost(kept, band=bd[~f])
    return r

paths = dict(a.split("=", 1) for a in extra)
work = tempfile.mkdtemp()
for name, W, skip, cut in [("Music-Martian/03", 2, 44, 8_000_000), ("Music-Martian/04", 2, 44, 8_000_000),
                           ("Music-Martian/05", 2, 44, 8_000_000), ("mr", 512, 0, None), ("x-ray", 1901, 0, None),
                           ("noise", 2, 0, 8_000_000)]:
    if name not in paths: continue
    p = paths[name]
    if cut:
        tmp = os.path.join(work, name.replace("/", "_"))
        with open(p, "rb") as src, open(tmp, "wb") as dst: dst.write(src.read(cut))
        p = tmp
    r = rows(p, W, skip)
    base = r["now (order 0)"]
    print(f"{name}: {r['values']:,} values, {100*r['zero share']:.2f}% predicted exactly", flush=True)
    for k in ("now (order 0)", "order 1", "trim 1 pass", "trim + order 1", "trim 2 passes", "trim + noise pass"):
        print(f"  {k:18} {r[k]/8:>14,.0f} bytes  {100*(r[k]/base-1):+7.2f}%")
    print(f"  {'T1':18} {r['T1']/8:>14,.0f} bytes")
    print(f"  {'trim + T1':18} {r['trim + T1']/8:>14,.0f} bytes  {100*(r['trim + T1']/r['T1']-1):+7.2f}% (of T1)", flush=True)
