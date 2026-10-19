# Changes: the rope in the weighing table, and the hex row's alignment (19 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files.

- **Fixed: the "address, written in hex" row sat one space to the right.** Its name is 23
  characters, one more than the name column, so everything after it shifted. The column is 24
  wide now, and every row lines up (`tools/cli/compare.cpp`).
- **The rope in the weighing table** (the File Locator's, and `sieve locate --weigh`):
  - Every file's rope code is now one more way to name it. It costs the code, the wave's
    parameters (written as text, e.g. `L8 W949 S1 P3 K0`) and the file's size, all at 8 bits a
    byte.
  - The rope shows as a line of its own: "shared: rope, N file(s): shape 96 bits". That's its
    name, `rope-wave-v1`, paid once, as each line's shape is.
  - The rope's search uses the search row's time for a file, and counts against the run's time.
    So the `search:` line now also includes the rope's time.
  - On the mixed test folder the rope is every file's best way. The total comes to 61.4% of
    the files' own addresses. For the text that "best way" is the rope with no wave
    (`P0 K0`): just its bytes coded by how often each occurs.
  - The existing weighing results in CI are unchanged: on those four small files the rope isn't
    the best way. A new CI line checks the rope's row.
- **Checked here:**
  - the full build, with no warnings;
  - the unit suite: 153,158 checks, 0 failures;
  - the weighing's CI lines, the new rope line, and the File Locator's CI lines;
  - a screenshot of the File Locator on the mixed folder.
- **Docs:** SPECIFICATIONS (the weighing), README, IDEAS §19.87, HANDOFF, help.
