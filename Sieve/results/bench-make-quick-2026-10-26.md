# Making Sieve instructions: time by stage (2026-10-26)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 373,802 | 120,929 | 0.52 | 0.71 | 0.06 |  | 0.01 | 0.01 |  | 0.06 | 0.23 | 0.16 | 0.15 | 0.21 | 0.10 |  |
| code.cpp | code | 163,098 | 34,825 | 0.28 | 0.59 | 0.06 |  | 0.01 | 0.01 |  | 0.06 | 0.10 | 0.05 | 0.06 | 0.08 | 0.04 |  |
| oracle.py | code | 416,425 | 98,473 | 0.62 | 0.67 | 0.09 |  | 0.02 | 0.02 |  | 0.08 | 0.26 | 0.15 | 0.13 | 0.28 | 0.12 |  |
| table.csv | table | 17,831 | 7,733 | 0.47 | 0.04 | 0.39 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.11 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,454 | 1.42 | 2.06 | 0.18 |  | 0.09 | 0.09 |  | 0.15 | 1.75 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,602 | 0.03 | 0.45 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.17 | 0.54 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.33 | 0.02 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.15 | 1.41 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.07 | 0.98 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,047 | 0.58 | 0.81 | 0.07 |  | 0.02 | 0.02 |  | 0.05 | 0.31 | 0.17 | 0.15 | 0.25 | 0.11 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.09 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 0.20 | 1.17 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.15 | 0.06 | 0.03 | 0.07 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,736 | 1.68 | 0.05 | 1.46 | 0.09 | 0.09 | 0.01 | 0.09 | 0.00 | 0.63 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.78 | 1.41 |  |  |  |  |  |  |  | 0.47 | 1.65 | 3.35 | 1.40 |  |
| folder.tar | tar | 5,079,040 | 1,552,345 | 6.42 | 0.79 | 1.03 |  | 0.16 | 0.16 |  | 0.16 | 3.27 | 0.41 | 1.58 | 3.39 | 1.53 |  |
