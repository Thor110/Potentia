# Filter list round (relative to origin/main b8f093b)

`files/` holds every changed file at its repository path. It includes the previous package (the
Locating-hang and thin-mode fixes), since that isn't committed yet. `full-vs-origin-main.patch` is
everything; `this-round-only.patch` is just this round, on top of the previous package.

## (1) Bugs
- **utf8-valid broke binary's percentage.** Past ~7.4 KB its ranking table doesn't fit, and with
  binary-kind or not-an-item it "counted on its own", so the tally gave up ("not countable").
  It is now counted:
  - **Method:** a file's kind is decided by its first 16 bytes, so the kind automaton and the UTF-8
    automaton are walked together over the head, then multiplied by UTF-8's completions for the rest.
  - **Exact** where the table fits (≤ ~7.4 KB).
  - **Estimated** past that, as a scaled-double matrix power, marked `~` in the tally (8.5 MB in
    microseconds).
  - **Compact** still needs utf8-valid alone; the footer says so.
  - **Checks:** the oracle computes the same count independently (its UTF-8 states come from Python's
    own decoder): 224 new vectors. Brute force over every file up to 2 bytes; estimate = exact to 1e-9.
- **X ticked neighbour-agreement instead of row-runs on video.** X kept whichever of two clashing
  filters came first in the list. It now keeps the one that **filters more on its own**. The
  shares are counted in the background (the footer says "weighing..."), and the window stays
  responsive. Result at defaults: row-runs on image and video.

## (2) Filtered: X% beside each filter
Each filter row shows what that filter removes on its own, with its settings as they stand.
Counted in the background and cached ("Filtered: counting..." until then).

## (3) Merging and retiring: first round
Retired because an automaton does exactly the same (each pair checked: equal exact counts at
several lengths, plus sampled survivors judged by the other, both ways):

| Retired | Replaced by | Notes |
| :--- | :--- | :--- |
| `clean-v1` | `clean-data-v1` | |
| `clean-v2` | `clean-data-v2` | **new plugin**: clean-data-v1 plus a padding state; checked exhaustively to 4 symbols and against the oracle's engine |
| `key-v1` | `key-data-v2` | all 12 tonics × 6 scales; key-data-v2 also covers every note set |
| `key-data-v1` | `key-data-v2` | v2 adds the scale |

Retired filters stay loadable (old stacks reproduce) on the Retired tab, which now says what
replaced each one; so does `sieve filters`. Plugins are retired by name **and** SHA-256.

What's left, and what blocks each, is written up in `docs/FILTERS-CONFLICTS.md` ("Merging and
retiring: what is left"):
- **words-v1, window-v1:** exact twins exist, but the plugins can only count up to a few hundred
  characters.
- **words-v2, window-v2, title-v1:** need a padding option in the token form.
- **max-run pair:** tidy-data requires the plugin.
- **Pictures:** row-runs, palette-size and neighbour-agreement would need their automata exposed.

## (4) Note
`docs/IDEAS.md` §12: wiring optimise-all into the COST pages, covering scoring with anchors, searching
parameter values, choosing sets rather than pairs, and dropping implied filters.
