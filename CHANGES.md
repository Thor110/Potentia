# Changes: the code review of 9 November 2026

A review of the whole Sieve application by several reviewers (the newest changes first), then the
findings checked one by one and fixed. Nothing here changes what any version makes: the same files
give the same instructions as before.

## Found by hostile files, fixed

- **Sizes that overflowed.** A CAF, Wave64 or RF64 whose sizes add up past 2^64, and a PNG whose
  height times row length did, passed the size checks; the CAF one crashed the packer. Each is now
  checked without the sum (flac_wav.cpp `layout_of`, png_resave.cpp `read_ihdr`).
- **packMP3.** An MP3 claiming more muted frames than it has frames crashed it (a null pointer in
  the unmute walk). It is now refused ("more muted frames than frames"); third_party/packmp3/SIEVE-CHANGES.md.
- **A listing that names more than it could hold.** A 40 KB .sieve whose listing claimed gigabytes of
  path names is refused at once: a path is at most 32,767 bytes, and all the names together at
  most 200 times the file's size (and never less than 64 MiB allowed).
- **Control characters and NUL in paths**, and **one path listed twice**, are refused by the reader,
  the packer and the oracle (SPECIFICATIONS' path rule says so). Messages escape control bytes, so
  a hostile name cannot clear the terminal.
- **Windows names.** CONIN$, CONOUT$, and COM/LPT with superscript 1 to 3 are refused, in folders
  and at the root too.
- **A delta whose base is itself a copy** is refused (reader and oracle).

## The packer

- **Sound as FLAC** renames (x.wav to x.flac) check the new name is free, ignoring case and as
  macOS compares names: before, Song.wav beside song.flac could clash.
- **Archives read only when they can be used.** Under temporary alone, a 7z, zip, or anything
  libarchive reads was decompressed whole and then refused; now it is read only with permanent, and
  a 7z already read for v21 is not read twice.
- **Less copying:** 7-Zip's LZMA2 encoder copies its input only for the x86 filter; a WAV's FLAC is
  checked in place; the first pass kept for a lone file is hashed outside its lock.
- The tar weighing reuses the sizes already found; a single file's listing records its new name;
  ffmpeg's report read on Windows (its \r\n); the pipe to ffmpeg opened close-on-exec, so another
  thread's ffmpeg cannot inherit it.
- The warning about sound now names the three-way button ("Set Sound as FLAC to Temporary") and
  says tags are kept.

## The hallway

- **Main menu:** back from Settings lands on Settings, not on File Locator.
- **Settings from the pause menu** now apply the view distance and graphics memory too.
- **File Locator:** the learned search shares are saved by the frame, not the job's thread.
- **AI Training Harness:**
  - START OVER (S) asks for a second press when a run has saved training, which it throws away.
  - The learning rate list includes the suggested rate, so it can be gone back to.
  - A picture model's steps and pictures stop at 2^30 instead of wrapping round to a small number.
  - The runs list scrolls, so the chosen row stays in view however many runs there are.
  - The keys at the foot go onto two lines where one would run off the window.
- **Start Sieve's details window:** indented lines wrap two characters earlier, so they stay inside the box.

## The release script and the CI

- **make_release.py** (the Windows failure): the lone installer run has no window on Windows too (as
  the CI runs it), and a ten-minute limit; release/source is removed however the script ends;
  temporary folders are removed as far as Windows lets them, without failing the release; an
  empty unit-test log no longer crashes it.
- **.gitattributes:** shell scripts keep LF line endings on every platform.
- **Checks that could pass by failing:** a few `if program | grep -q ...; then exit 1` checks passed
  when the program itself failed (steps 02, 04, 10, 11). Their output is now captured first.
- **Step 08** fails on a Linux runner where ffmpeg could not be installed, rather than skipping.
  Two of its checks from the lossless sound change (the 24-bit FLAC, the accented tag) could never
  pass in the CI: `ffmpeg -i` with no output always ends with status 1, and the step stops on a
  failed pipe. Its report is now captured first, then read.
- **ci/run.sh** stops if copying the folder fails.
- **Step 11's mixing-group check** read the start of METHODS.md and FORMATS.md, so the docs fixes
  changed its input: the pair then came out 20 bytes smaller without the match model's ways
  (4,904 bytes, against 4,924 with them), and the check for those ways failed. Its two texts are
  now kept in tests/texts as they were (the committed bytes), so editing the documents no longer
  changes it.

## Core

- An install's cancel flag (DecodeCancelScope) is cleared only by the scope that set it.

## Documents

The reviewer's 17 places where the documents no longer matched the code: the main menu's walk
times (now Start Sieve's map), the map's walk time and details window, training-v3 as the default
(MANUAL, SPECIFICATIONS), Sound as FLAC as what decides sound (SPECIFICATIONS §12.2), the O key in
the File Locator's key line, the details window in `hallway --help`, the permanent column for AIFF,
RF64, Wave64, .au and CAF (FORMATS), `sieve help locate` (v34's ways, v33 as the newest,
`--sound-permanent`'s bits), "Prioritise Decoding Speed" to Priority: Decoding speed, Optimise
Search taking temporary (METHODS, FEATURES), FORMATS-PLAN's built items, and the hallway's
`--optimise-compressed` (both boxes) against the command line's (temporary alone).

## Looked at, not changed

- The pause menu's "Return to the main menu? Y / N": N leaves Sieve, as its prompt says.
- Reset to defaults (R) takes one press: Edward asked for a button, and the CI presses it once.
- The chat's context limit: going past it already stops with a clear message.
- Deferred, for later: the training kernel's blocking and attention speed; the estimate taking
  spans; the C++ reader's check of entry order; digits read with sscanf in recipes; a tie in the
  rope wave's double log2; pinned dependencies, build caching and warnings as errors in CI.

## Also

- ODDITIES: Edward trained all four current models again on 9 November 2026 and each reproduced;
  the entry stays open.
- IDEAS 19.134: documents kept up to date from the code (a session of its own).
