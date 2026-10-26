# sieve-manifest-v22: a RAR made again by your own rar.exe, with a warning where it can't be (26 October 2026)

Unzip over the repository root. Commit d915694 locally (not pushed), on top of sieve-extract.zip (222fbcd).

## What it does
- **Packing** (Optimise Archive Formats):
  - Sieve reads a RAR 5 archive's headers for the switches it was made with: level, solid, locked, BLAKE2, owners, which times, quick open.
  - It writes the files out with their exact times and attributes, then runs your `rar.exe` on them.
  - It keeps the recipe only if the result is the archive byte for byte. Otherwise the RAR is carried as its contents, as before.
  - The report says, for each RAR, what came of it: `rar       x.rar: made again by RAR 7.01 x64 (top, -m3 -s -qo-)`, or why not.
- **Installing** (`sieve install`, `sieve-install`, the hallway): the files are written first. Then:
  - **with the same rar version:** the RAR is rebuilt, checked against its SHA-256 and put in their place;
  - **otherwise:** the files stay in a folder named after it, with a **warning**, e.g. `x.rar was not made again as a RAR: rar was not found (...); its files are in x`.
- **Where Sieve looks for rar:** `SIEVE_RAR`, beside Sieve's programs, the `PATH`, then WinRAR's folder in Program Files.
- **What it refuses**, leaving the RAR as its contents only, as before:
  - RAR 4;
  - encrypted archives, multi-volume sets, links, comments, recovery records;
  - an archive made on another OS.

## Warnings from one place (your note)
- The install code returns its notes, and every front end shows them, so a new warning later needs nothing more from them:
  - `sieve install` prints them as `note` / `warning` lines.
  - `sieve-install` lists them on its last screen and pops warnings up in a message box (with `--yes`, they go to the error output instead).
  - The hallway's File Locator adds them to its status line and pops warnings up.
- The old "carried as its contents" note now comes from the same place.

## Format
- `sieve-manifest-v22` (head byte 22) is v21 plus, after a RAR's `u` line:
  - a `ur` line: the version, the archive's SHA-256, how rar is run, the switches;
  - one `um` line per member: file or folder, attributes, times in nanoseconds, path.
- In bytes, those lines go as they are (record 3).
- v21 and older stay readable, and `--v21` makes v21. Full description in SPECIFICATIONS §12.2.
- The oracle reads v22 and gives the contents, as Sieve does without rar.

## Tested here, and what isn't
- **The stand-in:** there's no rar in this session (rarlab.com is refused). `ci/stand_in_rar.py` plays it: a small RAR 5 writer of stored files, following RARLAB's published format.
- **Step 11 checks, with it:**
  - a RAR alone and one in a folder;
  - with rar, given back byte for byte;
  - without rar, its contents and the warning;
  - `sieve-install` and the oracle;
  - `--v21`.
- Steps 10 and 11 pass on Linux. The block doesn't run on Windows, where the Python stand-in can't be started as a program.
- **Not measured: whether your real rar.exe gives your RARs back.** To try it:
  ```
  set SIEVE_RAR_TRACE=1
  sieve locate X.rar --installer x.sieve --items --optimise-archives
  ```
  Each try prints how many of its first bytes came out the same. Send me those lines for any RAR that doesn't come back and I'll adjust the switches.
- The message boxes (sieve-install and the hallway) aren't tested here: CI runs with no screen.
