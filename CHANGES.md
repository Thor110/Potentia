# Optimise Search, the draft as one number, the JPEG fix, GAME-OF-LIFE.md (commit 64b9ebc)

**Needs a rebuild:** both sieve and the hallway changed. `file-locator-optimise-search.png` shows
the File Locator with the v33 draft on. It's a screenshot, not part of the repository.

## 1. Optimise Search (the File Locator's box, O, ticked unless you untick it)

Try every way is chosen for each file by its kind, from what we've measured, and its own box is
greyed "(chosen file by file)" while Optimise Search does it:

| Aim | File | Try every way | Measured |
| :--- | :--- | :--- | :--- |
| Decoding speed | any | off | your 153 JPEGs gave the same bytes, 1.24 h against 5 min; enwik8 and enwik9 the same |
| Size | a JPEG | on | 2.7% smaller (median of 153) |
| Size | over 16 MiB or under 4 KiB | on | enwik8, enwik9, control.py |
| Size | anything between | off | the same size measured either way |

- **The box:** ini `[locator] optimise_search`, and `--optimise-search` on the command line. If you
  ask for `--search-every-wave` by name, the hallway unticks Optimise Search for that run.
- **Test here:**
  - Vitruvian's JPEG for size: 203,790 bytes in 10.6 s.
  - The same for decoding speed: 206,946 in 0.2 s.

  Each is the better of the box off and on.
- **Each file's choice and why** is written in the log.
- **Other kinds** get their own rule as their measurements come in.

## 2. The next version's draft, made one number

`tools/cli/draft.hpp` holds `kDraftVersion = 33`. These all follow from it:

- the switch `--v33-draft` (sieve and the hallway);
- `[locator] v33_draft` in sieve-hallway.ini;
- the help text;
- the File Locator's orange "v33 DRAFT ON (testing only) ..." line.

The next draft starts by changing that number. An older draft's ini line is read and ignored.

## 3. The five JPEGs (ODDITIES)

- **What went wrong:** in all five, mixing's code was no smaller than LZMA2's stream, and it
  installed about 30 times slower.
- **The fix:** mixing now replaces LZMA2 only by the packer's usual margin, 64 bytes and 0.5%. Your
  other wins have a median of 2.7%, well past it.
- **Please confirm** when you rerun the pictures.

## 4. docs/GAME-OF-LIFE.md, new

- **The gliders:** how they work, what was measured (15 bytes on a page of zeros with two glider
  trails drawn through it; gliders that meet, and the combinatorial explosion), and what's still to
  try: spaceships, glider guns, other Life-like rules, and searches that try gliders together.

## Tested

- The unit suite: 77,110 checks, 0 failures.
- CI step 10's File Locator blocks.
- Optimise Search and the draft from the command line.
- Screenshots: the box, the greyed Try every way, the draft line.
