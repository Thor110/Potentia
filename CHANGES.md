# Mixed files weighed against the stream; real-rope recorded (commit 498dbcf)

## 1. Mixed files weighed against the stream they'd share (`tools/cli/pack.cpp`)

The packer chose each file's way, and each group's, against LZMA2 of that file or group *alone*.
Files that share a lot with each other, like mozilla's programs, cost far less side by side in
the shared LZMA2 stream than they do mixed separately. After the groups are made, the packer now
weighs the mixed files against that stream: the files that aren't pictures first, then the
pictures. Where the stream with them is smaller, they go in as they are. No new way: every reader
reads it.

| Measured (no search limit) | Before | After |
| :--- | ---: | ---: |
| Silesia's mozilla, forced apart | 13,794,627 | 13,530,583 |
| Two 4.3 MB texts sharing 4 MiB | 1,992,382 | 1,042,336 |
| `docs/` and `tools/` (the same snapshot) | unchanged | unchanged |

mozilla whole is still smaller (13,376,239), and the packer keeps it whole as before. Every
result above installs back exactly.

- **The report** has a new line where the step takes the stream:
  `weighed   N file(s) mixed carried as they are instead, smaller in the stream they share (...)`.
- **For testing:** `SIEVE_MIX_GROUP_BYTES=N` makes the groups at most N bytes, so small files can
  show what happens past a group's end.
- **CI step 11** has a new block: two 65 KB texts, given back by Sieve, sieve-install and the
  oracle.
- **Docs:** METHODS (section 4) and SPECIFICATIONS (v30) describe it.

## 2. IDEAS §19.110: real-rope

Your design as you meant it: keep bytes *or* propagate noise, points that move, and splits up to
half the file. It sits beside what the rope does now (fixed points; keeping tried only on the best
wave), with today's measurements. The rope itself is unchanged.

## Tested

- The unit suite: 77,110 checks, 0 failures.
- CI step 11, the blocks the change touches: the map's group, the mixing blocks, t29/t30, g30, the
  new block, and the tar blocks.
- Here, sieve-install needs `SDL_VIDEODRIVER=dummy`, since there's no display.
