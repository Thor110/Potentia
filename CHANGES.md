# Sieve: melody and sound filters (list M2)

One commit on top of 9f751d2.

## Three sound filters

- **sound-envelope-v1:** one note's shape, as a struck or plucked note's attack and decay. The sound is cut
  into blocks of 256 samples, each with a loudness (its loudest sample, in quarters of full scale). The
  blocks' loudness rises or holds, then falls, and never rises again. The loudest block must reach the
  second quarter, and the last must be two quarters below it.
- **sound-syllables-v1:** the rhythm of speech, 3 to 7 syllables a second. A block is loud where a sample
  reaches 12% of full scale, and each loud block after a quiet one begins a syllable.
- **sound-interval-v1:** two notes sounding together a consonant interval apart. The two strongest peaks of
  the spectrum are a third, a fourth, a fifth or a sixth apart; an octave doesn't count, so one note and its
  own overtone don't pass. Judged only.

All three pick out a kind of sound, so X, Z, C and OPTIMISE FILTERS leave them for you to tick.

**Counted exactly.** At the line's own second of sound (8,000 samples), the envelope and syllable
filters are counted exactly, block by block. Their automata would need tables far beyond the filter
memory at that length, but where the tables do fit (at 4 bits, 200 samples and blocks of 16) they count,
rank and merge like any automaton. CI checks that both ways give the same count.

| Filter | Random sound kept (8 bits, 8,000 samples) | Speech | Struck notes | Chords | Steady tones | Noise |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| sound-envelope-v1 | 10^-19.05 | 0/6 | 6/6 | 0/6 | 0/6 | 0/6 |
| sound-syllables-v1 | 10^-466.83 | 6/6 | 0/6 | 0/6 | 0/6 | 0/6 |
| sound-interval-v1 | judged | 1/6 | 0/6 | 6/6 | 0/6 | 1/6 |

The speech is Sieve's own voice (`sieve speak`), six sentences, a second of each.

## Two melody filters

- **melody-cadence-v1** (a plugin, on every note set): the melody ends on the tonic, reached by a step of
  one or two semitones (or one only, with approach = half). A repeated note keeps how it was reached, so
  D C C ends by a step. It keeps 10^-1.91 of random 16-note melodies. Of seven traditional tunes, five
  pass; Frère Jacques and London Bridge end by a leap and do not. The oracle agrees with it at every
  length and setting checked.
- **melody-contrary-v1** (judged, two voices or more): wherever two voices begin a note at the same moment
  and both have moved, they move opposite ways. To judge this a filter has to see every voice at once, so
  filters can now ask for that (`whole_unit`); every other filter still judges each voice on its own.
  The Hurrian Hymn's two voices move together 29 times, only 2 of them contrary, so it does not pass.

A metre's bar (the third melody idea) was already there: melody-metre-v1.

**OPTIMISE FILTERS** ticks the cadence along with the other melody plugins. The optimised audio line goes
from 10^-13.63 to 10^-14.04, and tracks from 10^-93.08 to 10^-94.80.

## Checks

- **Unit tests (step 01):**
  - every one-bit sound of 20 samples, both filters against their rules and their counts;
  - the automaton's count against the block-by-block count;
  - 3,000 random, struck and burst sounds, judged three ways;
  - the interval on tones, octaves and noise;
  - the cadence against every melody of 3 notes;
  - contrary motion, alone and beside a per-voice filter.
- **CI step 07:** the counts, the small automata's shares, the tone's verdicts, and the melody filters.
- **CI step 11:** the cadence plugin against the oracle, on notes104 and on notes2 sets.
- **CI steps 01, 04, 07, 10 and 11** pass, as do the help and help-library checks.

## Files to delete by hand

None.
