# The documentation, brought up to date (23 October 2026)

The full readout now lives in the programs, and the READMEs point to it:

```sh
sieve help                 # every command, grouped by what it is for
sieve help COMMAND         # one command's options, defaults and examples (or: sieve COMMAND --help)
sieve help lines           # every line and its options
hallway --help             # the hallway's options
sieve-install --help       # the installer's (new)
```

## What was checked
`sieve help` already named every option every command reads. I checked that by comparing the
options the source code reads with each command's help page. What was wrong was facts that had
gone stale. Those are fixed:
- **`sift`:** said the default dictionary was `scowl-en-60`. It's `scowl-en-60-names`. The page
  now lists the `-names` lists and the CSV's columns.
- **`locate`:**
  - said `--installer` writes a v3 manifest. On its own it writes v4 (packed), the newest (v16, or
    v17) with `--items`, and v3 only with `--v3`;
  - said "v16 always now". It now names v14, v17 and v18 as well.
- **`install`:** described only v3 installers. It reads every version, v2 to v18.
- **`lines`:** said "the four lines". It now names every line, and which commands take which, and
  lists the `ascii96` and `bytes256` alphabets and Unicode blocks.
- **The overview (`sieve help`):**
  - commands grouped by purpose (Addresses, Filters, Guided text, Files, Books, Other lines,
    Tools);
  - long summaries wrapped;
  - every line named.
- **`version`:** claimed "every pinned rule version" but left many out. It now also prints:
  - the notes3 and pcm rules;
  - the other lines' versions;
  - the newest instructions format, and the installers' formats;
  - the rope, the book and map records, the vault's rules, and training.

  All of these come from the code's own constants, so they can't drift.
- **`mesh`, `world`:** now document `--short`, which they already took.
- **`dicts`, `bind`, `tensors`, `chat`, `cipher`:** now carry the measurements the README had. The
  cipher one was reworded and checked against a real run: 130 bits name one layer, 260 bits a
  recipe of up to two.
- **The hallway's `--help`:**
  - now names `--vertices`, `--faces`, `--coords` and `--limits-focus`, which it read but didn't
    list;
  - the AI line was missing from `--line`;
  - no longer says "five lines";
  - shows the tile size as a setting (2 to 4096, 128 unless given).
- **`sieve-install --help`:** new. It prints what the installer opens and installs, and its
  testing options. The claims in it were checked against the code: it installs maps and archives
  given back as their contents, and asks for Sieve only for items or remade archives.

## A check, so it stays this way
`tools/check_help.py` (run by the CI) fails if the code reads an option that isn't on that
command's help page, if a command has no page, or if the hallway reads an option its usage text
doesn't name. It reads the sources, so it needs no build.

## The READMEs
- **Sieve's README:**
  - **Commands section:** now a short pointer to `sieve help`, a quick start, `warp`'s example
    output, and a table of every command by group.
  - **Moved, not lost.** Everything that section held beyond options:
    - the File Locator's description and the every-degree demonstration went to the hallway
      section;
    - the history of the installer formats, with their measurements, went to
      `docs/SIEVE-INSTALL-USAGE.md` ("The formats, as they grew");
    - `measure`'s table went to M3 results;
    - the other figures went to the help pages above.
  - **Brought up to date:**
    - Layout: added `web/`, the release and test scripts, `check_help.py`, and the docs that
      weren't listed;
    - Key terms: no longer "four lines";
    - the corridor's tile size, which is now a setting.
- **`docs/SIEVE-INSTALL-USAGE.md`:**
  - explains the newest formats;
  - has an `--items` example;
  - says which `.sieve` files `sieve-install` can open.
- **SPECIFICATIONS:** the corridor paragraph no longer says 128 slots a tile is fixed at compile
  time.
- **The repository's README:**
  - eleven lines (it said seven);
  - version 0.13.75 (it said 0.13.1);
  - recorded, replayable training moved from Planned to Built, since `ai-train` does it;
  - the web extension and `sieve site` added.

## A question for you
`sieve locate FOLDER --installer X.sieve` on its own still makes the plain v4 installer. The File
Locator always makes the newest (v16 or v17), and the command line needs `--items` for that.
Should `--installer` make the newest by default, with a flag for the plain one? I've only
documented it as it is.

## Tested here
- Unit tests pass, the local CI (from step 6) passes with the new help check, and the libc++
  build compiles.
- `check_help.py`: 0 missing.
- Every command in `sieve help`'s list opens a page.

## To apply
1. Copy `Sieve/` over the repository's `Sieve/`, `.github/` over `.github/`, and the root
   `README.md` over the repository's.
2. Rebuild (`sieve-install` now also links `client/win_console.cpp`).
