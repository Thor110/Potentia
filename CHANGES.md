# Sieve: filters that count together (list W)

One commit on top of f08b48e.

## Why the percentages had dropped

X (tick every filter on every line) compares two filters that can't be counted together by how much
each one removes on its own, and keeps the stronger one. The new fractal filters in the other lines
(keep = include keeps almost nothing) and the filters that compare a unit with itself (a picture against
its mirror, a frame against the one before) won every one of those comparisons. So ticking everything
left the image line with only fractal-drawn and image-mirror, and the other lines just as weakly
filtered.

## >> OPTIMISE FILTERS << (your note)

A new row under FIND MY LIMITS. It calls the same function as X on the filters page, so the code isn't
duplicated, and it ends with every filter ticked. If a line's combined table would no longer fit the
filter memory, the weakest filters are unticked one at a time until it fits. The books' pages needed
1.4 GB with twenty-one filters ticked. The whole run took about 35 s here.

At the default sizes, every line keeps:

| Line | Kept |
| :--- | :--- |
| Binary | 10^-14.59 |
| Image | 10^-18.19 |
| Pages | 10^-20.38 |
| Books | 10^-115.95 |
| Audio | 10^-13.63 |
| Tracks | 10^-93.08 |
| Video | 10^-1.12 |
| Movies | 10^-43.05 |
| Models | 10^-21.48 |
| Worlds | 10^-124.49 |
| AI | 10^-9854.27 |

## Filters that can't be counted: now judge-only

These filters compare a unit with itself, so they can't be counted together with anything else:

- image-mirror
- life-causal
- video-pan
- video-moves
- video-smooth
- golden-word
- melody-motif
- sound-periodic

They're on the judge-only tab now. Each one still counts its own survivors when it's the only filter
ticked.

The fractals' filters in the other lines are set from the FRACTALS rows, so X and OPTIMISE now leave
them as they are.

## The filters page no longer recounts at every tick (your note)

With the page open, Z/X/C and ticking a box only mark the line "needs recalculating: counted when the
filters page is closed". The count runs once, when you close the page.

## "Needs merging" resolved where it can be

- **Fractal words and melodies now merge.** On the pages and audio lines, the fractal lists are now
  automata. Each sorted list is built directly into its smallest automaton. The survivors and their
  order are exactly the same as before, so no version changes.
  - **Pages:** 7,496 states for the 531 fractal pages.
  - **Audio:** 18,829 states for the 3,548 fractal melodies.
  - Both merge with the line's other automata. For example, 389 of the 531 fractal pages also have their
    brackets in order, and they now leave the line together with brackets-v1.
- **Fractal pictures and videos still don't merge.** As automata their counting tables would be:
  - fractal-drawn-v2: 1.9 GB (1,031,932 states for the 62,356 pictures).
  - fractal-moving-v2: 87.6 GB (5,903,640 states for the 23,830 videos).

  Both are far over the filter memory, so their own tries still count them. If the filter memory is set
  high enough to hold the table, they become automata too.
- **The other filters with rankers of their own stay as they are**, each for a reason:

  | Filter | Why it stays |
  | :--- | :--- |
  | neighbour-agreement | Its table depends on each pixel's position, which would multiply past any table. |
  | key-v1 | Already retired for key-data-v2, which is an automaton. |
  | utf8-valid | The binary line counts it with machinery of its own. |
  | palette-size, row-runs, figure (on rgb24) | 2^24 colours is more than an automaton's table takes. |

## The filter that overfills the memory is named

When the merged automata need more than the filter memory, the reason now names the largest of them, the
one to leave out first. For example: "...over the filter memory (1 MB); the largest automaton is
brackets-v1's (121 states): they judge only".

## Always countable: the list to work through (your note)

Your note is in IDEAS 19.248, in your words. FILTERS-CONFLICTS has a new section, "Working through: every
filter countable". It lists every filter that doesn't yet count together with the rest of its line, with
a way forward for each, and items get ticked off as they're done.

## The setup menu's rows by name (your note)

`Item:ID` only covered the main menu and the pause menu. The setup menu's list was still reached by
counting presses, so adding OPTIMISE FILTERS broke a step that counted Up presses to FIND MY LIMITS. Now
the setup menu takes names too:

- **A GLOBAL row by its setting**, for example `filter_memory`, `merge_cache` or `item_memory`.
- **A line's first row by its id**, for example `image`. Its other rows go by number from 1, for example
  `fractals:3` (in other lines) or `ai:6` (the picture model).
- **The four action rows:** `limits`, `optimise`, `reset` and `enter`.

An unknown name, or a name given while a window is open over the list, stops the run with an error.
Steps 04 and 10 now reach every setup row they set by name.

These are still counted:

- The rows inside windows: the filters page, the right-hand panel, the personal dimension's catalogue, the
  harness, the designer and the cipher. They get names as their scripts are next touched, along with
  splitting the CI into more individual steps.
- The tests of the Up and Down keys themselves, which count on purpose.

Your note is in IDEAS 19.249.

## Checks

- **CI step 04:** OPTIMISE FILTERS ticks palette-size on the image line and none of the fractal filters
  or the judged-only ones. The optimised books and image figures are checked.
- **CI step 10:**
  - fractal-written-v1 merges with brackets-v1.
  - The overfilled-memory reason names the largest automaton.
- **CI step 03:** tailoring the eight-note melody now counts 204,991 survivors instead of 204,994.
  fractal-scored now merges with the melody plugins and sets aside the three fractal melodies among them.
  The melody's own survivor number (46327) and route (b4f7) are unchanged.
- **CI step 10, `/random melody` and `/random picture`:** these come out "kept by the filters" when the
  line's stack counts, and plain otherwise. Which one you get depends on the filters that earlier steps
  in the same folder leave behind, so the check now accepts either.
- **CI runs:** step 01 passed on W1–W5; step 03 passed after the change above; steps 04 and 10 passed on
  the final tree. The filters unit tests pass on the final code.
- **Docs:** the help, help-library and settings checks pass.
- **Docs updated:** FILTERS-CONFLICTS, SPECIFICATIONS, MANUAL and IDEAS 19.247.

## An open question

OPTIMISE FILTERS leaves each line's display mode as it is, so a line whose mode is off stays off after
it. Should OPTIMISE also turn those lines to mark (or compact)?

## Files to delete by hand

None.
