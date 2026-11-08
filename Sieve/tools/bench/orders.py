# The same planes, each byte coded given the one before (order 1) or the two before (order 2),
# against order 0 as the rope codes them now: the structure a table of chunks finds, without one.
import sys, math, numpy as np
sys.argv, extra = sys.argv[:1], sys.argv[1:]
exec(open(__file__.replace("orders.py", "residual_repeats.py")).read().split("train = [")[0])
paths = dict(a.split("=", 1) for a in extra)
def H0(x):
    c = np.bincount(x, minlength=256).astype(float); p = c[c > 0] / len(x); return -(c[c > 0] * np.log2(p)).sum()
def Hk(x, order):
    ctx = np.zeros(len(x), dtype=np.int64)
    for j in range(1, order + 1): ctx = ctx * 256 + np.concatenate([np.zeros(j, np.int64), x[:-j].astype(np.int64)])
    key = ctx * 256 + x
    _, joint = np.unique(key, return_counts=True)
    _, cc = np.unique(ctx, return_counts=True)
    return -(joint * np.log2(joint)).sum() + (cc * np.log2(cc)).sum()
for n in ("Music-Martian/03", "Music-Martian/04", "Music-Martian/05", "noise"):
    pl = residual(paths[n], 2, skip=44 if n.startswith("Music") else 0)
    out = []
    for q in (0, 1):
        x = pl[q]
        h0, h1, h2 = H0(x), Hk(x, 1), Hk(x, 2)
        out.append(f"plane {q}: order 0 {h0/8:>12,.0f}  order 1 {h1/8:>12,.0f} ({100*(h1/h0-1):+.1f}%)  order 2 {h2/8:>12,.0f} ({100*(h2/h0-1):+.1f}%)")
    print(n); [print("  " + o) for o in out]
