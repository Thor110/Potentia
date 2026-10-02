# Locating hang with every filter maxed (relative to origin/main b8f093b)

**Cause.** BinarySieve::unrank with not-an-item-v1 binary-searched the survivor number across every
excluded page (27^32 ~ 2^152). On an 8.5 MB line that meant ~152 whole-file unranks (~140 ms each)
per item, times 64 items: the room never finished, so LOCATING never cleared.

**Fix (exact, same files).** Files run shortest first and every page is page_bytes long, so a
survivor past the page-length block has all pages below it: unrank(k) = counter.unrank(k + pages_in_).
The search is kept only for survivors of page length or shorter.

**Speed-ups (no behaviour change).**
- find_room_files(): the room's left-wall files are worked out on every core, then picked up by book().
  Compact: file by survivor number. Otherwise: title, head, place and, for filters needing the whole
  file (utf8-valid, not-an-item), the verdict.
- room_unit / file_place: the room's first unit and its file are computed once per room, not per item.
- filekind.cpp helpers build numbers from bytes, not hex strings; BigUint::mod_small (no copy) for short_big.
- Timing phases hallway.walk.length / .place / .room / .find.

**Measured** (4 cores, hallway.exe 8.5 MB, graph walk):
| Stack | Before | After |
|---|---|---|
| binary-kind compact | 9.9 s | 5.1 s |
| not-an-item compact | never | 5.7 s |
| utf8-valid (hides; can't rank that long) | 7.0 s | 3.0 s |
| all three | 7.0 s | 2.9 s |

**Checks.** Core suite: 49334 checks, 0 failures. CI walk checks pass (locate a.txt, big.bin past
length, graph walk, SORT tab, save-item), and so does the graph walk with all three binary filters in
compact and hide.

Files: core/src/filekind.cpp, core/include/sieve/filekind.hpp, core/{include/sieve/biguint.hpp,src/biguint.cpp},
client/hallway.{hpp,cpp}, client/file_locator.cpp, docs/HANDOFF.md.
