# Changes

One commit on top of b9e375e ("sieve O1").

## Pages of a range of lengths: O2 and O3
- **O2, no padding on a range line:** a text warped onto a line of a range of lengths keeps its own length; only a
  text shorter than the line's minimum is padded, up to the minimum. On a line of 1 to 32, "welcome to the sieve" is
  a page of exactly 20 characters. Lines of one length (all of them today) are padded as before: every pinned
  address is unchanged (CI step 02), and the existing canonicalisation vectors are unchanged.
- **O3, filters over a range:** the filters of a range line count and rank across every length: each length has its
  own stack, the counts add up, and a survivor's number is the survivors of every shorter length plus its number
  within its own. The degree window works over the whole range by a page's place. Checked on every page of 1 to 3
  letters.
- Nothing on the packing path changes: .sieve files and items are made exactly as before.
- Nothing uses the range yet; the hallway, menus and CLI come in O4 to O6.
- The version goes to 0.13.80 when list O is finished (REVIEW-ROUND O11).

## Files to delete by hand
None.
