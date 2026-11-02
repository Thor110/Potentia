# The roof: mozilla (2 November 2026)

Made by `tools/bench/bench_roof.py` on Silesia's mozilla (a tar of 51,220,480 bytes), its first 1, 4 and 16 MiB and the whole (49 MiB), in this session's container (Linux, 4 logical processors), so the times are not a desktop's. Making's memory grows faster than the file (254 MiB at 1 MiB, 2,152 MiB at 49 MiB, about 44 times the file); installing's stays near three times it. Where it bends past this is for enwik9 to show (Edward's machine, 32 GB).

Made 2026-11-02 05:51 on vm (Linux 6.18.44-fc-v185, 4 logical processors), with that day's build, no search limit. Each size is the file's first that many bytes. Peak memory: the most the process held at once.

| Size | Way | `.sieve` | Made in (wall) | Made in (cpu) | Making's peak memory | Installed in (wall) | Installed in (cpu) | Installing's peak memory | The same |
| ---: | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | :--- |
| 1 MiB | size | 640,069 | 18.0 s | 20.9 s | 254 MiB | 0.1 s | 0.1 s | 16 MiB | yes |
| 1 MiB | decoding | 640,069 | 16.1 s | 18.8 s | 254 MiB | 0.1 s | 0.1 s | 16 MiB | yes |
| 4 MiB | size | 3,238,705 | 34.8 s | 42.2 s | 439 MiB | 0.4 s | 0.3 s | 32 MiB | yes |
| 4 MiB | decoding | 3,238,705 | 35.0 s | 42.7 s | 439 MiB | 0.4 s | 0.3 s | 32 MiB | yes |
| 16 MiB | size | 5,937,937 | 109.4 s | 139.6 s | 994 MiB | 0.9 s | 0.9 s | 66 MiB | yes |
| 16 MiB | decoding | 5,937,937 | 115.1 s | 146.5 s | 994 MiB | 0.8 s | 0.8 s | 66 MiB | yes |
| 49 MiB | size | 13,376,248 | 377.6 s | 505.2 s | 2,152 MiB | 2.5 s | 2.4 s | 160 MiB | yes |
| 49 MiB | decoding | 13,376,248 | 392.5 s | 521.1 s | 2,124 MiB | 2.3 s | 2.2 s | 160 MiB | yes |
