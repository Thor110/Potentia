# Making Sieve instructions: time by stage (2026-10-24)

Intel(R) Xeon(R) Processor @ 2.80GHz, 4 threads, Linux 6.18.44-fc-v153. `sieve locate FILE --installer` (the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the pieces' rope includes its own search.

## Search 1 s a file

| File | Kind | Bytes | Made | s | MB/s | rope search | pieces' rope | cutting | rope code | LZMA2 quick | LZMA2 trial | LZMA2 pass 1 | LZMA2 tuned | 7-Zip pass | archives |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| prose.md | prose | 343,246 | 111,476 | 1.72 | 0.20 | 1.03 |  | 0.02 | 0.01 | 0.21 | 0.33 | 0.13 | 0.29 | 0.12 |  |
| code.cpp | code | 161,519 | 34,453 | 1.32 | 0.12 | 1.05 |  | 0.02 | 0.01 | 0.11 | 0.13 | 0.06 | 0.09 | 0.05 |  |
| oracle.py | code | 399,245 | 95,144 | 1.81 | 0.22 | 1.00 |  | 0.03 | 0.01 | 0.26 | 0.37 | 0.17 | 0.37 | 0.13 |  |
| table.csv | table | 17,831 | 7,739 | 1.07 | 0.02 | 0.86 | 0.08 | 0.09 | 0.00 | 0.34 | 0.02 | 0.01 | 0.01 | 0.00 |  |
| words.txt | word list | 2,921,105 | 633,244 | 6.74 | 0.43 | 1.06 |  | 0.15 | 0.12 | 2.05 | 1.01 | 1.82 | 3.48 | 1.43 |  |
| rope.txt | prose | 11,440 | 4,609 | 0.44 | 0.03 | 0.40 |  |  | 0.00 | 0.00 | 0.02 | 0.00 | 0.01 | 0.00 |  |
| screenshot.png | picture (PNG) | 92,628 | 80,512 | 2.67 | 0.03 | 1.03 | 1.02 | 0.42 | 0.00 | 1.70 | 0.06 | 0.02 | 0.05 | 0.01 |  |
| vitruvian.jpg | picture (JPEG) | 207,500 | 206,966 | 2.24 | 0.09 | 1.00 | 1.02 | 0.04 | 0.01 | 0.18 | 0.00 | 0.00 | 0.00 | 0.00 |  |
| door.jpg | picture (JPEG) | 69,298 | 63,866 | 1.10 | 0.06 | 1.00 |  | 0.01 | 0.00 | 0.04 | 0.04 | 0.01 | 0.02 | 0.01 |  |
| spitfire.obj | mesh | 469,717 | 69,054 | 2.00 | 0.23 | 1.00 |  | 0.04 | 0.02 | 0.32 | 0.52 | 0.27 | 0.36 | 0.19 |  |
| hymn.mid | melody (MIDI) | 1,377 | 236 | 0.02 | 0.07 | 0.01 |  |  | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |  |
| film.mp4 | video (MP4) | 234,633 | 233,874 | 2.44 | 0.10 | 1.02 | 1.06 | 0.05 | 0.01 | 0.23 | 0.14 | 0.06 | 0.14 | 0.04 |  |
| sound.wav | sound (WAV) | 88,244 | 3,742 | 2.23 | 0.04 | 1.71 | 0.19 | 0.28 | 0.00 | 1.07 | 0.01 | 0.01 | 0.01 | 0.01 |  |
| program.exe | program (Windows) | 5,337,607 | 4,275,368 | 5.66 | 0.94 |  |  |  |  |  | 1.09 | 2.69 | 4.62 | 1.97 |  |
| folder.tar | tar | 5,038,080 | 1,539,811 | 10.63 | 0.47 | 1.05 |  | 0.27 | 0.25 | 3.60 | 1.29 | 2.95 | 5.78 | 2.42 |  |
