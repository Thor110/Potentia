# The vault

*Built (28 September 2026): the exact check, the decoders for text, the perceptual check for pictures (PDQ), and `chunks` entries for pieces of files, everywhere below.*

A list of content that Sieve refuses to show, emit or pass on, checked
wherever content enters Sieve, leaves it, or is shown in the hallway. The space itself is
untouched (every address still exists; a number cannot be deleted), so this is a line about what
the tool does, not about what exists: forbidden content stays addressable in principle, and Sieve
declines to be the thing that produces it. Unlike the filters, the vault is not optional and not
a plugin: it is built in, on in every official build, and has no setting that turns it off.

## 1. What it does

- A unit or file that matches a vault entry is **withheld**. It keeps its address and its place
  on its line, so every address, count, survivor number and compact address stays exact, but it
  is drawn as a blank "withheld" item and cannot be taken, read, saved (F), exported, installed,
  mapped or walked to.
- Withholding sits above every filter mode: the excluded view never shows a withheld item.
- Nothing about a match is shown beyond the word "withheld": not the entry, not its source.

## What it governs, and what it does not

The vault is for known **files and pictures**: what the hash lists that exist in the world are
made of. It withholds them **in any form Sieve can recognise**: as files (the binary line, and every
file located, installed, mapped or saved), as pictures and video frames (by the perceptual check),
as melodies and models, and **written out as text**. Any alphabet of two symbols or more can
write out any file (hex, base64, the letters a to p for the sixteen nibbles, "a" and "b" for bits),
so every unit of text, every page, every title and every book's pages read as one are checked as
a file written out: their own bytes, and each encoding in a fixed set, decoded (below).

It never judges text by **what it says**. Ordinary writing decodes to noise, which matches nothing;
only text that *is* a known file, byte for byte or in a recognised encoding, is withheld. The
tradition of not outlawing ideas in writing is long and hard-won, and on a text line a vault that
matched words would be refusing ideas, which is where safeguarding becomes censorship. (What goes in
a vault is a list of files; a text file listed there is withheld as that file, on any line.)

**The decoders** (`vault-decoders-v1`, `tools/cli/vault_decode.*`), versioned like the filters and
never edited: *text* (its own bytes: Latin-1 when every character fits in a byte, else UTF-8; as it
is, without its padding, and with one final line feed), *hex* (either case, `0x` or `\x`
prefixes, common separators), *base64* (standard or URL-safe, padded or not, a `data:` prefix
dropped), *base32*, *ascii85*, *decimal* byte lists, *nibbles* (a-p), *spelled* hex digits (zero to
fifteen, or a to f, as words), and *binary* (any two symbols, both ways round). Whitespace is
ignored throughout, and each must account for the whole text. They cost about 0.05 ms for a
3,200-character page, and run only on what is shown or passed on.

**What it can and cannot do.** An address cannot be made secret: it is arithmetic (in positional
order, a withheld item's address is its neighbour's plus or minus one), and an address is its
content in another form. Nor can every encoding be caught: they are endless (a bespoke cipher, any
compression, the address itself), and a file split across many units matches no whole-file hash
(the `chunks` entries below catch pieces of a couple of kilobytes and more; smaller pieces pass). What the vault does is make sure Sieve is not the one
that hands a known file over, in the forms it shows, recognises or produces: it does not draw it,
show its address, let it be taken, save it, or pass it on. Compressed or enciphered data looks like
noise, which the default filters already exclude; the vault is what stays when filters are off.
Anyone determined can still do the arithmetic themselves; nothing can stop that, and the vault does
not pretend to.

## 2. Where it checks

| Where | What is checked |
| :--- | :--- |
| Content coming in | warp (T), the File Locator (a file, a folder, Go to it), `sieve warp`, `sieve locate`, installs of Sieve instructions and installer programs |
| Content going out | F (save), Sieve instructions and installer programs, held anchors added to maps, `sieve read --out`, exports |
| Content shown | every item drawn in the hallway and every unit the command line prints, on every line |

Checking what is shown matters as well as what comes in: as the filters and the guided ordering
improve, walking the space surfaces ever more real-looking content, so a match could one day be
walked to rather than brought in.

## 3. How it matches

- **Exact:** SHA-256 of the content (a file's bytes; a unit's canonical bytes). One hash and one
  set lookup; Sieve already computes most of these.
- **Perceptual:** for pictures, PDQ (Meta's, open source, BSD; `third_party/pdq`), the hash the
  industry's shared picture lists use; PhotoDNA is licensed and not available to open source. Its
  256 bits come from the picture's luminance, cut down to 64 x 64 and turned into frequencies, so a
  resized, recompressed, recoloured or lightly edited copy lands within a few bits of the original.
  The rule, `pdq-match-v1`: a picture whose PDQ quality is at least 50 is withheld when any of its
  eight orientations (as it is, three rotations, four flips) is within 31 bits of a `pdq` entry,
  the threshold PDQ's authors give. Checked on every picture Sieve draws (an image unit, each frame
  of a video, every cover) and on every file that is a picture (any format stb_image reads, each
  frame of an animation), wherever files are checked, so also on bytes decoded from text.
  **What it misses:** pictures below quality 50 (too plain to tell apart: PDQ's own advice is to
  ignore those hashes) or under 5 pixels either way; heavy crops (PDQ sees the whole frame, so a
  quarter cut away is a different picture); pictures reduced to a few colours (the mono and ega16
  palettes lose too much for the hash to survive); and a picture file over 8192 x 8192, which is
  not decoded (its exact hash is still checked). A picture file split across units, or hidden
  inside another file, is the `chunks` entries' job.
- **Pieces:** a file can also be listed by its content-defined chunks (`cdc-v1`,
  `core/include/sieve/chunks.hpp`). A rolling hash over the bytes cuts wherever the last 64 bytes
  meet a fixed condition (chunks of 64 to 1,024 bytes, about 300 on average), so where the cuts
  fall depends on the content alone, and a piece of a file, cut out anywhere, falls into step with
  the file's own cuts within a chunk: from there its chunks are the file's. Each chunk is named by
  its SHA-256. The rule, `chunk-match-v1`: bytes are withheld when they hold two different chunks
  of one entry (its only chunk, for a file of one); chunks of one repeated byte never count, and
  two are asked for so that one chunk that ordinary files happen to share cannot match on its own.
  Checked wherever bytes are: files (read a block at a time, so any size), units, pages and books,
  and bytes decoded from text. **Measured** on random data, a piece of a listed file is caught
  98% of the time at 2 KB, 86% at 1.5 KB, about two times in three at 1 KB, 22% at 600 bytes, and
  hardly ever under 500: a text unit of a long line, a page, a book or a file of the binary line
  is well past that; a short unit is not. Smaller chunks would catch smaller pieces at the price
  of longer entries: that would be a new version (`cdc-v2`), never an edit of this one. The
  Python oracle cuts independently and `tests/vectors_chunks_v1.tsv` pins both, so a list-holder's
  tools can make entries that match Sieve's cutting exactly.
- **Cost:** about a tenth of a millisecond for a 64 x 64 picture with all eight orientations, only
  for what is drawn or passed on; files are decoded only when their first bytes say they are a
  picture.
- The vault never changes the numbering, so it costs nothing when walking or filtering unseen
  units, and compact mode is untouched.

## 4. The vault files

- A format of its own (`sieve-vault-v3`), conceptually a map's cousin but holding only hashes: no
  names, paths or descriptions. Canonical, line feeds only:

  ```
  sieve-vault-v3
  entries 3
  v	sha256	3b19e41d21594052df12c58b539a105951c93be229ce6e1683b6db5a2e6de1f3
  v	pdq	69b3ded065e3895c02a91ddd969247727a186104d9166bd50e7957eb7184e4bd
  v	chunks	<64 hex digits>,<64 hex digits>,...
  end
  ```

  A `pdq` hash is written as PDQ's own tools write it, so a list's hashes are copied in as they
  are. A `chunks` entry is one file: its chunks' SHA-256s, commas between.
  `sieve vault --chunks FILE... [--common FOLDER] > list.vault` writes a whole vault file listing
  each FILE by its chunks, less chunks of one repeated byte and less any chunk also found in the
  files under FOLDER (a collection of ordinary files: headers and other structure that unrelated
  files share would otherwise be listed too). `sieve-vault-v1` (sha256 only) and `-v2` (no chunks)
  files are still read. `sieve vault` says how many
  entries are loaded, of each kind, and from which files; `sieve vault FILE...` says whether files
  are withheld; `--written` checks a text file as a file written out; `--pdq` prints a picture's
  PDQ hash and quality, so a deployment can check its hashes agree; `--parse` checks a vault file.
- The built-in entries are compiled in, so a program on its own (an installer program, say) still
  has them. Every `*.vault` file in the `vault/` folder beside the programs (or above an
  installation's `tools\`) adds to them, as does `data/vault` when run from the repository. A file
  there that fails to load fails the vault closed: everything is withheld until it is fixed or
  removed. Never open.
- **The list itself is not ours to make.** Lists of the worst material are kept by specialist
  bodies (the Internet Watch Foundation in the UK, NCMEC in the US) and supplied to vetted members
  under agreement; obtaining the material to hash it is itself a crime. Sieve ships the mechanism,
  with its format documented, so that a deployment with legitimate access to such a list can load
  it.
- **Test entries.** Sieve ships three harmless entries (as antivirus software ships the EICAR test
  file), so the mechanism can be seen working and checked by CI: the SHA-256 of the 16 bytes
  "sieve vault test", withheld everywhere, as a file and written out; and the PDQ hash of the
  **test picture**, 64 x 64 grey noise in 4 x 4 blocks whose shades are SHA-256 chained from "sieve
  vault test picture" (noise no photograph resembles, so it withholds nothing real).
  `sieve vault --test-picture t.png [--scale N]` writes it: it is withheld as it is, scaled,
  rotated, recompressed as a JPEG, blurred or brightened, warped onto an image line (rgb24 or
  rgb332), and base64'd. And the **test file**, 4,096 bytes SHA-256 chained from "sieve vault test
  file", listed by its 13 chunks only (not by its whole hash): `sieve vault --test-file t.bin`
  writes it, and any couple of kilobytes of it, between other bytes, located, or base64'd, is
  withheld.

## 5. Keeping it in

It cannot be made impossible to remove: the source is open, and anyone can delete the check and
rebuild. What can be done:

- It is built in and on in every official release, which CI checks (the test entry is withheld).
- The official releases are the ones published with their SHA-256 sums and maps, so a build
  without the vault is not an official Sieve.
- The licence question is separate. AGPLv3 lets recipients remove added restrictions, so a clause
  forbidding removal would mean moving from the standard AGPL to a custom licence (and Sieve would
  no longer be open source in the OSI sense). Not decided here; worth a lawyer's view before it is.

## 6. As built

- `tools/cli/vault.*` (in `sieve_locate`, so the installer has it too): loading, the built-in
  entries, fail-closed, and the checks. What is hashed: a file's bytes (and a unit of a line that
  holds every byte, which is a file); a melody's MIDI file; a model's canonical `.obj`; text as a
  file written out, through the decoders (`tools/cli/vault_decode.*`); pictures by PDQ
  (`tools/cli/pdq_hash.*` over `third_party/pdq`), both drawn and in files; and all bytes by their
  chunks (`core/src/chunks.cpp`, in the core since filters and maps can use it too).
- Files: every file walked or located (`walk_folder`, `manifest_of_file`, `sieve locate`), every
  file installed (`install_tree`, and each file the installer unpacks from a 7z), held anchors
  added to or read from maps.
- Units: warped input (`read_warp_input`: the command line and the hallway's T), every
  unit the command line prints or saves (`preview`, `read`, `browse`, `mesh`), every item the
  hallway builds (drawn blank, not in the excluded view, no picture, not hovered, taken, walked to,
  or saved; a book by its cover, title, each page, and its pages as one; an audio or video unit
  by its cover and title), and every file on the binary line when its bytes are worked out.
  `sieve vault --written FILE` checks a text file the same way.
- CI: the test entry refused as a file, in a map, by both installers, and written out (as it is,
  in hex, base64 and a-p nibbles); text that is not it ("sieve vault tests") not refused; PDQ
  agreeing bit for bit with Meta's `pdqhash` on a PNG; the test picture refused as it is, scaled,
  located, warped onto an image line and base64'd, while an ordinary picture is not; a piece of
  the test file refused as a file, located and base64'd; a vault file written by `--chunks`
  catching a piece of another file, and `--common` leaving shared chunks out; the chunk vectors
  (C++ against the oracle, and the oracle regenerated and compared); and a broken vault file
  failing it closed.
- Cost, measured with `--timings`: on a text line of 3,200 characters, the chunk check adds about
  0.007 ms per bytes checked (about 10 ms over a screen of 512 items); a unit under 64 bytes has
  no chunk to check. A located file is read once: its SHA-256 and its chunks come from the same
  blocks (`vault::hash_checked_file`); only a file whose first bytes say it is a picture is read
  again, whole, for PDQ.

## 7. Order of work, from here

1. Documentation for deployments loading a real list (and, for lists of millions, an index for
   the PDQ entries: PDQ's own multi-index hashing, in place of the scan through every entry).

## 8. Chunks beyond the vault

Content-defined chunking is not the vault's alone: it recognises known material inside anything,
at any offset, so it lives in the core. Uses to come: filters that mark units holding known
material (a corpus, the dictionaries' sources, a map's files), as a novelty or provenance mark;
maps and the locator recognising a new version of a mapped file, or a file that holds one, by
the chunks they share. A chunk filter cannot be counted exactly (compact mode needs a ranker), so
it would work in mark, hide and excluded modes. These come with the filter plugins.

**Chunk size as a parameter** (Edward's wish): the sizes (64 / about 256 / 1,024) are fixed in
`cdc-v1`. The plan is a family, each set of sizes its own pinned name (for example
`cdc-v1/32-128-512`), which a `chunks` entry and a chunk filter both name, so small pieces can be
caught where longer entries are worth it and big files cut coarsely where they are not. It comes
with the plugins, whose parameters raise the same question.
