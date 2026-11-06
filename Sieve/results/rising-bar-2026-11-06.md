# A rising bar for mixing over 16 MiB (6 November 2026)

Edward chose it from the list of what is left (item 5). Over 16 MiB a file is now mixed only where
mixing comes to at least 1% under LZMA2, and 0.5% more for each 100 MB of the file (1.25% at 51 MB,
1.5% at 100 MB, 6% at 1 GB), as mixing decodes at a few MB a second and LZMA2 at tens: a near tie
installs fast. Below 16 MiB the margin is as it was (0.25% and 64 bytes). Not where Try every way was
asked for itself, which asks for size at any cost.

Each file packed alone with `sieve locate FILE --installer X --optimise-search`, by this build
without the bar and with it, on this machine (4 logical processors, with the fuzzer running on three
of them, so the times are a busy machine's):

| File | Bytes | Without the bar | With it | Made in (without / with) |
| :--- | ---: | ---: | ---: | ---: |
| samba | 21,606,400 | 3,277,403 (mixed) | 3,277,403 (mixed) | 69.1 s / 70.6 s |
| nci | 33,553,445 | 1,281,583 | 1,281,583 | 70.6 s / 71.0 s |
| webster | 41,458,703 | 6,226,845 | 6,226,845 | 129.1 s / 123.4 s |
| mozilla | 51,220,480 | 13,318,582 (mixed) | 13,376,239 (LZMA2) | 186.8 s / 187.6 s |
| enwik8 | 100,000,000 | 21,431,896 | 21,431,896 | 489.1 s / 472.8 s |

Only mozilla changes (mixing was 0.43% under LZMA2, under its bar of 1.25%). Installed:

| mozilla | Bytes | Installed in | Peak memory | The same |
| :--- | ---: | ---: | ---: | :--- |
| mixed (without the bar) | 13,318,582 | 49.5 s | 447 MiB | yes |
| LZMA2 (with it) | 13,376,239 | 1.0 s | 161 MiB | yes |

57,657 bytes more, for an install about 50 times as fast.

## Later the same day: the bar against every way

The bar had been asked only where LZMA2 was the best before mixing. A FLAC (Symphony No. 6, 51,997,326
bytes), which LZMA2 cannot shrink, had the rope as its best, and was mixed whole for 0.556%,
installing in 65.9 s. Asked against whatever is best, it is cut into pieces and pulled tight by the
rope: 51,783,003 bytes against 51,707,991, installing in 1.3 s. The five files above, packed again
with it, came to the same bytes as in the table (results/flac-against-hx-2026-11-06.md).
