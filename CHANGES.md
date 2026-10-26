# tools\rar_check: every rar.exe test in one go (26 October 2026)

Unzip over the repository root. Commit on top of sieve-v22-rar.zip (d915694), local only.

Double-click `tools\rar_check.bat`, or run `py -3 tools\rar_check.py`. To include your own RARs, add them at the end: `tools\rar_check.bat C:\some\game.rar`.

- **What it uses:** your Release build (`out\build\x64-Release`) and your rar.exe (`SIEVE_RAR`, the PATH, or WinRAR's folder). `--sieve`, `--installer` and `--rar` override them.
- **What it makes:** RARs from the repository's own files with your rar, at 14 settings:
  - defaults, `-m0`, `-m1`, `-m5`, solid, solid with `-m5`;
  - BLAKE2, quick open on, quick open off;
  - creation times, times to the second, a 1 MB dictionary;
  - files without their folder, one file.
- **For each RAR:**
  1. Packs it, printing every try and how many of its first bytes matched.
  2. Installs it with rar: the RAR must come back byte for byte.
  3. Installs it with no rar: its files must arrive with the warning.
  4. Installs it with `sieve-install --yes`.
- **At the end:** a table, and the full log in `out\rar-check\rar-check.log`. Send me the log if any row isn't `yes / same / warned / same`.
- **Checked here** with the stand-in rar: the 8 settings it can make all pass. It only stores files, so it can't make the other 6; your rar.exe will.
