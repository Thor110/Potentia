# The roof, 1 MiB to 1 GB (6 November 2026)

One commit on top of the last zip (f091783). The last of the six.

enwik9 (954 MiB), and its first 1 to 256 MiB, made with the command line's default and installed
again. Every size came back the same.

| Size | `.sieve` | Made in | Making's peak memory | Installed in | Installing's peak |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 64 MiB | 16,855,762 | 234.9 s | 2,664 MiB | 1.2 s | 208 MiB |
| 256 MiB | 64,357,882 | 1,017.6 s | 4,950 MiB | 5.2 s | 772 MiB |
| 954 MiB | 211,195,707 | 3,395.3 s | 12,950 MiB | 16.6 s | 2,728 MiB |

- **The roof is making's memory.** From 64 MiB it is about 2 GB plus 11.5 MiB for every MiB read. The
  whole of enwik9 took 12.95 GB of this machine's 16. Installing stays small and fast.
- **With Optimise Search** (the File Locator's default) at 64 and 256 MiB: 14% and 13% smaller (it
  mixes them), at 1,597 and 3,038 MiB at the most.
- **The File Locator's memory estimate:** it held with Optimise Search ticked. With it unticked it
  said 9.2 GB for the whole, where it took 12.95. Unticked and from 64 MiB, it now says 2,000 MiB and
  11.5 times what is read, within 3% of all three runs.

results/roof-enwik9-2026-11-06.md has the full table. The whole came to 11 bytes more than your
machine's LZMA2 figure for enwik9.
