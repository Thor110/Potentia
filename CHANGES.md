# Infinite RAID: 3 copies, checking, and its own document

Builds on the loose-ends zip and the IDEAS §19.186 zip.

## What changed

- **3 copies by default.** Each piece shared now goes to 3 machines, or to each of them where there are fewer (it was 2). Set it with `[raid] copies` or `--peer-copies`. The index and manifest now go to as many machines as the chunks, not to every one.
- **Kept whole by its owner.** New command: `sieve raid check REMOTE --key KEY --peers ...`. It asks every machine which pieces it still holds and puts any kept on too few back on the next machines. `--no-repair` only reports.
- **Settings > Infinite RAID** (renamed from RAID) gains two rows:
  - **Copies**: how many machines keep each piece.
  - **Check my shares**: checks every save shared from this machine and repairs what is short.
- **docs/INFINITE-RAID.md** (new) covers:
  - what the RAID is for, in your words: helping humanity preserve all data of all types;
  - how it works and why 3 copies;
  - its safety valves and checks, with limits as rules over tags, an open and versioned list where the IP address is only the first tag;
  - what comes next: different networks, erasure coding, finding machines, identity, all data of all types, compute;
  - where everything is.
- **Docs:** IDEAS §19.176 records the placement decisions and §19.179 the safety-valve principle. RAID-NOTICE, the README, MANUAL, SPECIFICATIONS and ROADMAP are updated.

## Checked

- Unit tests: one machine loses everything; check finds every piece short, puts each back, then finds the save whole without sending anything more.
- CI 01, 04 (three servers this time, including the same loss and repair) and 10: all passed.

No files to delete.
