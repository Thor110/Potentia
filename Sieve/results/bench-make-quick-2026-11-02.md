# Making Sieve instructions: time by stage (2026-11-02)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 472,547 | 117,908 | 2.67 | 0.18 |  |  |  |  |  |  | 0.32 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.48 |
| code.cpp | code | 170,789 | 30,226 | 2.48 | 0.07 |  |  |  |  |  |  | 0.17 | 0.15 | 0.00 | 0.00 | 0.00 |  | 1.64 |
| oracle.py | code | 477,518 | 94,467 | 3.20 | 0.15 |  |  |  |  |  |  | 0.48 | 0.02 | 0.00 | 0.00 | 0.00 |  | 1.95 |
| table.csv | table | 17,831 | 7,249 | 1.13 | 0.02 | 0.60 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.42 |
| words.txt | word list | 2,921,105 | 280,994 | 9.11 | 0.32 | 0.15 |  | 0.10 | 0.10 |  | 0.11 | 2.43 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.18 |
| rope.txt | prose | 11,440 | 3,293 | 0.41 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.37 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.67 | 0.14 | 0.00 | 0.01 | 0.05 | 0.01 | 0.04 | 0.00 | 0.37 | 0.03 | 0.03 | 0.04 | 0.02 |  | 0.32 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.83 | 0.25 | 0.01 | 0.01 | 0.03 | 0.01 | 0.02 | 0.00 | 0.21 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.67 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.49 | 0.14 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.06 | 0.04 | 0.02 | 0.04 | 0.01 |  | 0.24 |
| spitfire.obj | mesh | 469,717 | 53,524 | 2.86 | 0.16 |  |  |  |  |  |  | 0.43 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.78 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.43 | 0.16 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.28 | 0.07 | 0.07 | 0.10 | 0.04 |  | 0.81 |
| sound.wav | sound (WAV) | 88,244 | 3,255 | 2.03 | 0.04 | 1.56 | 0.11 | 0.09 | 0.00 | 0.08 | 0.00 | 0.60 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.17 |
| program.exe | program (Windows) | 5,337,607 | 4,275,360 | 3.68 | 1.45 |  |  |  |  |  |  |  | 0.43 | 1.57 | 3.44 | 1.49 |  |  |
| folder.tar | tar | 5,253,120 | 1,200,186 | 17.25 | 0.30 |  |  |  |  |  |  | 1.95 | 0.00 | 0.00 | 0.00 | 0.00 |  | 8.01 |
