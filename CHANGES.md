# Changes: items 1 to 3, and the encode.su report (7 November 2026)

Commits since 34d1cea: 0c69fd0, b79e8dd, f22d385, ffd3ff9, fc6fb01, and the last (PNGs ahead, write-up).
Full write-up: Sieve/results/optimise-2026-11-07.md.

## A fault in yesterday's sampling, fixed first
A large file that repeats far apart looks incompressible in every 1 MiB sample. 20 MB of random
bytes three times over came out at 60,001,033 bytes (xz: 20,006,996). Samples now stand for the
whole only where nothing repeats far apart (a 50 ms pass): 20,006,995.

## 1. Optimise Search on files that will not shrink
Over 16 MiB, where LZMA2 cannot shrink the file, its three samples are mixed first; where mixing's
lead is under half the rising bar, Try every way stays off.
- The FLAC: 428.4 s to 43.4 s, the same 51,783,003 bytes.
- samba, mozilla, enwik8: the same bytes (3,277,403 / 13,376,239 / 21,431,896).

## 2. Archives' and spans' weighings a batch ahead on the threads
LZMA2's estimates, spans and PNGs' makers' results worked out ahead; the loop decides in order.
- docs/: the step 4.0 s to 1.6 s; Pages/: 1.58 s to 0.87 s. The same bytes on both and on the CI's
  spans folders.

## 3. What the pre-release review left
- A RAR's members must be files the install wrote; their modes put back if the RAR does not match.
- One strict reader of Sieve's numbers (sieve/varint.hpp); one copy of the vault's decoders; one
  bounded reader of a Llama config (a config.json claiming 10^18 layers ran the memory out).
- Fuzzed again (about 16,000 cases): nothing new.

## From encode.su
- An archive whose members cannot all be written on every system (a name ending in a dot, as in
  Adobe's cab; two names one where case is ignored) is now left as it is.
- --optimise-archives / --optimise-compressed alone is temporary only; --compressed-permanent must be
  given by name. Packing warns, naming each archive installed as its contents and each picture
  re-saved (command line and File Locator). CI lines that meant both now say both.

## Tests
Unit suite: 154,354 checks, 0 failures. CI blocks run here: v20 to v22 archives, RAR, v25 PNGs,
spans, the two new blocks (dot names, cab), and the hallway's archive block.
