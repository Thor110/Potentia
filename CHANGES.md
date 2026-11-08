# Changes (commits 8065ed6 and 14f4fb4, 8 November 2026)

Replace these files in the repository (paths from its root):

**AIFF and AIFF-C through FLAC (v34 draft), by the same "flac" recipe** (the skeleton's first bytes,
RIFF or FORM, say which kind it is, so the format number and the listing are unchanged):
- Sieve/tools/cli/flac_wav.hpp, flac_wav.cpp: AIFF's chunks walked (COMM, SSND), samples big-endian
  and signed; AIFF-C opened where uncompressed (NONE, twos, sowt).
- Sieve/tools/cli/pack.cpp: AIFFs opened as WAVs are; the report's line is now
  "sound     N file(s) of sound (WAV, AIFF), ...".
- Sieve/reference/sieve_ref.py: the oracle walks AIFF itself.
- Sieve/ci/steps/11-oracle.sh: six AIFFs written by hand beside the six WAVs (four of each opened;
  a float AIFF-C and one with COMM after SSND left alone); the grep follows the new report line.
- Sieve/docs/SPECIFICATIONS.md, FORMATS.md, NEXT-VERSION.md; Sieve/results/flac-way-2026-11-08.md.

**Proof:**
- Proof/rope_wav.py (new): the rope's wave on a 16-bit WAV as one encoder and decoder in plain
  Python, checked byte for byte, weighed against FLAC where flac or ffmpeg is found; it ends with
  your question.
- Proof/PROOF.md: a line for it.
