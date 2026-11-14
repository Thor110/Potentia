# Review 3 (the code compared with its documentation): every finding made to agree

57 findings, none refuted: 44 where the documentation or a comment was wrong, 13 where the code was. Each is now made to agree.

## The code (13, with the comments)
**Three real bugs:**
- **A replay of long typed text plays back.** The recorder wrote a typed-text event of any length, but the reader refused one over 32 bytes (about 11 CJK characters from an input method), so such a recording could not be played. The reader now takes what the writer writes; there is no need for a cap, as the text is no longer than its line. Checked: a 36-byte event recorded and played back, the second recording byte for byte the first. The build before refused it. CI step 10 checks it.
- **A world's own objects are kept.** `place model:SHA` lines (objects of your own, from a later build) were skipped and then lost when the world was saved, though the spec promised them kept. They are now kept and written back with the world's other unread lines, and not drawn until own objects are built. Checked: the line survives a save; the old build rewrote the file without it. CI step 10.
- **Keep others' saves says when Online is off.** Its Answering on row now reads "…: not answering, as Online is off (Settings > Online)" where it showed only the address. CI step 10.

**Words on screen and in help:**
- Your dimension's footer says "Esc: pause menu" (it said main menu).
- O on a character says a character is not an item of a line and does not open. On a favourite no longer kept it says it stands as a sheet of paper until taken away. Before, both called it one of Sieve's shelf items. CI step 10.
- "Settings > Infinite RAID", as the menu calls it, in the share note, the CHARACTER page and the settings file's comments (it said Settings > RAID).
- `sieve raid` with nothing after it names check too, and the help's `--key` and `--peers` say check uses them. CI step 04.
- The catalogue entries for actions and saves point to SPECIFICATIONS 12.0n, where they are specified; their versions change accordingly. CI step 04.
- The help of `--v35-draft`, `--v33-draft` and `--sound-temporary` is as things stand since the v34 freeze.
- The hallway's `--help` says your character is you in the hallway, `--animation` takes v1 or v2, and `--catalogue-entry` opens SYSTEMS.
- Features' help names its third toggle.
- `sieve import`'s help and `--formats` say joints' moves are carried (v2) and only their growth is left out.
- `sieve catalogue`'s help names data/languages, the kinds the hand-written entries add, and what `--folder` does.

**Comments made true:** your character in the hallway; the pause rows; the personal world's header (Esc, both file versions); the RAID's folder id; the side pages; char_store; text_width; the music modes.

## The documentation (44)
- **MANUAL:**
  - the main menu's thirteen rows and Settings' six screens;
  - the Personal Dimensions screen's five rows;
  - when a world is written as v2;
  - which keys can be changed (Esc and a few others are fixed);
  - when a favourite kept by its place opens: unless its line is compacted;
  - O on a character;
  - Ctrl+S's fourth version (your character's actions);
  - Keep others' saves with Online off;
  - the drawings (Pixels, Voxels, bones only in the wireframe);
  - YOURS as the third tab.
- **SPECIFICATIONS:**
  - v1 worlds include the bookcase;
  - what is kept of a v2 world's unread lines, and what is said;
  - a favourite's index line;
  - what is hashed into a RAID folder's id.
- **FEATURES:** the older-version switches as they stand.
- **ROADMAP and IDEAS 19.187:** Esc in your dimension.
- **The catalogue's entry count:** the number given now.

## REVIEW-ROUND.md
- Section 4: review 3 ticked (5, 5a, 5b).
- Item 22: your store's characters, not yours, are the spheres in the hallway.
- Item 2: the bookcase is the last of Sieve's own objects, before your favourites and characters.

## The second look
- **The code side:** one reviewer found a CI check that the Online-off fix made wrong (it expected the address alone, and a second check of the same row had been added). Fixed: one check, expecting the new words.
- **The documentation:** one reviewer found one sentence wrong. A favourite kept by its place opens unless its line is compacted, not "only while off or marking". Fixed.

## CI
- Steps 01, 04 and 10: all passed.

## Files to delete by hand
- None.
