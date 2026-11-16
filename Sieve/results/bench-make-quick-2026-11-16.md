# Making Sieve instructions: time by stage (2026-11-16)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 850,067 | 202,187 | 4.39 | 0.19 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.52 |
| code.cpp | code | 227,969 | 39,647 | 1.54 | 0.15 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.00 |
| oracle.py | code | 535,980 | 104,557 | 3.32 | 0.16 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.02 |
| table.csv | table | 17,831 | 7,249 | 0.89 | 0.02 | 0.45 | 0.06 | 0.02 | 0.00 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.34 |
| words.txt | word list | 2,921,105 | 287,033 | 11.08 | 0.26 | 0.29 |  | 0.44 | 0.43 |  | 0.19 | 1.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.13 |
| rope.txt | prose | 11,440 | 3,293 | 0.48 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.42 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.13 | 0.08 | 0.00 | 0.02 | 0.11 | 0.01 | 0.10 | 0.00 | 0.35 | 0.10 | 0.00 | 0.05 | 0.02 |  | 0.53 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.53 | 0.08 | 0.02 | 0.01 | 0.15 | 0.10 | 0.04 | 0.00 | 0.06 | 0.35 | 0.00 | 0.00 | 0.00 |  | 1.70 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 1.13 | 0.06 | 0.00 |  | 0.10 | 0.10 |  | 0.00 | 0.02 | 0.13 | 0.00 | 0.03 | 0.01 |  | 0.60 |
| spitfire.obj | mesh | 469,717 | 53,137 | 3.48 | 0.14 |  |  |  |  |  |  | 0.10 | 0.05 | 0.00 | 0.00 | 0.00 |  | 2.27 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.00 | 0.12 | 0.01 | 0.01 | 0.07 | 0.03 | 0.03 | 0.00 | 0.15 | 0.12 | 0.00 | 0.20 | 0.06 |  | 1.09 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.39 | 0.04 | 1.52 | 0.14 | 0.10 | 0.00 | 0.10 | 0.00 | 0.47 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.21 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 25.50 | 0.21 |  |  |  |  |  |  | 2.22 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.13 |
| folder.tar | tar | 5,744,640 | 1,306,738 | 18.88 | 0.30 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.98 |
