# Making Sieve instructions: time by stage (2026-10-24)

Intel64 Family 6 Model 167 Stepping 1, GenuineIntel, 12 threads, Windows 10. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search; cutting includes the blocks and the boundaries.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | blocks | boundaries | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 354,627 | 114,974 | 0.49 | 0.73 | 0.06 |  | 0.01 | 0.01 |  | 0.05 | 0.22 | 0.15 | 0.14 | 0.19 | 0.09 |  |
| code.cpp | code | 161,633 | 34,486 | 0.28 | 0.59 | 0.07 |  | 0.01 | 0.01 |  | 0.06 | 0.10 | 0.05 | 0.05 | 0.07 | 0.03 |  |
| oracle.py | code | 399,245 | 95,144 | 0.55 | 0.73 | 0.09 |  | 0.02 | 0.02 |  | 0.08 | 0.25 | 0.15 | 0.14 | 0.19 | 0.09 |  |
| table.csv | table | 17,831 | 7,739 | 0.48 | 0.04 | 0.41 | 0.04 | 0.02 | 0.00 | 0.02 | 0.00 | 0.12 | 0.01 | 0.00 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 564,462 | 1.43 | 2.04 | 0.18 |  | 0.08 | 0.08 |  | 0.15 | 1.68 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.02 | 0.52 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 0.18 | 0.52 | 0.00 | 0.01 | 0.04 | 0.01 | 0.03 | 0.00 | 0.32 | 0.02 | 0.02 | 0.03 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 0.15 | 1.40 | 0.01 | 0.01 | 0.01 | 0.01 | 0.00 | 0.01 | 0.15 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 0.06 | 1.07 | 0.00 |  | 0.00 | 0.00 |  | 0.00 | 0.03 | 0.01 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 0.56 | 0.83 | 0.06 |  | 0.02 | 0.02 |  | 0.05 | 0.28 | 0.17 | 0.16 | 0.26 | 0.11 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.02 | 0.09 | 0.01 |  | 0.00 |  |  | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 0.19 | 1.24 | 0.01 | 0.02 | 0.02 | 0.01 | 0.01 | 0.01 | 0.16 | 0.06 | 0.06 | 0.06 | 0.02 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 1.64 | 0.05 | 1.44 | 0.09 | 0.09 | 0.00 | 0.08 | 0.00 | 0.64 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 3.22 | 1.66 |  |  |  |  |  |  |  | 0.36 | 1.41 | 2.80 | 1.17 |  |
| folder.tar | tar | 5,048,320 | 1,542,983 | 6.24 | 0.81 | 1.02 |  | 0.17 | 0.16 |  | 0.14 | 3.25 | 0.40 | 1.52 | 3.25 | 1.48 |  |
