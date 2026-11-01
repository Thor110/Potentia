# Tar: sizes after leading spaces (commit ec62d61)

## What was wrong with mozilla

mozilla is an old GNU tar. It writes its file sizes as octal after leading spaces
(`"     120000 "`), and Sieve read every one of them as 0. That put the next header in the
wrong place, its checksum failed, and the tar was never taken apart on either box. The reader
now skips the leading spaces. That change is in `tools/cli/pack.cpp`.

## mozilla, measured after the fix

All runs used `--search-per-file 0`, and every one installed back, either as the tar byte for
byte or as its 525 files.

| Way | `.sieve` |
| :--- | ---: |
| Temporary, as the packer weighs it (kept whole) | 13,376,239 |
| Permanent, as the packer weighs it (kept whole) | 13,376,239 |
| Temporary, forced apart (`SIEVE_ARCHIVES_ALWAYS=1`) | 13,794,627 |
| Permanent, forced to its files (`SIEVE_ARCHIVES_ALWAYS=1`) | 13,781,869 |

So both boxes now see inside mozilla and choose to keep it whole, because apart is about 3%
bigger. Taking a tar apart only pays where its programs go through the x86 filter, and
mozilla's programs are for Alpha processors (Tru64 COFF), so the filter doesn't help them.

## Tested

- The unit suite: 77,110 checks, 0 failures.
- CI step 11's new block: a tar with space-padded sizes is taken apart and made again byte for
  byte by Sieve and the oracle.
