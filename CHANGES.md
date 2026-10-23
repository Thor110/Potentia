# The installer makes the newest instructions by default (23 October 2026)

- `sieve locate FOLDER --installer X.sieve` and `--program` now make the newest Sieve instructions
  (v16, or v17 where a gzip is remade), as the File Locator does. `--items` is no longer needed;
  it's still accepted and changes nothing.
- `--v4` (new) makes the plain installer: every file packed by LZMA2, with no items and no rope.
- The older-format flags (`--v5` to `--v16`) work without `--items`.
- The CI lines that expect the plain installer now pass `--v4`, 9 of them.
- `sieve help locate`, `docs/SIEVE-INSTALL-USAGE.md` and HANDOFF are updated.

**Not tested** (your usage limit): it compiles, but the unit tests and CI haven't been run.

To apply: copy `Sieve/` and `.github/` over, rebuild, and run the tests and the CI.
