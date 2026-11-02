# One file's estimate kept as the stream's first pass (2 November 2026)

Unzip this over the repository root, on top of your commit 3693b18. One more local commit, not
pushed. The unit suite passes (77,133 checks, 0 failures), and so do CI steps 10 and 11.

When a run has one file, carried as it is (enwik9 for decoding, say), its LZMA2 figure for weighing
its ways is now made as the stream's own first pass (preset 9e, the stream's dictionary) and kept.
The stream then reuses it, rather than packing the same bytes again. On your enwik9 that's the
617 s the estimate took.

| One file | Before | After |
| :--- | ---: | ---: |
| 12.9 MB of text, for decoding | 44.9 s | 29.4 s |
| mozilla's tar, 51 MB, for decoding | 433.0 s | 356.1 s |
| the same text, for size (mixed, so the kept pass is unused) | 67.0 s | 69.5 s |

The bytes are the same in every case. The LZMA2 figure the ways are weighed against is now the
pass itself, which is a little smaller than the old estimate. So in a near tie, LZMA2 now wins where
mixing won before.

The review (`results/weighing-order-2026-11-02.md`) has this added.
