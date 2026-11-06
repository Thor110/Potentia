# The RAR warning (6 November 2026)

Commit on top of your 9fda581 (local, not pushed).

## When a RAR is made again on installing, Sieve now says so when making it
- **`sieve locate`** prints a warning.
- **The File Locator** shows it in a message box when it reads the folder and when it saves.

The warning names each RAR and the rar version that made it. It says the RAR comes back byte for byte only where the installing machine has that version of rar. Elsewhere its files are installed in its place, and the install says so.

## Under Optimise Search, no RAR is made again at all
This holds even with a temporary tick left over from before. While Optimise Search is on, temporary's box is greyed, so that tick couldn't be taken back, and the warning's advice ("untick temporary") couldn't be followed.

So a RAR is made again only with Optimise Search off and temporary ticked. That is exactly where the box can be changed.

## Tested
With CI's stand-in rar:
- `--optimise-archives` warns, naming `made.rar (RAR 0.01 sieve-stand-in)`.
- With `--optimise-search` added, nothing is made again and nothing is warned.

CI step 11 now checks both. That block was run here.
