# Making Sieve instructions: time by stage (2026-11-04)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 500,553 | 124,980 | 3.83 | 0.13 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.86 |
| code.cpp | code | 171,683 | 30,379 | 2.23 | 0.08 |  |  |  |  |  |  | 0.04 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.79 |
| oracle.py | code | 487,013 | 96,232 | 4.11 | 0.12 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.04 |
| table.csv | table | 17,831 | 7,249 | 1.10 | 0.02 | 0.55 | 0.05 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.44 |
| words.txt | word list | 2,921,105 | 287,033 | 13.45 | 0.22 | 0.18 |  | 0.11 | 0.10 |  | 0.13 | 1.27 | 0.35 | 0.00 | 0.00 | 0.00 |  | 5.40 |
| rope.txt | prose | 11,440 | 3,293 | 1.02 | 0.01 | 0.02 |  | 0.00 |  |  | 0.00 | 0.00 | 0.23 | 0.00 | 0.00 | 0.00 |  | 0.58 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.35 | 0.04 | 0.01 | 0.02 | 0.52 | 0.16 | 0.35 | 0.00 | 0.39 | 0.18 | 0.00 | 0.05 | 0.02 |  | 1.23 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 2.45 | 0.08 | 0.01 | 0.01 | 0.09 | 0.07 | 0.01 | 0.00 | 0.09 | 0.01 | 0.00 | 0.00 | 0.00 |  | 2.22 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 1.05 | 0.07 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.05 | 0.04 | 0.00 | 0.04 | 0.01 |  | 0.78 |
| spitfire.obj | mesh | 469,717 | 53,144 | 4.32 | 0.11 |  |  |  |  |  |  | 0.10 | 0.00 | 0.00 | 0.00 | 0.00 |  | 3.27 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.02 | 0.06 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.88 | 0.08 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.21 | 0.09 | 0.00 | 0.11 | 0.03 |  | 2.18 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.65 | 0.03 | 1.94 | 0.14 | 0.11 | 0.01 | 0.10 | 0.00 | 0.64 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.32 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 27.94 | 0.19 |  |  |  |  |  |  | 1.71 | 0.00 | 0.00 | 0.00 | 0.00 |  | 18.93 |
| folder.tar | tar | 5,294,080 | 1,208,974 | 18.26 | 0.29 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 9.94 |
