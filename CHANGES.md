# Pictures keep their lines (canon-image-v2), and the hallway as a Windows program

Against your main ("notes"). Copy `Sieve/` over yours. New files:
- `client/dialogs.hpp`
- `client/win_console.hpp` and `.cpp` (already in CMakeLists.txt)
- `tests/vectors_image_v2.tsv`

## Vitruvian.jpg is no longer white: canon-image-v2
- **Why it was white:**
  - The old rule (`canon-image-v1`) averages each cell of the picture, then takes the nearest
    palette colour.
  - A line drawing on parchment averages to plain parchment, and plain parchment is nearest to
    white. So GO TO IT / J found a white picture: the right unit for that rule, but useless.
- **The new rule, now the default,** adds one step: unless the picture is already in the
  palette's own colours, its range of light and dark is stretched to black..white, with the
  darkest and lightest twentieth clipped (the usual "auto levels").
- **What it gives:**
  - Vitruvian.jpg at 10x10 black and white is a silhouette (v1: all white).
  - At 64x64 his head, arms, legs and circle are there (screenshot sent with the zip).
- **What stays the same:**
  - A picture saved from the line comes back as the same unit, since its colours are already
    the palette's. CI checks this.
  - On rgb24 nothing changes, for the same reason.
- **What changes:** the address of a photo or drawing warped onto the image or video line.
  - The old rule is still there, so old addresses can always be made again: `sieve warp --fit
    average`, and in the hallway `--image-fit average` / `--video-fit average`.
  - `sieve version` and every fitting report name the rule used.
- **Checked:**
  - The oracle has the rule, written from the specification, with 32 new vectors, 17 of which
    differ from v1.
  - The unit tests check both vector files, plus a direct ink-on-paper case.
  - CI checks:
    - Vitruvian under both rules;
    - an ega16 picture at three times the size coming back under both;
    - the oracle's new vectors.
- **SPECIFICATIONS §6.2** gives the rule exactly. README and `sieve help lines` describe it.

## The window comes back after a file dialog
- **What:** every file and folder dialog brings the hallway's window back to the front when it
  closes, in CHAT, the File Locator, saving, the media player, the setup menu, the node graph,
  the harness and the installer.
- **How:** a small wrapper, `client/dialogs.hpp`. After the dialog's own work, it raises the
  window on the main thread.

## No console window behind the hallway
- **What:** `hallway.exe` is now built as a Windows program, so starting it from Explorer (or
  from Visual Studio) opens no console window.
- **Started from a command prompt,** it attaches to that prompt and writes there. The prompt
  doesn't wait for a windowed program, so the output appears after the prompt returns.
- **With its output redirected or piped,** as CI and scripted runs do, it writes there as
  before.

## Also
- **IDEAS §19.39:** the whole title, *State Space : Near Zero : The Sieve of Potentia?.!!.?*
- **HANDOFF:** a new entry.

## Checked here
- **Unit tests:** 76,404 checks, 0 failures (once; this machine has no hardware SHA-256).
- **New CI lines,** run here and passing.
- **Run again here:** the existing CHAT card and J lines, on the rebuilt hallway.
- **Not tried here:**
  - *The two Windows changes.* There is no Windows machine here. On Linux they build and behave
    as before. Please check that:
    - double-clicking `hallway.exe` opens no console;
    - `hallway.exe --help` from cmd prints;
    - the window comes to the front after a dialog.
  - *The ffmpeg CI step.* There is no ffmpeg here; its picture comparison uses rgb24, which the
    new rule leaves alone.
