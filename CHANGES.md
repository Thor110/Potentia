# Sound filters, a drawn waveform, video frame rate, tagless MP3 (relative to origin/main b3a72e1)

To apply it, do one of these from the Potentia repository root:
- unzip `files/` over the repository;
- or run `git apply sound-filters.patch`.

**New files:** `Sieve/core/src/filters/sound.cpp` and `Sieve/tests/vectors_sound_v1.tsv`.

## 1. Filters for sound itself (the `pcm` set)
**Each channel is judged on its own.** That's what a note set already does with its voices, so a stack's survivors are one channel's to the power of the channels.
- **How:** the code that ranks several voices together now sizes its packing to the line, rather than allowing at most 7 voices of under 65,536 notes. That lets it hold up to 65,535 channels of any length.
- **Unchanged:** note sets rank exactly as before, since the change is internal.

**Three filters:**

| Filter | Passes when | Counting | Keeps |
| :--- | :--- | :--- | :--- |
| `sound-peak-v1` | no sample is louder than `percent` of full scale (90) | exact at every depth: an automaton to 24 bits, then its own count | 8-bit, 800 samples: 10^-35.70; 16-bit stereo, 8000: 10^-732.05 |
| `sound-step-v1` | neighbouring samples differ by at most `percent` of the range (50): noise jumps about, sound mostly moves a little | an automaton up to 11 bits; judged past that | 8-bit, 800 samples: 10^-86.76 |
| `silence-run-v1` | no run of more than `samples` silent samples (4000) | an automaton while it fits; judged past that | removes mostly-silent units |

**Together.**
- **At 800 samples (8-bit):** the three merge into one automaton and count exactly in about 6 s. They keep 10^-102.20 of the line, with `silence-run-v1` set to 800.
- **At the default 8000 samples:** counting would need about 31 GB, so they judge only there. They still mark and hide items; the hallway just can't close up the gaps between survivors (compact mode).

**Checked by hand:**
- white noise fails `sound-step-v1`;
- a loud tone passes all three;
- a 0.25 s tone padded to a full second fails `silence-run-v1`.

**The oracle has its own implementation:**
- a closed form for peak;
- tables for step and silence, each checked by brute force over every unit of small lines.

The engine matched all 69 vectors on its first run, and CI diffs them.

## 2. A drawn waveform
- **Item page:** in hand, a sound draws as its waveform: each channel a band, each column the line from its lowest to its highest sample. Each column reaches back to the middle of the one before, so samples a column apart join into a line rather than dots.
- **Viewer:** the SOUND tab is now a picture of the waveform. It's drawn a column a sample where the view is wide enough, and zooms and pans like any picture.
- **Text previews:** the CLI's text preview uses the same envelope. The one-line preview under the crosshair puts the channels side by side, where a line break used to show as `?`.

## 3. The video frame rate in the hallway
- **The setting:** `video-fps` in the settings file, or `--video-fps N` (8 unless given).
- **What it does:** sets the frame rate F uses when saving a video as GIF, MP4 or WebM through ffmpeg.

## 4. J and sound with no signature
- **Tagless MP3 and AAC:** a file the kinds table doesn't know, whose first bytes are an MPEG audio frame header or an AAC (ADTS) header, opens as sound. This decides where J opens it, never what's filtered, so the file-kinds table and every count stay as they were.
- **Sound without pictures:** an MP4, AVI, WebP or unknown file in which ffmpeg finds no picture (an `.m4a`, or a video's sound alone) opens as sound when the audio line holds pcm.
- **Tried in the hallway from the binary line:** a tagless MP3 and an `.m4a` both opened on the audio line.

## Checked
- **Unit tests:** 33,875 checks, 0 failures (the 69 sound vectors among them).
- **CI steps, run here, all passing:** same addresses, filters, models, the bytes256 line, both sound and media steps (now also pinning the sound filters' counts), books, the whole hallway step and the whole reference-oracle step (now also diffing the sound vectors).
- **Screenshots:** the item page, and the viewer showing a tone as a continuous line.

## Still to come (package B, next)
**Open-ended notes:** a note set between `notes2` and `pcm`, with:
- every MIDI pitch;
- lengths in ticks;
- loudness;
- an instrument per voice;
- tempo.
