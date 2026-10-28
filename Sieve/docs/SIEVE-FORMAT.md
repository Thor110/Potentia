# The `.sieve` format, on one page

What a `.sieve` file is, layer by layer, as Sieve writes it at `sieve-manifest-v31` (the format rests
there: HANDOFF's standing rules). It is for anyone writing a decoder of their own. The rules in full
are in [SPECIFICATIONS §12.2](SPECIFICATIONS.md), written as each version's changes to the one
before; this page puts the newest together in one place and says where each part's rules are.

There is already a second decoder: **the oracle**, `reference/sieve_ref.py unpack FILE.sieve [--to
DIR]`, written in Python from the specification and sharing no code with Sieve's C++. Where this
page and the oracle disagree, the oracle and SPECIFICATIONS are right and this page is to be fixed;
each section names the oracle's function to read beside it.

Numbers are unsigned. A **varint** is unsigned LEB128: seven bits a byte, lowest first, the top bit
set on every byte but the last. Text is UTF-8.

## 1. The outer layer: an address

A `.sieve` file is the **address of a manifest** on Sieve's binary line, written as raw bytes, most
significant first, with no header. The binary line numbers every file shortest first, so the
address of a manifest of L bytes is its bytes read as one number, plus the count of all shorter
files, which is `(256^L - 1) / 255` (the bytes `01 01 ... 01`, L of them). In practice the file reads
as the manifest with every byte one higher: a manifest beginning `sieve` begins `tjfwf`.

To read it: take the file as a number N; find the largest L with `(256^L - 1) / 255 <= N` (start
from the file's length less one: L is that or the file's length); the manifest is
`N - (256^L - 1) / 255` written in exactly L bytes. The file is L bytes long, or L + 1
where the carry ran past the first byte. (Oracle: `instructions_bytes`. Specification: §12.1 for
the binary line, §12.2 "Installer files".)

An **installer program** is `sieve-install` with a `.sieve` attached: the program's bytes, the
`.sieve`'s bytes, their count as 8 bytes (least significant first), then the 16 bytes
`sieve-attached-1` (§12.2, "Installer programs").

## 2. The head

The manifest begins with its head, in bytes (since `sieve-manifest-v16`):

| Field | Bytes |
| :--- | :--- |
| the bytes `sieve` | 5 |
| the version: 16 to 31 for `sieve-manifest-v16` to `v31` | 1 |
| flags | 1 |
| the root's name, if flag 1: a varint of its length, then it | varies |
| each stream present, in the order x86 (flag 2), lzma2 (4), items (8): a varint of its packed size, a varint of its size, then one byte, below | varies |
| the rope stream, if flag 16: a varint of its size (it is carried as it is) | varies |
| the tree's SHA-256 (section 5) | 32 |
| the body (section 3) | varies |

**Flags:** 1 a root is named (else the manifest is one file, named as the root); 2, 4, 8, 16 the
streams present; 32 the body is packed; 64 a graph part follows the entries (a map: required at
v18, allowed from v19); 128 the body is text, not records (from v20, where text came out smaller).
Any other bit set is refused.

**A stream's last byte:** 0 to 40, the dictionary as LZMA2's one-byte property (`2 | (p & 1)`
shifted left by `p / 2 + 11`; 40 is 4 GiB − 1); 255, carried as it is (no LZMA2); 254 (from v29,
the lzma2 stream only), mixed by mix-v4, followed by a varint of the length of its parameters and
the parameters as text (`B22`, or `B22 D60` with the word list; never a picture's).

Sieve writes the **lowest version that holds what a `.sieve` uses**, so a v31 writer's files carry
16 to 31 here. (Oracle: `v16_text`, which turns the head back into the older text form, then reads
that. Specification: §12.2, `sieve-manifest-v16` and each later version's "in bytes".)

## 3. The body: the listing

The body is packed where flag 32 is set (a varint of its packed size, a varint of its size, a
dictionary byte as a stream's, then raw LZMA2), else a varint of its size and then it. Unpacked, it
is the folder's listing.

From v20 the listing is **records** (unless flag 128), one after another, each standing for one line
of the older text form:

| Record | Holds | Stands for |
| :--- | :--- | :--- |
| 1 | a path | a folder, `d<TAB>path` |
| 2 | a varint size, a way, a path | a file, `f<TAB>size<TAB>way<TAB>path` |
| 3 | a varint length, then a line of text | any other line as it is: an item (`i<TAB>K<TAB>path`), a RAR's recipe (`ur ...`, `um ...`), a graph line (`g<TAB>...`) |
| 4 | a format, a varint size, the archive's path and the folder's (each a varint length and text) | an archive carried as its contents, `u<TAB>format<TAB>size<TAB>archive<TAB>folder` |
| 5 | text up to a line feed | a line of Sieve's: `line K <width or digit-histogram-v1> <space id> <options>` |

A **path** is a varint of how many of its first bytes it shares with the last path written (a
folder's or a file's), then the rest as a varint length and text. Paths are relative to the root,
`/` between parts, a folder's ending in `/`; entries come **sorted by their paths' bytes**.

A **way** is a byte, then: **1** `raw`; **3** `rope` and the rest of the way as text; **4** `pieces`
and the rest as text; **2** an archive made again on installing: its format number, the rest of its
recipe as text, then its inner way (a way again); **0** any other way, as text.

A **format** is a varint: 1 `xz`, 2 `gz`, 3 `bz2`, 4 `zst`, 5 `lz4`, 6 `br`, 7 `lzma`, 8 `lz`, 9 `7z`,
10 `zip`, 11 `rar`, 12 `cab`, 13 `iso`, 14 `lha`, 15 `cpio`, 16 `ar`, 17 `warc`, 18 `sk`, 19 `stb`,
20 `skm` (only ever added to); in a record 4, 0 then the name as text for one not listed.

The `line` lines come first; a reader puts the tree's line after them. A tab or line feed where the
record allows none is refused, as is a record, way or format the reader does not know. (Oracle:
`listing_from_bytes`, then `cmd_unpack_v7`. Specification: §12.2, `sieve-manifest-v20`.)

## 4. The streams, and each file's way

After the body come the streams' bytes, in the order **x86, lzma2, items, rope**, each its packed
size long. The files are then made in the listing's order, each taking the next bytes of the
stream its way names:

| Way | What it takes | Its rules |
| :--- | :--- | :--- |
| `raw` | its size, next in the lzma2 stream | §12.2 v4 |
| `x86` | its size, next in the x86 stream (raw LZMA2 after the x86 BCJ filter) | §12.2 v4 |
| `lines K` | a mask of file K's lines, one bit a line, lowest bit first, next in the lzma2 stream *after every other file's bytes there* | §12.2 v4 |
| `item K` (an `i` line) | its place, next in the items stream: `width` bytes big-endian, or with `digit-histogram-v1` a varint length and a code; made again as an item of line K | §12.2 v5 to v7 |
| `rope <parameters>` | a varint length and a code, next in the rope stream; `rope-wave-v1` to `v6` by its parameters | §12.2 `rope-wave-v1` to `v6` |
| `pieces z<n> r<n> ... rope <parameters>` | its z pieces next in the lzma2 stream; its r pieces joined, coded as one rope code, next in the rope stream; laid out in order | §12.2 v13 |
| `mix4 <parameters>` | a varint length and a code, next in the rope stream, decoded by mix-v4 (`B<bits>`, a picture's ` I<w>x<c>`, a text's ` D<n>` word list) | §12.2 `mix-v4`, v28 |
| `mixt <parameters> T<total>` and `mixn` | a group of files mixed together: the first file's way holds the group's code (next in the rope stream) and total size; each `mixn` file after it takes its size from the group | §12.2 v30 |
| `mix`, `mix2`, `mix3` | as `mix4`, by mix-v1, v2 and v3 (read; no longer written) | §12.2 v24, v26, v27 |
| an archive made again: `xz`, `gz`, `bz2`, `zst`, `lz4`, `br`, `lzma`, `sk`, `stb`, `skm`, each `<recipe> <inner size> <inner way>` | the inner bytes by the inner way, then the archive made from them by the named compressor and recipe | §12.2 v14, v17, v19 to v21, v25, v31 |

A stream left over, or one that runs out, is refused. The x86 and lzma2 streams are raw LZMA2 at the
dictionary their byte names (the lzma2 stream may instead be carried as it is, byte 255, or mixed,
byte 254); the items stream is raw LZMA2 or carried as it is; the rope stream is always carried as
it is, each code in it with its length first.

**Archives inside.** A `u` record says an archive in the folder was carried as its contents, in a
folder of its name. From v31, `skm <members> <skeleton size> <way>` carries a gzip, zip or 7z as its
skeleton (the file) and its data as the next `<members>` files of the listing, named
`<archive>/<n>`: those are not files of the folder, and are not written out. A RAR's `ur` and `um`
lines give the recipe for the machine's own `rar` program to make it again; without it, a reader
gives the contents and says so (§12.2 v22).

(Oracle: `cmd_unpack_v7`, the loop over `files`, and `rope_wave_file`, `mix4_file`,
`skeleton_remake`.)

## 5. The check

Every file is checked against its size, and against its own SHA-256 where the listing gives one
(`-` where the tree's covers it). Then **the tree**: the SHA-256 of, for every file in the listing's
order (not an archive's members), its path, a line feed, its bytes' SHA-256 in lower-case hex and a
line feed. It must equal the 32 bytes in the head. Sieve writes nothing to the disk until every file
and the tree are checked (§12.2 v5, and `docs/VAULT.md` for the vault's checks on top).

## 6. What a decoder needs

| To read | It needs | Where its rules are |
| :--- | :--- | :--- |
| the outer layer, the head, the body | big integers, varints, raw LZMA2 | sections 1 to 3 |
| `raw`, `x86`, `lines` | raw LZMA2 and the x86 BCJ filter (liblzma, Python's `lzma`) | §12.2 v4 |
| `rope` and `pieces` | rope-wave v1 to v6 | §12.2 `rope-wave-v1` to `v6`; oracle `rope_wave_file` |
| `mix4`, `mixt`, a mixed stream | mix-v4, and for `D35`, `D60` or `D80` the registered SCOWL list `scowl-en-35`, `-60` or `-80` (`data/dictionaries/`, each checked by its SHA-256 in `dictionaries.tsv`) | §12.2 `mix-v1`, `mix-v2`, `mix-v4`; oracle `mix4_file`, `mix_word_list` |
| `item` | the line's items made again from their places: Sieve's lines (pages, image, video, audio, AI), with `digit-histogram-v1` for coded places | §§3 to 12 and 12.2 v5 to v7; oracle `v7_item` (pages; PNG and MIDI items from a folder) |
| archives made again | the very compressor named, at the version named: zlib (`gz`), liblzma (`xz`, `lzma`), libbzip2 1.0.8, libzstd 1.5.7, liblz4 1.10.0, brotli 1.2.1, preflate and the LZMA SDK's encoder (`sk`, `skm`), stb_image_write (`stb`) | §12.2 v14 to v31 |

A decoder for most files needs only the first four rows. Where it meets what it cannot make (an
item, an archive made again), it should refuse that file and say why, as the oracle does for
preflate, the LZMA SDK and stb_image_write, which it does not have: it reads and checks their
skeletons and recipes, and names the files it could not make.

The tests hold examples of every version: `tests/` and the CI's step 11 (`ci/steps/11-oracle.sh`),
which makes `.sieve` files with Sieve and unpacks them with the oracle.
