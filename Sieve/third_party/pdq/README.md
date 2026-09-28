# PDQ: the perceptual hash for pictures

The C++ reference implementation of PDQ, by Meta Platforms, under the BSD licence (`LICENSE`
here, and `third_party_licenses/pdq/LICENSE`), from github.com/facebook/ThreatExchange, folder
`pdq/cpp`, at commit ec3671bafc70136c4178ac300b75281ee4c6a0c6 (2026-09-25). The vault matches
pictures with it (`docs/VAULT.md`); Sieve's only door to it is `tools/cli/pdq_hash.*`.

Only what hashing needs is here, as upstream wrote it, in upstream's own folders so its includes
(`<pdq/cpp/...>`) work unchanged: `common/pdqbasetypes.h`, `common/pdqhamming.h` (with
`USE_BUILTIN_POPCOUNT`, as upstream sets it, so the lookup-table `pdqhamming.cpp` is not needed),
`common/pdqhashtypes.*`, `downscaling/downscaling.*`, `hashing/pdqhashing.*`, `hashing/torben.*`.
Not taken: the image loading (CImg; Sieve reads pictures with stb_image), the command-line tools,
the index (MIH) and the regression data.

Checked against Meta's own results: a PNG hashes bit for bit as Meta's `pdqhash` package gives it
(CI checks `tests/example_book_cover.png`); JPEGs land within a few bits of Meta's regression
outputs, the difference being the JPEG decoder (stb_image against CImg's), well inside PDQ's
match threshold of 31 bits.

To update: copy the same files from a newer `pdq/cpp`, and note the commit here.
