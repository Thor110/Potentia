# (results/rope-roundings-2026-11-08.md)
# Do the rope's leftovers repeat across files? (Edward's common-roundings table, IDEAS task 101.)
# Each file's leftovers e = (x - prediction) mod 2^16 under a P3 wave (before + above - diagonal,
# rope-wave-v1's P3, held within range else the mean), cut into its two byte planes. A table of every
# k-byte chunk (k = 4, 8, 16) of the training files' planes; each test file's chunks looked up in it.
# A hit saves what the chunk costs under the test file's own plane table (order 0, as the rope codes
# a plane) less what it costs to name it: log2(table size), and every chunk pays a hit-or-miss flag.
import sys, math, numpy as np

def residual(path, W, S=1, skip=44):
    raw = np.fromfile(path, dtype=np.uint8)[skip:]
    raw = raw[: len(raw) // 2 * 2]
    x = raw.view("<u2").astype(np.int64)
    n = len(x)
    i = np.arange(n)
    col = i % W
    before = np.where((col >= S), np.roll(x, S), -1)
    above = np.where(i >= W, np.roll(x, W), -1)
    diag = np.where((i >= W + S) & (col >= S), np.roll(x, W + S), -1)
    p = np.zeros(n, dtype=np.int64)
    both = (before >= 0) & (above >= 0)
    full = both & (diag >= 0)
    g = before + above - diag
    ok = full & (g >= 0) & (g <= 65535)
    p = np.where(ok, g, np.where(both, (before + above) // 2, np.where(before >= 0, before, np.where(above >= 0, above, 0))))
    e = ((x - p) % 65536).astype(np.uint16)
    return [(e & 0xFF).astype(np.uint8), (e >> 8).astype(np.uint8)]

def chunks(plane, k):
    m = len(plane) // k * k
    a = plane[:m].reshape(-1, k).astype(np.uint64)
    h = np.zeros(len(a), dtype=np.uint64)
    for j in range(k):  # a 64-bit key: the bytes themselves for k <= 8, else a hash
        h = h * np.uint64(1099511628211) + a[:, j] if k > 8 else (h << np.uint64(8)) | a[:, j]
    return h, a

def bits_of(plane):
    c = np.bincount(plane, minlength=256).astype(np.float64)
    p = c / c.sum()
    with np.errstate(divide="ignore"):
        return np.where(c > 0, -np.log2(p), 0.0)

train = [("Music-Human/02", 2), ("Music-Human/03", 2), ("Music-Human/04", 2), ("Music-Human/05", 2), ("mr", 512)]
test = [("Music-Martian/03", 2), ("Music-Martian/04", 2), ("Music-Martian/05", 2), ("x-ray", 1901), ("noise", 2)]
paths = dict(a.split("=", 1) for a in sys.argv[1:])
skip_of = lambda name: 44 if name.startswith("Music") else 0  # a WAV's header
for k in (4, 8, 16):
    planes = [residual(paths[name], W, skip=skip_of(name)) for name, W in train]
    table = [np.unique(np.concatenate([chunks(pl[q], k)[0] for pl in planes])) for q in (0, 1)]
    del planes
    print(f"k = {k}: table of {len(table[0]):,} low-plane and {len(table[1]):,} high-plane chunks from the training files")
    for name, W in test:
        res = residual(paths[name], W, skip=skip_of(name))
        tot_bits = saved = n_all = hits_all = 0.0
        for q in (0, 1):
            h, a = chunks(res[q], k)
            b = bits_of(res[q])[a.astype(np.int64)].sum(axis=1)
            hit = np.isin(h, table[q])
            n, hits = len(h), int(hit.sum())
            rate = hits / n if n else 0
            flag = 0 if rate in (0, 1) else -(rate * math.log2(rate) + (1 - rate) * math.log2(1 - rate))
            idx = math.log2(len(table[q]))
            gain = float((b[hit] - idx).sum()) - n * flag
            tot_bits += float(b.sum()); saved += gain; n_all += n; hits_all += hits
        print(f"  {name:18} chunks {int(n_all):>12,}  in the table {int(hits_all):>11,} ({100*hits_all/n_all:5.2f}%)  "
              f"coded now {tot_bits/8:>13,.0f} bytes  net saving with the table {saved/8:>+12,.0f} bytes ({100*saved/tot_bits:+.2f}%)")
