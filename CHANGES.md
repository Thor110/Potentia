# Changes: IDEAS updated and proofread (20 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. One file: `docs/IDEAS.md`. No code changed.

## Size and decoding time as one setting: deprecated for now

- **§19.89:** its title is struck through and marked "deprecated for now, §17.3", as §19.80's CHAT
  point was. It also now says the decode times are your recollection; they weren't measured.
- **§17.3**, new in the deprecated ideas section: the proposal, and why you set it aside. As you
  recall, decoding takes fractions of a second, while the seconds go on loading and working in the
  hallway, which this wouldn't touch. It also notes that the deprecated ideas can be looked back
  over in the far future. That's in reported speech rather than a quotation, so as not to change
  your wording.

## Proofread

- **The whole file, checked by script:**
  - every reference to a section of IDEAS points to one that exists (the `12.x` ones point into
    SPECIFICATIONS);
  - no American spellings;
  - no doubled words.
- **This week's sections (§19.87 to §19.89), read through. Fixed:**
  - §19.87 still ended with a "Next (proposed)" step that has since been done and built. It's now
    "As first proposed (done since)", with what is still open listed: more phases, a wave per line,
    16- and 32-bit values, the byte above as context, and the other lines' places.
  - "the oracle decodes both apart" was unclear. It now reads "the oracle, written apart from
    Sieve's code, decodes both".
  - §19.88's two-photos point ended "(not measured)", though the next point records it done. It now
    reads "done the next day, below".
  - §19.88's version line now says plainly what was compared: GNU gzip 1.12 against zlib 1.3, and
    the `xz` program 5.4.5 against the liblzma Sieve was built with.

## Not touched

HANDOFF still refers to §19.89 as "noted for later". It's a one-line fix for the next round, since
this round was IDEAS only.
