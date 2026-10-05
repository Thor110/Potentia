# Sound itself on the audio line, and saving in other formats (relative to origin/main 2262e74)

**This package includes the earlier ffmpeg one** (`ffmpeg-media.zip`), so apply this one alone.

To apply it, do one of these from the Potentia repository root:
- unzip `files/` over the repository;
- or run `git apply media-and-sound.patch`.

**New files:**
- `Sieve/core/include/sieve/sound.hpp` and `Sieve/core/src/sound.cpp`;
- `Sieve/tools/cli/media_decode.cpp` and `media_decode.hpp`;
- `Sieve/tests/vectors_pcm_v1.tsv`.

## 1. Reading pictures and video through ffmpeg (from the earlier package)
- **What Sieve reads itself:** PNG, JPEG, BMP, GIF and TGA, exactly as before, so their addresses don't change.
- **Through ffmpeg:** every other format, when an ffmpeg is found:
  1. `--ffmpeg PATH`, or `ffmpeg=PATH` in the hallway's settings;
  2. the `SIEVE_FFMPEG` environment variable;
  3. an ffmpeg beside Sieve;
  4. the `PATH`.
- **Without one:** nothing calls it, and a file Sieve can't read itself says where to put an ffmpeg.
- **Long videos:** frames are fitted one at a time. The whole of your 52 MB anchor video took 57 s at 308 MB peak memory.

## 2. Sound itself: the `pcm` set on the audio line
**It's not a new dimension.** It's a third set on the existing audio line, beside `notes104` and `notes2`, and **`notes104` stays the default**.
- **In the CLI:** `--note-set pcm` with `--rate` (8000), `--bits` (1 to 31, default 8) and `--channels` (default 1). `--length` is samples per channel, one second at the rate unless given.
- **In the hallway:** setup menu, AUDIO SET.

**What a unit is.**
- Each sample is one digit, its own bits read as a number, so digit 0 is silence.
- A unit is channel 1's samples, then channel 2's, the way `notes2` lays out its voices.
- The address is the sound itself, read as one number.

**How a file is fitted to the set** (`canon-pcm-v1`), by fixed rules:
1. Channels are mixed or split: to one channel, their mean; to fewer, the first ones; to more, the last repeated.
2. The rate is resampled by area, the same way pictures are stretched, rounded half up.
3. The samples are rounded to the set's bits; anything outside the range is clipped and reported.
4. The sound is cut into units, and the last is padded with silence.

**Files.**
- **In:** a WAV file is read by Sieve itself. MP3, FLAC, OGG, Opus, AAC and a video's sound go through ffmpeg.
- **Out:** a unit saves as a WAV file.
- **Round trip:** the WAV a unit saves as reads back to exactly the same unit, at any rate, depth and channel count I tried (1 to 31 bits, 1 to 6 channels, 3 Hz to 48 kHz).

**In the hallway.**
- **Setup menu:** on pcm, the six existing audio rows become samples, audio set, sample rate, bits, (notes2) and channels. No rows were added, so the CI's row pins stand.
  - **Sample rate** steps through 8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000 and 192000 Hz. The settings file takes any rate.
  - **"note set" is renamed "audio set",** since it now chooses sound as well as notes.
- **P** plays the sound in hand. The music fades under it, as it does under a melody.
- **The item page and viewer** show each channel as a row of shades, on a SOUND tab with the format in its heading.
- **T** warps to a sound file.
- **F** saves a WAV file.
- **J on the binary line:**
  - WAV, MP3, OGG and FLAC open on the audio line when its set is pcm, and MIDI when it holds notes;
  - otherwise it says which set is needed.

**Filters.**
- The note filters don't apply to pcm.
- The general filters do (`not-a-file-v1`, `not-a-pattern-v1`, entropy).
- Filters of its own, and filters between sound and notes, are next (IDEAS §12).

## 3. Saving in other formats through ffmpeg
**What's offered.** A unit always saves in its own format: PNG, MIDI, WAV or text. With ffmpeg it can also save as:

| Kind | Formats |
| :--- | :--- |
| Pictures | JPEG, WebP, BMP, TIFF |
| Video | animated GIF, MP4, WebM (`--fps`, 8 unless given) |
| Sound | FLAC, MP3, Ogg Vorbis, Opus, AAC |

**Each format is offered only when your ffmpeg has its encoder.** Sieve asks ffmpeg for its encoder list once, so nothing is assumed about how it was built. That keeps a stripped-down LGPL build of ffmpeg workable.

**How you choose.**
- **CLI:** `read --out x.flac` chooses by extension. An extension none of these formats uses saves the line's own file, as before.
- **Hallway:** F lists the formats as filters in the save dialog. Typing an extension also chooses one.
- **Without ffmpeg:** a format that needs it says so, and lists what it can save instead.

**Which come back exactly.**
- **Lossless:** PNG, BMP, a GIF of the line's palette, WAV and FLAC read back to the same address.
- **Checked:** MP4 and WebM of a black-and-white video also came back exactly here.
- **Lossy:** other lossy saves may not come back.

## Checked
- **Unit tests:** 33,735 checks, 0 failures. That's 166 more than before. The total is half of earlier ones because this machine has no SHA instructions, so the SHA-256 checks run once, not twice. New tests:
  - the 13 pcm vectors, handed over in blocks of 3 frames so the resampling crosses block edges, each checked against its SHA-256;
  - each WAV written out reads back to the same unit;
  - every digit at 1 to 31 bits;
  - malformed ids are refused;
  - an empty sound is refused.
- **The oracle** has its own `canon-pcm-v1`, in exact fractions, with its own WAV reader and writer. It covers:
  - 8-, 16-, 24- and 32-bit WAV, float32 and float64, extensible headers, and streamed sizes;
  - mixing 2, 4 and 6 channels, keeping the first channels, repeating the last;
  - rates up and down, including 1 Hz to 2 Hz and 16 kHz to 7 Hz;
  - 1 to 31 bits, and clipping.
  
  The engine matched all 13 cases on its first run. CI diffs the vectors.
- **New CI step: sound needs no ffmpeg.** It checks:
  - a stereo 16-bit WAV mixed to 8 kHz 8-bit mono;
  - read back as WAV to the same address;
  - 12-bit, 3-channel at 22050 Hz;
  - notes104 still the default, and pcm options refused on notes;
  - the message when ffmpeg is missing.
- **The ffmpeg CI step now also checks:**
  - FLAC landing where the same WAV does;
  - FLAC and GIF exports coming back to the same address.
- **Run here and passing:** every CI step (same addresses, filters, models, the bytes256 line, both media steps, books, the whole hallway step, and the whole reference-oracle step), with a static ffmpeg 7.0.2.
- **By hand:**
  - MP3, FLAC, OGG and Opus in;
  - every format out;
  - the hallway saving GIF and FLAC through F;
  - screenshots of the item page, the viewer and the setup menu on pcm.

**Not run:**
- Windows and macOS, which CI covers;
- listening to the playback, since this machine has no sound device.

## Not yet
- **Filters for sound:** its own (silence, clipping, then pitch and timbre), and between sound and the note sets.
- **Open-ended notes between `notes2` and `pcm`:** every MIDI pitch, lengths in ticks, loudness, instruments.
- **Waveform:** a drawn waveform in place of the shaded text one.
- **Hallway frame rate:** a setting for saving video, which is 8 frames a second for now.
- **MP3 without an ID3 tag:** its kind is unknown, so J tries it as a picture. With the audio line on pcm, warp it with T instead.
