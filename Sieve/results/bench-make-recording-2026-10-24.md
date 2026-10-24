# Making Sieve instructions: time by stage (2026-10-24)

Intel(R) Xeon(R) Processor @ 2.80GHz, 4 threads, Linux 6.18.44-fc-v153. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| recording.mp4 | video (MP4, large) | 52,488,684 | 36,568,587 | 350.88 | 0.15 | 5.07 | 34.13 | 176.04 | 6.14 | 765.45 | 0.98 | 39.23 | 81.32 | 38.60 |  |
