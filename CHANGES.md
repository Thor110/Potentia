# `sieve alike`: does MAPS-Larger.WoW contain MAPS.WoW? (30 October 2026)

Unzip over the repository root, on top of the last zip ("delta.cpp builds with MSVC"). Committed
locally (not pushed). Code changed: rebuild `sieve`.

## Your WoWRevived run

Thank you for the figures. The `.sieve` came to **1,075,924,966 bytes**, exactly the size
STATISTICS already gives for WoWRevived, to the byte. So no file was carried as a delta on that run.
There are two possible reasons:

1. **The draft switch wasn't on.** It has no box: it needs `--v32-draft` on the hallway's command
   line, or `v32_draft = 1` under `[locator]` in `sieve-hallway.ini`. When it's on, the comparison
   table's "listing and files" line says `sieve-manifest-v32`.
2. **It was on, but the two maps share less than half.** The packer pairs files only where at
   least half of one is found in the other. If MAPS.WoW compresses each map inside it separately, a
   small change makes the rest of that map's bytes different too.

## A quick way to tell: `sieve alike`

```
sieve alike MAPS.WoW MAPS-Larger.WoW
```

It reads the two files and says, in seconds:

- how much of the second is found in the first;
- what the delta would come to;
- whether the packer, with `--v32-draft`, would carry it so.

Nothing is written. On my 160 MB test pair:

```
blocks    28057 of the file's 30009 found in the base, 151167110 bytes (94.4%): the packer pairs them where this is at least half
delta     316 copies covering 152104783 bytes (95.0%), 7963294 new bytes; a script of 7965551 bytes before LZMA2
taken     yes (over 8 MiB, at least half found, at least half copied)
took      21.15 s
```

If you send me what it says for the two maps, I'll know whether the delta way can help them. If it
says "no", I'll look at how much they do share and why it's less.

For the size-prioritised run, please tick Keep a log and check the draft is on. The log then names
each pair, or shows that none was found.

## Checked here

- `sieve` builds with no warnings.
- `sieve alike` says yes on the 160 MB test pair and no on an unrelated pair, with the reason.
- The help check CI runs: 0 options missing.
- `sieve_cli.cpp` compiles for Windows with MinGW (`-fsyntax-only`).

## Files
- `Sieve/docs/NEXT-VERSION.md`
- `Sieve/tools/cli/help.cpp`
- `Sieve/tools/sieve_cli.cpp`
