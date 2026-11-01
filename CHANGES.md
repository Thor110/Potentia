# Archive and compressed formats: tar (commit 3c2bf66)

**The rename.** The boxes are now **Archive and compressed formats**, in the File Locator, the
report lines, `sieve help` and the hallway's options. Unchanged: the command line's flags
(`--compressed-temporary`, `--compressed-permanent`, `--optimise-compressed`) and the
`sieve-hallway.ini` keys, so settings and scripts still work.

**A tar, under each box:**
- **temporary:** a POSIX or GNU tar (its `ustar` mark, every header's checksum right) is carried as
  v31's `skm`: its headers, padding and end as the skeleton, and each file's data as a member,
  carried its own way (a program through the x86 filter). It's checked to give the tar back before
  it's kept, and installs byte for byte.
- **permanent:** libarchive now reads a tar here too (before, only for extracting), so it's
  installed as its files where that's smaller.
- **No new version:** both are ways v31 and v20 already have.

**Measured here,** on a tar of two programs and three texts (9,584,640 bytes):

| | `.sieve` |
| :--- | ---: |
| no box | 3,287,224 |
| temporary | **3,069,619** (−6.6%), the tar byte for byte |
| permanent | 3,069,430, installed as its files |

A tar of three texts alone stays whole under permanent: it mixes smaller as the tar than its files
apart.

**Tested:**
- a new CI step 11 block for tar, both boxes, read back by Sieve, sieve-install and the oracle (which
  makes the tar again itself);
- CI step 10 in full, with the renamed report lines;
- `check_help.py`: no option missing.

The rest of step 11 wasn't run again here.

**For mozilla:** rerun Silesia with `bench_files_optimise_temporary.bat`.

**Files:**
- the code: `tools/cli/pack.cpp`, `locate.cpp`, `help.cpp`, `client/app_main.cpp`,
  `app_settings.cpp`, `file_locator.cpp`, `data/lang/en.txt`;
- CI steps 10 and 11;
- MANUAL, METHODS, FORMATS (tar's row), SPECIFICATIONS (v32 paragraph) and ROADMAP (C.4a built).
