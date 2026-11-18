# Sieve: page filters, the next round (list P)

One commit on top of b2fbd5a.

## Three new page filters

Each one is an automaton, so it is counted exactly, merges with the line's other filters, and OPTIMISE
FILTERS ticks it.

- **word-shape-v1:** no two words in a row both longer than 10 letters (adjustable, 4 to 30). A language's
  long words come one at a time.
- **punctuation-v1:** no space before , ; : . ! or ?, and after one a space, a line feed, a closing
  ) ] } " ' or the end of the page. After . ! ? another of them may follow. A digit may stand either side
  of , : and . (1,000, 12:30, 3.14). It only appears on alphabets that have these marks.
- **letters-and-digits-v1:** a run of letters and digits switches between them at most once (adjustable,
  0 to 6). "3rd", "1990s" and "x86" pass; "a1b2" doesn't. It only appears on alphabets with digits.

## What they keep

Measured on pages of 1,000 characters:

| Filter | Random pages kept | Don Quixote passes | Manual passes | Specifications pass |
| :--- | :--- | :--- | :--- | :--- |
| word-shape-v1 | 10^-8.39 (lower27) | 97.8% | 99.0% | 97.6% |
| punctuation-v1 | 10^-25.61 (ascii96) | 87.3% | under a fifth | under a fifth |
| letters-and-digits-v1 | 10^-16.36 (ascii96) | 89.8% | 91.7% | 89.2% |

- **Why some real pages fail:** most of the Don Quixote pages that fail contain the Gutenberg file's
  illustration names (p007b.jpg, c06a). The manual and specifications fail on file names, markdown and
  notation such as C4q and 2x32.
- **word-shape on ascii96:** it sets aside little there (10^-2.94), because random ascii96 pages are mostly
  short words between symbols.
- **The optimised Pages figure** stays at 10^-20.38. OPTIMISE ticks word-shape, but at the default
  32-letter page the dictionary filters already remove everything it would.

## Not built, and why (recorded in IDEAS 19.245)

- **Sentence length:** real prose needs room for 150-word sentences (97.1% of Don Quixote's pages pass at
  150), and random pages hit a full stop about every 32 characters, so it would remove almost nothing.
- **A line of verse:** it can only be judged, not counted.
- **No typo filter:** your decision, recorded in IDEAS 19.246 in your words.

## Also recorded

Compact on the fractals line is left for later, with the reasons (IDEAS 19.250 and REVIEW-ROUND F-B).

## Checks

- **Unit tests (step 01):** each automaton against its rule written as plain code, on sample sentences
  and 20,000 random pages, plus a check that they count and merge.
- **CI step 10:** the three random-page figures.
- **CI steps 01, 03, 04 and 10** pass, as do the help and help-library checks.

## Files to delete by hand

None.
