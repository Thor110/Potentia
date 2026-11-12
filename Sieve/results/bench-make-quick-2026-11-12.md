# Making Sieve instructions: time by stage (2026-11-12)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 627,039 | 151,927 | 2.81 | 0.22 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.75 |
| code.cpp | code | 207,727 | 36,315 | 1.31 | 0.16 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.89 |
| oracle.py | code | 535,980 | 104,557 | 4.62 | 0.12 |  |  |  |  |  |  | 0.09 | 0.61 | 0.00 | 0.00 | 0.00 |  | 2.22 |
| table.csv | table | 17,831 | 7,249 | 1.07 | 0.02 | 0.62 | 0.06 | 0.02 | 0.00 | 0.02 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.34 |
| words.txt | word list | 2,921,105 | 287,033 | 15.42 | 0.19 | 0.15 |  | 0.10 | 0.09 |  | 0.09 | 1.14 | 0.26 | 0.00 | 0.00 | 0.00 |  | 5.26 |
| rope.txt | prose | 11,440 | 3,293 | 0.71 | 0.02 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  | 0.62 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 0.98 | 0.09 | 0.00 | 0.01 | 0.09 | 0.03 | 0.07 | 0.00 | 0.34 | 0.05 | 0.00 | 0.06 | 0.02 |  | 0.50 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.11 | 0.19 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.12 | 0.01 | 0.00 | 0.00 | 0.00 |  | 0.91 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.66 | 0.10 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.05 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.40 |
| spitfire.obj | mesh | 469,717 | 53,137 | 2.68 | 0.18 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.73 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 1.73 | 0.14 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.22 | 0.12 | 0.00 | 0.15 | 0.06 |  | 0.94 |
| sound.wav | sound (WAV) | 88,244 | 3,233 | 2.12 | 0.04 | 1.58 | 0.13 | 0.11 | 0.01 | 0.11 | 0.00 | 0.66 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.15 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 20.97 | 0.25 |  |  |  |  |  |  | 1.79 | 0.00 | 0.00 | 0.00 | 0.00 |  | 13.38 |
| folder.tar | tar | 5,498,880 | 1,252,611 | 16.35 | 0.34 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 7.89 |
