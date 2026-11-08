# Making Sieve instructions: time by stage (2026-11-08)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 552,543 | 136,996 | 2.69 | 0.21 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.73 |
| code.cpp | code | 176,203 | 31,058 | 3.59 | 0.05 |  |  |  |  |  |  | 0.12 | 0.64 | 0.00 | 0.00 | 0.00 |  | 1.98 |
| oracle.py | code | 519,275 | 102,350 | 2.70 | 0.19 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.86 |
| table.csv | table | 17,831 | 7,249 | 0.93 | 0.02 | 0.49 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.34 |
| words.txt | word list | 2,921,105 | 287,033 | 17.72 | 0.16 | 0.16 |  | 0.09 | 0.09 |  | 0.10 | 1.08 | 0.26 | 0.00 | 0.00 | 0.00 |  | 9.52 |
| rope.txt | prose | 11,440 | 3,293 | 0.46 | 0.02 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.41 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 1.03 | 0.09 | 0.00 | 0.01 | 0.06 | 0.01 | 0.05 | 0.00 | 0.37 | 0.07 | 0.00 | 0.06 | 0.02 |  | 0.52 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.98 | 0.21 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.80 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.57 | 0.12 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.03 | 0.01 |  | 0.35 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.40 | 0.20 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.67 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.48 | 0.16 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.18 | 0.09 | 0.00 | 0.10 | 0.04 |  | 0.86 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.26 | 0.04 | 1.75 | 0.13 | 0.11 | 0.01 | 0.10 | 0.00 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 19.84 | 0.27 |  |  |  |  |  |  | 1.69 | 0.00 | 0.00 | 0.00 | 0.00 |  | 12.84 |
| folder.tar | tar | 5,376,000 | 1,228,915 | 14.29 | 0.38 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.68 |
