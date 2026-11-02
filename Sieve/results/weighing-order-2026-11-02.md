# The order the packer weighs things in: a first review (2 November 2026)

Edward: "it might be worth doing a review of the order weighing is done in soon anyway." This is
the first pass: the order as it stands, what was measured, what changed, and what to measure next.
Times are this session's container (4 logical processors) unless they are Edward's (12).

## The order, as `add_packed` (tools/cli/pack.cpp) runs it

1. Every file's bytes read.
2. Archives opened or made again (v14, v31), JPEGs carried as their brunsli form, PNGs re-saved or
   made again from their skeletons.
3. **A repeating byte or pattern** (`pattern_runs`; new today, first so that nothing after it
   looks over such a file for nothing).
4. Each file's way: a program to the x86 stream, an item of Sieve's lines, a text made from
   another's lines (a mask).
5. Files alike, over 8 MiB: the delta matcher's fingerprints, and deltas.
6. Copies (files with the same SHA-256), and the files each searched: the rope's search, the
   pieces, LZMA2's estimate, mixing (alone).
7. Mixing in groups; the files mixed weighed again against the stream they would share; the files
   mixed one by one mixed together; the lzma2 stream mixed.
8. The streams packed by LZMA2, its settings tried on the first MiB and the whole packed again at
   the settings chosen (up to four whole passes at once).

## Measured

**Where enwik9's time went (Edward's bench, decode/no, 4,759 s, LZMA2 taken):** the rope's search
3,123 s (66%), LZMA2's tuned passes about 1,600 s, the pieces 20 s. The rope did not carry it.

**The rope on text.** On 27 of the repository's texts (prose, code, the documentation, two books),
the rope's best after its whole search was 1.77 to 2.77 times LZMA2's. It wins on texts that are
tables (Edward's `pi6.txt`, 421,306 against 505,978; IDEAS 19.87), and here:

| File | Rope's whole search / LZMA2 | Its line wave on the first 256 KiB / LZMA2 there |
| :--- | ---: | ---: |
| a table of random digits, 504 KB | 0.92 | 0.92 |
| a CSV of numbers and words, 426 KB | 0.84 | 0.84 |
| a word list (scowl-en-60), 600 KB | 0.92 | 0.98 |
| Don Quixote, 600 KB | 1.81 | 1.74 |
| pack.cpp | 2.57 | 2.54 |
| MANUAL.md | 1.77 | 1.77 |

The bytes' order-0 entropy against LZMA2 does not tell them apart (the CSV 0.55, the word list
0.47, Don Quixote 0.56); the line wave on the start does (IDEAS 19.99: on text the rope's best is
always its line wave, `C2 G1 A1`).

**Built:** a text (95% printable) of 64 KiB or more has its first 256 KiB coded by the line wave
alone; where that comes to 1.25 times LZMA2's estimate or more, the rope is not searched and the
file is not cut into pieces (LZMA2, and mixing after it, as before). The same bytes either way:

| Folder | Before | After |
| :--- | ---: | ---: |
| 12.9 MB of the repository's texts as one file, for decoding | 97.7 s | 43.2 s |
| 27 texts (4.9 MB), for size | 69.3 s processor | 56.6 s processor |

Both made 2,226,048 and 1,091,589 bytes before and after.

**Built earlier today:** the repeating byte or pattern looked for first (above), and the weighing
against the shared stream not estimating the stream with the mixed files where their codes are too
few to come back (one 51 MB estimate saved on mozilla's tar with two small texts).

## What is left, each with what would show whether to change it

- **LZMA2's passes.** On the 12.9 MB text, once the rope was not searched, its passes were 36 s of
  46 s of processor time: the preset's pass, then the settings chosen on the first MiB at the
  preset's match length and at 64, and 7-Zip's encoder, each over the whole. Each pass's size
  (`SIEVE_PASS_SIZES=1`, a testing hook, new):

  | Stream | Preset | Chosen settings | Match length 64 | 7-Zip's encoder |
  | :--- | ---: | ---: | ---: | ---: |
  | 12.9 MB of the repository's texts | 2,232,131 | **2,225,984** | 2,228,201 | 2,228,675 |
  | 16 programs' x86 stream, 47.0 MB | 13,387,695 | 13,373,910 | 13,371,380 | **13,339,919** |
  | mozilla's Alpha libraries, 15.6 MB | 3,218,971 | 3,163,775 | 3,161,286 | **3,144,978** |
  | mozilla's tar whole, 51.2 MB | **13,376,180** | 13,701,908 | 13,699,344 | 13,658,793 |

  Each wins somewhere: the chosen settings on text (0.28% under the preset), 7-Zip's encoder on
  programs (0.36% and 2.3%), the preset's own on mozilla whole, where the settings its first MiB
  chose (lc0 lp0 pb0) came out 2.4% larger. The match length of 64 won none of the four. So the
  passes cannot be cut to one by these four; the 64 pass is the one to watch (a quarter of the
  whole passes' time): to measure on Edward's folders and enwik8 before it goes.
- **Copies and the delta matcher (looked at; nothing to change).** A file with the same bytes as
  another never reaches the delta matcher: the masks before it (step 4, a text made from another's
  lines) find every line of the one in the other and carry it as a mask of them, for almost nothing,
  and even random bytes have line feeds (two 10 MiB files of random bytes and a near copy: the copy
  carried as "lines 1", the near copy as a delta of 1,108 bytes; the whole 10,487,574 bytes). Finding
  copies by their SHA-256 first was built, tried and taken out: it found nothing the masks had not.
- **Mixing's four weighings** (alone, in groups, against the stream, the stream mixed). Measured on
  mozilla taken apart (525 files, 51.2 MB; 452 s here; processor seconds, all threads):

  | Step | Time | What it did |
  | :--- | ---: | :--- |
  | each file's search: the rope | 540 s | 391 files searched |
  | each file's search: mixing alone | 144 s | 75 files and 7 pictures mixed |
  | mixing in groups | 75 s | 7 groups made; 1 kept (group 5, 1,816,552 against 1,842,272 apart) |
  | weighed against the stream | 94 s | the 75 files (and the group) put back in the stream: 12,975,391 against 13,072,264 |
  | the stream mixed | (none) | |

  Of about 313 s spent mixing, all was undone but the 7 pictures (397,167 bytes in the rope
  stream); the lzma2 stream took the rest, 12,968,859. Two orders to try: weigh the files mixed
  alone against the stream before the groups are made (here it would have spared the 75 s of
  groups); or, where most of a folder's files are programs that share code (mozilla's are), weigh
  the stream first. Either changes what is decided where groups do win (Sieve's docs/: 5,740,563
  bytes alone against 4,884,492 in groups of 8 MiB, 27 October), so both are to be measured on the
  docs and on mozilla before one is taken.

  Measured (`SIEVE_MIX_ORDER=stream-first`, a testing setting: the files mixed alone weighed
  against the stream before the groups, and no groups made where the stream takes them back):

  | Folder | Default (bytes, s) | Stream first (bytes, s) |
  | :--- | :--- | :--- |
  | Sieve's docs/, 2.6 MB | 1,074,255, 34.0 | 1,074,255, 34.6 |
  | Sieve's tools/cli and core/src, 2.5 MB | 378,949, 18.3 | 378,908, 23.7 |
  | the public-domain anchors, 3.0 MB | 999,994, 27.7 | 999,994, 29.6 |
  | 16 programs, 60 MB | 16,485,171, 260.3 | 16,485,171, 239.2 |
  | mozilla taken apart, 51 MB | 13,371,335, 407.7 | 13,371,335, 429.0 |

  The same bytes on four, 41 fewer on the sources; quicker on the programs, slower on the rest
  (where the stream did not take the files back, the weighing runs twice). Neither order wins: the
  default stays, and the setting stays for measuring Edward's folders. (Beside it: on the docs the
  group's word list, chosen on the group's start, came out 202 bytes larger than without it on the
  whole, 1,073,679 against 1,073,477: 0.02%, noted, not acted on.)
- **The JPEG step and Optimise Search's rule (looked at; nothing to change).** The JPEG step runs
  only with Compressed formats ticked: a JPEG becomes its brunsli form (15.25% to 21.91% smaller on
  ten originals) and is left out of the search, its bytes being coded already, as any file carried
  by such a recipe is. Unticked, a JPEG is carried as it is and searched, and Optimise Search's rule
  for it (mixed whole, 2.7% smaller, the median of Edward's 153, measured with the box unticked)
  is the one that applies. The two do not meet.
- **The per-file estimate and the stream (built).** Where a run has one file, carried as it is, its
  LZMA2 figure for weighing its ways is now made as the stream's own first pass (preset 9e, the
  stream's dictionary) and kept, and the stream's first pass takes it rather than packing the same
  bytes again (on Edward's enwik9 the estimate took 617 s and the first pass 658 s, of the same
  bytes). The same bytes either way:

  | One file | Before | After |
  | :--- | ---: | ---: |
  | 12.9 MB of text, for decoding | 44.9 s | 29.4 s |
  | mozilla's tar, 51 MB, for decoding | 433.0 s | 356.1 s |
  | the same text, for size (mixed: the kept pass not used) | 67.0 s | 69.5 s |

  Where the file is not carried as it is (mixed, here), the 9e pass costs a little more than the
  preset 6 estimate did. The LZMA2 figure the ways are weighed against is now the pass itself, a
  little smaller than the estimate, so a near tie goes to LZMA2 where it went to mixing before.

## The hashes, and the delta check's place (2 November 2026, evening)

Edward: "As we already hash the files once for the format, shouldn't we just be reusing that work
already done and not hashing them again? ... Sounds like the delta check should be the first step
in the weighing process."

**The hashes.** The walk hashes each file once, with the vault's checks in the same read. The packer
hashed each file again in three places, and now takes the walk's hash in each where the bytes are
still the file's own:
- the archive step, where every file not opened or remade is listed again (the walk's hash kept;
  a member of an archive, or a file made again, is still hashed, its bytes being new);
- the JPEG step's copies (the walk's hash: a JPEG there is not remade);
- the search's copies and mixing's notes (the walk's hash, where the file is not remade).

One check stays, by design: the packer reads every file again and checks it against the walk's
hash, so a file changed between the two is refused rather than packed under the wrong hash. It ran
one file at a time; it now runs on every core.

**The walk itself was the larger cost.** It read and checked one file at a time; the vault's
picture check alone was 3.3 s of 7.8 s on 100 copies of a 1.2 MB JPEG. It now lists the folder
first, then hashes and checks the files on every core. Where a file cannot be read or is withheld,
the first such in the listing's order is still the one reported. The listings are the same bytes:

| Folder (the walk alone, `locate --manifest`, 4 processors) | Before | After |
| :--- | ---: | ---: |
| 100 copies of a 1.2 MB JPEG, 114 MB | 7.56 s | 2.10 s |
| four files of random bytes, 200 MB | 2.68 s | 0.88 s |
| Sieve's docs/, 2.6 MB | 0.55 s | 0.19 s |

Packed, the JPEG copies and the PNG copies came to the same bytes before and after, with and
without the v33 draft (978,332 and 963,838; 1,052,540 and 1,052,127).

**Seen on the way: docs/ is not packed the same twice.** The same build gave 594,466 bytes on two
runs and 598,620 on two others (the difference in the group of 22 texts mixed together: apart
438,831 against 458,440). The rope's search stops at its time limit, which is the likely cause;
not yet confirmed. Comparisons of packers here use folders whose results do not move.

**The delta check.** Timed on the 160 MB pair of maps (one 160,068,077 bytes, carried as its
differences from the other, 160,000,000; 1,518 s in all, `--timings`):

| Step | Time |
| :--- | ---: |
| reading again, the vault's chunks | 2.9 s, 3.0 s |
| the delta check (fingerprints, the script) | 1.8 s |
| the rope's search on the base | 768 s |
| the rope's code | 135 s |
| LZMA2's passes (first, tuned, 7-Zip's; the stream and the estimates) | about 800 s |

The delta check already runs before every step that costs: what comes before it took seconds. It
cannot go before the archive step (an archive's members are not files until it is opened), and
exact copies are found first already (`copy K`, v33 draft). So its place stays. What the pair
shows instead is the base's cost: 768 s of rope search on a 160 MB file the rope did not carry.

**The review's first pass is done.** Built from it: the pattern check first, the rope not searched
on texts it cannot win, the weighing's second estimate skipped where it cannot change anything, and
one file's estimate kept as its stream's first pass. Measured and left: LZMA2's passes (each wins
somewhere; the 64 pass to watch), mixing's order (`SIEVE_MIX_ORDER`), copies (the masks find them),
the JPEG step.

After this: the lines' filters read for what they set aside that a compressor could name in a few
bytes (Edward: "cross referencing filter designs for use in compression"); not-a-pattern-v1's
repeats and ramps are the first so taken (`runs`, v33 draft).
