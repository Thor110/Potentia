# Your photos: brunsli wins there too (30 October 2026)

Unzip over the repository root, on top of the last zip ("JPEG: brunsli and JPEG XL measured").
Committed locally (not pushed). Documents only.

## The originals

The zip kept your originals intact; the pictures posted in the chat had been resized and re-encoded
on the way. All three match STATISTICS to the byte, including 4,139,682 for `20230830_163945.jpg`,
so you did pick the right version of that one.

I measured them in a scratch folder and deleted them afterwards. Nothing from them is in the
repository but the figures.

| | Original | `.sieve` now | brunsli | `cjxl --lossless_jpeg=1` |
| :--- | ---: | ---: | ---: | ---: |
| A picture at 3653×5480 | 3,844,213 | 3,716,747 | **3,085,201 (−19.74%)** | 3,109,231 (−19.12%) |
| 20230830_163945.jpg | 4,139,682 | 4,111,478 | **3,284,887 (−20.65%)** | 3,312,460 (−19.98%) |
| 20240407_153939.jpg | 4,587,473 | 4,557,978 | **3,660,016 (−20.22%)** | 3,703,771 (−19.26%) |

- **Exact:** every file came back byte for byte from both.
- **brunsli:** makes each in 0.6–0.9 s and gives it back in about 0.5 s.
- **`cjxl`:** takes about 6.3 s to make and 0.2 s to give back.

So brunsli is the smaller on all ten original JPEGs measured (the repository's seven and your
three), about ten times quicker to make, and about a tenth of libjxl's size in `sieve-install`.
`cjxl` led only on two quality-90 re-saves. The recommendation is **brunsli**.

## Old figures corrected in STATISTICS

The `.jpg.sieve` sizes were measured again with this build:

| | was | now | against 7z |
| :--- | ---: | ---: | ---: |
| The 3653×5480 picture | 3,826,046 | **3,716,747** | −3.31% |
| 20230830_163945.jpg | 4,111,592 | 4,111,478 | −0.39% |
| 20240407_153939.jpg | 4,558,132 | 4,557,978 | −0.38% |

The `.7z.sieve` column and the sizes on disk are from the first measurement; I didn't have the
`.7z` files to measure again.

## Next

If you agree to brunsli, I'll build the `jpg` way into v32's draft:

- brunsli fetched at a pinned commit, as preflate is;
- the packer keeps it only where the JPEG comes back exact and smaller;
- `sieve-install` gets the decoder;
- the specification, the oracle (which reads the recipe), and CI.

## Files
- `Proof/STATISTICS.md`
- `Sieve/docs/NEXT-VERSION.md`
