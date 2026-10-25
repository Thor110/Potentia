# Making Sieve instructions: time by stage (2026-10-25)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 372,795 | 120,625 | 0.53 | 0.70 | 0.06 |  | 0.02 | 0.01 |  | 0.07 | 0.27 | 0.15 | 0.14 | 0.20 | 0.09 |  |
| code.cpp | code | 162,729 | 34,748 | 0.29 | 0.57 | 0.07 |  | 0.01 | 0.01 |  | 0.07 | 0.11 | 0.05 | 0.05 | 0.08 | 0.04 |  |
| oracle.py | code | 414,898 | 98,090 | 0.59 | 0.70 | 0.09 |  | 0.02 | 0.02 |  | 0.09 | 0.30 | 0.15 | 0.15 | 0.22 | 0.11 |  |
| table.csv | table | 17,831 | 7,733 | 0.47 | 0.04 | 0.39 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,454 | 1.48 | 1.98 | 0.18 |  | 0.10 | 0.09 |  | 0.14 | 1.89 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,602 | 0.03 | 0.40 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.19 | 0.50 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.32 | 0.03 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.17 | 1.23 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.08 | 0.86 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.02 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,047 | 0.64 | 0.73 | 0.06 |  | 0.02 | 0.02 |  | 0.07 | 0.34 | 0.19 | 0.18 | 0.26 | 0.12 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.08 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 0.23 | 1.01 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.19 | 0.07 | 0.06 | 0.09 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,736 | 1.88 | 0.05 | 1.67 | 0.09 | 0.09 | 0.01 | 0.09 | 0.00 | 0.63 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.79 | 1.41 |  |  |  |  |  |  |  | 0.41 | 1.60 | 3.49 | 1.47 |  |
| folder.tar | tar | 5,079,040 | 1,552,018 | 7.09 | 0.72 | 1.06 |  | 0.16 | 0.16 |  | 0.17 | 3.48 | 0.45 | 1.86 | 3.64 | 1.67 |  |
