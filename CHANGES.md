# Full mode, and the rotation transform set aside

Against `cf291d1` (tailor search).

## Why the titles went
Return didn't fail to reset anything. It saves the tailored filters in compact mode, and compact
mode on a titled line names the content alone; compact set in the setup menu does the same. The
settings file still said compact after a restart, so the titles stayed gone. As you put it,
compact should be compact. So the titles get a mode of their own.

## What changed
### A new display mode: full
- **What it is:** compact, with the titles and covers kept. On the pages, image, audio and video
  lines, every survivor stands with every title and cover, as on the bare titled line. Its address
  names its cover, its title and its number among the survivors. Compact still names a thing by
  its content alone.
- **The space:** cover × title × survivors, the content being the survivor's number in positional
  order. Its id is `titled/<content>/survivors=<stack id>+title=...+cover=.../key=<key>/titled-v1`,
  so scrambled order shuffles the whole of it, domain-separated by the stack. A unit warped in on
  its own has a blank title and cover.
- **Other lines:** on books, tracks and movies, which already keep their titles in compact, full is
  compact. On the models and binary lines it is compact for now. In the guided ordering the content
  alone is addressed, as in compact.
- **Settings:** `mode = full` in the settings file. The setup menu cycles off, mark, hide, compact,
  full, excluded, and the filters window explains full on a line of its own.
- **On screen:**
  - the readout says "N full";
  - the shelf readout and the item page say "full address";
  - COST's row reads "full: its cover, its title and its number among the survivors".
- **Tailoring:** Return keeps a line that is full in full mode, and its row says "full, tailored",
  with the title and cover bits added.

### IDEAS
- **§15, a new section of deprecated ideas:** the rotation transform and the decimal-places-only
  format, moved from §3.7. Your note is added: you hoped for a byte at least, and it is one bit at
  most, and nothing on an address.
- **§12:** your bitmask entry from the last round, unchanged.

## Checked
- **Build:** no warnings (Linux, GCC 13).
- **Unit tests:** 47,426 checks, 0 failures. `sieve filters` reads `mode = full`.
- **Screenshots:**
  - `full-shelf.png`: the audio line full under the tailored melody filters, the shelves titled,
    the melody survivor 4851 with a 68-digit full address;
  - `full-cost.png`: its COST tab;
  - `img-full.png`: the image line full under palette-size-v1;
  - `menu.png`: the filters window's mode help.

## Docs
- **SPECIFICATIONS:** §9's mode table and §11's titled lines (full mode's space and id).
- **README:** the mode list now has full, and excluded, which it had left out (so "one of six").
- **HANDOFF:** an entry.

## Still open
- **Full mode on the models and binary lines** (their compact titles are blank).
- **Other modes for other needs,** such as files with file names but no covers.
- **Filters that judge the titles and covers** themselves (1f: titled lines filtered bottom-up).
