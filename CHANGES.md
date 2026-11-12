# v34 frozen; the roadmap written (12 November 2026)

On top of b4d5fe0 ("sieve metrics"). Unzip at the repository root; nothing to delete by hand; no new
source files (CMake need not be re-run).

## v34 frozen (docs/NEXT-VERSION.md)

- Every instructions-maker now makes `sieve-manifest-v34` at its newest: GIFs, sound through FLAC (with
  floats and the formats beside WAV), MP3s through packMP3, TIFFs, PCX and PDFs opened, zlib streams
  inside files, mix-v4's x86 instruction model (E1), pictures' shapes from their headers, and the x86
  filter restarted at each program. A `.sieve` is v34 only where one of those is used.
- `--v33` makes v33 byte for byte as before; `--v34-draft` is still accepted and changes nothing. The
  next draft is v35 (`--v35-draft`, `[locator] v35_draft`), begun empty.
- The File Locator's Sound as FLAC button (W) is always shown now.
- Checked byte for byte on docs/, tests/ and a folder of one of each v34 kind: the new default equals
  the old draft, `--v33` equals the old default, `--v34-draft` equals the default; again with
  Compressed formats temporary and Sound as FLAC permanent.

## A bug found while freezing, fixed

With Sound as FLAC **Permanent**, a sound renamed past its neighbour (tone.wav installed as tone.flac,
which now sorts before tone.mp3) left the listing out of order and the whole folder was refused
("entries out of order"). The listing is now put back in name order where a sound was renamed; no
other listing changes. CI step 11 checks it (packed, installed by Sieve and by the oracle).

## The roadmap (docs/ROADMAP.md, "Next, in order")

1. v34 frozen (done), then the speed pass on mixing. 2. The System Viewer as a viewer of its own, and
the right-hand panel as a true mirror of the left (squashing, no white line, scroll bar on its left,
one panel open at a time). 3. This week's loose ends. 4. --record/--replay and music visualiser rings.
5. The long plan, stages 8 to 18. 6. Voices by WER; the power savings page. And what waits on you.

## Files

tools/cli/pack.hpp, pack.cpp, draft.hpp, items.cpp, args.hpp, help.cpp, locate.hpp, tools/sieve_cli.cpp,
client/app_main.cpp, file_locator.cpp, file_locator.hpp, app_settings.cpp, app_settings.hpp,
picture_source.cpp (comments), ci/steps/10-hallway.sh, 11-oracle.sh; docs: NEXT-VERSION, SIEVE-FORMAT,
SPECIFICATIONS 12.2 v34, HANDOFF, MANUAL, FORMATS, ROPE-POST, ROADMAP, README.
