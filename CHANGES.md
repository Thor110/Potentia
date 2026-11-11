# Changes since 939666d "sieve extend release" (11 November 2026)

Work on the long plan, done without you. Nothing pushed. Extract this zip from the repository root.

## Stage 1: loose ends

- **Entries out of order are refused** (`tools/cli/locate.cpp`): a listing whose entries are not
  sorted by their paths' bytes (SPECIFICATIONS §12.1) was installed by the C++ reader, while the
  oracle refused it. Now both refuse it ("manifest: entries out of order"). CI step 11 checks both.
- **Recipe numbers checked** (`tools/cli/pack.cpp`): every recipe word that should be a plain number
  (jbr, gif, flac, skr, pmp, pcx, tiff, sk, lz4, br, bz2; pngr's two signed; lzma's `e` ending) is
  checked before an archive is made again, so a broken listing stops with a clear message.
- **The rope's tie-break is portable** (`core/src/rope_wave.cpp`): column grouping compared floating
  costs, which can round differently between compilers. It now uses exact integer x·log2(x) in Q16,
  so every build makes the same choice.
- **The runs list sorts naturally** (`client/harness.cpp`): run 10 comes after run 9.
- **CI hygiene** (`CMakeLists.txt`, `.github/workflows/build.yml`):
  - each fetched dependency is checked against its pinned commit (`sieve_pinned`);
  - a `SIEVE_WERROR` option makes warnings errors (on for Linux in GitHub Actions; a full GCC build
    with it on was clean);
  - GitHub Actions caches the fetched sources.
- **The reproduction loop** (CI step 04): `universalbibliothek.sieve` is installed on 2 to 8 threads,
  and every result must be the same.

## Stage 2: v34 candidates

- **The x86 filter restarts at each program** (v34 draft only): a program and a near copy of it now
  stay alike for LZMA2.

  | Test | v33 | v34 draft | Change |
  | --- | ---: | ---: | ---: |
  | Two 200,000-byte programs, 10 bytes changed | 44,928 | 34,945 | −22.2% |
  | The same, 1,000 bytes changed | 47,240 | 37,556 | |
  | Two real builds of sieve-install | | | −1.46% |

  Changed: `pack.cpp`, `locate.cpp`, the oracle, SPECIFICATIONS §12.2, NEXT-VERSION,
  `results/x86-restart-2026-11-11.md`. CI step 11 checks the size, and that the C++ reader and the
  oracle install it the same.
  - **Note:** a v34 draft file made *before* this with two or more programs is now refused (its hash
    no longer matches) and has to be made again. v33 files are untouched.
- **Not built:** BMP RLE, PCM in AVI/MOV, and WAD/PAK game files measured too little to be worth it.
- **Freezing v34 is left to you,** since it changes what Sieve makes by default.

## Stage 3: the voice

- **Words CMUdict lacks** (`tools/voice/learn_lts.py`, `data/voice/lj-speech/lts.tsv`, `voice.cpp`):
  these were spelt out letter by letter. Now Sieve tries a known word plus an ending (running, cities),
  then letter-to-sound rules learnt from CMUdict.
  - On 2,000 held-out words, 58.6% come out exactly right, and 10.3% of phones are wrong.
  - The extension (`extend_release`) carries `lts.tsv`.
- **Menus read aloud in Sieve's own voice:**
  - "Read aloud" and announcements use Sieve's voice wherever it is installed, on every system;
    Windows' own voice is the fallback.
  - The voice now plays itself on its own thread.
- **`/say WORDS` in CHAT:** speaks the words, and the console logs `voice: ...`.
- **Three hours of units instead of one:** the error rate fell from 23.3% to 19.8%. Not taken, since
  it needs about twice as much again in the repository and about three times the memory. The figures
  are in `results/voice-units-2026-11-09.md`; it's yours to choose.

## Stage 4: training speed

- **Profiled** (`results/training-kernels-2026-11-11.md`): most of the time is in two kernels
  (backward pass/gradient 32%, forward 15%).
- **Tried a transposed form.** It gave the same result byte for byte, but was 5 to 9% slower, so it
  was not kept.
- **Next to try:** run a step's windows through each matrix together.

## Checks run here

- Unit suite: 154,460 checks, 0 failures.
- CI steps 04, 10 and 11 passed. Step 11's new block was rerun alone after a fix to the test
  itself (it used a helper that step doesn't have).
- The reader was fuzzed under ASan/UBSan for 30 minutes, with the x86 pair and out-of-order files
  added as seeds: 30,260 runs, 0 findings.

## Commits (local only)

- 01f1eac Loose ends and the x86 filter restarted per program (v34 draft)
- a50ab55 Sieve's voice: words it lacks, menus read aloud, /say
