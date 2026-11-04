# Try every way's duplicate-span fix, and ways_tally.bat (4 November 2026)

Commit 32a6bfb (local, not pushed), on top of 3342b46.

## The fix: Sieve/tools/cli/pack.cpp

**The case.** A page holding a picture written in base64, beside the same picture as its own file.
- **Prioritised for decoding:** the page was decoded and the picture carried once.
- **Try every way:** the page was mixed whole as text. Mixing makes base64 nearly as small as the picture inside it, so taking the page apart looked no better.

**The cause.** The packer's estimate didn't know that the decoded picture was the same bytes as a file already in the folder.

**The fix.** A decoded span whose SHA-256 matches a file in the folder is now weighed as the copy it will be.

| Test | Before | After |
| :--- | ---: | ---: |
| The test folder, Try every way | 419,182 | **353,014** |
| The test folder, fastest decoding | 375,365 | 375,365 |
| The page and door.jpg, Try every way | 127,034 | **63,976** |

Both install back identical. The format is unchanged: only the packer's weighing changed.

## CI

`Sieve/ci/steps/11-oracle.sh` has a new check after the base64 block: the page plus door.jpg, made with Try every way. It checks that the page is decoded, that door.jpg is carried as the lines of the page's member, and that the folder installs identical. I confirmed the old build fails it (mixed whole, 127,034 bytes).

## Sieve/tools/ways_tally.bat

It runs `ways_tally.py` the way the bench scripts run theirs:
- Double-click it, or give it a folder (or one .sieve) and optionally another sieve.exe first.
- It uses the Release sieve.exe to compare each file against LZMA2. Without one, it tallies the listings alone.
- The tables are written beside the folder as `<name>-ways-<date and time>.md` and `.tsv`.
- Line endings are CRLF.

`Proof/STATISTICS.md` now mentions it.

## Tested

- Unit suite: 154,318 checks, 0 failures.
- The base64 block of CI step 11, with the new check: passes, the oracle's lines included.
