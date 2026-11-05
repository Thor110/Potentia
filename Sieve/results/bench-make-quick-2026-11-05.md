# Making Sieve instructions: time by stage (2026-11-05)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives | mixing |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 500,553 | 124,980 | 3.66 | 0.14 |  |  |  |  |  |  | 0.07 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.60 |
| code.cpp | code | 171,683 | 30,379 | 1.78 | 0.10 |  |  |  |  |  |  | 0.03 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.40 |
| oracle.py | code | 487,013 | 96,232 | 3.59 | 0.14 |  |  |  |  |  |  | 0.06 | 0.00 | 0.00 | 0.00 | 0.00 |  | 2.61 |
| table.csv | table | 17,831 | 7,249 | 1.08 | 0.02 | 0.53 | 0.04 | 0.01 | 0.00 | 0.01 | 0.00 | 0.11 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.46 |
| words.txt | word list | 2,921,105 | 287,033 | 15.39 | 0.19 | 0.18 |  | 0.11 | 0.10 |  | 0.13 | 1.24 | 0.35 | 0.00 | 0.00 | 0.00 |  | 5.72 |
| rope.txt | prose | 11,440 | 3,293 | 1.73 | 0.01 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.55 | 0.00 | 0.00 | 0.00 |  | 0.96 |
| screenshot.png | picture (PNG) | 92,628 | 80,502 | 2.15 | 0.04 | 0.00 | 0.03 | 0.52 | 0.22 | 0.30 | 0.00 | 0.44 | 0.06 | 0.00 | 0.05 | 0.02 |  | 1.14 |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,946 | 1.91 | 0.11 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.13 | 0.00 | 0.00 | 0.00 | 0.00 |  | 1.75 |
| door.jpg | picture (JPEG) | 69,298 | 63,856 | 0.85 | 0.08 | 0.00 |  | 0.01 | 0.01 |  | 0.00 | 0.05 | 0.03 | 0.00 | 0.03 | 0.01 |  | 0.61 |
| spitfire.obj | mesh | 469,717 | 53,144 | 3.99 | 0.12 |  |  |  |  |  |  | 0.09 | 0.01 | 0.00 | 0.00 | 0.00 |  | 2.99 |
| hymn.mid | melody (MIDI) | 1,377 | 221 | 0.03 | 0.05 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |  |
| film.mp4 | video (MP4) | 234,633 | 233,863 | 2.78 | 0.08 | 0.01 | 0.01 | 0.02 | 0.01 | 0.01 | 0.00 | 0.19 | 0.08 | 0.00 | 0.11 | 0.04 |  | 2.18 |
| sound.wav | sound (WAV) | 88,244 | 3,263 | 2.44 | 0.04 | 1.74 | 0.13 | 0.10 | 0.00 | 0.10 | 0.00 | 0.62 | 0.00 | 0.00 | 0.00 | 0.00 |  | 0.32 |
| program.exe | program (Windows) | 5,337,607 | 4,255,565 | 27.61 | 0.19 |  |  |  |  |  |  | 1.48 | 0.00 | 0.00 | 0.00 | 0.00 |  | 18.24 |
| folder.tar | tar | 5,294,080 | 1,208,974 | 18.18 | 0.29 |  |  |  |  |  |  | 0.08 | 0.00 | 0.00 | 0.00 | 0.00 |  | 10.03 |
