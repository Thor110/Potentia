# Changes

One commit on top of 8ad6e67 ("sieve O2O3").

## Pages of a range of lengths: O4, the hallway
- **`--min-length`** on the pages line, in the hallway and the CLI (`hallway --min-length 1`, `sieve warp
  --min-length 1 --length 32 ...`). The setup menu's MIN LENGTH and MAX LENGTH rows, the settings file and the
  new default of 1 to 32 come in O6; until then nothing changes unless the option is given.
- **The pages line on a range:** shelving, walking, warping, the titles (which stay at the longest length) and
  the books (their pages stay at the longest length too). The hallway spawns at "welcome to the sieve" as a page
  of exactly 20 letters.
- **Compact and full on a range:** a filter stack on a range line counts, ranks and unranks over every length
  (list O3), so compact and full work as on any line. The guided order is not offered on a range yet (O5); the
  hallway shows it positional instead.
- **Items at any length:** a file is a page of the line when it is exactly the text of a page of any length in the
  range, byte for byte (the File Locator, F, not-an-item). not-an-item-v1 on the binary line sets aside the pages
  of every length of the range.
- **Lossless as before:** lines of one length are unchanged (every pinned address, CI step 02), and nothing on the
  packing path moves.
- Tests: the range stack against RangeStack and a compact line in both orders on every page of 1 to 3 letters,
  and not-an-item over pages of two lengths against every file of up to 2 bytes. CI step 02: a bare page (1 to 32)
  and a padded one (5 to 32, "hi   ") each warp and read back exactly, in both orders.

## Files to delete by hand
None.
