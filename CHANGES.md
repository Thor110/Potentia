# Speed, word frequencies, and a review (31 October 2026)

Unzip this over the repository root, on top of the last zip ("roadmap"). It is committed locally,
not pushed. Without `--v32-draft`, every `.sieve` checked comes out byte for byte as before, and
the unit suite passes (77,110 checks, 0 failures).

## 1. Speed (ROADMAP #4)

- **Two of the four items were already built** (ROADMAP was out of date):
  - the weighing reuses the packer's ways: 1 MB of Don Quixote takes 3.19 s with the weighing,
    against 3.33 s without;
  - the rope's decoding table is made only for long streams.
- **Mixing is about 12% quicker, at the same codes.** Each counter is now moved with its sign known
  from the bit, which takes 9.3% of the instructions off. All of Don Quixote with the word list,
  three runs each: coding 6.33 s → 5.60 s, decoding 6.40 s → 5.56 s.

## 2. Word frequencies for mixing (v32 draft: `F80`)

SCOWL has no frequencies, but its sizes rank its words, and the three lists Sieve ships nest
(35 ⊂ 60 ⊂ 80). Each word of scowl-en-80 now counts 4,096 if size 35 holds it, 64 if only size 60
does, else 1. No new file is needed. Against today's scowl-en-60 (mix-v4, bytes):

| Text | D60 | F80 | |
| :--- | ---: | ---: | ---: |
| Don Quixote, first 800 KB | 193,808 | 188,719 | −2.6% |
| HANDOFF, first 200 KB | 57,042 | 56,021 | −1.8% |
| SHOWCASE-PLAN | 3,247 | 3,047 | −6.2% |
| README, first 3 KB | 1,340 | 1,265 | −5.6% |
| GAME, first 3 KB | 1,242 | 1,133 | −8.8% |

- **Through the packer, with the draft:** one text 193,887 → 188,798 bytes; four texts mixed as a
  group 61,374 → 60,262.
- **Where it's built:** the reader refuses F80 below v32; the oracle builds its own trie and
  weights; `sieve mix --words 80 --by-use`; CI step 11 checks it. The weights were chosen by
  measuring (NEXT-VERSION §4a, 5, has the table).
- **On the way, the trie was made compact, for every build, at the same codes:** peak memory with
  scowl-en-60 91 MB → 59 MB, with scowl-en-80 279 MB → 120 MB.
- **Not measured:** enwik8 and enwik9, which aren't reachable from here. That's your run to make.

## 3. The review (docs/REVIEW.md, the fourth pass)

The work since your last sync was reviewed. Two more reviewers looked at what installing reads
from a `.sieve`.

**Main fix: a few bytes could claim gigabytes.** Installing sets no decode limit, so a damaged or
hostile `.sieve` could make the installer allocate whatever it claimed:
- 5.9 GB from a 108-byte file;
- 8.4 GB from an empty mixing code;
- 2.7 GB from a rope code of no bytes.

Every stream, mixing code and rope width is now bounded by what its bytes can make, before
anything is made. The same files are refused now at a 12 MB peak.

Also fixed:
- **`delta`'s pairing:**
  - blocks are cut on the hash's top bits, and at the ends of long runs of one byte;
  - a file of zero runs with insertions now finds 97.7% of its blocks in its original, against
    14.1% before;
  - the scripts themselves are made as before.
- **`sieve mix`:** `--words` and `--params` must name the same list.
- **The PNG re-save:**
  - an IHDR can't make it allocate past what its IDATs could hold;
  - each filter is made once, not twice, with the same bytes out.
- **JPEGs:** now recoded on every core, a stop heard at each one.
- **Repeated work:** SHA-256 is worked out once a file; the trie builds without allocating; the
  exact-PNG code is shared.
- **Listing numbers:** checked before they are read.

Left, and listed: the reader's audit was cut short, so a few corners (`skm`, `mixt`/`mixn` bounds,
`jbr`/`pngr` on installing) were read once rather than twice.

## Checked here
- Unit suite: 77,110 checks, 0 failures.
- CI step 11: the v32 stretch (delta, JPEG, PNG, F80, oracle, sieve-install) passes after the review fixes. The whole step, run after the reader's hardening, was still running when this was sent: I'll report it.
- The 160 MB delta pair: the same `.sieve` as before (17,947,687 bytes) under the new cut.
