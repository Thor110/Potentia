# List O: the RAID batch (O1, O2, O6)

The three RAID costs the reviews left, made. A share, check or fetch gives the same reports, and the held
folders the same pieces and counts, byte for byte.

## What changed

- **O1, reads buffered** (`tools/cli/raid_socket.cpp`). A connection's reads now come from a 64 KiB buffer
  rather than one `recv` a byte. Large reads still go straight to the caller. A `raid check` of 35 pieces
  on 3 peers made 485 `recvfrom` calls, and now makes 14.
- **O2, sharing's questions asked together** (`core/src/raid_net.cpp`, `raid_net.hpp`).
  - Each peer is asked which of up to 256 pieces it holds in one write (`RaidPeer::has_each`). The answers
    are read in turn, then each piece is placed in the same order as before.
  - So across a network, a share or check waits one round trip per peer per 256 pieces, rather than one a
    piece on each peer. That is worked out from the code, not measured: there is no way to add network
    delay here.
  - A peer that cannot answer that way is asked one piece at a time, as before, so its notes are the same.
    A piece put during the same share counts as held, as asking again would find.
  - The protocol is unchanged: the keeper already answered requests in turn.
- **O6, a held piece written outside the lock** (`RaidHeld::put`).
  - The space is counted, under the lock, before the write, and given back if the write fails.
  - Each write goes to a `.part` name of its own.
  - A second put of the same piece at once waits for the first, then finds it held.
  - The peers file is still written after every put. An older view of the counts never overwrites a
    newer one.

## How it was shown the same

The same scenario was run on the build before and the build after, each twice:
- four local peers, one refusing everything;
- a save with a chunk repeated in it;
- shared twice;
- checked;
- one peer wiped and checked, repaired and checked again;
- fetched back;
- four saves shared to one peer at once, against one after another.

Every report, every held folder's pieces and every peers file was the same byte for byte. Only the
servers' own logs differ: fewer requests answered, and concurrent connections ending in another order.

On one machine the time saved is small: a check took 8.0 ms before, and 5.3 to 6.1 ms after (medians of
seven).

New unit tests:
- `has_each` sends its lines in one write;
- eight puts of one piece at once keep it once, and say "had" seven times;
- of eight pieces with room for five, five are kept, the counts as one after another and no part file
  left.

## Also

- **REVIEW-ROUND:**
  - O1, O2 and O6 marked made.
  - List S's layouts noted as delayed, with your answers, which are also in IDEAS 19.191.
  - X1 added at the end of the list: review the fractal dimension and add as many fractals as can be
    tracked down.
- **OPTIMISATIONS.md:** entries 1, 2 and 6 removed. The others keep their numbers.
- **CI:** steps 01, 04 and 10 pass.

## Files to delete by hand

None.
