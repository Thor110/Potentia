# JPEG: brunsli and JPEG XL measured (30 October 2026)

Unzip over the repository root, on top of the last zip ("JPEG: the plan begun"). Committed locally
(not pushed). Documents only.

With GitHub open, I built Google's **brunsli** (MIT) and JPEG XL's **`cjxl`/`djxl`** (libjxl,
BSD-3) here from source. I ran each on the repository's seven JPEGs, and on two quality-90 re-saves
of Vitruvian (one progressive, one with optimised tables). **Every file came back from both byte
for byte.**

| | JPEG | `.sieve` now | brunsli | `cjxl --lossless_jpeg=1` |
| :--- | ---: | ---: | ---: | ---: |
| Vitruvian | 207,500 | 206,956 | 170,260 (−17.95%) | 173,542 (−16.37%) |
| audio | 70,028 | 66,894 | 57,310 (−18.16%) | 57,812 (−17.44%) |
| book | 82,905 | 80,569 | 67,684 (−18.36%) | 67,978 (−18.00%) |
| door | 69,298 | 63,860 | 54,117 (−21.91%) | 55,341 (−20.14%) |
| filters | 70,029 | 65,997 | 55,468 (−20.79%) | 56,318 (−19.58%) |
| picture-in-hand | 59,365 | 55,013 | 46,757 (−21.24%) | 47,910 (−19.30%) |
| vitruvian (site) | 116,168 | 116,235 | 98,454 (−15.25%) | 99,986 (−13.93%) |
| Vitruvian, progressive q90 | 282,087 | | 212,680 (−24.60%) | 196,754 (−30.25%) |
| Vitruvian, optimised q90 | 284,505 | | 212,027 (−25.48%) | 195,676 (−31.22%) |

- **brunsli** is a little smaller on the originals (by 0.4 to 1.7 points).
- **`cjxl`** is about 6 points smaller on the high-quality re-saves.
- **Speed:** both decode in hundredths of a second. To encode, brunsli takes 0.02–0.05 s and `cjxl`
  0.15–0.69 s.
- **Size of the decoder in `sieve-install`:** brunsli's library is 435,904 bytes; libjxl's is
  4,772,216, plus highway, skcms and brotli.

## Your decision: which way

- **(a) brunsli in Sieve. Recommended.** Fetched at a pinned commit, as preflate is. Small, fast,
  and the better on these originals. The oracle can read its recipe but not make the file, as with
  preflate.
- **(b) libjxl.** Better on high-quality JPEGs, but about eleven times the size in `sieve-install`.
- **(c) Sieve's own model.** The oracle could check it fully, but it's the most work, and the
  general-model test showed nothing comes cheap.

Your camera photos are high quality, which is where `cjxl` led. So one more measurement would settle
(a) against (b):

```
cjxl photo.jpg photo.jxl --lossless_jpeg=1
```

on your two photos, with JPEG XL's Windows release.

Recorded in NEXT-VERSION §4a (3) and IDEAS §19.88.

## Files
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/NEXT-VERSION.md`
