# Changes

One commit on top of f150a70 ("sieve N5 finish").

- **Windows build fix:** the degree-window and place-window tests used a 128-bit integer (`unsigned __int128`), which
  MSVC does not have (C4235 at test_core.cpp lines 3349, 3449 and 6706). They now work the bearing exactly with
  BigUint (`mul_div`). The tests pass as before (106 and 48 checks).

## Files to delete by hand
None.
