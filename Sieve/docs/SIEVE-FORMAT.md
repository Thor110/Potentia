# The `.sieve` format, on one page

What a `.sieve` file is, layer by layer, as Sieve writes it at `sieve-manifest-v34` (the format rests
there, frozen 12 November 2026; v33 was frozen 3 November 2026 and v32 31 October 2026: HANDOFF's
standing rules). v34's head is v32's, read by its version byte, 34 (v33's, 33); what v33 and v34 add
is in section 4. It is for anyone writing a decoder of their own. The rules in full
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
dictionaries folder it carries (since 5 November 2026) and their count as 8 bytes (least
significant first) and the 16 bytes `sieve-carried-d1`, then the `.sieve`'s bytes, their count as
8 bytes, then the 16 bytes `sieve-attached-1` (§12.2, "Installer programs"). (The oracle reads
`.sieve` files, not installer programs: take the `.sieve` off the end first.)

## 2. The head

The manifest begins with its head, in bytes (since `sieve-manifest-v16`):

| Field | Bytes |
| :--- | :--- |
| the bytes `sieve` | 5 |
| the version: 16 to 32 for `sieve-manifest-v16` to `v32` | 1 |
| flags | 1 |
| the root's name, if flag 1: a varint of its length, then it | varies |
| each stream present, in the order x86 (flag 2), lzma2 (4), items (8): a varint of its packed size, a varint of its size, then one byte, below (v32: below) | varies |
| the rope stream, if flag 16: a varint of its size (it is carried as it is; v32: left out where it is the last part) | varies |
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

**v32's head** (the version byte 32; the flags as before: the version says how the head is read).
Each stream (the x86, lzma2 and items streams, and the body when packed) is its packed size as a
varint, **left out for the file's last part** (the rope stream if there is one, else the items
stream, else the lzma2, else the x86: its size is what is left of the file after the head and the
parts before it), then a varint of its size times 4 plus its **form**:

| Form | The stream | After it |
| :--- | :--- | :--- |
| 0 | raw LZMA2, its dictionary the one its size gives (the smallest `2 | (p & 1)` shifted left by `p / 2 + 11`, at least 4 KiB, that is at least its size, at most 64 MiB) | nothing |
| 1 | as before v32 | the byte above (and for 254 its parameters) |
| 2 | one LZMA2 chunk, carried as its data alone, the dictionary as form 0, lc 3, lp 0, pb 2 | nothing |
| 3 | as 2 | the chunk's settings' byte, `(pb × 5 + lp) × 9 + lc`, below 225 |

For forms 2 and 3 a reader puts back before the data the chunk's control byte (`0xE0` with the top
five of the 21 bits of size − 1), the low 16 bits of size − 1 and data size − 1 (16 bits), both high
byte first, the settings' byte, and after it a 0 byte, and reads that as raw LZMA2: so such a stream
is 1 byte to 2 MiB, its data 1 to 65,536 bytes. (Oracle: `stream32` and `framed` in `v16_text`.)

Sieve writes the **lowest version that holds what a `.sieve` uses**, and at its newest every
`.sieve` in bytes is v32 for its smaller head (`--v31` and older ask for older), so a v32 writer's
files carry 16 to 32 here. (Oracle: `v16_text`, which turns the head back into the older text form, then reads
that. Specification: §12.2, `sieve-manifest-v16` and each later version's "in bytes".)

## 3. The body: the listing

The body is packed where flag 32 is set (a varint of its packed size, a varint of its size, a
dictionary byte as a stream's, then raw LZMA2; v32: its packed size, then its size and form as a
stream's, then its bytes as carried), else a varint of its size and then it. Unpacked, it
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
`/` between parts, a folder's ending in `/`; entries come **sorted by their paths' bytes**, but for
two, each keeping an archive's own order: an archive's members (v31, below), which follow their
archive at once (`a.zip`, `a.zip/1`, `a.zip/2`, then `a.zip-x`), and the contents of an archive
carried as its contents (a record 4), which come together where the archive was, in the archive's
order (a zip holding `y.txt` then `b.txt`: `m/y.txt`, `m/b.txt`). A reader should not refuse those
for their order.

A **way** is a byte, then: **1** `raw`; **3** `rope` and the rest of the way as text; **4** `pieces`
and the rest as text; **2** an archive made again on installing: its format number, the rest of its
recipe as text (v32: its words after the kind, as many as the kind has, each a varint: twice the
number for a word that is a number as Sieve writes one, digits only, no leading zero, at most 18
digits; else twice its length and one, then the word), then its inner way (a way again); **0** any
other way, as text.

A **format** is a varint: 1 `xz`, 2 `gz`, 3 `bz2`, 4 `zst`, 5 `lz4`, 6 `br`, 7 `lzma`, 8 `lz`, 9 `7z`,
10 `zip`, 11 `rar`, 12 `cab`, 13 `iso`, 14 `lha`, 15 `cpio`, 16 `ar`, 17 `warc`, 18 `sk`, 19 `stb`,
20 `skm`, 21 `jbr`, 22 `pngr`, 23 `skr`, 24 `spn` (v33) (only ever added to); in a record 4, 0 then the name as text for one not listed.

The `line` lines come first; a reader puts the tree's line after them. A tab or line feed where the
record allows none is refused, as is a record, way or format the reader does not know. (Oracle:
`listing_from_bytes`, then `cmd_unpack_v7`. Specification: §12.2, `sieve-manifest-v20`.)

## 4. The streams, and each file's way

After the body come the streams' bytes, in the order **x86, lzma2, items, rope**, each its packed
size long (v32: each as its form carries it, the last what is left of the file). The files are then made in the listing's order, each taking the next bytes of the
stream its way names:

| Way | What it takes | Its rules |
| :--- | :--- | :--- |
| `raw` | its size, next in the lzma2 stream | §12.2 v4 |
| `x86` | its size, next in the x86 stream (raw LZMA2 after the x86 BCJ filter) | §12.2 v4 |
| `lines K` | a mask of file K's lines, one bit a line, lowest bit first, next in the lzma2 stream *after every other file's bytes there*; K counts the listing's files from 0, and file K is carried as itself (`raw`, `x86`, an item, the rope, pieces, mixing or a group's; never derived itself) | §12.2 v4 |
| `delta K L` | its script, L bytes, next in the lzma2 stream at its turn, applied to file K once every other file is made (delta-v1: a varint t; t even, t/2 new bytes follow; t odd, t/2 bytes copied from file K from where the last copy ended moved by a zigzag varint) | §12.2 v32 |
| `item K` (an `i` line) | its place, next in the items stream: `width` bytes big-endian, or with `digit-histogram-v1` a varint length and a code; made again as an item of line K | §12.2 v5 to v7 |
| `rope <parameters>` | a varint length and a code, next in the rope stream; `rope-wave-v1` to `v6` by its parameters | §12.2 `rope-wave-v1` to `v6` |
| `pieces z<n> r<n> ... rope <parameters>` | its z pieces next in the lzma2 stream; its r pieces joined, coded as one rope code, next in the rope stream; laid out in order | §12.2 v13 |
| `mix4 <parameters>` | a varint length and a code, next in the rope stream, decoded by mix-v4 (`B<bits>`, a picture's ` I<w>x<c>`, a text's ` D<n>` word list, or ` F80`, scowl-en-80 with each word counted 4,096, 64 or 1 by whether scowl-en-35 or only scowl-en-60 holds it; v33: after a picture's shape ` P<n>`, its pixels mixed as one-byte indices into a palette of n colours, then the palette; then, for text, ` W1`, the model primed on MDN's examples first (`data/dictionaries/web-mdn-2026-11-03-64k.bin`, its SHA-256 checked); then the match model's ways ` R1`, ` M4`, ` C2`; and last, for a file's own `mix4` with no picture or word list, ` X`, its bytes through the x86 filter) | §12.2 `mix-v4`, v28, v32, v33 |
| `runs` (v33) | a varint length and a code, next in the rope stream: segments, each a varint count and a varint pattern length and the pattern, repeated count times; or a ramp, count, a pattern length of 0, a byte of width and order, the first value and the step | §12.2 v33 |
| `copy K` (v33) | nothing: once every other file is made, file K's bytes (K before it, carried as itself, not a member, its size) | §12.2 v33 |
| `mixt <parameters> T<total>` and `mixn` | a group of files mixed together: the first file's way holds the group's code (next in the rope stream) and its total size, and the file is the group's first bytes; each `mixn` file after it (its own size in its record) is the group's next bytes. The group is used up exactly, before the next group and at the end; files of other ways may come between its files | §12.2 v30 |
| `mix`, `mix2`, `mix3` | as `mix4`, by mix-v1, v2 and v3 (read; no longer written) | §12.2 v24, v26, v27 |
| an archive made again: `xz`, `gz`, `bz2`, `zst`, `lz4`, `br`, `lzma`, `sk`, `stb`, each `<recipe> <inner size> <inner way>` (for `stb`, the inner bytes are the pixels); `skm <members> <skeleton size> <inner way>` | the inner bytes by the inner way (`raw`, `rope`, `pieces`, a mix way, `mixt` or `mixn`: never `x86`, nor another recipe), then the archive made from them by the named compressor and recipe; for `skm`, the inner bytes are its skeleton, and its data the members below | §12.2 v14, v17, v19 to v21, v25, v31 |
| a text with binary written as text (v33): `spn <members> <skeleton size> <inner way>` | the inner bytes are its skeleton: a varint of its spans; for each, a varint of the text before it, a byte of how it is written (base64, hex, or a list of byte values, and their details), a varint of its width, and for a list its prefix, separator and line break; then the text. Its spans' bytes, decoded, are the next `<members>` files, `<text>/<n>`, each its own way (one recipe allowed among them: `jbr`); each written again and put back | §12.2 v33 (`tools/cli/text_spans.hpp`) |
| a JPEG or PNG made again (v32): `jbr <form size>`, `pngr <filter> <strategy> <split> <head size> <inner size>`, `skr <skeleton size> <row bytes> <pixel bytes> <rows> <inner size>`, each then its inner way | `jbr`: the JPEG's brunsli form, made into the JPEG by brunsli; `pngr`: the PNG without its IDATs, then its rows unfiltered, the PNG re-saved by zlib at level 9 (each row by `filter`, or 5 each its own, at `strategy`), one IDAT put in at `split`; `skr`: the rows unfiltered, their filter bytes, then a v21 skeleton, the rows filtered again by their bytes and the PNG made as `sk` makes it | §12.2 v32 |
| v34's recipes, each then its inner bytes as any recipe's (§12.2 v34): `gif <sub-block size> <skeleton size> <inner size>`, `flac <skeleton size> <inner size>`, `pmp <inner size>`, `tiff <skeleton size> <inner size>`, `pcx <skeleton size> <inner size>` | `gif`: each frame's colour indices then the GIF without its LZW, which is coded again as compress codes it; `flac`: a FLAC stream then the sound file without its samples (WAV, AIFF, RF64, Wave64, .au, CAF; floats of whole 24-bit steps written back bit for bit); `pmp`: packMP3's form of an MP3, made into it by packMP3's decoder; `tiff`: as `sk`, with strips coded again by libtiff's LZW or PackBits, the predictor undone; `pcx`: the pixels, planes interleaved, then the header, the runs coded again. A `skm` member may also carry `jbr`, `flac` or `pmp` (v34) | §12.2 v34 |
| mix-v4's `E1` (v34) and a picture's shape from its header | `E1`: a program's mixing given contexts from where each byte falls in its x86 instruction, its mixers' weights chosen by that, and one stage after them; the shape is the packer's choice, read like any `I<w>x<c>` | §12.2 v34 |

A stream left over, or one that runs out, is refused. The x86 and lzma2 streams are raw LZMA2 at the
dictionary their byte names (the lzma2 stream may instead be carried as it is, byte 255, or mixed,
byte 254); the items stream is raw LZMA2 or carried as it is; the rope stream is always carried as
it is, each code in it with its length first. From v34, where the x86 stream holds two programs or more, its filter is undone on each program's
bytes from their own start (position 0, the filter's state fresh), not over the stream as one.

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
order (not an archive's members), its path as listed (without the root's name), a line feed, its bytes' SHA-256 in lower-case hex and a
line feed. It must equal the 32 bytes in the head. Sieve writes nothing to the disk until every file
and the tree are checked (§12.2 v5, and `docs/VAULT.md` for the vault's checks on top).

From v20 the records carry no file's own SHA-256, so the tree is the only check of the bytes. A
decoder that cannot make every file (below) cannot check the tree, and so checks the others by
their sizes only: the oracle says so (`the tree's SHA-256 not checked`) and writes them on that
understanding.

## 6. What a decoder needs

| To read | It needs | Where its rules are |
| :--- | :--- | :--- |
| the outer layer, the head, the body | big integers, varints, raw LZMA2 | sections 1 to 3 |
| `raw`, `x86`, `lines` | raw LZMA2 and the x86 BCJ filter (liblzma, Python's `lzma`) | §12.2 v4 |
| `rope` and `pieces` | rope-wave v1 to v6 | §12.2 `rope-wave-v1` to `v6`; oracle `rope_wave_file` |
| `mix4`, `mixt`, a mixed stream | mix-v4, and for `D35`, `D60` or `D80` the registered SCOWL list `scowl-en-35`, `-60` or `-80` (`data/dictionaries/`, each checked by its SHA-256 in `dictionaries.tsv`) | §12.2 `mix-v1`, `mix-v2`, `mix-v4`; oracle `mix4_file`, `mix_word_list` |
| `item` | the line's items made again from their places: Sieve's lines (pages, image, video, audio, AI), with `digit-histogram-v1` for coded places | §§3 to 12 and 12.2 v5 to v7; oracle `v7_item` (pages; PNG and MIDI items from a folder) |
| `F80` | the registered `scowl-en-35`, `-60` and `-80` (each checked by its SHA-256) | §12.2 v32; oracle `mix_word_list` |
| `delta` | nothing but file K's bytes | §12.2 v32 |
| `runs`, `copy`, `spn` | nothing more (base64, hex and decimal written again) | §12.2 v33; oracle `spans_made` |
| `P`, `X` in mixing | the palette's indices made back into pixels; the x86 filter | §12.2 v33; oracle `mix4_file`, `x86_filter` |
| `W1` in mixing | the primer `web-mdn-2026-11-03-64k.bin` (`data/dictionaries/`, checked by its SHA-256, which the format names) | §12.2 v33; oracle `mix_primer` |
| archives made again | the very compressor named, at the version named: zlib (`gz`, `pngr`), liblzma (`xz`, `lzma`), libbzip2 1.0.8, libzstd 1.5.7, liblz4 1.10.0, brotli 1.2.1, preflate and the LZMA SDK's encoder (`sk`, `skm`, `skr`), stb_image_write (`stb`), brunsli at the commit pinned in CMakeLists.txt (`jbr`) | §12.2 v14 to v32 |

A decoder for most files needs only the first four rows. Where it meets what it cannot make (an
item, an archive made again), it should refuse that file and say why, as the oracle does for
preflate, the LZMA SDK, stb_image_write and brunsli, which it does not have: it reads and checks their
skeletons and recipes, and names the files it could not make.

The tests hold examples of every version: `tests/` and the CI's step 11 (`ci/steps/11-oracle.sh`),
which makes `.sieve` files with Sieve and unpacks them with the oracle.

## 7. Compatible forever (Edward, 12 November 2026)

Every `.sieve` that any version of Sieve could read stays readable, even if a later version drops
it: the repository is public and every commit can be checked out and built, so the reader that
read it still exists. The history works as a lens on Sieve itself, looking backwards in time. When
the manifest versions are one day folded into one clean version and one addressing system, that
system keeps every earlier one reachable through it, by naming the version, and so the commit,
that reads it. Until Sieve becomes an operating system and a game engine (milestones to be worked
on together), the format is not changed beyond what keeps every earlier version readable. IDEAS
§19.137 has the notes.
