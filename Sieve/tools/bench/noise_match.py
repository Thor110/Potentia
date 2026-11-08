# Edward's noise idea: noise from a seed run through the file; the bytes it matches clipped out and put
# back when decoding. Best of 2^s seeds: matches found, what clipping them saves (8 bits each), and
# what saying where they were costs (the matched positions as a bitmap, coded at its entropy) plus
# the seed's s bits.
import sys, math, numpy as np
data = np.fromfile(sys.argv[1], dtype=np.uint8)[44:44 + 65536] if sys.argv[1] != "noise" else np.random.default_rng(10**9 + 7).integers(0, 256, 65536, dtype=np.uint8)
n, seeds = len(data), 1 << 16
best, best_seed = -1, 0
for seed in range(seeds):
    noise = np.random.default_rng(seed).integers(0, 256, n, dtype=np.uint8)
    m = int(np.count_nonzero(noise == data))
    if m > best: best, best_seed = m, seed
p = best / n
where = n * -(p * math.log2(p) + (1 - p) * math.log2(1 - p))
saved = 8 * best
print(f"{sys.argv[1][-30:]}: {n} bytes; expected matches a seed {n/256:.0f}; best of {seeds:,} seeds {best} (seed {best_seed})")
print(f"  clipping saves {saved:,} bits; saying where costs {where:,.0f} bits + 16 for the seed; net {saved - where - 16:+,.0f} bits")
