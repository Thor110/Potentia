# make_release fixed: "cannot read .../sieve-install.exe/1" (4 November 2026)

Copy `Sieve/` over the repository's. Two files: `tools/sieve_cli.cpp` and `ci/steps/11-oracle.sh`.
(The Manual Navigation log zip from earlier is separate; apply both.)

## What went wrong

The release's step 5 packs the release folder with `sieve locate ... --compare`. The packer found
spans of text-encoded binary inside `sieve-install.exe` and carried them decoded, v33's `spn` way
(as it did in ALTEngine's `data/Patches.json`). It lists such spans as members of their file:
`tools/sieve-install.exe/1`. `--compare` then went through the listing and read every entry from
the disk, the members included, but a member isn't a file on the disk. `sieve` failed there, and
the release script stopped after making the folder.

It reproduces on any folder with such a text, with `--compare`. The Linux release passed only
because Linux's `sieve-install` has no such spans.

## The fix

The comparison skips members. Their bytes are already counted in the file they're part of, so the
"original" is the folder's own size, as before.

## Checks

- A folder with base64 spans now packs with `--compare` (and `--items`), and installs the same.
- CI step 11 has a new line: `--compare` on its spans folder, checking that the original is the
  five files' own bytes. That block passes here.
- Your make_release should now get past step 5. Do run it again.
