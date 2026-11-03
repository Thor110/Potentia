# The web host fixed; damaged v33 files tested; STATISTICS at v33 (3 November 2026)

Copy `Proof/` and `Sieve/` over the repository's.

## Your CI failure (step 11, line 322): fixed

`sieve web-host`, which the browser extension starts, was never given the word lists or the
primers. Since `W1`, web pages under 4 KiB are mixed too, with the word list, so the host refused
the test site's `directory.sieve` (its `index.html`). The host now has what installing has
(`tools/sieve_cli.cpp`, one call before `run_web_host`). Step 11 then passed in full here (1,782 s),
and the unit suite passes (154,308 checks, 0 failures).

## The damaged-file test (`Sieve/results/damaged-v33-2026-11-03.md`)

3,160 damaged copies of `.sieve` files using `W1` in each place it can be (a page alone, a group,
the lzma2 stream), plus the earlier review's samples of v33's other ways. They went through
`sieve install`, `sieve-install`, the oracle, and a build with AddressSanitizer and
UndefinedBehaviourSanitizer. Every one was refused or installed the original files: none
different, none hung, and no memory error. The sanitizer build's only report is a deliberate
unaligned read inside 7-Zip's own x86 filter (`Bra86.c`), which the undamaged file reports too.
The script is kept as `Sieve/tools/damage_test.py`.

## STATISTICS

- Your bench at v33: the three photographs (3,715,757, 3,978,510 and 4,424,154; each a little under
  v32), and the brunsli figures; pi6, Don Quixote and control.py as before.
- ALTEngine v0.9.4 from your logs: **7,345,022 bytes, 4.77% under your 7z** (7,712,804), made in
  109.1 s, installed in 14.8 s. Six of its seven programs are mixed through the x86 filter. They
  install slower than LZMA2 (v0.9.2 at v32 installed in 3.7 s). Your hostname and paths are left out.

## POST-DRAFT.md

Unchanged: every number in it is already at v33. The photograph's 3,715,757 and 3,085,301, with
their on-disk sizes, are right; so are pi's 417,049, 1,808 and 0.44%, and control.py's 591. The AI
model's 4,180 is an item of the AI line, carried as its place, which v33's ways don't touch.

## The release script

On Linux, `make_release.py` ran every step and made everything: `sieve-setup`, `sieve.sieve`,
`sieve-source.7z`, `source.sieve`, `metrics.txt`, `SHA256SUMS.txt` and the notes. Each installer
installed back to the release folder, and `source.sieve` to the source. So I can't reproduce the
fault here. The script stops at the first command that fails, and it printed
`make_release: ... failed (N)` just before the window's "Press any key". Could you send that window's
last 30 lines or so? On your machine the release folder was made, so it failed at step 5 (packing
the folder) or later.

Also: the source 7z here is 49.5 MB. Most of that is the anchors folder, which is in the
repository's tree.
