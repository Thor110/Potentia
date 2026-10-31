# Your decisions, built (commits 5e7abd7 and 95ba0c7)

## B.2: (a), groups with another file in the middle

- **The rule now:** SPECIFICATIONS v30 allows files of other ways between a group's files. A group
  stays open until its total is used, which it must be before the next group and at the end.
- **Oracle:** its refusal is gone. The three files it refused before now read back the same.
- **No `.sieve` changes.** Every packer has made such groups, and Sieve's reader has always read
  them.
- **CI step 11:** two texts grouped with random bytes between them, read back the same by Sieve and
  the oracle.

## B.5: the version byte says how a head is read

There is no second flags byte. A head whose byte is 32 is read as v32's.

## B.3 and B.4: v32's head with less overhead (behind the draft)

**Each stream in the head** (x86, lzma2, items, and the listing when packed) is written as its
packed size, then its size × 4 plus a form:
- **0:** LZMA2 with the dictionary its size gives, so no dictionary byte;
- **1:** as before v32, with a byte after (the dictionary, carried as it is, or mixed);
- **2:** a stream that is a single LZMA2 chunk, carried as the chunk's data alone. The reader puts
  the chunk's six header bytes and the end byte back, so every reader after that is unchanged;
- **3:** as 2, with the chunk's settings byte after.

**The last part of the file carries no packed size;** it's what is left of the file.

**Remade files' recipes** in the listing write their numbers as varints rather than text.

**With the draft, every `.sieve` in bytes is v32.** Measured, each given back the same:

| | before | v32 |
| :--- | ---: | ---: |
| `tools/fetch_corpus.py` | 1,153 | 1,144 |
| `tools/png_unfiltered_probe.py` | 1,365 | 1,356 |
| `ci/trace_times.py` | 1,249 | 1,240 |
| the extension's four icons (temporary) | 16,706 | 16,680 |
| three 7-Zip archives and a gzip (temporary) | 16,413 | 16,395 |

**The saving is 9 bytes rather than the 11 estimated.** I left in the stream's unpacked size: the
listing could give it, but working it out needs every way's share of the stream. Getting that wrong
would refuse good files, to save 2 bytes.

## Found on the way

The oracle refused a `lines` file whose base is mixed (`all.txt` as `mix4`, `third.txt` from its
lines). Every packer since v24 makes these, and Sieve's reader takes them, so it's the same case as
the groups. Following your rule (allowing it changes no file; forbidding it would make files
larger), the specification and the oracle now allow any base carried as itself. Tell me if you'd
rather decide that one separately.

## Tested here

- Unit suite: 77,110 checks, 0 failures.
- CI step 11, every block from the groups block through the v32 section (including the new ones),
  passed.
- A folder with a program, lines, a remade gzip, a `skr` PNG and mixing, with the draft: installed
  the same by Sieve, and read the same by the oracle (but for the PNG, which needs preflate).
- Without the draft, eight files' `.sieve`s are byte for byte as before.
