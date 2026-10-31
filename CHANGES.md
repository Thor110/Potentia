# v32 frozen (commits e7107ec to d786e40)

Holds everything since your last update: the two WoWRevived records (sent before), the freeze, the
statistics, and the CI's checks brought to v32.

## The freeze (38e0ec7)

- **v32 is the newest.** The draft switch is gone:
  - `kNewestRope` is 15, so `sieve`, the hallway's File Locator, maps, sites and training runs make
    `sieve-manifest-v32` at their newest.
  - **`--v31`** makes v31, byte for byte as before (checked on a small file: 1,153 bytes, the same
    file).
  - **`--v32-draft`** is still accepted and does nothing, so old command lines work.
  - **The hallway:** an old `[locator] v32_draft` line in `sieve-hallway.ini` is read and ignored.
    The File Locator's orange draft line and the log's "v32 draft" setting are gone.
- **The report** no longer calls anything "a draft: not to be published". The installer's summary
  has a v32 line.
- **`sieve help locate`** has `--v31`, and `--compressed-permanent` says what v32 installs.
- **Documents:**
  - SIEVE-FORMAT.md is now v32's edition: the head's stream forms, recipe numbers, the formats 21 to
    23 (`jbr`, `pngr`, `skr`), `delta`, `F80`, and the group and `lines` rules.
  - SPECIFICATIONS' v32 paragraph is no longer a draft.
  - HANDOFF's standing rule: the format is frozen at v32, and v33 would be gathered the same way.
  - README, SIEVE-INSTALL-USAGE (with a v32 row), MANUAL, METHODS, FORMATS, NEXT-VERSION and ROADMAP.

## Statistics (Proof/STATISTICS.md)

- **WoWRevived:** the at-a-glance row is now v32's (1,001,994,144, −6.68% against 7z), and both v32
  runs are in its section beside the v31 figure.
- **ALTEngine v0.9.2** (its commit linked; the build itself says 0.9.0): 7,546,366 against 7z's
  7,664,970 (−1.55%), installed in 3.7 s. Its seven programs are 96% of the `.sieve`: the place to
  weigh BCJ2.

## CI brought to v32

At the newest every `.sieve` in bytes is v32 now, so checks that meant "the newest" or "the version
before" changed:
- **Steps 10 and 11:** the runs that compare with the version before now pass `--v31`. That's v20's
  checks of a small folder in step 10 (the same bytes as before), and each v32 block's "nothing is
  v32" run.
- **Step 11:** a map at the newest is v32, a site's page mixes with `F80`, and the report lines'
  version ranges run to v32.

## Tested here

- **Steps 1 to 9 and 12** passed in a full run.
- **Steps 10 and 11** pass with the fixed scripts. Step 10 passed whole; step 11 passed across
  three runs that between them cover the whole step.
- **The unit suite:** 77,110 checks, 0 failures.

Next for you: the full CI on Windows, Linux and macOS, then the Proof samples and STATISTICS
measured again.
