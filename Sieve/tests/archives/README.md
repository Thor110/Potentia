# Archives for the archive checks

Five small archives that CI step 11 reads with Optimise Archive Formats (`sieve-manifest-v20`), one
for each kind Sieve reads through libarchive and cannot make itself. Each is copied unchanged from
libarchive 3.8.10's own test suite (`libarchive/test/*.uu`, decoded), where they were made for
libarchive's tests.

| File | Format | What it holds |
| :--- | :--- | :--- |
| `test_read_format_cab_1.cab` | cab, MSZIP (deflate: zlib) | `dir1/file1`, `dir2/file2`, `empty` |
| `test_read_format_cab_2.cab` | cab, LZX | the same, and `zero` |
| `test_read_format_rar_noeof.rar` | RAR 4 | `test.txt` |
| `test_read_format_rar5_compressed.rar` | RAR 5 | `test.bin` |
| `test_read_format_lha_header3.lzh` | LHA, level 3 header | `file1`, `file2` |

The SHA-256 of each file they hold is checked in `ci/steps/11-oracle.sh`, as `bsdtar` (libarchive)
extracts them.

**Licence:** libarchive's test files are part of libarchive and under its licence, BSD 2-clause
(`third_party_licenses/libarchive/COPYING`). They hold only libarchive's own test data.

## Made by 7-Zip (for v21)

Three more, made here with 7-Zip 23.01 from `tests/plugins/moby-sentences.txt` (its time set to
2026-10-25 12:00 first), for CI step 11's `sieve-manifest-v21` checks: each is made again exactly
from a skeleton and what it holds, 7-Zip's own LZMA, LZMA2 and deflate made by the LZMA SDK's encoder
and by preflate.

| File | Made with | Holds |
| :--- | :--- | :--- |
| `sieve-7zip-lzma2.7z` | `7z a -mx1 -mtm=off -mtc=off -mta=off` | `moby-sentences.txt` (LZMA2, level 1) |
| `sieve-7zip-lzma.7z` | `7z a -m0=LZMA -mtm=off -mtc=off -mta=off` | the same (LZMA) |
| `sieve-7zip-deflate.zip` | `7z a -tzip -mx=5` | the same (7-Zip's own deflate) |

These are Sieve's own, under Potentia's licence, as `moby-sentences.txt` is.

