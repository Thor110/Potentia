# Wave paths, waves in turn, and other waves (8 November 2026)

Edward's idea (8 November 2026): a wave is a path that curves through the file laid out as a grid of
bytes, with one to n phases; where a value on the path matches, it is cut, and the values that do
not match are kept; decoding puts the cut values back procedurally. The wave may propagate up to n
times, and the best result is chosen through seeds and propagation, within limits, so the search is
not exhaustive unless asked.

**What has to hold for it to decode.** A value matches when its wave predicts it exactly. Every value
keeps its place in the grid: if the cut values were taken out of the grid itself, every later value
would move into another column (in a stereo WAV laid out two values a line, left and right would swap
for the rest of the file), and the predictions would no longer mean anything. So the cuts come out of
the stream of leftovers, and the decoder, rebuilding in order, fills a cut place from its prediction.
The decoder knows the path from its parameters, so off the path no yes/no is needed; on it, a yes/no
says which values were cut. A second pass with the same wave would predict the same, so each pass is
a different wave: another predictor, or another path.

**How.** `tools/bench/wave_paths.py`. Files as before: WoWRevived's Music-Martian/03 to 05 (8 MB of
samples each, after the 44-byte header), Silesia's `mr` (lines of 512) and `x-ray` (lines of 1,901),
and 8 MB of random bytes, the control. The base is the WAVs' recipe: `P3` (before + above −
diagonal) with `T1`'s nine bands, each band's two byte planes coded adaptively. The waves:
- `P1` (the value before), `P3`, `P4` (LOCO-I's median);
- `O2`, `O3`: the line above continued by its last change, or its last two (a wave continued from its
  last two or three values, as FLAC's fixed predictors of order 2 and 3);
- `O2X`: the value before, moved on by how the line above's difference to its own before is changing.

The cuts, each pass choosing the wave and the cells whose estimated net is best, at most three passes,
the choice paid for in bits:
- **every value**: the wave's yes/nos over every value still kept;
- **paths**: Edward's curves, 600 shapes (grids of 2 to 512 columns, periods of 2 to 512 rows, eight
  phases, 1, 2 or 4 phases of the wave), each row's cell on phase j at column G/2 · (1 + sin(2π r / P
  + φ + 2πj / k));
- **seeded paths**: 128 seeds, each row's cells chosen by the seed (one or four in rows of 32).

Every variant is costed by one adaptive coder (each symbol at its count so far, plus one, so no table
is free). The search estimates each pass with static shares by band; the totals are adaptive.

Two variants of mine:
- **the wave fitted to each block**: `tools/bench/wave_fitted.py`, each channel (the left, and the right
  less the left) predicted from its last N values with weights fitted to each block of 4,096 by least
  squares, quantised to 1/2^14 and paid for (16 bits a weight); the leftovers coded by bit length
  (adaptive, given the bit length before) and the bits below the top one. This is FLAC's linear
  prediction, measured with these coders;
- **the reversible 5/3 wavelet** of lossless JPEG 2000, four levels (the WAVs: each channel along time;
  the pictures: rows then columns), coded by bit length per band. Against it, the same bit-length
  coder on the predictors' leftovers.

**Results**, each against the base (`P3` with `T1`); − is smaller:

| | Martian/03 | Martian/04 | Martian/05 | mr | x-ray | Noise |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: |
| Base, bytes | 4,752,171 | 6,100,288 | 5,548,162 | 2,608,653 | 4,330,051 | 8,000,746 |
| Cut, waves in turn, every value | −1.82% | −0.45% | −0.81% | 0.00% | −0.04% | 0.00% |
| Cut, paths | −1.82% | −0.45% | −0.81% | −0.09% | −0.04% | 0.00% |
| Cut, seeded paths | −0.25% | −0.13% | −0.21% | 0.00% | 0.00% | 0.00% |
| Best single wave as the base | −15.39% (O3) | −0.85% (O2X) | −5.43% (O2) | −0.35% (P4) | −2.56% (P1) | 0.00% |
| Best wave, then cuts | −15.64% | −1.30% | −6.10% | −0.35% | −2.56% | 0.00% |
| Best wave, bit-length coder | −24.12% | −0.80% | −7.75% | −17.31% | −13.85% | −0.01% |
| 5/3 wavelet, bit-length coder | +1.86% | +17.79% | +6.05% | −5.70% | −15.69% | +3.20% |

The wave fitted to each block, on the same 8 MB of each WAV, and FLAC (ffmpeg, `-compression_level
12`) on the same samples:

| | Martian/03 | Martian/04 | Martian/05 |
| :--- | ---: | ---: | ---: |
| Base (`P3`, `T1`) | 4,752,171 | 6,100,288 | 5,548,162 |
| Fitted, 3 values back | 3,145,176 | 5,458,351 | 4,694,108 |
| Fitted, 8 values back | 3,032,515 (−36.19%) | 5,314,523 (−12.88%) | 4,560,925 (−17.79%) |
| Fitted, 32 values back | 3,041,684 | 5,294,764 | 4,564,659 |
| FLAC | 2,894,624 | 5,087,330 | 4,263,596 |

And FLAC on the seven whole WAVs the rope carries in ROPE-POST section 4 (each decoded back to the
same samples): 224,783,126 bytes against the rope's 276,730,651, 81.2% of it; per file 72.7% to
85.3% of the rope's code.

**What it shows.**
- **The paths find nothing the waves do not.** Where a path paid, the best was a shape that covers
  every cell (a grid of 2, all phases), so it is "every value" again; on `mr` a path took 0.09%.
  Seeded paths do worse than either, because a seed's cells are where the seed put them, not where the
  wave is exact. A wave's exact hits are where the signal is quiet or smooth, and the neighbours, not
  a curve drawn in advance, say where that is.
- **Waves in turn pay a little**: 0.45 to 1.82% on the WAVs (O3, O2 and O2X each cutting what the one
  before missed), nothing on the pictures or the noise. (On `mr` the search's estimate left the trim
  of results/rope-trim-2026-11-08.md, 4.36%, out: it costs the yes/nos by band alone, not by the
  yes/nos before them.)
- **Choosing the wave matters far more than cutting**: a wave continued from its last three values
  takes 15.39% off Martian/03, but adds 7.66% on Martian/04; so the choice must be per file (or per
  block), as the search does with `P`.
- **Coding the leftovers by bit length** rather than in two byte planes takes 17.31% off `mr` and 10.83
  to 13.85% off `x-ray`; on the WAVs it is close to the planes for `P3` (−3.06% to +0.11%).
- **The wave fitted to each block** is the largest gain: 12.88 to 36.19% off the base on these WAVs,
  and within 4.4 to 7.0% of FLAC on the same samples. Eight values back is enough; 32 adds nothing here.
- **The wavelet** wins on `x-ray` (−15.69%, the best there), loses on the WAVs and the noise.
- **FLAC is 18.8% smaller than the rope on the WAVs today.** The rope's audio gain is against LZMA2;
  FLAC, made for audio, does better.
- **The control** gains nothing anywhere; the wavelet's +3.20% on noise is its larger high bands.

**Verdict.** The path and seed versions of the cut are not worth building. The rope's audio would
gain most from a wave fitted to each block (FLAC's linear prediction, which is prior art) and from
coding the leftovers by bit length; with those it would be near FLAC, not past it. A way that codes
sound as FLAC does is the option to weigh against it (IDEAS §19.132). Not built.

## Other sound: speech, jazz and an orchestra (added the same day)

Is it only electronic music? 8 MB from the middle of three recordings on archive.org, each made by
the packer as it is now (`sieve locate FOLDER --installer`, the WAV alone in its folder; each way's
size from `--progress`) and by FLAC level 12 (each decoded back to the same samples):
- speech: an interview, "Carl Rosen on the legacy of UE Hall" (`CarlRosenInterviewOnly.wav`, 16-bit
  stereo, 44.1 kHz);
- jazz: "Denny Zeitlin Odyssey mix" (16-bit stereo, 44.1 kHz);
- orchestra: Musopen's premix of Brahms's Symphony No. 1, left channel (24-bit mono, 96 kHz);
- and, for comparison, the same 8 MB of Music-Martian/03 as above.

| Piece | LZMA2 | The rope (its wave) | Taken by the packer | Wave fitted, 8 back | FLAC |
| :--- | ---: | ---: | ---: | ---: | ---: |
| Martian/03 (electronic) | 6,211,704 | 4,781,958 (`L32 W1 S1 P2 K1`) | 4,473,548 (mixing) | 3,032,515 | 2,894,624 |
| Speech | 3,700,929 | 3,788,488 (`L16 W1 S1 P2 K0`) | 3,356,322 (mixing) | 2,684,089 | 2,463,881 |
| Jazz | 7,164,309 | 5,903,111 (`L32 W1 S1 P2 K0`) | 5,864,151 (mixing) | 5,029,644 | 4,885,475 |
| Orchestra, 24-bit | 6,746,797 | 6,551,902 (`L8 W6758 S3 P1 K0`) | 5,657,580 (mixing) | (not run: 24-bit) | 4,402,356 |

(Bytes for 8,000,078-byte WAVs, the orchestra's 8,000,100. The fitted wave is the estimate of
`wave_fitted.py`, not a made file.)

- The rope beats LZMA2 on the music (electronic 23.0%, jazz 17.6%, the orchestra 2.9% smaller) and
  loses on speech (2.4% larger): it is not only electronic music, but it needs a steady signal.
- On these 8 MB pieces mixing beats the rope on all four, and FLAC beats both: the packer's choice is
  20 to 54% larger than FLAC (Martian/03 54.5%, speech 36.2%, jazz 20.0%, the orchestra 28.5%).
- The rope reads values of 8, 16 or 32 bits, so 24-bit sound is read as bytes in rows; it gains
  little there.
- The fitted wave comes within 3.0% (jazz) to 8.9% (speech) of FLAC.
