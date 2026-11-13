# The infinite RAID

**The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia).**
Its licence and the request that comes with it: [RAID-NOTICE.md](RAID-NOTICE.md).

## What it is for

Edward, 13 November 2026: "alongside the Sieve of Potentia as it is and the eventual game engine and or
operating system, infinite raid will hopefully transform and evolve into a system that helps humanity
preserve all data of all types."

It began with saves (IDEAS §19.150, 12 November 2026): "with all of this we are going to need a strong
save system", "an opt-in help preserve all data for all time option where people can contribute their
own data or just assist with storing save data", "providing all of humanity an infinite raid network for
processing, storing and preserving cultural identity throughout time". A save is only the first kind of
data it keeps; nothing in it is particular to saves. Any file, of any type, is cut, named, checked and
kept the same way, so the same system that keeps a character can keep a book, a recording, a game, a
dimension's file or a culture's archive.

Three things hold for everything it keeps, at every stage:

- **Nothing is trusted.** Every piece is named by the SHA-256 of its bytes, so a piece that has been
  damaged, swapped or forged is refused rather than used. No copy, drive or machine has to be believed.
- **Nothing is shared unless its owner asks.** Keeping your data on other machines, and keeping theirs on
  yours, are each opt in, behind red warning boxes (IDEAS §19.150), and off until ticked.
- **What others keep for you, they cannot read.** It leaves your machine sealed under a key only you hold.

## How it works now

### On your own drives (stages 1 and 2, `sieve-raid-v1`; SPECIFICATIONS §12.0l)

- A save is cut into **chunks** where its content says (16 to 256 KiB, about 80 KiB on average), so
  two versions of a save share every chunk an edit did not touch.
- Each chunk is named by its SHA-256 and kept on **2 of the drives** you name (`--copies` for another
  number): a spare drive, a second disk, a phone's storage.
- A small **manifest** lists a version's chunks; its SHA-256 is the version's id, it names the version
  before it, and it is kept on every drive. Storing a save again under its name makes a new version.
- `verify` checks every chunk against its name; `repair` writes again every copy that is damaged or
  missing, from a drive that holds a good one; a chunk lost everywhere is refused, never read wrong.

### Between machines (stage 3; SPECIFICATIONS §12.0o)

- **Your key** (`sieve raid key`, or made when sharing is first ticked in Settings > Infinite RAID): 32
  random bytes, `sieve-raid-key-v1`. Lose it and what others keep for you cannot be read by anyone,
  you included: keep it safe, and keep a copy.
- **Sealing** (`sieve-seal-v1`): ChaCha20 and HMAC-SHA-256 under keys drawn from yours. The same chunk
  seals to the same bytes, so a chunk a machine already holds is never sent again.
- **Placement: 3 copies, chosen piece by piece** (decided with Edward, 13 November 2026). Each sealed
  piece (every chunk, the manifest, and the index that lists them) goes to 3 machines, or each of them
  where there are fewer (`[raid] copies`, `--peer-copies`). The machines are ranked for each piece by
  SHA-256 of its name and their address, highest first, so a large save spreads across many machines,
  and a machine joining or leaving moves only the pieces that ranked it highest. This is mirroring,
  like RAID 1, done piece by piece over a changing set of machines rather than over one fixed group.
- **The remote id**: the name of the sealed index. With it and your key, `sieve raid fetch` brings a
  save back on any machine. The hallway keeps each one it shares in `raid-shared.txt` beside your key.
- **Kept whole by its owner** (`sieve raid check`, Settings > Infinite RAID's Check my shares). Machines
  come and go, so the owner asks every machine which pieces it still keeps; a piece kept on too few is
  had from one that holds it, checked against its name, and put on the next machines down. A piece no
  machine keeps is lost, and said.
- **Keeping others' data** (`sieve raid serve`, or Keep others' saves in the menu): a folder of sealed
  pieces it cannot read, up to the space you give in all (1 GiB by default) and the most for any one
  address (64 MiB), counted by address kept only as a salted hash (IDEAS §19.179). Nothing kept is
  deleted to make room; a piece over either limit is refused. Up to 32 connections are answered at once.
- **Online, off by default** (Settings > Online, `[online] enabled`; IDEAS §19.189). Edward: "We
  should make sure online isn't a requirement and that it is off by default". Sharing, keeping others'
  saves and checking shares all need it on as well as their own box; while it is off each says so and
  nothing is sent or answered. The RAID on your own drives needs no network and is never gated.
- **The protocol** (`sieve-raid-net-v1`): four requests (`has`, `get`, `put`, `bye`) over TCP, every
  piece checked against its name on arrival.

### Why 3 copies

A worked example, not a measurement: if any one machine is away a tenth of the time, a piece with 3
copies is out of reach only when all three are away, about one time in a thousand; a save of 100 such
pieces then has about a one in ten chance that some piece is out of reach at a given moment, and with 2
copies that becomes close to certain. More copies cost more space (3 copies, three times the save kept
by others), which is why the number is a setting, and why checking matters: a piece put back after a
machine leaves is a piece that stays at 3.

## Safety valves and checks

Edward, 13 November 2026: "there are going to need to be lots of safety valves and checks, for now I
think we were just using IP, but alongside that many more details can be tagged in the long run, so as
per anything in the system it should all be dynamic and scalable so as to support the nature of the
state space mechanics that demand its existence".

**Built so far:**

| Guard | What it stops |
| :--- | :--- |
| Every piece named by its SHA-256 and checked on arrival, on reading and on fetching | damage, swapping, forgery: a bad piece is refused, never used |
| Sealing under the owner's key (ChaCha20, HMAC-SHA-256), the MAC checked first | anyone keeping a piece reading or altering it unseen |
| Opt in, behind red boxes, off until ticked | data leaving a machine, or a machine keeping others', without its owner choosing it |
| The space given in all, and the most for any one address (a salted hash, never the address) | one machine, or one address, filling another's disk |
| Nothing kept is deleted to make room | an offer of new data pushing out data already kept |
| The protocol's limits: lines of at most 200 bytes, a piece of at most 4 MiB, 30 seconds of silence, 32 connections at once | a peer holding a server up, or flooding it |
| 3 copies of every piece, and the owner's check putting back what is short | machines leaving taking a save with them |

**The principle for what comes:** every limit is a rule over **tags**, and the tags are an open,
versioned list rather than one fixed field. The address is the first tag, as built. Others to come, each
added as a new version of the list and none taking the place of another: a verified identity, a
character (one per identity, IDEAS §19.178), a household, how much a machine has kept for others, how
long it has stayed. Each check names the tags it reads and has its own settings, and is written as data
the way Sieve's other formats are (measured figures as `KEY=N` records, filters as `.sfilter` walks), so
that adding a tag or a check is a new version, not a new design, and every check can be counted, tested
and shown. Like the lines, the RAID's rules are a state space of their own (IDEAS §19.185): which
machines may keep what, for whom, and how much, each answer checkable.

## Where to find it

| | |
| :--- | :--- |
| Commands | `sieve help raid`: put, get, list, verify, repair, key, serve, share, fetch, check |
| The menus | Settings > Online (needed for everything between machines); Settings > Infinite RAID (share, copies, check, serve); the CHARACTER page's Ctrl+S |
| Settings file | `[online]`: enabled; `[raid]`: drives, share, peers, copies, key, serve, listen, held, space_mb, per_peer_mb |
| Specifications | SPECIFICATIONS §12.0l (stages 1 and 2), §12.0o (stage 3) |
| The plan | IDEAS §19.150 (why), §19.154 (the store), §19.176 (the stages), §19.177 (the licence), §19.178 and §19.179 (identity and limits) |
| Code | `core/include/sieve/raid.hpp`, `raid_net.hpp`, `seal.hpp`; `tools/cli/raid_socket.hpp`; `client/raid_service.hpp` |
| Tests | the unit tests (`test_raid`, `test_raid_net`: sealing against RFC 8439 and RFC 2104, damage, repair, a machine losing everything and the save made whole again); CI steps 04 and 10 |

## What comes next

In the order they are likely to be needed, none built yet:

1. **Homes on different networks**: two copies in one house are not two. Placement preferring machines
   on different networks once there are enough of them.
2. **Erasure coding**: a chunk cut into k pieces and n - k more, any k of which rebuild it (the idea
   behind RAID 6). Ten of fourteen survives four machines gone for 1.4 times the space, where whole
   copies would need five times. A new version beside mirroring, never in place of it: what is kept
   now stays readable (IDEAS §19.137).
3. **Machines finding one another**: today a person names the machines (`[raid] peers`). Then peers
   passing on other peers, and the first known machines; who runs those is Edward's decision.
4. **Identity**: limits by address first, then by verified identity, then by what a person has kept
   for others (IDEAS §19.178, §19.179), one character per verified identity against spam.
5. **All data of all types**: any file, folder or archive offered to the RAID as saves are now, from
   the File Locator and the hallway; Sieve's own `.sieve` packing first, so what is kept is as small as
   Sieve can make it, and every piece still checked by its name.
6. **Compute as well as storage**: the same opt-in machines sharing work, the filters run across all
   dimensions at once (IDEAS §19.173), each result checked.

The RAID's place in the larger plan: Sieve of Potentia as it is (every possible file an address on its
lines, and `.sieve` packing), the game engine or operating system it may become (IDEAS §19.150), and the
RAID beneath both, keeping what people make and what they choose to preserve, for as long as people
keep machines running for one another.
