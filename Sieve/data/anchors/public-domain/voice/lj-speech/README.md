# LJ Speech: an hour of one voice (public domain)

The voice Sieve's own voice is to be made from (Edward, 9 November 2026: "gather some voice samples
from public domain spaces that can be used to train the new small voice model"; his choice, an hour
in the repository).

- **What:** the first 547 clips of [LJ Speech 1.1](https://keithito.com/LJ-Speech-Dataset/), LJ001-0001
  to LJ003-0024 in order: 3,604.5 seconds (79,479,159 samples), each with its transcript.
- **Who:** Linda Johnson, reading for LibriVox in 2016-17 from non-fiction books published between
  1884 and 1964; aligned and transcribed clip by clip by Keith Ito.
- **Licence:** public domain. The dataset card: "The texts were published between 1884 and 1964, and
  are in the public domain. The audio was recorded in 2016-17 by the LibriVox project and is also in
  the public domain" ([LibriVox](https://librivox.org/pages/public-domain/)). The dataset is released
  under the Unlicense.
- **From:** the dataset's first Parquet shard on Hugging Face (`keithito/lj_speech`,
  `refs/convert/parquet`, `main/train/0000.parquet`, SHA-256 `18c8a13f9a89...b396d6d8`), downloaded
  on 9 November 2026.

## Files

- `clips/*.flac`: each clip as FLAC (22,050 Hz, 16-bit, mono), made by ffmpeg 6.1.1 at its highest
  level (12). Each was checked to decode to exactly the samples of the WAV it came from. 93,580,290
  bytes in this folder; the WAVs are 159,150,858.
- `metadata.csv`: LJ Speech's own form, a clip a line: `id|text|normalized text` (the second with
  numbers and abbreviations written out as said).
- `manifest.tsv`: each clip's samples, and the SHA-256 of its WAV as LJ Speech has it.

`python3 tools/fetch_voice.py` makes the same WAVs again, byte for byte, from the pinned shard
(checked against `manifest.tsv`); with `--all`, the shard's other clips too (1,638 in all, about
three hours), for a voice trained on more than the hour.

The original recordings were 128 kbps MP3 before LJ Speech decoded them, so the clips may carry
that coding's traces.
