# Changes: Sieve's own voice, of units (9 November 2026)

Edward's choice: a voice of units first (a trained one after), CMUdict for the words, espeak-ng for
the alignment. Measured in `results/voice-units-2026-11-09.md`.

## The voice

- **`tools/cli/voice.hpp/.cpp`**: speech joined from diphones (the middle of one speech sound to the
  middle of the next) cut from the anchors' hour of LJ Speech: 1,022 diphones, 33,750 takes, from
  503 aligned clips. A text is read into speech sounds by CMUdict (numbers as words; a word it lacks
  spelt out); for each pair of sounds a take is chosen, all at once, by how smoothly it joins its
  neighbours (takes next to each other in a recording join at no cost) and how usual its length is;
  each join is lined up on the waveform (up to 5 ms) and crossfaded (3 ms). The same text gives the
  same samples.
- **How well it is understood:** an offline recogniser (whisper.cpp, base.en) got 76.7% of the words
  of 60 sentences not in the recordings (a word error rate of 23.3%), against 90.3% for espeak-ng's
  own voice. Every change tried on the way is in the results, with its rate.
- **`sieve speak TEXT [--out FILE.wav] [--phones]`**: says a text into a WAV (22,050 Hz, 16-bit,
  mono). About 2.6 s on this machine, almost all of it loading the voice; 228 MB at its peak.
- **Text to Voice** now speaks in Sieve's own voice wherever its files are, on any system: loaded the
  first time it speaks, on a thread of its own, each reply played through SDL by the frame. Without
  its files, Windows' voice as before; the setting's row says "no voice here" where there is neither.
- `flac_wav`: `flac_mono16`, a FLAC of one 16-bit channel decoded (MD5 checked), for the clips.

## Its data

- `data/voice/lj-speech/cmudict.dict`: CMUdict, unchanged (135,166 lines; BSD-style licence, kept
  beside it and in `third_party_licenses/cmudict`, listed in the licences' README).
- `data/voice/lj-speech/alignment.tsv`: where each speech sound lies in each clip (503 clips).
- `tools/voice/align_voice.py` makes the alignment again, byte for byte (checked), from the WAVs
  `tools/fetch_voice.py` gives; it needs espeak-ng (a build-time tool only, through its library;
  GPL, never linked into Sieve or shipped) and numpy. Its check: fricatives' zero-crossing rate is
  above vowels' in 95.2% of pairs as aligned, 62.9% and 57.4% with it moved 100 ms either way.
- The build copies `data/voice` beside the programs; the clips are found in the anchors, above a
  build inside the repository.

## Checked

- Unit suite: 154,460 checks, 0 failures. `tools/check_help.py`: nothing missing.
- CI step 10: a text read into its sounds (`--phones`, "42" as "forty two"), said, and said the same
  again, sample for sample.

## Documents

- MANUAL (Text to Voice), `sieve help speak`, IDEAS 19.135 (the voice of units, built), the voice
  folder's README, results/voice-units-2026-11-09.md and the 60 test sentences beside it.

## For you to decide

- **The release** does not carry the clips (93.6 MB): there, Text to Voice falls back to Windows'
  voice. Carrying them would put Sieve's voice in the release, at that size.
