# Changes

One commit on top of 54d09c8 ("sieve O5").

## Pages of a range of lengths: O6, the setup menu and the new default
- **MIN LENGTH and MAX LENGTH:** the PAGES rows in the setup menu begin with them. MIN LENGTH never passes MAX
  LENGTH, and lowering MAX LENGTH brings MIN LENGTH down with it. MIN = MAX is the line of one length, every address
  as it was.
- **The default is 1 to 32:** a fresh start spawns at "welcome to the sieve" as a page of its own 20 letters, and a
  short text is a page of its own length, with no padding.
- **Your settings file:** a file from before this has a `length` and no `min_length`, and keeps its one length. Yours
  will still be 32 to 32: set MIN LENGTH to 1 in the setup menu (or delete the `length` line from `[setup]`) to
  get the new default.
- **Command line:** `--length` alone is one length, as before; `--min-length M` with it makes a range.
- **The menu's figures** (sizes, the map, the survivors counted) are over every length of the range.
- **FIND MY LIMITS stays quick:** the range's size is worked out in closed form rather than one power per length. Done
  per length, FIND MY LIMITS (which tries pages of tens of thousands of characters) took about 50 minutes here; now 6 seconds.
- **Unchanged:** titles and the books' pages stay at the longest length, and the packer's items are of one length as
  before. A page the hallway saves shorter than 32 is carried in a `.sieve` as its own bytes, exactly.
- **CI:** step 10's pins moved with the default. The guided zooms are shorter (the greeting is 48 bits now), the
  compact count is over the range, J opens "hello world" as an 11-byte file, and the v5 sample carries 1,350 items.
- **Docs:** MANUAL, SETTINGS (from the code), REVIEW-ROUND O6.

## Files to delete by hand
None.
