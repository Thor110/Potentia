# Changes: `sieve-manifest-v6`, and the search's time in the File Locator (17 October 2026)

Copy `Sieve/` over the repository's `Sieve/`.

- **`sieve-manifest-v6`: the minimum viable address** (`--items` now makes it; `--items --v5` makes
  the older one; SPECIFICATIONS §12.2 defines it).
  - Each item's place is stored as raw bytes, using the fewest bytes its line's places need,
    instead of hex. All the places go in one stream, carried raw or as LZMA2, whichever is smaller.
  - A line's items are carried as places only where that's smaller than packing their files.
    Otherwise they stay files. (The first version made the degree sample 1.87 MB: its 360
    near-identical AI models pack to almost nothing as files.)
  - Models of the AI line are now items, carried as their digits.
  - Measured here:
    - the degree sample: **391,033 bytes** (v5 443,973, v4 482,496);
    - the universalbibliothek model (34,072-byte file): **4,443 bytes** (v4 5,775; your 7z of it
      was 5,698).
  - Both were installed by `sieve install` and by the oracle, byte for byte. `sieve-install`
    refuses v6 as it does v5, and says to open it with Sieve.
  - The File Locator makes v6 too (the AI line's shape comes from the setup menu's settings).
  - Fixed: `--v5` and `--widen-weights` each swallowed the argument after them.
- **How long to search for shorter addresses** (tailoring):
  - **File Locator:** a new row above "Tailor the filters to these files", with − and + for
    **a file** and for **the whole run**, or the keys `[` and `]` (Shift for the run). Moving
    one moves the other, through the number of files: the run's time is the file's times the
    files. The first step down is no limit. Choosing a new folder works the other value out again.
  - **Command line:** `sieve locate --weigh --tailored --search-per-file S` or `--search-total S`.
    Give one and the other is worked out; give both and each one is a limit. The hallway's
    scripted `--locate` takes them too.
  - A search stopped by its time keeps the best it found. The table says when a line was stopped,
    found nothing in time, or never started because the run's time was used up. A `search:` line
    gives the times set and the time taken.
  - Measured on the CI's four-file folder: no limit, 530 bits in 2.7 s; 600 s for the run, the same
    530 bits in 1.4 s; 0.001 s a file, 544 bits in 0.19 s.
  - Caveat: a step already running isn't cut short, so a very small time can be overrun by one
    step (0.004 s set, 0.19 s taken, above).
- **Not done yet:**
  - The times aren't saved in `sieve-hallway.ini`, so they last one session.
  - v6 still stores each item's positional place. Storing a tailored stack's shorter address in
    the manifest is the next step that would tie the search to the instructions.
- **Checked here:** the whole hallway and tools built (no warnings), the unit suite, every new CI
  line (v6 on the degree sample and on the model, the search times on the command line and in the
  File Locator), and screenshots of the new row.
- **Docs:** SPECIFICATIONS §12.2 (v6, the search's time), README, HANDOFF (a new entry), and your
  "aiming in the right direction" quote in VOICES.
