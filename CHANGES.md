# Changes: PDFs opened (v34 draft) and the hallway's usage text

Two commits on top of db89863.

## eb7b799: PDFs as a skeleton and members

The first item of docs/FORMATS-PLAN.md. With `--v34-draft`, a PDF is carried as a skeleton and
members, as v31 carries a tar (`skm`):
- each `/FlateDecode` stream is made again by preflate from what it holds;
- each `/DCTDecode` JPEG is a member of its own and may be carried as its brunsli form (`jbr`). This
  is the one change a reader sees: in v34, a skeleton's member may carry `jbr`.

JBIG2, JPX and chained filters stay in the skeleton. A PDF is opened only where it comes out clearly
smaller, and only after checking that it gives the PDF back exactly.

Measured with Balanced and Optimise Search; every file installed byte for byte
(results/pdf-2026-11-08.md):

| PDF | Before | After | Change |
| :--- | ---: | ---: | ---: |
| arXiv 1512.03385 | 708,231 | 416,529 | −41.2% |
| arXiv 1706.03762 | 1,030,038 | 527,623 | −48.8% |
| arXiv 2005.14165 | 5,485,411 | 2,827,052 | −48.5% |
| archive.org scan (JBIG2, JPX) | 19,247,926 | 19,112,191 | −0.7% |
| archive.org scan 2 | 5,482,450 | 5,482,450 | not opened |
| WoWRevived manual | 8,422,844 | 6,894,364 | −18.1% |

**Making is slower** where a PDF has many streams. arXiv 2005.14165 took 11 s before and 498 s now,
mostly the rope's search over its 343 members. The command line has no time limit per file unless
`--search-per-file` gives one; in the File Locator, the time for a file bounds it.

**Other changes:**
- The oracle reads the listing and every member. It has no preflate, so it reports the PDF as not
  made again by the oracle, as it does for a zip's deflate.
- CI step 11 writes an 8-page PDF with a JPEG and checks it through sieve and sieve-install.

**Sound measured, not built:** mu-law and A-law are not worth it. Sieve's mixing on the 8-bit codes
beats FLAC on the widened samples (1,026,561 vs 1,614,712 bytes, and 162,491 vs 236,962).

## f3b7f25: `--sound-to-flac` in the hallway's usage text

This fixes the failure in your CI run: step 11's help check found the option read by the client but
missing from its usage text. tools/check_help.py now reports 0 missing.

Unit suite: 154,388 checks, 0 failures. The PDF and FLAC blocks of CI step 11 pass here.

## Next

Float sound whose every value is a whole 24-bit step, through FLAC (WAV, AIFF-C fl32, CAF). It
measured 21% to 24% under Sieve's ways. After that, compressed TIFF.
