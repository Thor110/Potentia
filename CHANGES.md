# The small program items (7 November 2026)

Eight commits on top of a30ddd0, from 147b4c9 to e770cb7. None changes a manifest version or
what any .sieve holds. The unit suite was run once on the result: 154,388 checks, 0 failures; the CI blocks
each change touches were run here, and the new CI lines are in steps 03 and 10.

## What was built

1. **binary-kind-v1 takes several kinds** (IDEAS §16.4). `kinds = png,jpg` keeps the files of
   either kind. Each part is checked against the choices, and an empty or unknown part is refused
   (`png,,jpg`, `png,nothing`, `png,`). The setting was already text, so no version changes.
2. **The Encryption Engine's catalogue on screen** (§19.52). DO now goes round ENCRYPT, DECRYPT,
   BREAK and CATALOGUE. The catalogue is counted by the same core code as
   `sieve cipher --catalogue` (moved into `cipher::catalogue`), and the status line places the
   recipe among those of its depth ("the recipe has 2 layer(s): ... named by 260 bits").
3. **The completion estimate on the main menu** (§19.49). Under the rule, one line per dimension
   in its colour, binary first: its size under the setup menu's settings and the years to walk
   every unit at one a second. The setup menu's line-size counts are now a free function
   (`line_sizes_of`) shared by both menus. At 720 pixels high the last five fold into "and 5
   more dimensions"; at 1080 all eleven show. `hallway --main-menu --screenshot` prints them.
4. **The setup menu's counts say how far to go** (§19.78). A stack's survivors line adds "at one
   a second, ~10^38.3 years to walk them" (pages with max-run ticked).
5. **Read aloud in the setup menu and the File Locator** (§19.74):
   - the setup menu says the row chosen after its section, with its value;
   - in a line's filters and in the alphabets, it says the entry chosen, with a tick read as on
     or off;
   - the File Locator says the button the mouse comes onto, a button as pressed ("Optimise
     Search, Off") and its status line when it changes. While a job runs, only the job's first
     status is said, then its result when it ends.

   One announcement per frame, so nothing cuts itself short. `hallway --read-aloud` turns it on
   for one run without saving, and screenshots print what would be said. That is how CI checks
   it here, with no voice. Not yet heard on Windows.
6. **CHAT `/docs` with GO TO IT and J** (§19.48). With a document or a section comes a card of its
   whole text as a file. GO TO IT walks to it on the binary line, J opens it as text on the pages
   line and S saves it. The card is not given to the model.
7. **CHAT `/unit LINE HEX`** (§19.32). The place at that number on a line, in hex as the navigator
   and `/random` show it, as a card with GO TO IT. A number past the line's end is refused.
8. **COST searches further** (§16.1 (a)). L doubles the bearing's decimal places, from 20 up to
   640, on the route's worker. The route found so far stays on screen, and a line says how far
   it has searched. Searching to 640 places took under a second on a page and on a binary-line
   file, and found nothing shorter than at 20, as the evaluation expected for most units. A
   bearing past 20 places is marked as written down, not typed, because the compass goes to 20.
   `--route-places N` runs the search in scripted runs.

## Notes

- `--read-aloud` and `--route-places` are new hallway options.
- A language-file value that starts with a quote mark is read as a quoted string. One new
  string tripped on this and was reworded.
