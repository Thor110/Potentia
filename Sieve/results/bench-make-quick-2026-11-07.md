# Making Sieve instructions: time by stage (2026-11-07)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 543,640 | 134,771 | 5.52 | 0.10 |  |  |  |  |  |  | 0.07 | 0.87 | 0.00 | 0.00 | 0.00 |  | 2.65 |
| code.cpp | code | 175,715 | 30,922 | 1.45 | 0.12 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.97 |
| oracle.py | code | 500,046 | 98,713 | 2.52 | 0.20 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.68 |
| table.csv | table | 17,831 | 7,249 | 0.99 | 0.02 | 0.50 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.38 |
| words.txt | word list | 2,921,105 | 287,033 | 18.09 | 0.16 | 0.16 |  | 0.10 | 0.10 |  | 0.12 | 1.17 | 0.25 | 0.00 | 0.00 | 0.00 |  | 10.14 |
| rope.txt | prose | 11,440 | 3,293 | 0.42 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.37 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.91 | 0.10 | 0.00 | 0.01 | 0.05 | 0.01 | 0.05 | 0.00 | 0.37 | 0.04 | 0.00 | 0.05 | 0.02 |  | 0.51 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.99 | 0.21 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.83 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.60 | 0.12 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.05 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.36 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.41 | 0.19 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.68 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.46 | 0.16 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.07 | 0.00 | 0.10 | 0.04 |  | 0.87 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.24 | 0.04 | 1.71 | 0.15 | 0.10 | 0.00 | 0.10 | 0.00 | 0.63 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.16 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 20.27 | 0.26 |  |  |  |  |  |  | 1.70 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.23 |
| folder.tar | tar | 5,345,280 | 1,222,853 | 14.92 | 0.36 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.06 |
