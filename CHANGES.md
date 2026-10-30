# Similar files measured, and v32 as one version (30 October 2026)

Unzip over the repository root. This goes on top of the last zip ("The release notes"). Committed
locally (not pushed). Documents only: nothing to rebuild.

## Would finding similar files need a "contains similar files" box?

No: it's cheap. I measured it with a prototype (outside Sieve) on a pair the size of MAPS.WoW:

- **A:** 160,000,000 bytes, built from the repository's own files.
- **B:** A with 40 regions of 200 KB replaced by random bytes, and 30 insertions of up to 5,000 bytes,
  so everything after an insertion is shifted.

| Step | Time | Memory | Result |
| :--- | ---: | ---: | :--- |
| Telling they're alike: a fingerprint of each block, with blocks cut where the content says | 0.71 s | under 1 MB (117,234 fingerprints) | 96.6% of B's blocks found in A |
| Writing B as copies from A | 2.69 s | about 92 MB of index, beside the two files' 308 MB | 332 copies covering 152,101,975 bytes; 7,966,102 new bytes (the random ones) |

Two things decide it:

- **The packer already holds every file of a folder in memory** (`add_packed`), so nothing is
  loaded twice.
- **The index is built only for a pair already found alike**, so a folder with no such pair pays
  only the fingerprints.

The alternative within v31, a larger LZMA2 dictionary, needs about twelve times itself to encode:
about 2.25 GiB for the 192 MiB MAPS.WoW would want.

One lesson from the prototype: blocks cut at fixed places found only 4.4%, because one insertion
shifts every block after it. They have to be cut where the content says. That is now written down.

Recorded in `Sieve/docs/IDEAS.md` §19.106.

## The plan updated: v32 as one version (`Sieve/docs/NEXT-VERSION.md`)

Your decisions are in: the pause ends with work on v32, JPEG recoding is in, and similar files are
found automatically.

**Keeping it to one version.** v32 gets built behind a draft switch (`--v32-draft`, for testing
only). Without it every build keeps writing v31, as now, until v32 is complete, specified, read by
the oracle, and frozen. Then it becomes the newest in one step, and no half-made v32 file ever goes
out.

**The order to build:**
1. the `delta` way;
2. a small file's overhead;
3. the archive's format as a number;
4. JPEG, the largest, last so it doesn't hold up the rest;
5. BCJ2 and word frequencies, only if measuring earns them a place.

## Files
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/NEXT-VERSION.md`
