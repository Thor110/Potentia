# Making Sieve instructions: time by stage (2026-10-27)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 382,286 | 97,513 | 2.63 | 0.15 | 0.07 |  | 0.01 | 0.01 |  | 0.06 | 0.39 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| code.cpp | code | 164,708 | 29,365 | 1.45 | 0.11 | 0.06 |  | 0.01 | 0.01 |  | 0.06 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| oracle.py | code | 445,466 | 89,291 | 3.19 | 0.14 | 0.13 |  | 0.02 | 0.02 |  | 0.11 | 0.46 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| table.csv | table | 17,831 | 7,262 | 0.78 | 0.02 | 0.49 | 0.04 | 0.05 | 0.00 | 0.04 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| words.txt | word list | 2,921,105 | 310,401 | 11.63 | 0.25 | 0.20 |  | 0.09 | 0.09 |  | 0.19 | 1.94 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 3,633 | 0.16 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.75 | 0.12 | 0.01 | 0.01 | 0.04 | 0.00 | 0.03 | 0.00 | 0.36 | 0.06 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.87 | 0.24 | 0.01 | 0.02 | 0.01 | 0.01 | 0.01 | 0.01 | 0.15 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.41 | 0.17 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.73 | 0.17 | 0.06 |  | 0.02 | 0.02 |  | 0.05 | 0.47 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.41 | 0.17 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.22 | 0.07 | 0.07 | 0.10 | 0.03 |  |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.26 | 0.04 | 1.67 | 0.11 | 0.10 | 0.01 | 0.09 | 0.01 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.92 | 1.36 |  |  |  |  |  |  |  | 0.51 | 1.69 | 3.56 | 1.50 |  |
| folder.tar | tar | 5,120,000 | 1,204,501 | 21.05 | 0.24 | 1.06 |  | 0.18 | 0.17 |  | 0.15 | 3.72 | 0.00 | 0.00 | 0.00 | 0.00 |  |
