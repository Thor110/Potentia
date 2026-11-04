# Oddities

Results that did not come out as expected, kept so they are not lost: what was seen, what is
known, and whether it is resolved. Begun 1 November 2026 (Edward: "we have encountered a few other
oddities along the way ... perhaps I should [note them] when they pop up again"). Add one whenever
a result surprises; close it with what explained it.

## Open

### Some of Edward's JPEGs
- *Seen:* noted by Edward as results that surprised; the figures are in his logs, not here.
- *Next:* Edward is to run his pictures again under the new JPEG and PNG ways (v32's `jbr`, `skr`,
  `pngr`), and the entry is to be filled in or closed from what comes back.

## Resolved

### Silesia's mozilla was not made smaller than LZMA2 makes it (overcome 2 November 2026, with the v33 draft)
- *Seen:* a tar of 51,220,480 bytes (525 files, most of them programs for the Alpha processor,
  Tru64's COFF). Sieve keeps it whole, by LZMA2: 13,376,239 bytes, level with xz -9e on the same
  files (13.34 to 13.51 MB by their order).
- *Known (1 November 2026):*
  - Its sizes are written after leading spaces, which Sieve's tar reader read as 0 until ec62d61;
    since then it is read, and the packer still keeps it whole, rightly.
  - Taken apart it came to 13,794,627 with the groups, 13,530,583 once mixed files were weighed
    against the stream they share (498dbcf): its programs share much with each other (packed
    together by xz 9.71 MB, one by one 11.0 MB), which one LZMA2 stream over the whole tar finds.
  - The x86 filter does not fit Alpha code; an Alpha branch filter measured at 0.6% (54,736 bytes
    of 9.71 MB), so not built.
  - Mixing the whole tar as one (`sieve mix`, mix-v4) came to 13,778,604, 3.0% larger than LZMA2:
    on these programs mixing's model of earlier copies does not reach as LZMA2's matches do.
- *The cause, found (2 November 2026):* mixing's match model already reaches the whole file; what
  it lacked was keeping a match past one wrong byte (as LZMA's last distance does), shorter matches
  and a second candidate. With those (the v33 draft's R1, M4, C2) the whole tar mixed is 13,318,505
  bytes, under LZMA2's 13,376,180 for the first time (results/mixing-match-ways-2026-11-02.md).
- *Overcome (2 November 2026, night):* two things more stood in the way, both packer-only:
  - Mixing was weighed against LZMA2 with a margin of 0.5% (set that day for five small JPEGs), and
    mozilla's gain is 0.43%. The five's estimates ran 0.06% to 0.26% over their streams; a margin of
    64 bytes and 0.25% keeps them as LZMA2 and lets mozilla through.
  - Mixing on its start guessed a shape, 48 bytes wide, which won the start by 0.56% and lost the
    whole (13,373,422 bytes as a picture). A guessed shape is now taken only where it wins its start
    by 2% (shapes that were pictures won by more: a designed diagonal 7.07%, a MIDI 4.41%).
  - With both, `sieve locate mozilla --optimise-search` (Try every way chosen by
    Optimise Search, the file being over 16 MiB) mixes it as bytes with the match model's ways,
    `mix4 B22 R1 M4 C2`: **13,318,582 bytes**, against LZMA2's 13,376,239 (−0.43%), installed back
    byte for byte in 96 s (LZMA2's in about 3 s). With `--v32` it stays LZMA2.

### Try every way made five small JPEGs a little larger (Edward's pictures, 1 November 2026; fixed 2 November)
- *Seen:* of 153 JPEGs (no box ticked), five of 58 KB to 176 KB came out 18 to 225 bytes larger
  with Try every way than without (0.03% to 0.13%): it took mixing as a picture where without it
  LZMA2 was taken.
- *Known:* a file's ways are weighed against LZMA2's estimate (preset 6), and the stream as made
  (preset 9, its settings tried) comes out smaller than that estimate; mixing beat the estimate and
  lost to the stream, its way's text in the listing a little longer too.
- *The five (Edward's pictures):* K2_00001.jpg 175,693 bytes, LZMA2's stream 174,795 against
  mixing's code 174,829 (the `.sieve` 174,857 against 174,908); K2_00000.jpg 169,433 against 169,641;
  a 85,821-byte JPEG 80,613 against 80,683; two of 64,624 and 58,664 bytes 64,008 against 64,009 and
  57,176 against 57,180. In every one mixing's code was no smaller than LZMA2's stream, and it
  installed about thirty times slower (0.16 s against 0.005 s on the largest).
- *Fixed (2 November 2026, no new version):* mixing now replaces LZMA2 only by the margin the
  packer's other weighings use, 64 bytes and 0.5% (mixing's wins on the other 148 had a median of
  2.7%). Confirmed on the five (Edward sent them): with Try every way each now comes out exactly as
  without it (174,857; 169,495; 80,681; 64,109; 57,273 bytes), LZMA2 kept. All five are small baseline
  JPEGs whose entropy-coded data leaves mixing nothing to find; with the temporary box, brunsli makes
  them 17.5% to 21.8% smaller (143,565; 137,154; 70,813; 52,376; 45,863), their route.

### control.py, a small script, came out larger than its 7z (overcome by 31 October 2026)
- *Seen:* a Python script of 1,597 bytes, first measured at 832 bytes, 10 more than its 7z (822):
  what every `.sieve` carries beside the file (the listing, and its SHA-256) cost more than Sieve
  saved on a file so small.
- *Overcome:* the head and the listing written in bytes (v16, v20) took it to 731, 11.07% under the
  7z; at v32 with Try every way (which lifts mixing's 4 KiB floor) it is 591 bytes, 28.10% under the
  7z, the script mixed in 522 against LZMA2's 690 (Proof/STATISTICS.md, "A small script").

### Try every way made `docs/` larger than the defaults (1 November 2026)
- *Seen:* 1,069,076 bytes against the defaults' 1,056,189.
- *Explained:* Try every way mixes more files alone, and a file mixed alone (as a picture, or by the
  rope) was left out of the groups, though it cost more alone than in one.
- *Resolved:* such files are candidates for the groups, and a group holding one is weighed with it
  and without it; `docs/` now comes out the same as with the defaults.

### The release script's `.sieve` was smaller than the File Locator's (1 November 2026)
- *Seen:* Edward's `source.sieve` from the release script was 47,322,056 bytes, and `sieve.sieve`
  6,461,548, both smaller than his own runs with Try every way.
- *Explained and resolved:* the same group bug as above (the release script does not tick Try every
  way).

### Silesia's mozilla was never taken apart (1 November 2026)
- *Seen:* the same size with the boxes ticked or not.
- *Explained:* older GNU tars write their octal sizes after leading spaces (`"     120000 "`);
  Sieve read each as 0, so the next header failed its checksum and the tar was not read.
- *Resolved:* ec62d61 reads the spaces; CI step 11 holds a tar written so.

## Measured, not odd: the floor

A `.sieve` holds a file's name, size and SHA-256 (32 bytes) beside its bytes, so below some size it
is larger than the file. Measured on the start of the README (1 November 2026, `--search-per-file 1`;
gzip as zlib -9 with its header and trailer, no name):

| File | `.sieve` | Against the file | gzip |
| ---: | ---: | ---: | ---: |
| 8 | 60 | +52 | 34 |
| 16 | 69 | +53 | 42 |
| 32 | 86 | +54 | 56 |
| 64 | 118 | +54 | 81 |
| 128 | 159 | +31 | 128 |
| 192 | 208 | +16 | 174 |
| 256 | 248 | −8 | 214 |
| 512 | 407 | −105 | 359 |
| 1,024 | 664 | −360 | 602 |
| 2,048 | 1,188 | −860 | 1,146 |
| 4,096 | 1,735 | −2,361 | 2,112 |

For text the floor is between 192 and 256 bytes; gzip, with a 4-byte check, is smaller below about
2 KiB, Sieve from about 4 KiB. Left so (Edward: files that small are rarely ones that need to be
sent anywhere). The roof (where memory and time bend the curve, likely between 100 MB and 1 GB) is
yet to be measured.
