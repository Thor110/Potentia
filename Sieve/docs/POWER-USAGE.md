# Power usage: what Sieve saves the networks, and what it moves onto users

The measured results of `tools/bench/bench_power.py` (`tools\bench_power.bat`), and what follows
from them. Each new run's results are added below as their own section, with the date and the file.

## What is measured, and what is assumed

**Measured**, by the bench, for each way a file is carried (Sieve for size, Sieve for decoding
speed, zip at deflate 9, 7z at LZMA2 9e, brotli at 11):
- the bytes;
- the processor seconds to make it and to install it (user and system time, every thread), with
  Sieve's own start-up (measured on a 1-byte file) taken off. Every way writes what it installs;
  Sieve's install also checks each file's SHA-256.

**Assumed** (the bench's settings; no counter of the machine's own energy is read where none is to
be had):
- moving a gigabyte across the networks costs 0.006 to 0.06 kWh (6 to 60 Wh);
- the user's processor draws 35 W while installing, and the maker's 65 W while making.

From those, against 7z (the reference): what a fetch saves the networks (the bytes it does not
move), and the energy it moves onto its user (installing, beyond 7z's own install).

Not counted: storing the bytes (on servers, mirrors and backups, over the years they are kept),
which favours the smaller file; and the maker's energy, which is spent once however many fetch it.

## enwik8 (2 November 2026)

`enwik8`, the first 100,000,000 bytes of English Wikipedia (the Large Text Compression
Benchmark's), on Edward's machine (Windows 10, 12 logical processors), Sieve at v32 for size and
for decoding speed, neither with Try every way: enwik8 is over 16 MiB, so neither was mixed, and
both came to the same bytes.

| Way | Bytes | Made in (cpu s) | Installed in (cpu s) | A fetch saves the networks (Wh) | A fetch moves onto the user (Wh) |
| :--- | ---: | ---: | ---: | ---: | ---: |
| Sieve, for size | 24,768,406 | 786.06 | 1.625 | 3.79e-4 to 3.79e-3 | +5.01e-3 |
| Sieve, for decoding speed | 24,768,406 | 786.12 | 1.625 | 3.79e-4 to 3.79e-3 | +5.01e-3 |
| zip (deflate 9) | 36,475,792 | 4.64 | 0.375 | −6.99e-2 to −6.99e-1 | −7.14e-3 |
| 7z (LZMA2 9e) | 24,831,587 | 64.69 | 1.109 | (the reference) | (the reference) |
| brotli (11) | 27,045,057 | 157.83 | 0.297 | −1.33e-2 to −1.33e-1 | −7.90e-3 |

**What it comes to.** Sieve's file is 63,181 bytes smaller than 7z's (0.25%). A fetch of it saves
the networks 0.00038 to 0.0038 Wh, and costs its user 0.0050 Wh more to install (0.516 processor
seconds more, at 35 W). So each fetch loses energy overall, at every count of fetches, and making
it (786 against 65 processor seconds, about 13 Wh more at 65 W) is never paid back.

**Mixed** (made with Try every way, measured 31 October 2026, Proof/STATISTICS.md): 21,547,893
bytes, 3,283,694 under this 7z, installing in 112.6 s on one thread (mixing decodes at about 0.9 MB
a second). A fetch would save the networks 0.020 to 0.197 Wh and cost its user about 1.09 Wh more
(111 s more, at 35 W): a larger loss.

**Where it would break even.** For a fetch to save as much as it costs, moving a gigabyte would
have to cost:

| Sieve, against 7z | Bytes saved | Install time added | Break-even network energy |
| :--- | ---: | ---: | ---: |
| not mixed | 63,181 | 0.516 cpu s | 0.079 kWh a GB (1.3 times the assumed high) |
| mixed | 3,283,694 | about 111 s | 0.33 kWh a GB (5.5 times the assumed high) |

So on enwik8, as the bench's settings have it, Sieve saves the networks and costs its users more
than that: overall a loss of energy, the cost moved from where the bytes travel and are kept onto
the machine that installs them. Mixing makes the loss larger: it saves about 30 KB for each second of
decoding it adds.

## Where it could be a gain, to measure next

A fetch gains where the bytes saved are many for each second of installing they add. Ways measured
elsewhere that may do so, each to be put through the bench:
- **JPEGs as their brunsli form**: 15% to 22% smaller than the JPEG itself (10 originals,
  NEXT-VERSION §4a); its decoding time against reading the JPEG to be measured.
- **Repeating bytes and patterns** (v33's `runs`): up to 535 times smaller than v32 made
  them (a 16-bit counter over 8 MiB, 83 bytes against 44,470), and installed by copying.
- **Programs through the x86 filter and mixing** (v33's `X`): 3% to 15% under LZMA2, but
  decoded by mixing, so likely a loss as enwik8 mixed is.
- **Files alike as deltas** (v32): a copy of most of a large file for a few bytes.

The measure to keep: bytes saved per second of installing added. Against the assumed settings
(0.006 to 0.06 kWh a GB, 35 W), a fetch gains where it saves at least 0.16 to 1.6 MB for each
processor second of installing it adds (35 W for a second is 9.7e-3 Wh; 1.6e-4 to 1.6e-3 GB moves
that much).
