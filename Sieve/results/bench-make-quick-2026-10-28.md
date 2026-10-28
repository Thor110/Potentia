# Making Sieve instructions: time by stage (2026-10-28)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 399,087 | 101,709 | 2.64 | 0.15 | 0.07 |  | 0.01 | 0.01 |  | 0.06 | 0.37 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| code.cpp | code | 165,882 | 29,630 | 1.51 | 0.11 | 0.07 |  | 0.01 | 0.01 |  | 0.07 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| oracle.py | code | 447,532 | 89,567 | 3.03 | 0.15 | 0.12 |  | 0.02 | 0.02 |  | 0.10 | 0.46 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| table.csv | table | 17,831 | 7,262 | 0.71 | 0.03 | 0.47 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.14 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| words.txt | word list | 2,921,105 | 310,401 | 11.49 | 0.25 | 0.18 |  | 0.12 | 0.12 |  | 0.16 | 2.37 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 3,633 | 0.16 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,506 | 0.67 | 0.14 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.32 | 0.03 | 0.03 | 0.03 | 0.02 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,956 | 0.83 | 0.25 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,860 | 0.41 | 0.17 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.02 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 53,519 | 2.42 | 0.19 | 0.06 |  | 0.02 | 0.02 |  | 0.07 | 0.42 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| hymn.mid | melody (MIDI) | 1,377 | 230 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,867 | 1.27 | 0.18 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.20 | 0.06 | 0.05 | 0.06 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,257 | 2.15 | 0.04 | 1.59 | 0.10 | 0.10 | 0.00 | 0.09 | 0.01 | 0.67 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,365 | 3.58 | 1.49 |  |  |  |  |  |  |  | 0.40 | 1.58 | 3.10 | 1.25 |  |
| folder.tar | tar | 5,140,480 | 1,209,279 | 21.01 | 0.24 | 1.03 |  | 0.18 | 0.17 |  | 0.16 | 3.59 | 0.00 | 0.00 | 0.00 | 0.00 |  |
