# Making Sieve instructions: time by stage (2026-10-27)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 381,916 | 97,419 | 4.08 | 0.09 | 0.08 |  | 0.02 | 0.02 |  | 0.06 | 0.35 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| code.cpp | code | 164,544 | 29,340 | 2.09 | 0.08 | 0.07 |  | 0.01 | 0.01 |  | 0.07 | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| oracle.py | code | 443,190 | 89,003 | 4.54 | 0.10 | 0.13 |  | 0.02 | 0.02 |  | 0.10 | 0.37 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| table.csv | table | 17,831 | 7,262 | 0.79 | 0.02 | 0.45 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| words.txt | word list | 2,921,105 | 310,401 | 16.75 | 0.17 | 0.18 |  | 0.11 | 0.10 |  | 0.17 | 2.28 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 3,633 | 0.26 | 0.04 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.66 | 0.14 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.32 | 0.03 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.86 | 0.24 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.42 | 0.16 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 53,519 | 3.65 | 0.13 | 0.05 |  | 0.02 | 0.02 |  | 0.05 | 0.39 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.26 | 0.19 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.18 | 0.06 | 0.06 | 0.06 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.17 | 0.04 | 1.64 | 0.10 | 0.09 | 0.01 | 0.08 | 0.01 | 0.61 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.23 | 1.65 |  |  |  |  |  |  |  | 0.37 | 1.41 | 2.78 | 1.14 |  |
| folder.tar | tar | 5,120,000 | 1,204,092 | 29.18 | 0.18 | 1.06 |  | 0.17 | 0.16 |  | 0.15 | 3.47 | 0.00 | 0.00 | 0.00 | 0.00 |  |
