# Changes since 377c166 (12 November 2026)

Extract from the repository root. This replaces the notes zips.

## Your notes (GAME.md section 11, IDEAS 19.136 and 19.137, SIEVE-FORMAT section 7)

- The lenses (executable, console and source folder).
- Alien Trilogy as the first target; ALTEngine sieved and streamed first.
- Graphics that scale from pixels to NURBS.
- A source tree across gaming and computing.
- History constrained and fraud made impossible; all of history mapped, and where it crosses over
  with what is lost.
- Compatible forever, and no change beyond compatibility until Sieve becomes an operating system and
  a game engine.

## Stage 6 measurements (nothing built)

**Streaming a game's files** (results/stream-layouts-2026-11-12.md, IDEAS 19.104). 95 game-like
files (11.5 MB):

| Layout | Size | Time to the first file |
| --- | --- | --- |
| One `.sieve` | 10,342,984 bytes | 0.97 s (has to decode all of it) |
| One per kind | −0.06% | |
| One per file | +0.21% | 18 ms (median) |

So ALTEngine's streaming needs no new format version.

**Films that share packets** (results/video-packets-2026-11-12.md, IDEAS 19.116):

- **A new tool,** `tools/bench/video_packets.py`, hashes every packet with ffprobe, without decoding,
  and counts the bytes already seen. On a film and a trailer cut from it, 90% of the trailer was
  packets seen before. It's ready to run on WoWRevived's films.
- **It found a gap in the packer, for you to decide on.** On that pair, Sieve's newest was 10.5%
  larger than its own plain installer (`--v4`) and 7-Zip. The film's frames went to the rope, in a
  stream of their own, so the footage the trailer shares with it was lost. A remedy that needs no
  new version: before writing, weigh the whole set once more as the plain installer and keep it
  where it's smaller. It's never larger, but it changes output.
- **No gap on the game-like set:** Sieve's newest beat both plain and 7-Zip there.

## Commits (local only)

5c8d249, b3f6b2b, 540aac7, 7e21398, 2eb4f77, 0a59872, dd4f388 (notes); f7282a2 (measurements).
