# Your notes, and a full code review (29 October 2026)

Unzip over the repository root. Committed locally as two commits (not pushed). Code changed:
rebuild everything (the hallway, `sieve-install`, `sieve`, the tests), then run the unit suite and
CI steps 02, 10 and 11 (all run here: see the end).

Nothing here changes a `.sieve`, an address or a coded stream. This repository's docs folder packed
by the new build is the same `.sieve`, byte for byte, as by the build before. The unit suite's pinned
codes and the rope's 100 cases come out as before.

## Your notes

- **The window's title** is "The Sieve of Potentia".
- **Choosing a folder on Windows** now opens Explorer's own picker (folders only), with its address
  bar and places, instead of the tree of folders. SDL 3.2 uses the old `SHBrowseForFolder`; the new
  one is `IFileOpenDialog` on a thread of its own, with SDL's as the fallback.
  - It compiles with MinGW. I could not open it here, so please try Browse in the File Locator and
    in `sieve-install`.
  - Elsewhere the system's own dialog is used, as before.
- **An installer program straight from a `.sieve`:** the File Locator has a new button, "Installer
  program from a .sieve" (key M).
  - It asks for the `.sieve`, then where to save, and writes `sieve-install` with it attached.
  - Nothing needs loading or installing first.
  - The name it offers is `<name> installer.exe`.
- **The installer's address bar is a real text field:**
  - a cursor, and selection by mouse (a double click takes it all) or with Shift and the arrows;
  - Ctrl+A, C, X and V, Home and End;
  - pasting over a selection replaces it;
  - a path pasted in quotes (Explorer's "Copy as path") is taken without them;
  - Escape while typing leaves the field, rather than the installer;
  - AltGr letters type rather than select.
- **The installer reads its file on a thread of its own,** so the window answers while a large one
  is read. It shows "Reading ..." meanwhile.
- **MAPS.WoW and MAPS-Larger.WoW:** written up as IDEAS §19.106.
  - Today each file is carried on its own, so what the two share is paid for twice.
  - The way to fix it is a file carried as its differences from another. That is a new way, so it
    waits for the next format version.
  - One thing could be measured on the pair now, with no new version: putting the pair side by
    side in the lzma2 stream.

## The code review

Five reviewers went through the code at once (the core; packing and installing; the web host and
tools; the client; the oracle and CI). Each finding was checked before it was fixed. The full list,
and what was left, is in `docs/REVIEW.md` ("The third pass"). The ones that matter most:

- **An installer could choose where it installs.** A `.sieve`'s root was never checked. A root of
  `/somewhere` or `..` installed there, whatever folder was picked; this was tried and it did. A map's
  root did the same for `sieve map`. Both are refused now.
- **A full disk was reported as a success.** A small file reaches the disk only as it closes, after
  the check. Installing onto a full disk said every file was checked and left some empty, and a
  `.sieve` could be written cut short with exit 0.
  - Every file Sieve writes is now closed before it is checked.
  - A failed `.sieve` or installer program is removed again.
  - A failed install is undone.
  - Tried on an 8 KB disk.
- **Hostile `.sieve` files:** sizes that wrap round past 2^64, in pieces and in the head's streams,
  are refused before they can ask for gigabytes or read past a buffer.
- **The web host and extension:**
  - One page could end the host for every tab, with a reply over 1 MB or a message over 64 MiB.
  - A failed fetch told the page the other server's status.
  - A `%` in an address broke the viewer.
  - Listing a `.sieve` of 100,000 files took 19.9 s; it is linear now.
- **Memory:**
  - Installing no longer holds every file twice.
  - Extract stops a decompression bomb.
  - Model files are read in pieces.
  - The packer budgets each worker for its largest file.
- **CI could pass when it should fail.** Thirteen `! grep -q` checks never failed, and neither did the
  first half of eleven `a && b` lines. All are fixed, and none of them hid a failure (the run below
  passed). `ci/run.sh 1` no longer runs steps 10, 11 and 12 as well.
- **The oracle:**
  - It refuses paths that leave the folder (it wrote `../../escaped.txt` two folders up).
  - It also refuses broken groups, bad `lines K`, unknown lines, repeated paths, and streams with
    bytes after their end.
  - It ends with a message rather than a traceback.
  - It is linear on large listings: 80,000 records took 25.8 s before.
- **Speed:**
  - The rope's segments use tables of 256 entries rather than 65,536. On a text in 1,024 columns
    the rope codes about 7 times faster and decodes about 3 times faster, with the same codes.
  - A file's LZMA2 estimate now runs beside its rope search.
  - `site build` is linear on pages with many scripts: a 4 MB page of 4,000 scripts took 35.5 s.
- **From the earlier review:**
  - Item 5 is done: each filter finds its settings by its own name, so raising a version can't
    leave the two out of step.
  - `read --around` is now checked in step 02.
- `docs/SIEVE-FORMAT.md` is corrected where the reviewer found it wrong: the members' order, `skm`'s
  recipe, `mixn`, `lines K`, and what the tree covers.

## Checked here

Linux, GCC, a Release build with the hallway and `sieve-install`:

- the unit suite: 154,218 checks, 0 failures;
- CI steps 04, 08, 10 and 11 on the build before the last fixes: all passed;
- CI steps 02 (addresses, now with `read --around`, 4 s), 10 (the hallway, the File Locator and
  installers, 581 s) and 11 (the oracle, archives, the web host, 1,490 s) on the final build: all passed;
- every changed client and tool file compiles for Windows with MinGW (`-fsyntax-only`).

## Files
- `.github/workflows/build.yml`
- `Sieve/CMakeLists.txt`
- `Sieve/ci/run.sh`
- `Sieve/ci/steps/02-same-addresses.sh`
- `Sieve/ci/steps/04-models.sh`
- `Sieve/ci/steps/08-ffmpeg.sh`
- `Sieve/ci/steps/10-hallway.sh`
- `Sieve/ci/steps/11-oracle.sh`
- `Sieve/client/app_main.cpp`
- `Sieve/client/dialogs.hpp`
- `Sieve/client/file_locator.cpp`
- `Sieve/client/file_locator.hpp`
- `Sieve/client/harness.cpp`
- `Sieve/client/installer_main.cpp`
- `Sieve/client/window_front.cpp`
- `Sieve/core/include/sieve/filter.hpp`
- `Sieve/core/src/filters/audio.cpp`
- `Sieve/core/src/filters/figure.cpp`
- `Sieve/core/src/filters/media.cpp`
- `Sieve/core/src/filters/pattern.cpp`
- `Sieve/core/src/filters/picture.cpp`
- `Sieve/core/src/filters/sound.cpp`
- `Sieve/core/src/filters/statistics.cpp`
- `Sieve/core/src/filters/text_m1.cpp`
- `Sieve/core/src/filters/weights.cpp`
- `Sieve/core/src/filters/written.cpp`
- `Sieve/core/src/plugin.cpp`
- `Sieve/core/src/rope_wave.cpp`
- `Sieve/core/src/space.cpp`
- `Sieve/core/src/training.cpp`
- `Sieve/core/src/weights.cpp`
- `Sieve/core/src/weights_rans.cpp`
- `Sieve/data/lang/en.txt`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/REVIEW.md`
- `Sieve/docs/SIEVE-FORMAT.md`
- `Sieve/reference/sieve_ref.py`
- `Sieve/tools/cli/chat.cpp`
- `Sieve/tools/cli/filter_config.cpp`
- `Sieve/tools/cli/image_io.cpp`
- `Sieve/tools/cli/lines.cpp`
- `Sieve/tools/cli/locate.cpp`
- `Sieve/tools/cli/map.cpp`
- `Sieve/tools/cli/map_sieve.cpp`
- `Sieve/tools/cli/media_decode.cpp`
- `Sieve/tools/cli/pack.cpp`
- `Sieve/tools/cli/rar_remake.cpp`
- `Sieve/tools/cli/rope.cpp`
- `Sieve/tools/cli/site.cpp`
- `Sieve/tools/cli/vault.cpp`
- `Sieve/tools/cli/vault.hpp`
- `Sieve/tools/cli/web_host.cpp`
- `Sieve/tools/sieve_cli.cpp`
- `Sieve/web/crawl_site.py`
- `Sieve/web/extension/background.js`
- `Sieve/web/extension/viewer.js`
