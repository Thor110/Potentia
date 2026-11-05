# Installer programs carry the dictionaries (5 November 2026)

Commit 414a0e7 (local, not pushed), on top of your "sieve metrics" (99b44fd).

## The cause

The release's files were packed using the word lists. In the release made here, 25 of the 30 files packed that way (by "mixing") used a word list. Unpacking those files needs the same word lists.

`sieve.exe` (`sieve-install` with the release attached) didn't carry them. Downloaded and run on its own, it found no dictionaries folder, so it said "Nothing to install yet." with the dictionaries.tsv error.

The release script's check didn't catch it, because it installed with the build's own `sieve`, which has the dictionaries beside it.

## The fix

**Every installer program now carries the whole dictionaries folder,** packed: 855,664 bytes for its 12,061,527, measured here. The layout is:

    <sieve-install> <dictionaries> <8-byte length> "sieve-carried-d1" <installer> <8-byte length> "sieve-attached-1"

**`sieve-install` uses them only when it finds no dictionaries folder of its own.** It:
- checks every size before decoding, and every file against its SHA-256 after;
- writes them to a folder of their own in the temporary folder, and looks there first;
- removes that folder when it ends.

**What changed where:**
- Both ways of making a program carry them: `sieve locate --program` (which reports what it carried) and the File Locator.
- Installer programs made before are read as before.

## Tested here (Linux)

- **Reproduced first:** I made an installer program of three docs files that were packed using a word list, and ran it from an empty folder. The old layout fails with your message. The new one installs every file identically and leaves nothing in the temporary folder.
- **CI step 10:** passes (626 s). It now checks the new layout and runs that same empty-folder test.
- **Release script:** `make_release.py` now also runs `sieve.exe` itself, copied alone into an empty folder with its own temporary folder, as someone who downloaded it would. Here that check passed on the Linux `sieve-setup`, along with the whole release.
- **Unit suite:** 154,318 checks, 0 failures.

## Please

Rebuild, run the release script again, then run the new `sieve.exe` from a folder that has nothing else in it. The release script's new check does this too.

## Files

- **Installer and packing:**
  - `tools/cli/locate.cpp` and `.hpp`: the layout, reading it, and unpacking and checking the dictionaries;
  - `tools/cli/pack.cpp` and `.hpp`: `carried_dictionaries_block`;
  - `tools/cli/dictionaries.cpp` and `.hpp`: `set_dictionaries_folder`, looked at first;
  - `client/installer_main.cpp`: uses them where none are found;
  - `tools/sieve_cli.cpp` and `client/file_locator.cpp`: both carry them.
- **Checks:** `ci/steps/10-hallway.sh` and `tools/make_release.py`.
- **Docs:** `docs/SPECIFICATIONS.md` (§12.2, "Installer programs"), `docs/SIEVE-FORMAT.md`, `docs/HANDOFF.md` and `docs/MANUAL.md`.
