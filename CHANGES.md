# Sieve: space for the infinite RAID on its page, and a glass on every column

One commit on top of 3f0e128.

## Space for others (your note)

Potentia Settings > Infinite RAID has two new rows under Answering on:

- **Space for others** (1 GiB by default): the most this machine keeps for everyone, in all.
- **Space for any one** (64 MiB): the most any one address may keep, so no one machine takes all the
  space.

Left and Right halve or double each value, from 1 MiB to 1 TiB. The rows write `[raid] space_mb` and
`per_peer_mb`, and while Keep others' saves is on they take effect at once. The Answering on row above
them still shows how much is kept now.

## The magnifying glass on every column

As you said, every column always has one now, FRACTALS included. The glass no longer depends on the
line's kind, so a new dimension can't lose it the way the others did on 16 November.

## Checks

- **CI step 10:** Right on Space for others writes 2048; Left on Space for any one writes 32 and reads
  aloud as "32 MiB each".
- **Docs:** settings, help and help-library checks pass.
- **CI steps 04 and 10** ran on this tree.

## Files to delete by hand

None.
