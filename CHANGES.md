# Changes: Text to Voice, and an hour of public-domain voice (9 November 2026)

## Text to Voice

- A new setting, **Text to Voice** (Settings, Controls, after Read menus aloud; `[controls]
  text_to_voice` in the settings file; off by default). With it on, each reply of the AI in the
  CHAT tab (a model of the AI line in hand, or a model file, talked to) is spoken as it comes.
- For now it speaks with the system's voice, as Read menus aloud does: Windows' own speech; elsewhere
  the row says "Windows only" and nothing is said. Sieve's own voice is to take its place (below).
- A reply is bytes, and an untrained model's are mostly not text: what is spoken is its valid UTF-8
  text, with control bytes and broken sequences left out as spaces.
- `--text-to-voice` turns it on for one run (not saved); with `--screenshot` the hallway prints the
  reply spoken ("voice: ..."), as `--read-aloud` prints what is read.
- CI step 10: a reply spoken, and the setting's row read aloud in Settings, Controls.

## The voice: an hour of LJ Speech, public domain

`data/anchors/public-domain/voice/lj-speech` (its README says where it is from and how it was made):

- The first 547 clips of LJ Speech 1.1 (LJ001-0001 to LJ003-0024): 3,604.5 seconds, one reader
  (Linda Johnson, for LibriVox), each with its transcript (`metadata.csv`, LJ Speech's own form).
  Public domain: the books (1884 to 1964) and LibriVox's recordings both.
- As FLAC (22,050 Hz, 16-bit, mono): 93,580,290 bytes for the folder; the WAVs are 159,150,858.
  Every FLAC was checked to decode to exactly the samples of its WAV.
- `manifest.tsv`: each clip's samples and the SHA-256 of its WAV as LJ Speech has it.
- `tools/fetch_voice.py` makes the same WAVs again, byte for byte, from the dataset's first Parquet
  shard on Hugging Face (pinned by its SHA-256), and with `--all` the shard's other clips (1,638 in
  all, about three hours). It needs pyarrow. Checked here: all 547 written, none mismatched.

## Documents

- MANUAL: Text to Voice under Settings, Controls.
- IDEAS 19.135: Sieve's own voice, in Edward's two steps (a voice of units first, then a trained one).

Not changed: data/anchors/anchors-notes.txt is Edward's own list; the voice is not added to it.
