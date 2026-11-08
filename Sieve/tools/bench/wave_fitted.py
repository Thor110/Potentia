# A wave fitted to each block (results/wave-paths-2026-11-08.md): each channel of 16-bit stereo sound
# (the left, and the right less the left) predicted from its last N values with weights fitted to the
# block by least squares, quantised to 1/2^Q and paid for (Q+2 bits a weight, plus the order); the
# leftovers coded by bit length, adaptively, given the bit length before. As FLAC's linear prediction
# does it, measured here with the same coder as wave_paths.py. Use: wave_fitted.py RAW16STEREO...
import sys, numpy as np
exec(open(__file__.replace("wave_fitted.py", "wave_paths.py")).read().split("# --- the files")[0])
Q = 14
def block_bits(sig, N):
    n = len(sig)
    if n <= N + 1: return None
    X = np.stack([sig[N - j - 1: n - j - 1] for j in range(N)], 1).astype(float); y = sig[N:].astype(float)
    w, *_ = np.linalg.lstsq(X, y, rcond=None)
    wq = np.round(w * (1 << Q)).astype(np.int64)
    pred = (np.stack([sig[N - j - 1: n - j - 1] for j in range(N)], 1) @ wq) >> Q
    res = np.r_[sig[:N] - np.r_[0, sig[:N - 1]], sig[N:] - pred]
    return res, N * (Q + 2) + 6
for path in sys.argv[1:]:
    x = np.fromfile(path, dtype="<i2").astype(np.int64)
    L, R = x[0::2], x[1::2]; chans = [L, R - L]
    for N in (2, 3, 8, 16, 32):
        side, allres = 0.0, []
        for ch in chans:
            for s in range(0, len(ch), 4096):
                r, hb = block_bits(ch[s: s + 4096], N); allres.append(r); side += hb
        res = np.concatenate(allres)
        z = np.where(res >= 0, 2 * res, -2 * res - 1); k = np.zeros(len(z), np.int64); nz = z > 0
        k[nz] = np.floor(np.log2(z[nz])).astype(np.int64) + 1
        bits = adaptive(k, 40, np.r_[0, k[:-1]]) + float(np.maximum(k - 1, 0).sum()) + side
        print(f"{path.rsplit('/',1)[-1]}: fitted order {N:2}: {bits/8:>12,.0f} bytes", flush=True)
