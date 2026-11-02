# Oddities

Results that did not come out as expected, kept so they are not lost: what was seen, what is
known, and whether it is resolved. Begun 1 November 2026 (Edward: "we have encountered a few other
oddities along the way ... perhaps I should [note them] when they pop up again"). Add one whenever
a result surprises; close it with what explained it.

## Open

### Silesia's mozilla is not made smaller than LZMA2 makes it
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
- *Still open:* the packer mixes a file whole only where mixing on its start is 3% under LZMA2,
  or with Try every way, and mozilla is over mixing's 16 MiB; so with `--v33-draft` alone it stays
  LZMA2, and with Try every way as well it can be mixed. Whether the 3% test should weigh the match
  ways' gain is the next question; the group size (8 MiB) and the limit could be searched too, at
  the cost of a combinatorial search (Edward).

### Try every way made five small JPEGs a little larger (Edward's pictures, 1 November 2026)
- *Seen:* of 153 JPEGs (no box ticked), five of 58 KB to 176 KB came out 18 to 225 bytes larger
  with Try every way than without (0.03% to 0.13%): it took mixing as a picture where without it
  LZMA2 was taken.
- *Known:* a file's ways are weighed against LZMA2's estimate (preset 6), and the stream as made
  (preset 9, its settings tried) comes out smaller than that estimate; mixing beat the estimate and
  lost to the stream, its way's text in the listing a little longer too.
- *Next (no new version):* weigh mixing against the stream as it will be made, or keep LZMA2 unless
  mixing is under it by a margin; measured on these five first.

### Some of Edward's JPEGs
- *Seen:* noted by Edward as results that surprised; the figures are in his logs, not here.
- *Next:* Edward is to run his pictures again under the new JPEG and PNG ways (v32's `jbr`, `skr`,
  `pngr`), and the entry is to be filled in or closed from what comes back.

## Resolved

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
