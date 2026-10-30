# 0.13.77, and a plan for the next manifest version (30 October 2026)

Unzip over the repository root. This goes on top of the last zip ("Every way tried"). Committed
locally (not pushed). Only the version number changes in code: rebuild if you want the programs to
say 0.13.77.

## Version 0.13.77

Set in all seven places that carry it, so the next build is kept apart from your 0.13.76 release:

- `CMakeLists.txt`
- `vcpkg.json`
- the extension's `manifest.json` and `web/README.md`
- `make_release.py`'s examples
- SIEVE-INSTALL-USAGE
- VISION

Rebuilt here: `sieve --version` says `sieve 0.13.77`. No test or CI step pins the version.

## The plan: `Sieve/docs/NEXT-VERSION.md` (new)

I went over the documentation (IDEAS, HANDOFF, REVIEW, SHOWCASE-PLAN, SPECIFICATIONS, SIEVE-FORMAT,
STATISTICS) and the code. The plan has six parts:

1. **Where things stand**: the freeze in your words, and where Sieve is weakest now, with the figures.
2. **The rules for v32**, gathered from the documents:
   - one version for several ideas;
   - measure first;
   - every older version still read;
   - a file names the oldest version that holds what it uses;
   - the oracle written from the specification;
   - decoders pinned;
   - JPEG and the like last, by your order.
3. **Every candidate**, with what it has measured, in four groups:
   - ready to design: the `delta` way, and a small file's overhead;
   - a lead to measure first: BCJ2, word frequencies for mixing, mixing as a kind of piece, decoding
     one file alone, the web dictionary;
   - small, or measured as not worth a version alone: the archive as a number, headers modelled, the
     rope's next contexts, other lines' places;
   - large and last: JPEG, a sound model, zlib's PNGs.
4. **Two shapes for v32:**
   - small: the overhead, and the archive as a number;
   - with `delta`: if WoWRevived's log shows that a larger dictionary or grouping isn't enough for
     MAPS.WoW.
5. **The order of work:**
   - measure first, with no version: WoWRevived's log, a byte breakdown of `control.py`, BCJ2 outside
     Sieve, Silesia;
   - then you choose;
   - then the specification;
   - then the build, with a table of every place to change, taken from the commits that added v30
     and v31;
   - then the proof;
   - then freeze again at v32.
6. **Five decisions for you.**

One thing to plan around: **the head's flags byte is full.** All eight bits are in use, so a new
optional part needs the version byte to say so, or a second flags byte. IDEAS still said two bits
were free; I've added a note there.

The plan is linked from IDEAS's waiting list, HANDOFF and the README.

## Don Quixote, corrected in `Proof/STATISTICS.md`

Going over the documents turned up an old figure. STATISTICS gave Don Quixote as 683,205 bytes
(−0.41% against 7z), which is about v23's size, from before mixing. I measured it again (it's in
the repository's public-domain anchors): **533,148 bytes**, installed back identical. That is
exactly HANDOFF's v28 figure, and **22.28% under 7z** (4.49:1).

The "at a glance" row and the section now say so. Making it took 13.3 s and installing it 5.3 s, on
the Linux machine here. Its size on disk wasn't measured, so that cell is empty.

## Files
- `Proof/STATISTICS.md`
- `Sieve/CMakeLists.txt`
- `Sieve/README.md`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/NEXT-VERSION.md` (new)
- `Sieve/docs/SIEVE-INSTALL-USAGE.md`
- `Sieve/tools/make_release.py`
- `Sieve/vcpkg.json`
- `Sieve/web/README.md`
- `Sieve/web/extension/manifest.json`
- `VISION.md`
