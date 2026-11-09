# Sieve's voice of units (9 November 2026)

Edward: "gather some voice samples from public domain spaces that can be used to train the new small
voice model ... make it a setting \"Text to Voice\" on / off". His choice of the way (IDEAS 19.135):
a voice of units first, a trained one after; CMUdict for the words, espeak-ng for the alignment.

## What it is made from

- **The voice:** an hour of LJ Speech (public domain; `data/anchors/public-domain/voice/lj-speech`),
  547 clips, 3,604.5 seconds, one reader.
- **The words:** CMUdict (`data/voice/lj-speech/cmudict.dict`, 135,166 lines, BSD-style licence). Of
  the 9,427 words of the clips' transcripts, 135 (1.4%, in 114 clips) are not in it, mostly old names
  (Marshalsea, Giltspur, Ludgate).
- **The alignment** (`data/voice/lj-speech/alignment.tsv`, by `tools/voice/align_voice.py`): espeak-ng
  1.51 speaks each transcript's CMUdict phones, through its library, and says where each starts; dynamic
  time warping over the two recordings' MFCCs (13 coefficients, 10 ms frames) carries each start to the
  clip. espeak adds sounds of its own (`;` between vowels) and names some differently, so its events
  and the phones are lined up by edit distance. 503 clips aligned; 44 left out, where espeak's phones
  were too unlike CMUdict's. Run again, the script gives the same file, byte for byte.

## Is the alignment right?

Fricatives (S, Z, SH, ZH, F, TH) cross zero far more often than vowels. Over every aligned phone of at
least 200 samples:

| Alignment | Fricatives' zero-crossing rate (median) | Vowels' | A fricative's above a vowel's |
| --- | ---: | ---: | ---: |
| As aligned | 0.482 | 0.070 | 95.2% of pairs |
| Moved 100 ms later | 0.171 | 0.080 | 62.9% |
| Moved 100 ms earlier | 0.141 | 0.088 | 57.4% |

## The voice

Units: every diphone in the aligned clips, from the middle of one phone to the middle of the next (and
50 ms of the silence before the first phone and after the last): 1,022 diphones, 33,750 takes, all
kept. A text's takes are chosen together (Viterbi): a join costs the squared distance between the log
spectra (Hamming-windowed 512 samples, every sixth bin from the fifth) at the end of one and the start
of the next, nothing for two takes that were next to each other in a clip, and 20 times a take's
distance from its diphone's median length (as a share of it). Joins are crossfaded over 64 samples.

## How well it is understood

No one listened here; an offline recogniser did. whisper.cpp (commit f92e1e4, 8 November 2026; MIT)
with its base.en model, on 16 kHz copies, transcribed each sentence, and the word error rate is the
edit distance between its words and the sentence's, over all the words. It transcribed a real LJ clip
(LJ001-0002, "in being comparatively modern.") exactly. The sentences: 60 of everyday English, none
from the clips' transcripts (`results/voice-sentences-2026-11-09.txt`; the first 20 were the first
set).

| Voice | Sentences | Word error rate |
| --- | ---: | ---: |
| espeak-ng 1.51, en-us (for comparison) | 20 | 5.7% (9 of 157) |
| Units, the 8 takes of each diphone nearest its usual length, joins by spectrum alone | 20 | 24.8% (39 of 157) |
| Every take, takes next to each other in a clip joined at no cost | 20 | 19.7% (31 of 157) |
| The same, the length's weight 5 / 60 / 200 instead of 20 | 20 | 21.7% / 21.7% / 22.3% |
| espeak-ng 1.51, en-us (for comparison) | 60 | 9.7% (47 of 484) |
| Every take, adjacent takes free (weight 20) | 60 | 25.4% (123 of 484) |
| The same, vowels' stress kept apart (0 and 1) | 60 | 25.6% (124 of 484) |
| The same, crossfades of 220 samples | 60 | 28.3% (137 of 484) |
| The same, each take's loudness brought to its diphone's median | 60 | 26.4% (128 of 484) |
| The same, both of the last two | 60 | 25.8% (125 of 484) |
| **The same, each join lined up on the waveform (up to 110 samples)** | 60 | **23.3% (113 of 484)** |
| The same, up to 220 samples | 60 | 26.7% (129 of 484) |
| **Sieve's own (`sieve speak`, the C++ of the line in bold)** | 60 | **23.3% (113 of 484)** |

So the recogniser understood 76.7% of the words of Sieve's voice, against 90.3% of espeak-ng's. The
first set's 20 sentences gave rates a few points apart for changes that the 60 did not tell apart:
differences of two or three points here are within what so few sentences can show.

`sieve speak` loads the voice (the clips decoded, every take's spectra) and says a sentence in 2.6 s
on this machine, almost all of it the loading; the hallway loads it once, on a thread of its own.
Loaded, it holds every clip's samples (159 MB as 16-bit samples): `sieve speak` peaked at 228 MB.

## Next

The joins are where it is lost: what is heard between takes. The trained voice of IDEAS 19.135 is
to learn sound from text; before it can speak alone, it may smooth the units' joins.
