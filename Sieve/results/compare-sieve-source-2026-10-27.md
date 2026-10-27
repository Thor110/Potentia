**Sieve**: 743 files, 35,396,955 bytes. Linux x86_64, 4 logical processors; `tools/bench/compare.py`, 27 October 2026.

| Method | Bytes | Ratio | Of the folder | Make (s) | Unpack (s) |
| :--- | ---: | ---: | ---: | ---: | ---: |
| Sieve (.sieve) | 5,196,966 | 6.81:1 | 14.68% | 224.7 | 19.39 |
| 7z (LZMA2 9 extreme, solid) | 6,844,677 | 5.17:1 | 19.34% | 52.8 | 1.04 |
| tar.xz (xz -9e) | 6,852,652 | 5.17:1 | 19.36% | 50.3 | 0.58 |
| tar.br (brotli 11, window 24) | 6,943,118 | 5.10:1 | 19.62% | 70.1 | 0.14 |
| tar.zst (zstd 22, long window) | 7,082,928 | 5.00:1 | 20.01% | 28.0 | 0.07 |
| tar.zst (zstd 19) | 7,144,868 | 4.95:1 | 20.18% | 16.1 | 0.05 |
| tar.gz (gzip -9) | 10,857,358 | 3.26:1 | 30.67% | 7.2 | 0.17 |
| tar.bz2 (bzip2 -9) | 11,029,078 | 3.21:1 | 31.16% | 3.9 | 1.66 |
| zip (deflate 9) | 11,194,250 | 3.16:1 | 31.62% | 7.1 | 0.26 |
