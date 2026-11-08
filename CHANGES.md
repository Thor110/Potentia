# Changes (commits 89aba6a to f6d004f, 8 November 2026)

Replace these files in the repository (paths from its root). All in the v34 draft (--v34-draft).

**1. More sound through FLAC (89aba6a)**, by the same "flac" recipe: RF64 and BW64, Sony's Wave64,
Sun's .au (including one whose size was never written) and Apple's CAF. Oracle and CI step 11 too.

**2. Sound as FLAC (92b3cca)**: --sound-to-flac, the File Locator's box (key W, shown while the draft is
on), [locator] sound_to_flac. A WAV, AIFF, RF64, Wave64, .au or CAF (1 or 2 channels, up to 24 bits)
installed as name.flac that any player opens; existing FLAC files re-coded at the priority's effort,
their tags, pictures and cue sheets kept. Packing warns, naming each. CI steps 10 and 11.

**3. FLAC on every core (46bb09c)**: the seven WoW WAVs at Size made in 110.5 s here (4 cores) from
203.1 s, the same bytes.

**4. Plain pictures (14998d8)**: a BMP, uncompressed TGA, binary PNM or uncompressed TIFF offers
mixing its header's shape. 24 pictures 0.62% smaller; one screenshot 1.26% larger (recorded).

**5. docs/FORMATS-PLAN.md (f6d004f)**: the formats still to add, in order.

Files: tools/cli/flac_wav.cpp/.hpp, pack.cpp/.hpp, locate.cpp, help.cpp, args.hpp, sieve_cli.cpp;
client/app_main.cpp, app_settings.cpp/.hpp, file_locator.cpp/.hpp; data/lang/en.txt;
reference/sieve_ref.py; ci/steps/10-hallway.sh, 11-oracle.sh; docs (SPECIFICATIONS, FORMATS,
FORMATS-PLAN, MANUAL, NEXT-VERSION); results/flac-way-2026-11-08.md, plain-pictures-2026-11-08.md.
