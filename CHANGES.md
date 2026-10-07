# The cleanest digits, files weighed, and full mode

Against `cf291d1` (tailor search). This includes the last round (full mode, and IDEAS §15), which
isn't committed yet: its own notes follow at the end.

## The search's target: the cleanest digits
- **What changed:** the search no longer pushes for the fewest survivors. It scores a stack by
  each anchor's **shortest route** among the survivors: its number with leading zeros dropped, or
  a bearing and a walk. So it pushes the item's own number towards 0 and takes whatever it can
  land on.
- **How:** ranks jump about as a stack changes, so searching on routes alone gets stuck. Stacks
  are grown two ways from every setting kept: by shrinking the count (which bounds every number)
  and by shortening the route. The shortest route wins.
- **Results:**
  - the melody: 16 bits (`b4f7`, survivor 46327 of 204994), as short as by the count;
  - "welcome to the sieve": 84 bits, with a stack that costs 41.5 bits to write down (it was
    58.4).
- **Several anchors under one stack:** `sieve tailor --unit all`.
- **The description:** a stack is priced at one bit for each filter offered, plus each setting at
  log2 of its values. `--describe` makes the search pay for it, as it must wherever the stack
  travels with the addresses.
- **COST's K section** now says "tailored, shortest route" and gives the stack's description in
  bits.

## Files weighed: the locator, the manifest maker and the File Locator
- **The base:** each file's own address on the binary line, as long as the file.
- **Its other ways:**
  - its number among the binary line's survivors, where `[binary]` filters rank and it passes
    them;
  - where it is exactly an item of the pages, image, audio or video line (byte for byte what F
    saves), its place there and its number among that line's survivors.

  Each number is weighed as its shortest route.
- **What's shared:** each line's shape (symbols and length) and each stack is paid once for all
  the files that use it. A line or stack that costs more than it saves is dropped.
- **`--tailored`:** each line's filters are tailored to the files that are its items, with the
  description paid for. `--out-filters` saves the result.
- **`sieve locate FILE|FOLDER --weigh [--tailored [--out-filters PATH]] [--page-length N]`.**
- **The File Locator** weighs everything it reads with the hallway's lines and filters. It has a
  **Tailor the filters to these files** button, and then **Use the tailored filters**, which saves
  them and builds the hallway again, as Return on COST does.
- **One test for "exactly an item":** `item_of()` returns the unit, and not-an-item-v1's judges
  now use it too.

## Measured
| Folder | Own addresses | Weighed |
| :--- | :--- | :--- |
| Sieve's own files: 3 pages, 3 pictures, 2 melodies | 4,216 bits | 699 bits (16.6%), tailored |
| CI: 2 melodies, a picture, a note | 2,488 bits | 530 bits (21.3%) |

A file that isn't one of Sieve's own items keeps its own address: weighing doesn't compress, it
finds the files a line already names shorter.

## Not done yet: the installer
The installer still holds every file's bytes. I've proposed a `sieve-manifest-v4` in IDEAS §12
that would name files by these ways, with the shapes and stacks once at the top. It's a format
change, so it waits for your go-ahead. So does fitting a line to each file (a page of the file's
own length and alphabet), which would let far more ordinary files be named shorter.

## Checked
- **Build:** no warnings (Linux, GCC 13), tools and client.
- **Unit tests:** 47,426 checks, 0 failures.
- **CI's filters step**, run here in full, passes. It now:
  - tailors the melody (survivor 46327 of 204994, route `b4f7`, compact `0b4f7`) and reads it
    back;
  - weighs the folder above.
- **Screenshots:**
  - `locator.png`: the File Locator weighing a folder;
  - `locator-tailored.png`: the same, tailored, with **Use the tailored filters**.

## Docs
- **README:** weighing, `--weigh`, the new target for K and `sieve tailor`.
- **SPECIFICATIONS §12.2:** weighing.
- **`sieve help tailor` and `sieve help locate`.**
- **HANDOFF:** an entry, and the file map.
- **IDEAS §12:** what was built, and what's open.

---

# From the last round: full mode, and the rotation transform set aside
- **Full mode:** compact with the titles and covers kept, on the pages, image, audio and video
  lines (cover × title × survivors). It's `mode = full` in the settings file, and the menu cycles
  off, mark, hide, compact, full, excluded. Return keeps a line that's full in full mode.
  Screenshots: `full-shelf.png`, `menu.png`.
- **IDEAS §15, Deprecated ideas:** the rotation transform and the decimals-only format.
- **IDEAS §12:** the bitmask entry.
