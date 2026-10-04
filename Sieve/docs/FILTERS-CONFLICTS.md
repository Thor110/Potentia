# Filters that cannot be counted together

A line compacts (only its survivors on the shelves, closed up, with exact counts and addresses)
only when its whole ticked stack can be counted as one. Each filter below counts exactly on its
own, but some pairs cannot yet be counted *together*. Until they can, ticking one of such a pair by
hand unticks the other, and the menu shows why in red on the selected filter. There are two
messages:

- **filters need merging.** Both filters are automata underneath (state machines over the unit's
  symbols), so they *can* be counted together exactly. They just do not yet hand the engine their
  automata to merge. This is fixable, and it is the next piece of work.
- **conflicting filters.** One of them is counted by arithmetic on the whole unit, not by walking
  its symbols, and there is no exact way known to count it together with anything else.

The rule lives in one place (`filter_conflict`, `core/src/filter.cpp`), from how each filter
counts (`FilterSpec::counts_as`):

| Counts as | Filters | Combines with |
| :--- | :--- | :--- |
| automaton | every custom filter (the `.sfilter` plugins: clean-data, max-run-data, tidy-data, window-data, words-data, the Moby grammars, key-data, the melody filters; not title-data, which has `within`), `max-run-v1`, `not-other-line-v1`, `not-packed-v1`, and on a small palette `palette-size-v1` and `row-runs-v1` | other automata, and `not-written-v1` |
| written | `not-written-v1` | automata |
| own ranker | `clean-v1`, `window-v1`, `words-v1`, `clean-v2`, `window-v2`, `words-v2`, `title-v1`, `title-data-v1` (all but the last retired), `neighbour-agreement-v1`, `key-v1`, `utf8-valid-v1`, and on a large palette (rgb24) `palette-size-v1` and `row-runs-v1` | only a filter it implies, or that implies it |
| arithmetic | `not-a-file-v1`, `not-a-pattern-v1` | nothing |
| model rule | `distinct-vertices-v1`, `distinct-indices-v1`, `every-vertex-used-v1`, `canonical-mesh-v1` | each other (canonical-mesh implies the first two and counts with every-vertex-used) |
| judges only | `symbol-entropy-v1`, `model-information-v1`; on the binary line `binary-kind-v1` and `not-an-item-v1` | not part of this rule (see below) |

A filter never conflicts with one it implies, or that implies it: `words-v1` implies `clean-v1`
and `window-v1`, `window-v1` implies `clean-v1`, the v2 versions the same, and `title-v1` implies
`clean-v2`, `window-v2` and `words-v2`.

## Line by line

**Pages (and the books' title and pages parts).**
- *Filters need merging:* any word filter (`clean`, `window`, `words`, v1 or v2, `title`) with
  `not-written-v1`, `not-other-line-v1` or any custom filter; and the word filters with each other
  where neither implies the other (`clean-v1` with `clean-v2`, `words-v1` with `window-v2`,
  `title-v1` with `words-v1`, and so on); and the word filters with `max-run-v1`.
- *Conflicting filters:* `not-a-file-v1` and `not-a-pattern-v1` with each other and with every
  counting filter above.
- *No conflict:* the custom filters, `not-other-line-v1` and `not-written-v1` together (merged
  exactly today).

On a line of every byte (`bytes256`) only `not-written-v1`, `not-a-file-v1` and
`not-a-pattern-v1` count, and the last two conflict with the first and with each other.

**Image and video.**
- *Filters need merging:* `neighbour-agreement-v1` with `not-packed-v1`, `palette-size-v1` and
  `row-runs-v1`. On a small palette `palette-size-v1` and `row-runs-v1` are automata (FilterSpec
  `counts_as_on`: built where states times symbols stay under 2^24, judged at the largest settings
  the line allows), so they merge with each other and with `not-packed-v1`; on a large one (rgb24)
  they count their own ways and need merging with each other and with `not-packed-v1` too.
- *Conflicting filters:* `not-a-file-v1` and `not-a-pattern-v1` with each other, with
  `neighbour-agreement-v1` and with `not-packed-v1`.

**Audio.**
- *Filters need merging:* `key-v1` with any custom filter (key-data, the melody filters).
- *Conflicting filters:* `not-a-file-v1` and `not-a-pattern-v1` with each other, with `key-v1`
  and with every custom filter.

**Models.**
- *Conflicting filters:* `not-a-file-v1` with `distinct-vertices-v1`, `distinct-indices-v1` and
  `every-vertex-used-v1`. The three rules count together exactly.

**Binary.** No conflicts: `binary-kind-v1` and `not-an-item-v1` count together exactly when
not-an-item asks only for pages. `utf8-valid-v1` ranks on its own; with either of them it is
counted with them exactly (the kind is decided by the first 16 bytes, so the head is walked by
both automata side by side, then UTF-8's completions), but compact needs it alone, so the line
hides and says why. Past its table (about 7.4 KB) its survivors are estimated (`~` in the tally). With melodies, pictures or models it judges file by file, and
the line hides and says why (see below).

## Why each kind conflicts

**Filters need merging.** Counting a stack means counting the units every filter passes. For
automata that is one automaton, their product, walked once, and the engine already does it for the
custom filters, `not-other-line-v1`, `not-packed-v1` and `not-written-v1`. The word filters,
`neighbour-agreement-v1` and `key-v1` are automata too: a word trie with a few flags, a window of
neighbouring pixels, a key's allowed notes. But they count with their own machinery and do not
expose the automaton, so the engine has nothing to merge. Once they do, these pairs stop
conflicting. A merged automaton can be too big to count (a large dictionary crossed with a Moby
grammar). The merge will be built with a size limit so that it gives up quickly, and then the line
hides and says why, without stalling.

**Conflicting filters.** `not-a-file-v1` asks whether the unit's own number, as a place on the
binary line, holds a file with a signature. That is a question about the number as a whole in base
256, while every other filter reads the unit symbol by symbol in its own base. Some signatures leave
gaps (WAV, AVI and WEBP: "RIFF", four free bytes, then a tag; MP4: four free bytes, then "ftyp"),
which split what it removes into billions of separate ranges of numbers. No small state machine
follows them. `not-a-pattern-v1` removes repeats of a block of up to 16 values: a machine that
checked them symbol by symbol would have to remember the block, which is far too many states.
Each counts exactly alone, by arithmetic, but there is no exact way known to count either together
with another filter. The models line's `not-a-file-v1` is the same number read on the binary line,
against rules that walk the model's digits.

## Not in this rule: filters that judge only

`symbol-entropy-v1` (except on black-and-white pictures, where it counts) and
`model-information-v1` measure the whole unit (a spread of symbols, an information content) and do
not count survivors at all. (`max-run-v1` was one of them, and is now an automaton: its state is the
last symbol and how often it has come in a row, so it counts, ranks and merges.) Ticking one means that line hides, whatever else is
ticked. On the binary line, `not-an-item-v1` with melodies, pictures or models judges file by file.
These are not auto-unticked: they are the next thing to discuss.

## Retired filters

A retired filter stays loadable (earlier stacks reproduce), on a third tab of the filters window,
and is ticked only by hand (or by Z on that tab). There are two reasons to retire one.

**It judges only.** `symbol-entropy-v1` and `model-information-v1`: a line with one ticked cannot
compact. If a countable way to express them is found, they come back as new versions.

**An automaton does the same** (`FilterSpec::replaced_by`, shown in the list and by `sieve
filters`). The built-ins below count with their own machinery, so they clash with every automaton;
their replacements are the same rule as automata, which count at any length and merge with all the
other automata, so retiring them loses nothing and removes the clashes. Each pair was checked to be
the same set, not only the same size: equal exact counts at several lengths, and survivors of each
sampled and judged by the other, both ways (and the unit tests and the oracle as listed).

| Retired | Replaced by | Checked |
| :--- | :--- | :--- |
| `clean-v1` | `clean-data-v1` | counts at lengths 1 to 200, every unit judged the same (existing tests) |
| `clean-v2` | `clean-data-v2` (new) | counts at 1, 2, 3, 4, 6, 32, 200; every unit up to 4 symbols; padding-heavy units at 32 and 200; the oracle's own engine at 1, 2, 5, 17, 40 |
| `key-v1` | `key-data-v2` | counts and sampled survivors on all 12 tonics x 6 scales at lengths 6 and 16; `key-data-v2` also covers every note set, `key-v1` only notes104 |
| `key-data-v1` | `key-data-v2` | its tonic-only major key is `key-data-v2`'s major (plugins are retired by name and SHA-256, so someone else's file of the same name is not) |
| `max-run-data-v1` | `max-run-v1` (built-in) | the same rule, both automata; the built-in covers every text alphabet. Exhaustive at length 3, run-heavy units at 24 (tests) |
| `tidy-data-v1` | `tidy-data-v2` | it requires `max-run-v1` (`max_run=3`) in place of `max-run-data-v1`; with what each brings, the same counts at 1, 2, 5, 12, 32, 64, 200 and 400 and the same verdicts (tests, CLI, and the oracle in CI) |
| `title-v1` | `title-data-v1` | the token form with `padding trailing` and `within max_length`: the same counts at 12, 80 and 200 with `max_length` 10 and 64, every unit to length 4, and the oracle's own build in CI. Both count on their own (not a merge, but one machinery fewer) |
| `melody-lengths-v1`, `melody-ending-v1` | `melody-lengths-v2`, `melody-ending-v2` | the same rules on every note set, by real lengths (`LENGTH`, `SIXTEENTHS`): on notes104 the same counts at every setting of their choices (tests, and the CLI at 3 and 8 notes) |
| `words-v1`, `window-v1` | `words-data-v1`, `window-data-v1` | the token form; where its automaton's table does not fit (long pages) it counts by its words' lengths as the built-ins do: the same counts at 1 to 9 (every unit to 4), 20, 32, 400, 1,500 (forced, at 1 MB) and 3,200 |
| `words-v2`, `window-v2` | `words-data-v2`, `window-data-v2` | the token form with `padding trailing`: the same counts at 1, 2, 5, 12, 32, 80, 400, 1,500 (forced) and 3,200, every unit to length 4, padding-heavy random units, and the oracle's own build in CI |

## Merging and retiring: what is left

With each filter's share alone in the list (Filtered: X%), the remaining duplicates and clashes, and
what stands in the way of each:

- *Done (4 October 2026).* **`words-v1`, `window-v1`, `words-v2` and `window-v2`** retired for
  `words-data-v1`, `window-data-v1`, `words-data-v2` and `window-data-v2`. A plugin of one
  dictionary set and no grammar (SPACE between tokens, on lower27) now counts long units the way the
  built-ins did, by its words' lengths, wherever its automaton's table would not fit the filter
  memory (plugin.hpp `word_counting`); below that it is an automaton and merges as before. The same
  counts at 400 and 3,200 characters, and 20,000 counts in under a second.
- *Done (4 October 2026).* **`title-v1`** retired for **`title-data-v1`** (words with padding,
  `within max_length`), the same counts at every length and setting tried. Both count on their own.
- *Done (4 October 2026).* **`max-run-data-v1` and `tidy-data-v1`** retired for the built-in
  **`max-run-v1`** and **`tidy-data-v2`**, which requires it (`max_run=3`): the same counts.
- *Done (4 October 2026).* **Pictures:** `palette-size-v1` and `row-runs-v1` are automata where
  they are small (row-runs: the column, the changes so far in the row and the last colour;
  palette-size: the colours used, and per frame the place in the frame), and merge with each other,
  with `not-packed-v1` and with plugins: on the black-and-white image line, one colour with both and
  `not-packed-v1` counts exactly (2). On rgb24 they keep their own rankers. Every picture vector
  (counts, ranks both ways, rgb24 included) passes through the automata unchanged.
  `neighbour-agreement-v1` (a transfer matrix) is left as it is: it does not count at all at 16x16
  or 5x5x8, so there is nothing to merge.
- **Arithmetic** (`not-a-file-v1`, `not-a-pattern-v1`) conflicts with everything that counts; that
  is in their nature (they test the unit's number, not its symbols) and is not a merge to make.

## Ticking many at once

Z (this tab), C (both main tabs) and X (both main tabs of every line) untick everything in reach if
anything is ticked, and otherwise tick it all, skipping any filter that clashes with one already
ticked. The kept one of a clashing pair is the one that filters more on its own, its settings as
they stand (each is counted alone on the workers first, which can take seconds: the footer says so,
and the window answers meanwhile); then newer versions first, the arithmetic filters last. `title-v1` is ticked only on a book's title part, and retired filters never. A settings file edited by hand
is never changed when it is loaded: a stack with a conflict in it simply hides, and says why.
