# Making's memory cut, timings in At a glance, WoWRevived's figures, and the rope post (7 November 2026)

Seven commits on top of a2c558a (the catch-up zip). Unpack over the repository.

- 078667b Making's memory, first pass: the same bytes in less
- 4c91ab8 STATISTICS: WoWRevived at v33, Balanced, with 7z and zip timed (Edward's run)
- f07dfdb STATISTICS: times in At a glance, Sieve's and 7z's, and where each was timed
- 96342a2 Making's memory, second pass: the read-back's bytes and the ways cache
- ef28fe9 Making's memory, third pass: the files not held twice at the read-back
- 5f52a13 docs/ROPE-POST.md: the rope and the state space, a draft for encode.su
- 7e5854d results: making's memory, cut; WoWRevived now made here (8,547 MiB)

## Making's memory (results/making-memory-2026-11-07.md)

The same `.sieve` byte for byte, in less memory:
- **WoWRevived, whole:** now made on the 16 GB machine here, at 8,547 MiB at the most. Before, it
  was killed at 13.3 GiB.
- **A 528 MB part of it:** 3,931 MiB, from 4,750.
- **A 190 MB part:** 2,183 MiB, from 2,878.

The changes:
- The read-back runs after the packer's buffers are let go.
- Reading a compact `.sieve` no longer holds several copies of its streams. Installing gains too.
- The files aren't held twice at the read-back: they're checked against their SHA-256s and taken
  back from it.
- The ways cache keeps settings and sizes, not codes. It's capped at 64 MiB, where it held up to
  2 GiB.
- The pieces cache keeps where the cuts fall, not a copy of each file.
- The rope's estimate counts in one pass, in 32 bits.

WoWRevived made for size with Optimise Search on this build: 1,005,544,163 bytes, 6.35% under 7z.
Made in 4,224 s; installed in 221.3 s, every file the same.

Not changed yet: the File Locator's memory estimate now says more than these folders take (12.6 GiB
for WoWRevived, which took 8.5). It is to be fitted again after enwik9's prefixes are measured on
this build.

## STATISTICS

- **"At a glance":** each row now has the `.sieve`'s make and install times, 7z's make and unpack
  times, and which machine timed them. Rows timed here today made the same bytes as their figures.
- **WoWRevived:** your Balanced run is added, with 7z and zip timed.

## docs/ROPE-POST.md

A draft for encode.su:
- where Sieve comes from, dated by your GOB and Potentia commits;
- why filtering the state space saves almost nothing, with each line's measured share converted to
  bits;
- how the rope works, and its nearest relatives;
- what it does, measured: WoWRevived's seven WAVs at 83.1% of LZMA2's code, and where it loses.

Also: IDEAS §19.116, WoWRevived's film packets measured (in the catch-up zip already).
