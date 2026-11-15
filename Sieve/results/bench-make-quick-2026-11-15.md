# Making Sieve instructions: time by stage (2026-11-15)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 841,133 | 200,267 | 3.75 | 0.22 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.20 |
| code.cpp | code | 219,842 | 38,362 | 1.32 | 0.17 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.86 |
| oracle.py | code | 535,980 | 104,557 | 3.04 | 0.18 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.88 |
| table.csv | table | 17,831 | 7,249 | 2.96 | 0.01 | 1.21 | 0.10 | 0.67 | 0.17 | 0.50 | 0.00 | 0.14 | 0.06 | 0.00 | 0.00 | 0.00 |  | 0.78 |
| words.txt | word list | 2,921,105 | 287,033 | 13.36 | 0.22 | 0.15 |  | 0.09 | 0.08 |  | 0.09 | 1.04 | 0.09 | 0.00 | 0.00 | 0.00 |  | 4.67 |
| rope.txt | prose | 11,440 | 3,293 | 0.99 | 0.01 | 0.03 |  | 0.00 |  |  | 0.00 | 0.00 | 0.26 | 0.00 | 0.00 | 0.00 |  | 0.54 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.98 | 0.05 | 0.00 | 0.02 | 0.52 | 0.17 | 0.35 | 0.00 | 0.35 | 0.11 | 0.00 | 0.06 | 0.02 |  | 0.92 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.47 | 0.14 | 0.01 | 0.01 | 0.08 | 0.05 | 0.02 | 0.00 | 0.08 | 0.06 | 0.00 | 0.00 | 0.00 |  | 1.17 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.82 | 0.08 | 0.00 |  | 0.03 | 0.03 |  | 0.00 | 0.04 | 0.06 | 0.00 | 0.03 | 0.01 |  | 0.47 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.97 | 0.16 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.98 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.55 | 0.15 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.16 | 0.08 | 0.00 | 0.13 | 0.04 |  | 0.91 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.18 | 0.04 | 1.45 | 0.12 | 0.11 | 0.00 | 0.10 | 0.00 | 0.59 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.16 |
| program.exe | program (Windows) | 5,337,607 | 4,223,640 | 25.09 | 0.21 |  |  |  |  |  |  | 1.90 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.74 |
| folder.tar | tar | 5,724,160 | 1,303,506 | 18.76 | 0.31 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.78 |
