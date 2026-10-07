# COST's balance

Against `86a06f7` (uv unwrap optimisation idea).

## What changed
- **The balance**, at the top of the COST tab. It weighs the cheapest address found for the item
  in hand against the item as a file, and gives the change as a share of the file:
  `Neutral : 0%`, `Positive : X%` (the address is longer than the file) or `Negative : X%` (it is
  shorter).
  - **The file:** the one F saves (and J opens): a page as text, a picture as a PNG, notes as MIDI
    or WAV, a model as .obj, a file on the binary line as itself.
  - **The cheapest address**, under the filters and the line as they stand: the address the item
    is held by (compact among the survivors on a compact shelf, guided when walking guided), its
    guided address, and its variable length route. The route is worked out on a worker, so the
    balance can change once it arrives.
  - **The bar:** grows outward from a line at the centre, red for Positive and green for Negative,
    bordered in white. It reaches full width at 100% and stays there; past that, only the figure
    grows.
  - Under the bar, which way was cheapest, and both sizes in bits.
- **Two new rows:** "as a file" (its bits and bytes), and "compact" for a survivor of a compact
  shelf (its number among the survivors).
- **Notes fitted to the panel:** a row's note is cut to the panel's width (the route's note ran
  off it).

## A point to know
A titled item's file holds its units only (a book's, its title and pages), while the address
names the title and cover too. The "as a file" row says so ("without the title or cover"). So on a
titled line a random item reads Positive: on the binary line a 32-byte file is 256 bits, its
address 408 (screenshot `binary4.png`).

## Checked
- **Build:** no warnings (Linux, GCC 13), client and tools.
- **Unit tests:** pass.
- **Screenshots:**
  - `text.png`: Negative 78.1%, by guided.
  - `binary4.png`: a random address, Positive 57.8%, red.
  - `compact.png`: a compact image shelf, with the compact row.
  - Also checked, not included: image and binary items near a bearing (Negative, by the route).

## Docs
- **README:** the balance, under the COST tab.
- **HANDOFF:** an entry.
- **IDEAS §12:** the optimise-all note marks the balance built. The search is left: the filters
  and values that make the item's address cheapest while keeping it.

## Still open
- **1e's search:** try filters and their values for the cheapest address that keeps the item in
  hand (the anchor), and show it beside what the filters as they stand give.
- **1f;** filters made only for the joins; the CLI's MIDI reading on notes104/notes2; the
  colour-blindness pass.
