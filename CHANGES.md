# Tailoring the filters to the item in hand (1e, begun)

Against `b322f7c` (balance).

## What changed
### The search (`tools/cli/tailor.*`)
The item is the anchor: whatever is chosen, it survives. The search looks for the filters, and
their settings, that leave the fewest survivors with the item among them, so that its compact
address is as short as the search can make it.
- **Each filter on its own:** every filter that can be counted is tried at the settings it has,
  then each setting in turn over the values it allows:
  - an integer over its range, coarsely (7 values), then between the best value's neighbours,
    until they are adjacent;
  - a text over its choices, or every registered dictionary.

  A value is kept only where the item passes, and of those the one with the fewest survivors.
- **The set:** from each filter kept, strongest first, a stack is grown by adding every other
  filter where it can be counted with those already there and removes more. The best stack wins.
  A filter that counts its own way (title-data, window-data) clashes with the rest, so it makes a
  stack of one; the automata, which merge, make another.
- It is a search, not a proof: values are sampled and the set is grown greedily. Every count it
  reports is exact.

### The hallway: K and Return on the COST tab
- **K**, holding an item on the pages, image, audio or video line (not guided), runs the search on
  a worker and shows COST. The tab says how far it has got; K again stops it.
- **When it is done,** the tab shows the item's **compact, tailored** address (bits, characters,
  and a share of the address you hold it by) and each filter with the settings found.
- **Return uses them:**
  - this line's filters are replaced by those found, in compact mode;
  - they are saved to the settings file, as the setup menu saves them;
  - the hallway is built again on the same line, with the item in hand on its COST tab, so the
    balance shows what the new filters do for it.

### `sieve tailor`
The same search from the command line. It prints each filter with the settings found and its
survivors alone, ticked where it was used and otherwise why not; then the tailored compact address
and the survivor number. `--out` writes the settings file; `--unit N` picks the unit of the input.

## Measured
| Item | Unfiltered | Tailored | Time | Stack found |
| :--- | :--- | :--- | :--- | :--- |
| melody, 8 notes | 53.6 bits | 16.3 bits | 2 s | key-data-v2 (C major pentatonic), melody-lengths-v2, melody-range-v1, melody-ending-v2, melody-rests-v1 |
| page, 32 characters | 152.2 bits | 67.3 bits | 30 s | words-data-v2 (scowl-en-35), word-cost-v1, letter-triples-v1, max-run-v1 |
| mono picture, 8×8 | 64 bits | 9.3 bits | 0.3 s | neighbour-agreement-v1 |

## Checked
- **Build:** no warnings (Linux, GCC 13), client and tools.
- **Unit tests:** 47,426 checks, 0 failures.
- **CI** (added to the filters step, and run here): it tailors the melody, checks the scale found
  and the survivor number (4851 of 78125), warps the melody compact under the file written
  (`012f3`), and reads it back.
- **Hallway screenshots** (`--tailor PATH` waits for K and saves what Return would apply):
  - `running.png`: K pressed from the item tab, searching;
  - `found.png`: the result, with the filters and their settings;
  - `applied.png`: the saved settings opened, with the melody compact at 16 bits and the balance
    at Negative 98.1%.
- **Not run:** I read through the Return path in `app_main` (save, rebuild, the item in hand
  again) but could not drive it: there is no way to send keys to a live window here. Please try it
  on Windows: K, wait, Return.

## Docs
- **README:** K and Return under the COST tab; `sieve tailor` with an example.
- **`sieve help tailor`.**
- **HANDOFF:** an entry, and a file-map row.
- **IDEAS §12:** the search marked as built, and what is still open.

## Still open in 1e
- **Several anchors at once** (a map's items, a folder).
- **The line's own settings** (alphabet, palette, length) as part of the search.
- **The other lines:** books, models, binary, tracks and movies, and the guided ordering.
- **Settings searched together** rather than one at a time.
