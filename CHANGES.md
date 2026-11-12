# The infinite RAID, stage 3: between machines (12 November 2026)

On top of d72258d ("sieve language setup"). Unzip at the repository root; nothing to delete by hand.
**Re-run CMake**: there are new source files (core/src/seal.cpp, core/src/raid_net.cpp,
tools/cli/raid_socket.cpp), and on Windows `sieve` now links ws2_32 and the core links bcrypt.

## What it does (opt in only)

- `sieve raid key --out my.key` makes a key only you hold (never written over an existing file;
  readable by its owner alone). Lose it and what others keep for you cannot be read by anyone.
- `sieve raid serve --held D:/kept --listen 0.0.0.0:47100` keeps others' saves on this machine while it
  runs, with limits: `--space` MiB in all (1024) and `--per-peer` MiB for any one address (64). The
  address is kept only as a salted hash. It prints a warning box when it starts.
- `sieve raid share game.sav --drives ... --key my.key --peers a:47100,b:47100` sends a version out
  sealed (ChaCha20 + HMAC-SHA-256, checked against RFC 8439 and RFC 4231 test vectors), each chunk to
  two peers (`--peer-copies N`), and prints the save's remote id. A chunk a peer already has is not sent.
- `sieve raid fetch REMOTE --key my.key --peers ... --out game.sav [--drives ...]` brings it back on any
  machine, every piece checked.

Measured (4 MiB save, three peers on one machine, two copies): first share 124 pieces, 8,420,593 bytes;
one byte changed 8 pieces, 95,555 bytes; ten bytes inserted near the start 8 pieces, 92,007 bytes;
fetched back byte for byte, also with one peer gone; the wrong key gets nothing.

## Decisions taken as defaults, for you to overrule (IDEAS 19.176)

- Others' saves are sealed, so helpers cannot read them.
- Space given: chosen by whoever serves (1 GiB / 64 MiB an address by default).
- Finding machines: the addresses you give with --peers. No starting nodes run yet: who runs them is yours.
- Spam limit: by IP address first, as you chose (19.179), as a salted hash.
- Not yet: the menu behind the red warning boxes, peers passing on other peers, verified identity.

## Files

- New: core/include/sieve/seal.hpp, core/src/seal.cpp, core/include/sieve/raid_net.hpp,
  core/src/raid_net.cpp, core/src/raid_files.hpp (the RAID's file helpers, one copy),
  tools/cli/raid_socket.hpp, tools/cli/raid_socket.cpp. Each carries the 7(b) attribution notice.
- Changed: CMakeLists.txt, core/src/raid.cpp, core/include/sieve/raid.hpp, core/src/catalogue.cpp
  (4 new entries and 2 general ones: encryption, protocol; 193 in all), tools/sieve_cli.cpp,
  tools/cli/help.cpp, tests/test_core.cpp (test_raid_net), ci/steps/04-models.sh (two servers on
  127.0.0.1:47131-47132), docs: SPECIFICATIONS 12.0o, IDEAS 19.176, MANUAL, RAID-NOTICE, ROADMAP.
