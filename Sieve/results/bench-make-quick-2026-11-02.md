# Making Sieve instructions: time by stage (2026-11-02)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 463,258 | 115,609 | 3.58 | 0.13 | 0.06 |  | 0.02 | 0.02 |  | 0.06 | 0.52 | 0.10 | 0.00 | 0.00 | 0.00 |  | 2.31 |
| code.cpp | code | 169,890 | 29,998 | 1.67 | 0.10 | 0.10 |  | 0.01 | 0.01 |  | 0.08 | 0.19 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.12 |
| oracle.py | code | 471,849 | 93,529 | 3.09 | 0.15 | 0.15 |  | 0.03 | 0.03 |  | 0.14 | 0.60 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.84 |
| table.csv | table | 17,831 | 7,249 | 1.04 | 0.02 | 0.60 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.35 |
| words.txt | word list | 2,921,105 | 280,994 | 7.97 | 0.37 | 0.16 |  | 0.11 | 0.10 |  | 0.10 | 2.32 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.73 |
| rope.txt | prose | 11,440 | 3,293 | 0.34 | 0.03 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.31 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.56 | 0.17 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.33 | 0.02 | 0.02 | 0.03 | 0.01 |  | 0.28 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 0.64 | 0.33 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.16 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.51 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.35 | 0.20 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.05 | 0.02 | 0.01 | 0.02 | 0.01 |  | 0.19 |
| spitfire.obj | mesh | 469,717 | 53,524 | 1.98 | 0.24 | 0.04 |  | 0.02 | 0.02 |  | 0.03 | 0.44 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.30 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.07 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.05 | 0.22 | 0.01 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.21 | 0.06 | 0.06 | 0.06 | 0.02 |  | 0.59 |
| sound.wav | sound (WAV) | 88,244 | 3,255 | 1.90 | 0.05 | 1.46 | 0.11 | 0.09 | 0.00 | 0.08 | 0.00 | 0.60 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,275,360 | 3.20 | 1.67 |  |  |  |  |  |  |  | 0.38 | 1.44 | 2.83 | 1.21 |  |  |
| folder.tar | tar | 5,232,640 | 1,196,747 | 14.50 | 0.36 | 1.03 |  | 0.18 | 0.17 |  | 0.06 | 3.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 6.63 |
