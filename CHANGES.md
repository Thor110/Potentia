# Changes: code review and optimisation pass (6 November 2026, evening)

Commits since 4320887: 0a65311, e34a2b1 and its revert 2abaab7, 1cc5260.
Full write-up: Sieve/results/review-2026-11-06b.md.

## Sixteen fixes from the review
- pack.cpp: the LZMA2 figure made beside the mixing is waited for on every way out (it could read freed bytes).
- compress.cpp: an unreadable folder is refused rather than packed empty; no `clock_cast` (macOS lacks it);
  CRC-32, gzip and bzip2 read-backs work in 1 GiB pieces (4 GiB and over failed); no extra whole copy.
- gather.cpp: Create .sieve and each Compress-to format gather apart; each path acknowledged, taken back after 8 s.
- case_names / gen_case_fold.py: names fold to uppercase as NTFS does (1,423 pairs); the Kelvin sign and k stay two.
- ci/steps/11-oracle.sh: the compress check uses `/`; a Kelvin-sign line.
- tools/fuzz/fuzz_reader.py: limits passed to spawned workers.
- chat.cpp: `/random file` lands on a file every time; exchange rate for pictures only.
- navigator.cpp: a narrow window no longer stops the program.
- app_main.cpp: Create .sieve gathers before any window starts.
- speech.cpp, menus: a reopened menu speaks again; turning Read aloud off stops speech.
- app_settings / menu.cpp: widen_weights auto follows `--item-memory` and ITEM MEMORY changes.
- harness.cpp: a failed melody model shows the melody page.
- locate.hpp: comment back in place.

## Optimisation
Files of 16 MiB or more are sampled (1 MiB at start, middle and end, preset 6). If every sample saves
under 2%, the 9e LZMA2 pass is skipped. Measured:

| | Before | After |
| :--- | ---: | ---: |
| The FLAC, default | 33.6 s, 51,783,003 bytes | 11.6 s, 51,783,003 bytes |
| samba, Optimise Search | 3,277,403 bytes | 3,277,403 bytes |
| mozilla, Optimise Search | 13,376,239 bytes | 13,376,239 bytes |

## Tried and reverted
Skipping mixing for files over 16 MiB when their start foretold it would not clear the bar: samba came out
3,728,015 bytes against 3,277,403, the FLAC gained nothing and enwik8 was slower. Reverted; samba is back to
3,277,403.

## Tests
Unit suite: 154,324 checks, 0 failures.
