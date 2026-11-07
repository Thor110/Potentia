# Catch-up: every file changed since the sieve-defaults zip (7 November 2026)

The build error (`sieve/hex.hpp`: no such file) was mine: three commits made after the
sieve-defaults zip were never sent, and the zips after them held only their own changes. This zip
holds every file changed from 1a2ee55 (sieve-defaults) to a2c558a, so it replaces the
small-items and preflate-512 zips: unpack it over the repository.

Commits it covers, oldest first:
- 2d063f2 Balanced weighs E1 against mixing without it: 6% of the file asked of E1, 4% of mixing
- 6a15df7 A zip's skeleton made a batch ahead on the threads, and a 7z's contents read and estimated so
- a30ddd0 One copy each of u8/from_u8, a whole file's read and bytes as hex
- 147b4c9 binary-kind-v1: kinds takes several kinds apart by commas
- f55477d Encryption Engine: the catalogue on the screen
- 5a79658 Main menu: the completion estimate (IDEAS 19.49)
- 898017f Setup menu: survivors say how long they would take to walk
- 1b100d3 Read aloud in the setup menu and the File Locator
- 3d5f0ed CHAT /docs: a card of the text, with GO TO IT and J
- 8f2a1d5 CHAT /unit LINE HEX: the place at a number, as a card
- e770cb7 COST: L searches the shortest route further
- 24c1cd7 preflate: guard decodeBits against damaged counts
- c8996bc Roof results: 512 MiB with Optimise Search, the memory estimate held
- a2c558a IDEAS 19.116: WoWRevived's film packets measured, the route not worth building

The three not sent before:
- 2d063f2: Balanced weighs E1 against mixing without it (E1 asked to save 6% of a file, mixing 4%).
- 6a15df7: a zip's skeleton made a batch ahead on the threads, and a 7z's contents read and
  estimated the same way. The same bytes come out.
- a30ddd0: one copy each of u8/from_u8, a whole file's read and bytes as hex. It adds three new
  headers, `core/include/sieve/hex.hpp`, `paths.hpp` and `whole_file.hpp`, which the build was
  missing.

New since the zips you have: IDEAS §19.116, WoWRevived's film packets measured (0.0016% repeated:
not worth building).

No file was deleted in this range. The unit suite on these commits: 154,388 checks, 0 failures
(run after e770cb7; the commits since touch CMake's preflate step and documents only).
