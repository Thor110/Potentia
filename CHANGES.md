# Sound as FLAC: WavPack, Monkey's Audio, TTA, ALAC (9 November 2026)

With Sound as FLAC ticked (`--sound-to-flac`, v34 draft), a `.wv`, `.ape`, `.tta` or ALAC `.m4a` is
decoded by ffmpeg (the one set in the hallway's Settings or by `--ffmpeg`), its tags and cover kept,
coded again by libFLAC at the priority's effort, and installed as `name.flac` where that is smaller.
Checked first: no ffmpeg error (CRCs), 16 to 24-bit integer sound, and the FLAC's samples' MD5 equal
to the source's. Without ffmpeg these files are left as they are and the listing says:
"N WavPack, Monkey's Audio, TTA or ALAC file(s) left as they are: ... (link ffmpeg in the hallway's
Settings, or give --ffmpeg PATH)".

Measured (results/lossless-sound-2026-11-09.md): a 54.7 MB WAV's WavPack, TTA and ALAC (ffmpeg's
encoders) became 27.7 MB FLACs (from 28.7, 27.9 and 29.2 MB). Monkey's Audio beat FLAC at every level
on that track, so such files mostly stay as they are.

Files:
- new: Sieve/tools/cli/lossless_sound.hpp/.cpp, Sieve/results/lossless-sound-2026-11-09.md
- Sieve/tools/cli/media_decode.hpp/.cpp: ffmpeg_run (output, errors, exit status)
- Sieve/tools/cli/pack.cpp/.hpp, locate.cpp, help.cpp; client/app_main.cpp, app_settings.cpp (texts)
- Sieve/CMakeLists.txt: lossless_sound.cpp in sieve_compare, which now links sieve_lines
- Sieve/ci/steps/08-ffmpeg.sh: a new block at its end
- Sieve/docs/FORMATS.md, NEXT-VERSION.md, FORMATS-PLAN.md (also: MPEG-2/2.5 MP3s noted as still to
  do, a coder to track down or build)

Checked here: unit suite; the new step 08 block; installed FLACs' samples equal their sources' (16
and 24-bit WavPack, TTA, ALAC, Monkey's Audio); tags and an ALAC cover kept; damaged and 8-bit files
left as they are.

Please run: build, CI step 08 (needs ffmpeg) and the usual full CI.
