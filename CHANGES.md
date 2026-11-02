# Optimise Search and the draft (as before), plus the CI temp-folder fix (commits 5349a04, 3684660)

This zip is made against **your** latest commit (128b9c6, "sieve metrics"), so it holds everything
since then: the Optimise Search zip's contents (you were extracting it) and today's two fixes. Unzip
over the repository and rebuild.

## 1. Optimise Search, the draft as one number, the JPEG margin, GAME-OF-LIFE.md

The same as `sieve-optimise-search.zip` (see its notes). `file-locator-optimise-search.png` shows
the File Locator with it. It's a screenshot, not part of the repository.

## 2. The five JPEGs, confirmed on your files

With Try every way, each now comes out exactly as without it (LZMA2 kept):

| JPEG | Bytes | size/try now | temporary box (brunsli) |
| :--- | ---: | ---: | ---: |
| K2_00001.jpg | 175,693 | 174,857 | 143,565 (−18.3%) |
| K2_00000.jpg | 170,042 | 169,495 | 137,154 (−19.3%) |
| 1fuv6bxsbj7f1.jpeg | 85,821 | 80,681 | 70,813 (−17.5%) |
| 358023683_…_n.jpg | 64,624 | 64,109 | 52,376 (−19.0%) |
| 362302622_…_n.jpg | 58,664 | 57,273 | 45,863 (−21.8%) |

- **What they share:** all five are small baseline JPEGs whose entropy-coded data leaves mixing
  nothing to find.
- **Their route:** brunsli, with the temporary box.
- **ODDITIES** now lists them under Resolved.

## 3. CI's temporary folder (`ci/run.sh`)

**Why it was left:** `ci/run.sh` removed its copy with one unchecked `rm -rf`. On Windows a
read-only file, or a file still held for a moment by a program or a virus scanner, made it fail
silently. Failed or stopped runs keep their copy on purpose, and nothing ever cleared those.
(The steps themselves make no temporary folders; only `run.sh` does.)

Now:
- **Removing:** everything is made writable first, then tried five times a couple of seconds apart,
  then by Windows' own `rmdir`. It says plainly if a folder still can't be removed.
- **The reminder:** each run starts by saying how many earlier copies are left, and their size.
- **`ci/run.sh --clean`:** removes every `sieve-ci-*` folder left in the temporary folder.

Tested here:
- A copy with read-only files was removed.
- A one-step run removed its folder at the end.

On your machine, please try `ci/run.sh --clean` once, then a normal run.

## On step 11's time

On your machine it took over 20 minutes before this. The new blocks add about 1 to 2 minutes (the
v33 mixing check, about 40 s here; the stream-weighing check, about 55 s). The trace's times
(`python ci/trace_times.py LOGS/11-oracle.trace ci/steps/11-oracle.sh`) show which blocks take the
longest, if you'd like to trim it.
