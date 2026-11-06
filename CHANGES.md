# The easy-wins list, worked down in order (6 November 2026)

Ten commits on top of 8bc923d. Nine items built; one measured and left. Nothing needed a new
manifest version. Every speed change was compared byte for byte, and gave the same bytes.

| # | Item | Result |
| ---: | :--- | :--- |
| 1 | A single file's LZMA2 figure made beside its mixing (IDEAS §19.120 a) | dickens 34.9 s to 23.6 s, the same bytes |
| 2 | `sieve_tests tests --only NAME[,NAME...]` and `--list` | `--only mix4,rope_wave`: 242 checks in 4 s |
| 3 | Installed files checked on up to 4 threads | check and write: six GIFs 1.41 s to 0.66 s, docs/ 0.26 s to 0.08 s |
| 4 | "Compare with zip and 7z" box (Z), ticked by default | unticked: six GIFs 25.7 s to 21.6 s in the File Locator |
| 5 | Non-x86 programs mixed without the start test | **measured and left**: no gain, and one Alpha library came out 0.6% larger |
| 6 | Mixing's tables at least B20 from 32 KiB | 0.05% to 3.3% smaller on ten files mixed alone; sieve_cli.cpp 30,967 to 30,754 bytes |
| 7 | `sieve warp/bind --file` read MIDI on every note set | the hymn's 73 events on notes104 and notes2, where it had been refused |
| 8 | "Create .sieve" on every file and folder (`hallway --create PATH`) | the File Locator alone, NAME.sieve saved beside it, closed with 0 |
| 9 | The File Locator's memory estimate, with a warning over the memory free | within about a quarter on six measured |
| 10 | Names folded as Windows folds them, beyond A to Z | É/é and Σ/σ one name; straße/STRASSE two |

## For you to try on Windows

- **Item 8, "Create .sieve":**
  - run `context\install_context.bat` again, then right-click a file or folder; on Windows 11 it is
    under "Show more options";
  - check that it finds `hallway.exe` above `tools\` in a release;
  - the batch files are CRLF.
- **Item 4, the comparison box:** untick "Compare with zip and 7z" (Z); More formats greys out with
  it.
- **Item 9, the memory line:** shown beside the estimate as a reading starts.

## Notes

- **Item 6** changes v33's bytes, though only to make them smaller, where a file of 32–512 KiB is
  mixed alone. Older versions (`--v32`) are as before.
- **Item 5:** ARM ELF is already taken as a program. Of mozilla's five Alpha libraries, three are
  already mixed (as pictures 4 bytes wide), one goes to LZMA2 either way, and libgklayout came out
  0.6% larger mixed without the test. So the test stays.
- **Item 8:**
  - several files chosen are still one window each, not one `.sieve` together;
  - "Other compressed formats" is still to come.
- **Item 10:** NFC and NFD forms of a name are still two names; macOS takes them as one.

**Tested:** the unit suite (154,318 checks, 0 failures), plus the CI lines each item touches. New CI
lines: step 3 (MIDI) and step 11 (the case of É and é).
