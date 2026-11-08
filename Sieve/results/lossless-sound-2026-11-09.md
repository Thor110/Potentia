# Sound as FLAC for WavPack, Monkey's Audio, TTA and ALAC (9 November 2026)

Edward: "let's move on to the other formats next". The plan's item 5 (docs/FORMATS-PLAN.md): the
other lossless sound formats installed as FLAC under Sound as FLAC (`--sound-to-flac`, v34 draft).

## How

Sieve links none of their decoders. Each file goes to ffmpeg, run as its own process (the one the
hallway and `--ffmpeg` already name for other formats: cli/media_decode.hpp), which decodes it once
into a FLAC file with its tags and cover pictures and, in the same run, gives the MD5 of its samples.
libFLAC codes that FLAC again at the priority's effort (`flac_recoded`, as a FLAC file is re-coded),
and ffmpeg's own FLAC decoder gives the MD5 of the result's samples: the two must be equal. Only the
packer needs ffmpeg; what is installed is a plain `.flac`, chosen only where it is smaller.

Left as they are: 8-bit, floating-point and 32-bit sound (FLAC's subset holds 16 to 24 bits as
Sieve uses it; ffmpeg would widen 8-bit samples to 16, the same sound at another depth); WavPack's
lossy hybrid mode and DSD; a file in which ffmpeg reports any error (ffmpeg 6.1 exits with 0 after a
CRC error, so its report at `-v error` is what is read); an `.m4a` that is not exactly one ALAC
sound track (AAC is never taken). With no ffmpeg, all are left as they are and the listing says how
many waited for it ("link ffmpeg in the hallway's Settings, or give --ffmpeg PATH").

## Measured

Two sounds: WoWRevived's Music-Human/02 (stereo, 16-bit, 44.1 kHz, 54,686,396 bytes as WAV) and a
mono speech recording (1,764,078 bytes as WAV). Each coded by ffmpeg 6.1.1's own encoders at their
default levels, and by Monkey's Audio 3.99 (built here from a Linux port, for the test files only;
it crashed on the mono file).

| Form | Music-Human/02 | speech |
| --- | ---: | ---: |
| WAV | 54,686,396 | 1,764,078 |
| Sieve's FLAC from the WAV, Size | 27,730,764 | 786,145 |
| Sieve's FLAC from the WAV, Speed | 27,807,532 | 798,528 |
| WavPack (ffmpeg) | 28,694,651 | 833,507 |
| TTA (ffmpeg) | 27,925,358 | 776,508 |
| ALAC (ffmpeg) | 29,201,862 | 821,762 |
| Monkey's Audio, -c1000 (fast) | 27,721,064 | |
| Monkey's Audio, -c2000 (normal) | 27,324,252 | |
| Monkey's Audio, -c3000 (high) | 26,999,720 | |
| Monkey's Audio, -c4000 (extra high) | 26,575,744 | |
| Monkey's Audio, -c5000 (insane) | 26,471,992 | |

Through Sound as FLAC, Music-Human/02's WavPack, TTA and ALAC files each became a FLAC of 27,739,043
bytes under Balanced and 27,730,770 under Size (the ALAC's 75 bytes more: its encoder's tag), the
whole `sieve locate` taking 22.0 to 23.3 s under Balanced and 24.6 to 26.2 s under Size on this
machine (packing the FLAC included). The speech's TTA was left as it is (776,508 bytes, under
FLAC's), and every Monkey's Audio file of the music: FLAC is larger than each. WavPack's own program
(its higher modes) was not available here, so files it makes were not measured.

## Checked

- Installed FLACs decoded by ffmpeg have the same samples (MD5) as their sources: 16 and 24-bit
  WavPack, TTA and ALAC, and a Monkey's Audio file made where FLAC wins (16-bit samples in steps of
  4: 1,563,540 bytes as APE, 1,140,338 as FLAC).
- Tags (a title with an accent, an artist) and an ALAC's cover picture kept.
- A WavPack and a TTA with a byte changed (CRC errors), and an 8-bit TTA, left as they are.
- Two files that would both be `name.flac` (`s24.wv`, `s24.m4a`): one installed so, the other left.
- CI step 08 (where ffmpeg is): WavPack, ALAC (tagged, and 24-bit) installed as FLAC of the same
  samples; an 8-bit TTA and a damaged WavPack as they were; with `--ffmpeg no-such-ffmpeg`, all five
  as they were and the listing's line.

## Limits

ALAC frames carry no checksum: a damaged ALAC is decoded as ffmpeg decodes it, and those samples are
what the FLAC holds. A file's tags are kept as ffmpeg maps them to Vorbis comments; anything ffmpeg
does not map (WavPack's stored RIFF chunks, Monkey's Audio's APE tag items it does not name) is not.
