# The documentation read through, during the v31 pause (28 October 2026)

Unzip over the repository root, then **delete the folder `Sieve/lt/`** (a zip cannot remove files).
Committed locally (not pushed). Documentation and one script's notes only: no code, format or
figure changed, so nothing needs building or the CI.

- `Sieve/docs/IDEAS.md`
  - A guide at the top: how to read the file, where the built, set-aside and framing parts are, the
    ideas that would need a new manifest version (and so wait for the pause to end), and the work
    that needs none.
  - Entries found built or answered, marked so: §4.1, §4.3 (partly), §4.4, §4.5, §4.6 (no signing),
    the licence (§8, §11: AGPL v3), §10's portal signs, warp by content (T), warp-to-start (Home),
    CI on three systems, `Previous/`, §11's names list, §9.4.
  - §16.3's "folding maps in: set aside" and §19.53's plan marked superseded.
- `Sieve/docs/SIEVE-INSTALL-USAGE.md`: said the newest version was v16 and releases were v4; now
  v31. Its one 2,000-word paragraph of version history is a table (v5 to v31, each with the figure
  measured when it was built). "Publish the four files" (it is five), and the icon "not yet" (it
  exists), removed.
- `Sieve/tools/make_release.py`: its notes said the release is v4; the code has used the newest
  (`--items`) since 22 October.
- `Sieve/docs/MANUAL.md`, `Sieve/web/README.md`: maps were called v18 and `source.sieve` a sealed
  map.
- `Sieve/README.md`: "why a library and not a generator" (IDEAS §9.4) under The Gallery of Babel.
- `Sieve/docs/SHOWCASE-PLAN.md`: a running order for the tutorial video; where it stands (encode.su).
- `Sieve/docs/HANDOFF.md`: this session's entry, and the 27 October plan read against the pause.
- `Sieve/lt/` removed.

## Files
- `Sieve/README.md`
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/MANUAL.md`
- `Sieve/docs/SHOWCASE-PLAN.md`
- `Sieve/docs/SIEVE-INSTALL-USAGE.md`
- `Sieve/tools/make_release.py`
- `Sieve/web/README.md`
- removed: `Sieve/lt/`
