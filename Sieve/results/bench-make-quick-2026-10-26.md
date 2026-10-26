# Making Sieve instructions: time by stage (2026-10-26)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 379,238 | 97,156 | 2.06 | 0.18 | 0.07 |  | 0.01 | 0.01 |  | 0.06 | 0.30 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| code.cpp | code | 163,926 | 29,338 | 0.97 | 0.17 | 0.07 |  | 0.01 | 0.01 |  | 0.07 | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| oracle.py | code | 436,359 | 87,521 | 2.32 | 0.19 | 0.10 |  | 0.02 | 0.02 |  | 0.10 | 0.32 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| table.csv | table | 17,831 | 7,250 | 0.64 | 0.03 | 0.46 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| words.txt | word list | 2,921,105 | 320,641 | 11.02 | 0.27 | 0.21 |  | 0.10 | 0.10 |  | 0.18 | 2.17 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 3,983 | 0.14 | 0.08 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.03 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.65 | 0.14 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.31 | 0.03 | 0.02 | 0.04 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 1.12 | 0.18 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.41 | 0.17 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 53,295 | 2.04 | 0.23 | 0.06 |  | 0.02 | 0.02 |  | 0.05 | 0.39 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.44 | 0.16 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.16 | 0.06 | 0.06 | 0.08 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,256 | 2.31 | 0.04 | 1.59 | 0.10 | 0.09 | 0.01 | 0.09 | 0.01 | 0.62 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.41 | 1.57 |  |  |  |  |  |  |  | 0.39 | 1.52 | 2.86 | 1.18 |  |
| folder.tar | tar | 5,109,760 | 1,201,040 | 20.59 | 0.25 | 1.06 |  | 0.17 | 0.17 |  | 0.14 | 3.60 | 0.00 | 0.00 | 0.00 | 0.00 |  |
