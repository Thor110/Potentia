# The CI run ending, and copies (2 November 2026)

Unzip this over the repository root, on top of your commit 81eeda4 ("sieve filters"). Three local
commits, not pushed: a72f732, b362980 and 34a5f06. The unit suite passes (77,133 checks,
0 failures). The new CI block passes; the full steps 10 and 11 haven't been rerun on this yet.

## 1. The full CI run on Windows ends again (a72f732)

My fix from the night's review caused this. It turned Git Bash's argument conversion off for
Windows' `rmdir`, but kept `cmd //c`. Only that conversion turns `//c` into `/c`, so `cmd` opened a
prompt and waited for input: the run printed its summary and never ended. Now `/c /s /q` reach `cmd`
exactly as written, and `cmd` is given no input, so it can never wait at a prompt. I couldn't run
this on Windows here, so your next full run is its test.

## 2. Copies cost one file's work, at every version (b362980)

The output is the same as before:
- A JPEG identical to an earlier one reuses its recoding instead of recoding again.
- A PNG's making is kept by its SHA-256 where the folder has a copy of it.
- Every LZMA2 estimate is kept by the SHA-256 of what it measured, so copies stop repeating it.

| Copies | Before | After |
| :--- | ---: | ---: |
| 100 of 1 MB of random bytes | 97.2 s | 7.8 s |
| 100 of a 1.2 MB JPEG | 160.2 s | 90.2 s |
| 40 of a 1 MB PNG | 210.3 s | 91.6 s |

Sieve's own `docs/`, sources and anchors make the same bytes, a little quicker (`docs/` 34 s → 27 s).

## 3. Copies carried as copies (v33 draft, 34a5f06)

With `--v33-draft`, a file identical to an earlier one is carried as `copy K`. That's decided before
anything else, so the copy isn't recoded, searched or packed. It works however far apart the files
sit and however the first is carried. Sieve, sieve-install and the oracle all read it.

| Copies | v32 | With the draft |
| :--- | :--- | :--- |
| 100 of a 1.2 MB JPEG | 978,332 bytes, 90.2 s | 963,838, 13.6 s |
| 40 of a 1 MB PNG | 1,052,542, 91.6 s | 1,052,129, 8.5 s |
| 100 of 1 MB of random bytes | 1,049,107, 7.8 s | 1,048,728, 7.0 s |

The v32 sizes above were only that small because the copies sat next to each other, within
LZMA2's 64 MiB window. A game with duplicate assets spread far apart would have paid for each copy
in full. The `copy` way doesn't depend on that.
