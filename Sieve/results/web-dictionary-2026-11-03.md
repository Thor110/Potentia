# A web dictionary, learnt and weighed (3 November 2026)

IDEAS §19.92's web dictionary, measured again on today's sources. On 22 October a dictionary
learnt from a 2014 snapshot, used as LZMA2's preset dictionary, came to 0.5% under brotli on unseen
pages and 6.6% over it on Edward's. This time the dictionary was also tried as mixing's primer: the
model reads the dictionary first, coding nothing, then codes the page.

## The sources

Learnt from (each with a licence that lets a dictionary made from it ship with Sieve):

| Set | What | Licence | Bytes (64 KiB pieces, copies left out) |
|---|---|---|---|
| mdn | MDN's example repositories: html-examples, css-examples, dom-examples, js-examples, webaudio-examples, web-components-examples (HTML, CSS, JS) | CC0 | 4,394,372 |
| libs | npm: bootstrap 5.3.8, jquery 4.0.0, normalize.css, html5-boilerplate, bulma, foundation-sites, purecss, milligram, water.css, modern-normalize, htmx.org, alpinejs, vue, react-dom, lodash, tailwindcss, materialize-css | MIT, BSD-3-Clause, 0BSD | 32,719,367 |
| both | the two together | | 37,113,739 |

Edward suggested mdn/html-examples (CC0), and the other MDN repositories carry the same licence.
Each set was learnt into 64 KiB and 128 KiB dictionaries by zstd's trainer (python-zstandard 0.25.0).

Tested on, never learnt from (no test page is a copy of any learning file, by SHA-256):

- **2014:** the 258 pages of the npm package htmlparser-benchmark (1.1.3), real sites as saved in
  2014, 24,580,483 bytes;
- **Edward:** his site's 11 pages (index.html and Pages/), 113,176 bytes;
- **mdn-la:** MDN's learning-area repository, 154 pages, 281,821 bytes. These are written in the same
  house style as the mdn set, so they flatter it: the first two are the fair tests.

## How each page was weighed

- **brotli:** quality 11, its default window (python brotli), with its own 120 KiB built-in dictionary.
- **LZMA2:** raw, preset 9 extreme.
- **mixing:** `sieve mix --words 80 --by-use --params "B20 F80"` (mix-v4 without v33's match ways).
- **With a dictionary:** the size of dictionary + page, less that of the dictionary alone, the same
  coder carrying on. For mixing this is exactly what a primed model codes. For LZMA2 it stands in for
  a preset dictionary, within a few bytes.
- **Sieve:** the whole `.sieve` that `sieve locate PAGE --installer` makes today (v33): name, SHA-256,
  listing and all.

## Results

Bytes, all pages of each set together; the percentage against brotli; and on how many pages each
was smaller than brotli.

| | 2014 (258) | Edward (11) | mdn-la (154) |
|---|---|---|---|
| brotli | 4,194,084 | 32,518 | 71,656 |
| LZMA2 | 4,665,092 (+11.2%, 0) | 39,210 (+20.6%, 0) | 98,520 (+37.5%, 0) |
| LZMA2 + mdn 128 KiB | 4,419,028 (+5.4%, 0) | 32,484 (−0.1%, 7) | 56,482 (−21.2%, 150) |
| LZMA2 + both 128 KiB | 4,409,133 (+5.1%, 0) | 32,198 (−1.0%, 8) | 58,762 (−18.0%, 149) |
| mixing | 3,895,568 (−7.1%, 252) | 31,324 (−3.7%, 8) | 74,602 (+4.1%, 25) |
| mixing + mdn 64 KiB | 3,748,231 (−10.6%, 258) | 26,647 (−18.1%, 11) | 46,379 (−35.3%, 154) |
| mixing + mdn 128 KiB | 3,748,471 (−10.6%, 258) | 26,575 (−18.3%, 11) | 44,982 (−37.2%, 154) |
| mixing + libs 64 KiB | 3,793,816 (−9.5%, 258) | 27,596 (−15.1%, 11) | 56,445 (−21.2%, 153) |
| mixing + both 64 KiB | 3,770,462 (−10.1%, 258) | 27,043 (−16.8%, 11) | 51,838 (−27.7%, 154) |
| mixing + both 128 KiB | 3,771,290 (−10.1%, 258) | 26,739 (−17.8%, 11) | 49,287 (−31.2%, 154) |
| Sieve today, the whole `.sieve` | 3,955,146 (−5.7%, 221) | 33,911 (+4.3%, 2) | 100,888 (+40.8%, 2) |

Edward's pages one by one (bytes):

| Page | Size | brotli | Sieve today | mixing | mixing + mdn 64 KiB |
|---|---|---|---|---|---|
| index.html | 4,858 | 1,455 | 1,505 | 1,407 | 1,091 |
| Pages/audio.html | 6,345 | 1,840 | 1,976 | 1,854 | 1,441 |
| Pages/metrics.html | 19,084 | 5,577 | 5,561 | 5,294 | 4,637 |
| Pages/quest.html | 6,924 | 1,863 | 1,930 | 1,818 | 1,491 |
| Pages/release.html | 29,841 | 8,147 | 7,948 | 7,546 | 6,969 |
| Pages/secure/gallery.html | 3,695 | 1,297 | 1,696 | 1,395 | 1,092 |
| Pages/secure/index.html | 3,696 | 1,308 | 1,743 | 1,396 | 1,125 |
| Pages/secure.html | 7,170 | 2,114 | 2,184 | 2,057 | 1,676 |
| Pages/sieve.html | 3,878 | 1,121 | 1,510 | 1,120 | 815 |
| Pages/space.html | 12,173 | 3,215 | 3,227 | 3,050 | 2,501 |
| Pages/specs.html | 15,512 | 4,581 | 4,631 | 4,387 | 3,809 |

## What it says

- **Primed mixing is the way.** On the fair tests, mixing primed on the mdn 64 KiB dictionary came
  out 10.6% under brotli on the 2014 pages and 18.1% under on Edward's. It was smaller than brotli on
  every one of the 423 test pages. As LZMA2's preset dictionary, the same dictionary only brings
  LZMA2 level with brotli on Edward's pages (+0.5%) and leaves it 5.9% over on the 2014 pages.
- **mdn, 64 KiB.** The CC0 set did best or nearly best on all three tests, 128 KiB gained little over
  64 KiB for mixing, and CC0 asks nothing of what is made from it. The libraries (MIT and the like)
  did worse alone and added nothing to mdn.
- **Sieve today loses its lead on small pages.** It doesn't mix a file under 4 KiB, so Edward's
  sieve.html is 1,510 bytes where mixing alone makes 1,120. Its fixed cost is small: a 1-byte file's
  `.sieve` is 51 bytes. Most of the gap between "Sieve today" and "mixing" is the way chosen: LZMA2
  under 4 KiB, and smaller tables than B20 above it.
- **The cost:** priming reads the dictionary through the model, about 0.11 s for 64 KiB here, on
  packing and on installing. That is paid once per mixed stream, so a group of pages pays it once.

## What building it would take

A new format (mix-v4 primed, with the dictionary registered by its SHA-256 as the word lists are),
so a draft version; the oracle priming its own model the same way; mixing tried on web text under
4 KiB; and the dictionary shipped in `data/dictionaries/`.

Repeatable with `tools/measure_web_dictionary.py` and the sources above. The trainer gave the same
mdn 64 KiB dictionary on three runs (SHA-256 3902a572…7fa0b, 65,536 bytes).
