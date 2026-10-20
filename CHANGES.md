# Changes: `sieve-manifest-v12`, less around a file's bytes (20 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files.

## Why a PNG came out larger

A PNG saved at zlib's strongest has nothing left for LZMA2 or the rope to take out (`xz -9e`
grows it too). So the only change is what the instructions add around it. Measured here:

| file | its size | v11 | `xz -9e` |
|---|---|---|---|
| a photo as PNG | 715,596 | 715,801 (+205) | 715,692 (+96) |
| noise as PNG | 270,448 | 270,632 (+184) | 270,520 (+72) |

Of the noise picture's 184 bytes:
- 168 were the listing, 19 of them an empty `stream items 0 0 0` line;
- 16 were LZMA2's framing on bytes it couldn't shrink.

## What v12 changes

- **No line for an empty stream:** no items line where there are no items, and no rope line where
  no file is pulled tight. A v12 reader takes a missing line as an empty stream.
- **Files carried as they are** where LZMA2 makes them no smaller, written `stream lzma2 B B 0`
  (dictionary 0). Only the lzma2 stream, and only in v12. Older readers now refuse such a line
  with a clear message, rather than failing inside LZMA2.
- **Made only where one of these applies**; otherwise the version is v11 or older, as before.
  `--v11` (with `--items`) makes v11.
- `sieve locate` says when it carried the files as they are (an `as is` line).

## Measured here

| file | v11 | v12 |
|---|---|---|
| photo PNG (715,596) | 715,801 | **715,741** (+145) |
| noise PNG (270,448) | 270,632 | **270,593** (+145) |
| pi6.txt | 419,115 | **419,096** |

The 145 bytes left are the version line, the stream's line, the SHA-256 that checks the file on
installing, the file's line (size, how it's carried, name) and `end`. No format of this kind can
drop the name or the check. What would make a PNG itself smaller is its pixels, under the deflate.
That's the "other schemes" kind of work you put last, so it's in IDEAS §19.88 beside JPEGs.

All three of these install byte for byte with `sieve install`, `sieve-install` and the oracle.

## Your two photos

They're in IDEAS §19.88. On both, Sieve on the 7z beat Sieve on the JPEG, because the two find
different savings:
- **LZMA2** finds what repeats, mostly the headers and tables.
- **The rope** finds how unevenly the byte values are spread.

7z stores what it can't pack as it is, so the rope still finds that unevenness in the 7z, and the
two gains nearly add up. One set of instructions could use both; I haven't measured that.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,210 checks, 0 failures.
- The hallway CI step, start to end. New in it:
  - a file of noise carried as it is (`stream lzma2 50000 50000 0`): 153 bytes over the file in a
    folder, against v11's 179, installed by all three readers;
  - the same instructions relabelled v11, refused by Sieve and by the oracle;
  - v11 still made with `--v11` for the digit table.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.88, README, HANDOFF, help.
