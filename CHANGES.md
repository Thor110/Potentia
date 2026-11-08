# Changes (commit 9f79bd5, 8 November 2026)

The FLAC way, in the v34 draft. Replace these files in the repository (paths from its root):

New:
- Sieve/tools/cli/flac_wav.hpp, Sieve/tools/cli/flac_wav.cpp: a WAV opened (its samples coded by
  libFLAC, the rest of the file kept as it was) and made again byte for byte.
- Sieve/results/flac-way-2026-11-08.md: the settings chosen, WoWRevived's seven WAVs, the checks.
- Sieve/third_party_licenses/flac/COPYING.Xiph: libFLAC's licence (BSD 3-clause).

Changed:
- Sieve/CMakeLists.txt: libFLAC 1.4.3 fetched from github.com/xiph/flac and built as a static
  library (its library alone); linked into sieve_compare (so sieve and sieve-install have it).
- Sieve/tools/cli/pack.cpp, pack.hpp: the WAV opened under Compressed formats temporary or Optimise
  Search, with --v34-draft; libFLAC's level by the priority; the installer's remake; the report's
  "wavs" line; a WAV weighed first on a sample; a FLAC stream keeps the search's shortcuts.
- Sieve/tools/cli/locate.cpp: the reader knows "flac" (format number 26, three recipe words).
- Sieve/tools/sieve_cli.cpp: the v34 draft's description names WAVs.
- Sieve/reference/sieve_ref.py: the oracle decodes FLAC itself (RFC 9639) and makes the WAV again.
- Sieve/ci/steps/11-oracle.sh: six WAVs written and checked (Sieve, sieve-install, the oracle;
  Decoding speed; none without the draft).
- Sieve/tools/fuzz/fuzz_reader.py: flac recipes among the fuzzer's ways.
- Sieve/docs/SPECIFICATIONS.md, FORMATS.md, NEXT-VERSION.md, IDEAS.md (19.132), ROPE-POST.md,
  Sieve/third_party_licenses/README.md.

To try it: sieve locate FOLDER --installer OUT.sieve --v34-draft --optimise-search --priority size
(or balanced, speed). In the File Locator: the v34 draft on, Optimise Search ticked.

The first configure fetches libFLAC (a shallow git clone, as the other libraries are).
