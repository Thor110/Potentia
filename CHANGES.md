# Filters, section 1: merges, retirements, padding, long pages, pictures, notes2 melodies (relative to origin/main e4d6866)

To apply it, do one of these from the Potentia repository root:
- unzip `files/` over the repository;
- or run `git apply filters-section1.patch`.

**New files:**
- six plugins in `Sieve/data/filters/`: `tidy-data-v2`, `words-data-v2`, `window-data-v2`, `title-data-v1`, `melody-lengths-v2`, `melody-ending-v2`;
- two test plugins in `Sieve/tests/plugins/`: `toy-padding-v1`, `toy-padding-whole-v1`.

## What retired, and for what
| Retired | Replaced by | What the replacement does better |
| :--- | :--- | :--- |
| `max-run-data-v1` | `max-run-v1` (built-in) | Works on every text alphabet, not only lower27 |
| `tidy-data-v1` | `tidy-data-v2` | Requires the built-in `max-run-v1` |
| `words-v1`, `window-v1` | `words-data-v1`, `window-data-v1` | Merges with the other automata, and still counts long pages |
| `words-v2`, `window-v2` | `words-data-v2`, `window-data-v2` | The same, with padding |
| `title-v1` | `title-data-v1` | The same rule as a plugin (padding and `within`) |
| `melody-lengths-v1`, `melody-ending-v1` | `melody-lengths-v2`, `melody-ending-v2` | Works on every note set, by real note lengths |

**Retired filters stay loadable:**
- earlier stacks and settings files reproduce exactly;
- they're marked retired;
- X and tick-all skip them.

**Each pair counts the same.** Every pair was checked at many lengths, and every unit was checked to
length 4 where that's possible. The details are in `docs/FILTERS-CONFLICTS.md`'s table.

## 1a. `tidy-data-v2`
**What it is.** It's `tidy-data-v1`, but requiring the built-in `max-run-v1` (`max_run=3`) in place
of `max-run-data-v1`.

**Checked.** The same counts at 12, 64 and 400 characters as v1. A different `max_run` in the stack
is flagged, as before.

## 1b. Padding and `within` in the token form
**`padding trailing`.** A unit ending in two or more SPACEs passes when what comes before them
passes on its own: the last page of a text.
- **How:** it changes the compiled automaton.
- **So:** a padded plugin is still one automaton and merges with the others.

**`within N`.** Past the first N symbols, only separators are allowed: a title on a page.
- **Counts on its own:** it does, as `title-v1` did. Merging it would multiply the automaton's
  states by the length.
- **N is an expression:** so `max_length` is a setting.

**The new plugins.** `words-data-v2`, `window-data-v2` and `title-data-v1` count exactly as their
built-ins. Words and window were checked from 1 to 80 characters. Titles were checked at 12, 80 and
200 characters, with `max_length` 10 and 64.

**The oracle has both, built its own way:**
- padding as two more states of its NFA;
- `within` as the count at N times the separators' choices.

**Engine and oracle agree:**
- on the new plugins;
- on two toy grammars with two separators and cut and whole edges.

## 1c. Dictionary plugins count long pages
**The problem.** A dictionary plugin's automaton needs a table of states × length. On a 3,200-character
page that doesn't fit, so it could only judge.

**The fix.** A plugin of one dictionary set, with no grammar, now counts the built-ins' way where that
table won't fit: by its words' lengths, keeping one row of counts.

**Checked.** The same counts as the built-ins at 400 and 3,200 characters. A 20,000-character page
counts exactly in under a second.

**So the four word built-ins retired.** Below those lengths the plugins are automata and merge as
before.

## 1d. Picture filters merge on small palettes
**What changed.** `palette-size-v1` and `row-runs-v1` now build an automaton where it's small enough:
states × symbols under 2^24, the bound `max-run-v1` uses. Each automaton's state:
- **row-runs:** the column, the changes so far in the row, and the last colour.
- **palette-size:** the colours used, plus the place in the frame when counting per frame.

**What merges.** The two filters merge with each other, with `not-packed-v1`, and with plugins.
- **Example:** on the black-and-white image line, a one-colour palette, both filters and
  `not-packed-v1` count exactly (2).
- **rgb24:** they keep their own arithmetic rankers.
- **Picture vectors:** every one still passes, rgb24 included.

**Whether filters clash now depends on the line.** A spec can say what it counts as on a given line
(`counts_as_on`), judged at the largest settings that line allows. The setup menu passes the line,
so X and the filter list see the picture filters merging where they do.

## `notes2` melody filters
**The new functions.** The v2 expression language gains `LENGTH(i)` and `SIXTEENTHS(k)`, in the
engine and the oracle. They give a duration's real length in sixteenths:
- `LENGTH(i)`: the line's own i-th duration;
- `SIXTEENTHS(k)`: the k-th of the codes `s e E q Q h H w`.

**The plugins.** `melody-lengths-v2` and `melody-ending-v2` work on every note set (`symbols notes*`).
- **On `notes104`:** they count exactly as the v1s at every setting.
- **On `notes2` sets:** engine and oracle agree on sets of other ranges and durations, with one and
  two voices.

## A CI fix on main
**What was wrong.** The oracle step checked that exactly 23 shipped plugins load. Commit ac9096e
added a 24th, so this check has been failing on main since then.

**The fix.** CI now compares the loaded count with the number of `.sfilter` files in `data/filters`,
with no fixed number.

## Checked
**Tests.** `tests/test_core.cpp`: 67,136 checks, 0 failures. New tests cover:
- the retirements, and the `tidy-data` equivalence;
- padding and `within`, including exhaustive comparisons to length 4;
- the long-page fallback, forced with 1 MB of filter memory at 1,500 characters;
- merging the picture filters, and rgb24 staying apart;
- the melody v2s on `notes104` at every setting.

**CI, run locally.** Every step I can run here passes:
- same addresses;
- filters;
- models and the setup menu;
- the bytes256 line;
- books;
- the whole hallway step;
- the reference oracle, with its new comparisons.

**Not run.** I didn't time X in the setup menu.

## Still to do in section 1
- **Optimise-all wired into COST:** it needs known real content per line to protect. Where that
  content comes from is your decision.
- **Ascii85 in `not-written`.**
- **The cross-line filters:**
  - pictures that are text or files;
  - MIDI that's also something else;
  - pages that are models, and the reverse.
- **Transformed copies.**
- **Models:** the third tier and an `.obj` filter.
- **Signed files:** checked past their signatures.
- **Titled lines:** filtered bottom-up.
- **Left as it is:** `neighbour-agreement-v1` doesn't count at all at 16×16 or 5×5×8, so there's
  nothing to merge.
