# Ways tally, PROOF.md links, docs pass (4 November 2026)

Commit 3342b46 (local, not pushed), on top of your 898169f.

## The tally: Sieve/tools/ways_tally.py

    python tools/ways_tally.py FOLDER_OR_SIEVES... --sieve sieve.exe --md out.md --tsv out.tsv

- **What it reads:** which way carried each file, straight from each `.sieve`'s listing. That covers Sieve's own ways, LZMA2, and files made again around an inner way (brunsli, archives, PNGs, base64 decoded). Each way is labelled with whose it is.
- **Its own check:** every code of a file's own is checked to add up to the rope stream exactly.
- **With `--sieve`:** each file with a code of its own is set against LZMA2 alone on the same bytes. A mixed group is set against LZMA2 on its files joined.
- **Run here, all with Try every way:** the twelve Silesia files, Sieve's docs folder and the web test site. Mixing carried all twelve Silesia files: 41,710,177 bytes against LZMA2's 48,455,266 on the same files (−13.92%).
- **Where the figures went:**
  - `Proof/STATISTICS.md` has a new section, **What carried each file**: who owns each way, the Silesia table, and your bench's results by setting.
  - `Sieve/results/ways-tally-2026-11-04.md` has the full output.
- **Run it on your own `.sieve` files** (MAPS, ALTEngine, WoWRevived, your bench) to add them.

## PROOF.md

A `# Specifications` section at the end, with links to SIEVE-FORMAT, METHODS, SPECIFICATIONS, FILTER-PLUGINS, STATISTICS and the oracle. Your proof above it is unchanged.

## Docs pass

- **SPECIFICATIONS.md:** retitled "Sieve Specifications". Its first line now says Sieve builds and walks the Gallery of Babel, the space, rather than naming Sieve as the Gallery.
- **"Gallery of Babel" elsewhere:** the READMEs and VISION.md already use the name for the space, so they are unchanged.
- **The format now named as v33:** in Sieve/README.md and SIEVE-INSTALL-USAGE.md.
- **NEXT-VERSION.md:** its title, and its line in the README's table, now cover v34 begun and the plans for v32 and v33.
- **Release figures:** both READMEs now use v32's measured release (6,470,236 of 45,967,736 bytes, 7.10:1) in place of v31's. STATISTICS' table now says which row the READMEs use.
- **STATISTICS:** the pi6 row is labelled "v32 (v33 the same)".
