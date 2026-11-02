# The walk's hashes reused, and the walk on every core (2 November 2026)

Unzip this over the repository root, on top of sieve-copies.zip (a72f732, b362980, 34a5f06). One
more local commit, not pushed: 4076d90. The unit suite passes (77,133 checks, 0 failures). The full
CI steps 10 and 11 haven't been run on it.

## 1. The walk's hashes reused

The walk hashes every file once (in the same read as the vault's checks). The packer hashed them
again in three places, and now takes the walk's hash wherever the bytes are still the file's own:
- the archive step's listing (an archive's members and files made again are still hashed: their
  bytes are new);
- the JPEG step's copies;
- the search's copies and mixing's notes.

One check stays, on purpose: the packer reads each file again and checks it against the walk's
hash, so a file changed between the two is refused, not packed under the wrong hash. It used to
run one file at a time; it now runs on every core.

## 2. The walk itself on every core

The walk was the larger cost: one file at a time, and the vault's picture check alone was 3.3 s
of 7.8 s on 100 copies of a JPEG. It now lists the folder, then hashes and checks the files on
every core. If a file can't be read or the vault withholds it, the first such in the listing's
order is still the one named. The listings are the same bytes.

| Folder (the walk alone, 4 processors) | Before | After |
| :--- | ---: | ---: |
| 100 copies of a 1.2 MB JPEG, 114 MB | 7.56 s | 2.10 s |
| 200 MB of random bytes, four files | 2.68 s | 0.88 s |
| Sieve's docs/ | 0.55 s | 0.19 s |

The JPEG and PNG copies pack to the same bytes as before, with and without the v33 draft.

## 3. The delta check's place: measured, it stays

On the two 160 MB maps (1,518 s in all), the delta check took 1.8 s. Everything before it (reading
again, the vault's chunks, the ways) took a few seconds; it already comes before every step that
costs. Those were the rope's search on the base map (768 s; the rope didn't carry it in the end),
the rope's code (135 s) and LZMA2's passes (about 800 s). It can't go before the archive step
(members aren't files until the archive is opened), and exact copies are already found first
(`copy K`, v33 draft).

## 4. Noticed: docs/ doesn't pack the same twice

The same build packed `docs/` to 594,466 bytes on two runs and 598,620 on two others. The
difference is in the group of 22 texts mixed together. The rope search stops at a time limit, the
likely cause; not yet confirmed. It's in results/weighing-order-2026-11-02.md with the rest.
