# The commands chart on Build or Buy, put right (26 October 2026)

Unzip over the repository root. Committed locally (not pushed), on top of your 0f46a05. Nothing here needs a rebuild; it's a Python script and the record.

## What was wrong
- The chart was drawn the right way up. The numbers were wrong.
- `tools/log_metrics.py` counted commands as the `` ### ` `` headings in README.md.
- On 23 October I moved each command's details from the README into `sieve help` (the "sieve docs help" commit). Those headings went, so every build from then on recorded **0 commands**. That's the drop at the right of the chart.
- The old count was also short before that. The README covered only 20 or 21 of the commands.

## What changed
- **The count now comes from `sieve help` itself:** its pages in `tools/cli/help.cpp`, except "lines", which is a page about the lines and not a command. It counts 32 today. `log_metrics.py --dry-run` shows the same.
- **The script now stops with an error if it finds no commands,** so the count can't drop to 0 unnoticed again.
- **All 51 records were counted again** from each build's own commit, the same way.
  - Every commit in the record is in the repository.
  - The old README count, redone on those commits, gave exactly the recorded 20, 21 and 0. So the redo reads the same files the builds did.
  - Commands now reads 28 → 29 → 31, a steady line instead of the cliff. Only the `commands` line in each record changed. `metrics/index.tsv` was rewritten from the records by the script's own `write_index`.
- **The next build will record 32:** `mix`, added with v24, is a page of its own.

## Files
- `Sieve/tools/log_metrics.py`
- `Sieve/metrics/builds/*.txt` (the `commands` line only) and `Sieve/metrics/index.tsv`
