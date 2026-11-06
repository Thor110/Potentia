# The FLAC, HX, and the bar against every way (6 November 2026)

One commit on top of the last zip (4320887).

## Statistics

Proof/STATISTICS.md has a new "Music (FLAC)" section, and a line in "At a glance", with your zip, 7z
and `.sieve` figures for Symphony No. 6 and the ones measured here.

## Their repository (CombaryMaker/Harringtons_Insights, cloned at ff6585f)

- **A, anything lifted from Sieve: nothing.**
  - **Code:** every line of 45 characters or more in their sources (13,272) was searched for in Sieve's.
    Four matched, all ordinary Python boilerplate.
  - **Words and names:** none of Sieve's own terms appear.
  - **hs_sieve:** the only part that touches Sieve. It runs your `sieve` program and uses none of its
    source.
- **B, the FLAC:** every result below was decoded and checked byte for byte.

| | Bytes | Off the FLAC | Decoded in |
| :--- | ---: | ---: | ---: |
| 7z / zip (yours) | 51,944,928 / 51,942,347 | 0.10% / 0.11% | |
| HX Fast | 51,934,580 | 0.12% | 0.15 s |
| HX `--complete`, its best | 51,893,617 | 0.20% | 9.35 s |
| hs_sieve | 51,784,112 | 0.41% | 0.52 s |
| Sieve now (default and Optimise Search) | 51,783,003 | 0.41% | 1.3 s |
| Sieve, mixed whole (your 51,708,015) | 51,707,991 | 0.56% | 65.9 s |

HX's best takes 0.20% off; Sieve takes twice that while installing in 1.3 s, and 2.8 times that mixed.
hs_sieve is Sieve in blocks: 1,109 bytes bigger than Sieve alone, and faster on four cores.

## A fix this found

Your `.sieve` mixed the whole 52 MB file for 0.56%, and that installs in 65.9 s. The rising bar
(today's) had only been checked when LZMA2 was the alternative, and LZMA2 can't shrink a FLAC. It is
now checked against whatever way is best. Here that means the rope's pieces: 75,012 bytes more, and
it installs in 1.3 s. samba, nci, webster, mozilla and enwik8 were packed again with the fix and came
to the same bytes.

The full write-up is in results/flac-against-hx-2026-11-06.md.
