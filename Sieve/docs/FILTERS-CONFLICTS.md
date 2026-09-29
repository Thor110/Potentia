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
| automaton | every custom filter (the `.sfilter` plugins: clean-data, max-run-data, tidy-data, window-data, words-data, the Moby grammars, key-data, the melody filters), `max-run-v1`, `not-other-line-v1`, `not-packed-v1` | other automata, and `not-written-v1` |
| written | `not-written-v1` | automata |
| own ranker | `clean-v1`, `window-v1`, `words-v1`, `clean-v2`, `window-v2`, `words-v2`, `title-v1`, `neighbour-agreement-v1`, `key-v1` | only a filter it implies, or that implies it |
| arithmetic | `not-a-file-v1`, `not-a-pattern-v1` | nothing |
| model rule | `distinct-vertices-v1`, `distinct-indices-v1`, `every-vertex-used-v1` | each other |
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
- *Filters need merging:* `neighbour-agreement-v1` with `not-packed-v1`.
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
not-an-item asks only for pages. With melodies, pictures or models it judges file by file, and
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

`symbol-entropy-v1` and `model-information-v1` are **retired**: they judge only, so a line with one
ticked cannot compact. They stay loadable (earlier stacks reproduce), on a third tab of the filters
window, and are ticked only by hand (or by Z on that tab). If a countable way to express them is
found, they come back as new versions.

## Ticking many at once

Z (this tab), C (both main tabs) and X (both main tabs of every line) untick everything in reach if
anything is ticked, and otherwise tick it all, skipping any filter that clashes with one already
ticked. The kept one of a clashing pair is the more useful: newer versions first, the arithmetic
filters last. `title-v1` is ticked only on a book's title part, and retired filters never. A settings file edited by hand
is never changed when it is loaded: a stack with a conflict in it simply hides, and says why.
