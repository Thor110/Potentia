# Sieve's changes to packMP3

packMP3 1.0g (github.com/packjpg/packMP3, commit e61c11941552f4ffe6e219a847f441d9520d2e50, 27 April
2020), LGPL v3 (LICENSE). Changed by Sieve, 8 November 2026, so that its decoder may be given damaged or
hostile input (a `.sieve` from anywhere) without reading or writing outside its buffers: each change is
marked "Sieve" in the source. What and why: results/mp3-pmp-2026-11-08.md in Sieve.

- aricoder.cpp, aricoder.h: an allocation that fails throws (packMP3 ended the program, exit 0); a
  damaged stream throws: a count at or past its scale, a step of 0, an escape from the table of order
  -1 (which holds every symbol), a symbol past the model's, a context past the model's, or more than 16
  bytes read past the stream's end.
- packmp3.cpp: a PMP header's count of frames held to 1 to 2^20; region sizes held to 20 bands (as an
  MP3's own side info is); every frame's and granule's side info checked before its main data
  (pmp_check_side_info: region tables 0 to 31 but 4 and 14, region bounds rising, even, within 576,
  block types, lengths, gains and flags within their fields); the small values' bound within 578
  (9 November 2026: first 576, which refused MP3s that shine and other encoders make, whose last
  quadruple reaches two past the granule; packMP3's arrays hold 580); the main data's size within the bitrate predictor's table, and a decoded bitrate 1 to 14;
  padding and ancillary bits within the 2048 bytes kept for them; the first frame's search ended where
  a file under 4 bytes left nothing to search (it counted to INT_MAX).
- packmp3.cpp (9 November 2026, review): a form saying more muted frames than it has frames refused
  (the unmute walk went past the last frame into a null pointer).
- bitops.cpp (9 November 2026): stdin and stdout no longer switched to binary when a stream opens
  (packMP3's own program did; MSVC has no `setmode` by that name, and in Sieve it would change Sieve's
  own console).
- Unchanged: packMP3's coding itself. Every MP3 it gave back before it gives back the same, from the
  same form (checked on 17 MP3s).
