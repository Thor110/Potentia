// IDEAS §19.128: build with gcc -O3 -fopenmp noise_seeds.c -lm.
// Noise from a seed run over a block of bytes; for each seed, the bytes it matches. Reports, at every
// power of two seeds, the best seed so far: matches, bits the matched bytes cost under the block's own
// order-0 table (what clipping them saves), bits to say where they were (the matched places as a
// bitmap, at its entropy) and bits for the seed. Use: seeds FILE OFFSET BLOCK LOG2SEEDS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
static inline uint64_t mix(uint64_t x) { x ^= x >> 31; x *= 0x94D049BB133111EBull; x ^= x >> 29; x *= 0xBF58476D1CE4E5B9ull; return x ^ (x >> 32); }
int main(int argc, char** argv) {
    FILE* f = fopen(argv[1], "rb"); long off = atol(argv[2]); int n = atoi(argv[3]); int lg = atoi(argv[4]);
    unsigned char* b = malloc(n); fseek(f, off, SEEK_SET); if (fread(b, 1, n, f) != (size_t)n) return 1;
    double cnt[256] = {0}; for (int i = 0; i < n; ++i) cnt[b[i]]++;
    double cost[256]; for (int v = 0; v < 256; ++v) cost[v] = cnt[v] ? -log2(cnt[v] / n) : 0;
    double best_save = -1e18; int best_m = 0; uint64_t total = 1ull << lg;
    for (int p = 0; p <= lg; ++p) {
        uint64_t lo = p ? (1ull << (p - 1)) : 0, hi = 1ull << p;
        #pragma omp parallel for schedule(dynamic, 256)
        for (uint64_t s = lo; s < hi; ++s) {
            int m = 0; double sv = 0;
            uint64_t base = mix(s * 0x9E3779B97F4A7C15ull + 1);
            for (int i = 0; i < n; ++i) {
                unsigned char z = (unsigned char)mix(base + (uint64_t)i * 0xD1B54A32D192ED03ull);
                if (z == b[i]) { ++m; sv += cost[b[i]]; }
            }
            #pragma omp critical
            if (sv > best_save) { best_save = sv; best_m = m; }
        }
        double q = (double)best_m / n;
        double where = q > 0 && q < 1 ? n * (-q * log2(q) - (1 - q) * log2(1 - q)) : 0;
        printf("%2d  seeds 2^%-2d  best matches %5d  saves %8.0f bits  where %8.0f bits  seed %2d bits  net %+8.0f bits\n",
               p, p, best_m, best_save, where, p, best_save - where - p);
        fflush(stdout);
    }
    return 0;
}
