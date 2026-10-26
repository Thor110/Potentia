# Making Sieve instructions: time by stage (2026-10-26)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 377,380 | 102,198 | 2.20 | 0.17 | 0.07 |  | 0.02 | 0.02 |  | 0.06 | 0.35 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| code.cpp | code | 163,738 | 34,923 | 0.47 | 0.35 | 0.06 |  | 0.01 | 0.01 |  | 0.06 | 0.09 | 0.05 | 0.04 | 0.07 | 0.04 |  |
| oracle.py | code | 427,485 | 100,799 | 0.95 | 0.45 | 0.10 |  | 0.02 | 0.02 |  | 0.09 | 0.32 | 0.15 | 0.13 | 0.21 | 0.10 |  |
| table.csv | table | 17,831 | 7,514 | 0.52 | 0.03 | 0.41 | 0.04 | 0.02 | 0.00 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,454 | 1.80 | 1.62 | 0.16 |  | 0.09 | 0.09 |  | 0.16 | 1.81 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,008 | 0.04 | 0.28 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.25 | 0.37 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.29 | 0.02 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.55 | 0.38 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.17 | 0.41 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,047 | 1.00 | 0.47 | 0.05 |  | 0.02 | 0.02 |  | 0.05 | 0.36 | 0.18 | 0.17 | 0.29 | 0.14 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 0.74 | 0.32 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.17 | 0.07 | 0.06 | 0.07 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,736 | 1.96 | 0.04 | 1.61 | 0.10 | 0.09 | 0.00 | 0.08 | 0.01 | 0.59 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.39 | 1.57 |  |  |  |  |  |  |  | 0.39 | 1.45 | 2.97 | 1.25 |  |
| folder.tar | tar | 5,099,520 | 1,555,700 | 18.01 | 0.28 | 1.03 |  | 0.20 | 0.19 |  | 0.14 | 3.95 | 0.45 | 1.82 | 4.20 | 1.98 |  |
