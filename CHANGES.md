# preflate guarded, and the memory estimate at 512 MiB (7 November 2026)

Two commits on top of e770cb7: 24c1cd7 and c8996bc.

## preflate's decodeBits, guarded (Edward's call)

- CMake now adds two guards to the fetched `support/arithmetic_coder.h`: a bit count of 32 or more,
  or a step of 0, makes `decodeBits` return 0. Before, a damaged count shifted past the width
  (undefined in C++). A count of 31 could also leave the step at 0, which the next line divides by;
  that is why the proposed one-line fix alone was not enough.
- Valid streams never ask for more than about 16 bits, so nothing they make changes. Damaged ones
  are still refused.
- The patch step handles a Windows checkout's CRLF, is applied only once, and stops configuring if
  preflate's text is not what it expects.
- The patched lines are marked `SIEVE_PATCHED`, and third_party_licenses/README.md says so.
- The fuzzer's suppression for this report is removed (`tools/fuzz/ubsan.supp`).
- Checked: CI's png25 pair was made again through preflate and installed byte for byte the same.

## The File Locator's memory estimate at 512 MiB

- enwik9's first 512 MiB, with Optimise Search ticked: 5,319 MiB at the most against the estimate's
  5,258 (1.2% under).
- Made in 2,680 s, 99,749,675 bytes; installed in 676 s at 1,869 MiB, and the same bytes.
- The estimate has now held at 64, 256 and 512 MiB, so it is unchanged. The figures are added to
  results/roof-enwik9-2026-11-06.md.
