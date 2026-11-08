# Which 16-byte chunks do the test WAVs' hits come from? Their spread, and what a small table of the
# most common ones saves (its own size charged), against the same test.
import sys, math, numpy as np
sys.argv, extra = sys.argv[:1], sys.argv[1:]
exec(open(__file__.replace("hits_detail.py", "residual_repeats.py")).read().split("train = [")[0])
paths = dict(a.split("=", 1) for a in extra)
k = 16
train = ["Music-Human/02", "Music-Human/03", "Music-Human/04", "Music-Human/05"]
test = ["Music-Martian/03", "Music-Martian/04", "Music-Martian/05"]
for q in (0, 1):
    keys = np.concatenate([chunks(residual(paths[n], 2, skip=44)[q], k)[0] for n in train])
    u, cnt = np.unique(keys, return_counts=True)
    order = np.argsort(-cnt)
    u, cnt = u[order], cnt[order]
    print(f"plane {q}: {len(u):,} distinct chunks in training; the most common covers {cnt[0]:,} of {cnt.sum():,}")
    for T in (1, 16, 256, 4096, 65536):
        table = np.sort(u[:T])
        tot_bits = saved = 0.0
        for n in test:
            pl = residual(paths[n], 2, skip=44)[q]
            h, a = chunks(pl, k)
            b = bits_of(pl)[a.astype(np.int64)].sum(axis=1)
            hit = np.isin(h, table)
            N, hits = len(h), int(hit.sum()); rate = hits / N
            flag = 0 if rate in (0, 1) else -(rate * math.log2(rate) + (1 - rate) * math.log2(1 - rate))
            idx = math.log2(T) if T > 1 else 0
            saved += float((b[hit] - idx).sum()) - N * flag; tot_bits += float(b.sum())
        print(f"  table of the {T:>6,} most common: net {saved/8:>+12,.0f} bytes ({100*saved/tot_bits:+.3f}% of the plane), the table itself {T*k:,} bytes")
