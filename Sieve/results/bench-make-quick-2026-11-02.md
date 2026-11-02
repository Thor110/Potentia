# Making Sieve instructions: time by stage (2026-11-02)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 475,736 | 118,766 | 3.78 | 0.13 |  |  |  |  |  |  | 0.14 | 0.01 | 0.00 | 0.00 | 0.00 |  | 2.47 |
| code.cpp | code | 170,789 | 30,226 | 1.54 | 0.11 |  |  |  |  |  |  | 0.05 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.10 |
| oracle.py | code | 482,854 | 95,264 | 2.70 | 0.18 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.67 |
| table.csv | table | 17,831 | 7,249 | 0.95 | 0.02 | 0.49 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.37 |
| words.txt | word list | 2,921,105 | 280,994 | 8.90 | 0.33 | 0.17 |  | 0.10 | 0.10 |  | 0.10 | 1.21 | 0.00 | 0.00 | 0.00 | 0.00 |  | 4.03 |
| rope.txt | prose | 11,440 | 3,293 | 0.37 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.33 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.60 | 0.15 | 0.00 | 0.01 | 0.04 | 0.00 | 0.04 | 0.00 | 0.34 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.31 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.79 | 0.26 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.64 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.40 | 0.17 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.04 | 0.02 | 0.00 | 0.02 | 0.01 |  | 0.24 |
| spitfire.obj | mesh | 469,717 | 53,524 | 2.27 | 0.21 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.44 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.20 | 0.20 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.06 | 0.00 | 0.08 | 0.02 |  | 0.68 |
| sound.wav | sound (WAV) | 88,244 | 3,255 | 2.13 | 0.04 | 1.67 | 0.11 | 0.09 | 0.00 | 0.08 | 0.00 | 0.60 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.18 |
| program.exe | program (Windows) | 5,337,607 | 4,275,360 | 3.21 | 1.66 |  |  |  |  |  |  |  | 0.37 | 1.47 | 2.83 | 1.19 |  |  |
| folder.tar | tar | 5,263,360 | 1,201,813 | 16.22 | 0.32 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.39 |
