# List F, part two: world v2 as a grammar (F1's last piece), and the RAID's pieces (F2)

## F1: `sieve-personal-world-v2` as a filter plugin

- **`data/filters/format-world-v2.sfilter`** (1,881 states), compiled by `tools/grammars.py` like the
  other four. It holds every line in the order the writer writes them: `seed`, `day`, `ground`,
  `model`, `music`, the lines kept without being read, `characters placed`, then what is placed. There
  must be something after the seed, because a world that needs nothing of v2 is written as v1.
- **Its `music` line** is walked as the Media Player reads it:
  - each key's value within its bounds;
  - a note set's range within C2 to C7, at least an octave, in any spelling the reader takes (a
    melody's set in its one spelling);
  - durations listed once each, shortest first;
  - a later build's keys kept.
- **Held to the reader** by `tools/check_formats.py`: 561 documents, 525 verdicts the same, and 36 of
  the kinds a walk cannot weigh. Those kinds are:
  - the ground's corners out of order, or one given twice;
  - a model named twice, or placed but never named;
  - a melody's digits past its set's base, or not a whole number of units and voices;
  - numbers that no float is;
  - a world with nothing of v2's but where its places stand (that needs the seed's ground).
- The oracle's counts equal the engine's at 33 and 60 characters. At 33 there are 940 survivors: ten
  one-digit seeds, each with a kept line of one character.
- The catalogue's v2 entry names its grammar. FILTER-PLUGINS §23, IDEAS and REVIEW-ROUND are updated.

## F2: the RAID's pieces (IDEAS 19.210), fixed and not switched on

- **`sieve-raid-piece-v1`** (SPECIFICATIONS 12.0s, `core/include/sieve/raid_piece.hpp`):
  - A whole (a save sieved, then sealed) is cut into K pieces of data and M of parity, and any K of
    them are enough to make it again. The code is Cauchy Reed–Solomon over GF(2^8).
  - When pieces are joined, the whole is checked against its SHA-256, so a damaged piece is found.
  - Each piece has a three-line head and then its bytes. It is named by its SHA-256, like every blob
    sent between machines.
- **`sieve-raid-tally-v1`:** a line per machine giving the space it gives, the largest piece it keeps,
  and what it keeps now.
- **Placement:** every machine works it out the same way, one piece of a whole per machine, within
  each machine's limits. A whole with a piece that has nowhere to go is refused and said.
- **Leaving:** a machine going away hands its piece on first. The other pieces are placed again by the
  same rule.
- **How a whole is cut:** up to 256 KiB, one piece plus two copies (three in all, as sharing keeps
  now); above that, four of data and two of parity. **Both figures are chosen, not measured**, and
  the spec says so.
- **Commands to try it on files:** `sieve raid pieces FILE --out DIR`, `join DIR --out FILE`,
  `place FILE --tally T` and `leave FILE --tally T --address A`. Nothing is sent between machines.
- **Tests:**
  - Unit tests: every choice of K pieces for six shapes; refusals of heads not as written, of
    damage, and of one piece too few; placement, limits and leaving.
  - The piece names are pinned against a separate Python implementation, which gives the same six
    pieces byte for byte.
  - CI step 4 checks the commands on the 2.2 MB save.
- Catalogue entries for both formats (CI step 10's count of the "document" kind's doors goes from 9 to 10 with the tally); the `raid` help; IDEAS 19.210 "Built" note; REVIEW-ROUND F2.
- **Still to come before it is used:**
  - `share`, `check` and `fetch` sending pieces instead of copies;
  - the tally kept and agreed between machines;
  - the measurements of where splitting pays.

## Checked

- CI: all 12 steps passed (step 10 after its door count was updated; the other eleven on the run before, with the same code).
- The help check: 0 missing (683 / 202). The settings check: 141 match. Catalogue: 206 of 206.
