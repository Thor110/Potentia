# Films that share packets (12 November 2026)

IDEAS §19.116's first measurement: how much of a set of films is packets seen before (the same
footage in a film and its trailer, two cuts, a game's films sharing an intro). Films already
compressed code most frames as differences from others, so the frames themselves are no help; but
films encoded together can hold the very same packets, byte for byte.

## The tool

`tools/bench/video_packets.py FILM...` (or a folder): ffprobe lists every packet with a SHA-256 of
its data, without decoding a frame, and the tool counts the bytes of packets already seen (in the
same film or an earlier one), by stream kind. It reads only. Edward's films (WoWRevived's) are
the set it was written for; they are not here.

## A test pair

Made with ffmpeg here: an intro (20 s of `testsrc2`, H.264, 320×240) and a story (20 s of
`mandelbrot`), joined by stream copy into a film (3,459,714 bytes), and a trailer cut from the
intro's first 10 seconds by stream copy (366,275 bytes), as a trailer of the same encode is.

The tool: the trailer's packets seen before (in the film) are 330,169 bytes, 90.14% of it; 8.63%
of the pair.

## What Sieve makes of it

| | Bytes |
| --- | ---: |
| The pair as files | 3,825,989 |
| 7-Zip, `-mx9` | 3,449,028 |
| Sieve, newest (v33, Balanced; Size and Speed the same) | **3,811,383** |
| Sieve, the plain installer (`--v4`: every file in one LZMA2 stream) | 3,449,600 |
| The film alone: Sieve newest / 7-Zip | 3,448,512 / 3,448,662 |

**Sieve's newest packer is 10.5% larger than its own plain installer on this pair.** The film alone
it packs as well as 7-Zip. The film is cut into pieces at its MP4 boxes and its frame data carried
by the rope (which saves 0.4% on it), the trailer is carried in LZMA2's stream, and since the
film's frames are in another stream, LZMA2 never sees the trailer's packets beside the film's: the
trailer costs 362,871 bytes where 7-Zip and the plain installer pay a few hundred.

On the game-like set of results/stream-layouts-2026-11-12.md (no shared content across kinds) it
does not happen: newest 10,342,984 bytes, plain 10,372,911, 7-Zip 10,359,216.

## What it says

- **The packet route of §19.116 is worth it where footage repeats:** here 90% of a trailer. How
  often that is so in real sets is what the tool on WoWRevived's films would say.
- **A gap in the packer, found by it:** each file's way is weighed alone, so files that share
  content but are given different ways lose what they share. A remedy that needs no new version:
  before writing, weigh the whole set once more as the plain installer (one LZMA2 stream) and keep
  that where it is smaller. It costs one more LZMA2 pass over the set while making. It changes what
  Sieve writes (never larger, readable by every Sieve since v4), so it is for Edward to decide.
  Narrower remedies: keep a file whose rope gain is under some share in LZMA2's stream, where what
  other files share can reach it.

## Built (12 November 2026, Edward: "yes build the whole-set plain weighing fix")

`weigh_plain` (tools/cli/items.cpp), after the newest is made by `sieve locate --installer`, the
File Locator and everything that makes instructions of the newest version: the set packed once more
as the plain installer and the smaller kept, for two files or more where no file was changed by
packing. The report says it in a `plain` line. No new version: the plain installer is read by every
Sieve since v4.

| | Before | After |
| --- | ---: | ---: |
| The film and trailer | 3,811,383 | **3,449,600** (the plain installer kept; 7-Zip 3,449,028) |
| The game-like set (95 files) | 10,342,984 | 10,342,984 (the newest kept; plain 10,372,911); made in 49.8 s against 44.5 s |
| Two text files of 25 bytes | 89 | 89 (plain 223) |
| The manual and IDEAS | 178,933 | 178,933 (plain 229,337) |
| Ten small meshes | 1,191 | 1,191 (plain 2,188) |

Both kept files install back byte for byte. The cost is one more LZMA2 pass over the set while
making: 12% longer on the game-like set.
