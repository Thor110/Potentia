# Compressed TIFFs opened: deflate, LZW and PackBits strips (v34 draft, 8 November 2026)

Edward (8 November 2026): "work through all of the things on that list next and make sure licenses
are compatible as per usual". Third on docs/FORMATS-PLAN.md.

**Measured first.** Six pictures (two 1920 by 1080 screenshots, the 949 by 1300 Vitruvian scan, three
800 by 450 photographs) written by ImageMagick 6.9.12 (libtiff) as deflate, LZW and PackBits TIFFs, the
first two with the horizontal predictor, as ImageMagick writes them. Sieve carried the deflate and LZW
ones nearly as they were (each within 4% of its size); the same pictures as uncompressed TIFFs came to
2 to 5 times less.

**Built.** `tiff_skeleton` (tools/cli/pack.cpp) walks every image of the TIFF and opens each strip or
tile whose coding gives it back: deflate through preflate (as a zip's), LZW and PackBits as libtiff
codes them (`tools/cli/tiff_codec.cpp`, written for Sieve from libtiff 4.7.0's algorithms; nothing of
libtiff linked; its licence, permissive, in third_party_licenses/libtiff), and the predictor undone so
that the pixels themselves are carried. A new recipe, `tiff <skeleton size> <inner size>` (format 27),
v34's `sk` with three more kinds of segment; mixing given the image's width from its tags. libtiff's
LZW gave back every strip of the twelve ImageMagick wrote here, and of the six Pillow (libtiff 4.7.1)
writes in CI.

**Measured**, each alone, `--priority size --optimise-search`, v34 draft against without it (before);
every one installed byte for byte.

| TIFF | Bytes | Before | After | | Made in | Installed in |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| hallway screenshot, LZW | 739,978 | 725,735 | 157,371 | −78.3% | 16.3 s | 4.2 s |
| hallway screenshot, PackBits | 2,663,710 | 189,613 | 157,342 | −17.0% | 16.1 s | 3.8 s |
| hallway screenshot, deflate | 403,138 | 388,799 | 158,045 | −59.4% | 16.0 s | 4.2 s |
| AI screenshot, LZW | 475,802 | 458,344 | 115,456 | −74.8% | 14.4 s | 3.5 s |
| AI screenshot, PackBits | 5,261,368 | 168,518 | 115,437 | −31.5% | 16.4 s | 3.8 s |
| AI screenshot, deflate | 378,990 | 366,613 | 116,336 | −68.3% | 14.9 s | 3.9 s |
| Vitruvian, LZW | 2,230,642 | 2,192,139 | 1,355,520 | −38.2% | 27.0 s | 3.9 s |
| Vitruvian, PackBits | 3,731,108 | 1,348,691 | 1,355,503 | **+0.5%** | 26.9 s | 3.8 s |
| Vitruvian, deflate | 1,956,330 | 1,956,161 | 1,355,837 | −30.7% | 26.4 s | 4.6 s |
| book, LZW | 205,418 | 195,332 | 122,780 | −37.1% | 4.5 s | 0.3 s |
| book, PackBits | 221,392 | 137,423 | 122,771 | −10.7% | 4.2 s | 0.3 s |
| book, deflate | 161,952 | 161,998 | 122,796 | −24.2% | 4.7 s | 0.4 s |
| door, LZW | 423,154 | 410,949 | 188,295 | −54.2% | 7.3 s | 0.9 s |
| door, PackBits | 1,057,534 | 217,713 | 188,285 | −13.5% | 9.1 s | 0.8 s |
| door, deflate | 289,006 | 288,946 | 188,360 | −34.8% | 9.1 s | 0.9 s |
| picture-in-hand, LZW | 328,222 | 320,299 | 154,853 | −51.7% | 8.6 s | 0.8 s |
| picture-in-hand, PackBits | 915,246 | 182,745 | 154,850 | −15.3% | 9.4 s | 0.8 s |
| picture-in-hand, deflate | 253,990 | 253,761 | 154,932 | −38.9% | 8.0 s | 0.9 s |

All 18 together: 9,963,779 bytes before, 6,284,769 after (−36.9%).

**One loss.** The PackBits scan came out 0.5% larger: mixing already did well on its runs as they
were, and the estimate that decides whether to open a TIFF cannot tell a difference so small. Weighing
mixing's estimate of the file as it is as well (tried) did not change the choice, so it was not kept.

**Checked** by CI step 11: Pillow's TIFFs (LZW with and without the predictor, deflate with it,
PackBits, 16-bit grey, three pages in one file) through Sieve, sieve-install and the oracle, which
codes LZW and PackBits itself from the specification; an uncompressed TIFF and one whose strips
overlap left alone; nothing opened without the draft.
