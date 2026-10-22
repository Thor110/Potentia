# Maps are Sieve instructions now: `.map` deprecated, the release's map is `source.sieve` (22 October 2026)

## Built
- **One format** (IDEAS §19.93, SPECIFICATIONS §12.2 and §12.3). A map is written as Sieve
  instructions, `sieve-manifest-v18`: v17 with a **graph part**, the map's own text (nodes, edges,
  notes, the seal) as lines `g<TAB>…` after the entries. In the binary head, the version byte is 18
  and flag 64 marks the graph part. Older readers refuse it, and ordinary instructions are exactly
  the bytes they were. The files a map holds are the instructions' own files, packed like any
  others, so `sieve install` puts them back. Files the map only names are in the text alone, and
  installing says how many there are. Measured: a map naming a 6-byte and a 3,000-byte file came
  to 271 bytes; the same map holding a 200,000-byte file of noise as well, sealed, came to 200,353.
- **`sieve map`** writes `.sieve` only. `--out NAME.map` is refused with a message saying why.
  It reads a folder, some files, any `.sieve` or an older `.map`. `sieve map OLD.map --out NEW.sieve`
  converts an old map, and the text stays the same. The help and the README now say `.sieve`.
- **Any `.sieve` opens as a node graph**, as you suggested. One without a graph part (made by
  `sieve locate`) shows the tree it carries, every file named by its SHA-256, read-only. It has no
  notes, which is fine, as you said.
- **The hallway** (`client/node_graph.cpp`). `maps/` lists `.sieve` and `.map` files. Any change
  (V, Remove from map, a new map, export) is saved as `.sieve`. A changed `.map` is written beside
  it as a `.sieve` and read from there; the `.map` itself is left alone.
- **The release** (`tools/make_release.py`). It publishes `source.sieve` instead of `sieve.map`,
  and checks that installing it gives the source archive back byte for byte. A bug was fixed on
  the way: `--map-with` used a name it never defined, so it would have stopped with an error. It
  now holds the source again, as the published map does.
- **The oracle** reads v18 and `.sieve` files directly. `graph FILE` prints a map's text, and
  `map … --text` writes the text alone.
- **CI.** The map checks now make `.sieve` files and compare their text with the oracle's.
  They install the held files with both readers, read a plain `.sieve` as a map, and convert an
  older `.map` to the same text. The hallway's anchor checks (V, META, Delete, sealed) use `.sieve`.

## Checked here
- Unit suite: 0 failures.
- Every workflow step from 6 onwards passed locally (Linux), with the hallway checks included.
- A libc++ build (as on a Mac) makes the same map text, and installs the held file back.

## Worth knowing
- If `maps/` holds both `finds.map` and the `finds.sieve` it became, both are listed. Delete the
  `.map` once you're happy with the `.sieve`.
- The old `sieve.map` from published releases still opens; only new releases use `source.sieve`,
  with a new SHA-256.

**Docs:** SPECIFICATIONS §12.2 and §12.3, IDEAS §19.93 (marked built), README, HANDOFF, VOICES
(your two quotes from today).

To apply: copy `.github/` and `Sieve/` over the repository's top folder. There are two new
files: `Sieve/tools/cli/map_sieve.cpp` and `map_sieve.hpp`.
