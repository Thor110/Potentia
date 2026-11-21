# Changes

One commit on top of 9229ab4 (review 5's findings and ideas): review 5's fixes. docs/REVIEW-5.md says what was fixed
and the few parts left, with why.

## Nothing you have changes
Every .sieve byte and every address is as before: folders packed with this build and with 0.13.80 came out byte for
byte the same, and all twelve CI steps pass.

## Files that could hurt Sieve no longer can
- A glTF file can no longer make the importer write past its buffer, and long clip libraries have a roof.
- A tiny MIDI file, a few bytes of notes3 notation, or a WAV that declares a 1 Hz rate can no longer ask for gigabytes:
  each is refused with a message saying why.
- A small GIF can no longer make the vault ask for many gigabytes; one too large to decode is withheld.
- A JPEG of a few hundred bytes no longer takes 15 seconds.
- Your own models stop being read past their triangle limit (a 60 MB .obj took 2 GB before being refused).
- raid serve: one address can hold four connections at most, so it cannot keep everyone else out.
- Filter plugins: a word file must be in the plugin's folder; a value out of range is refused by name; an edited word
  list is read again (before, the old one stayed in the cache).

## Faster
- CI: step 01 126 s (631 s before), step 05 496 s (996 s), step 11 2,084 s (2,668 s), here.
- Guided codes 2 to 3 times faster; lists of numbers in a file 36 s to 0.018 s for 200 KB.
- The setup menu no longer reads faces.ini hundreds of times a frame; the fractals line judges what you see on several
  threads.

## Files to delete by hand
None.
