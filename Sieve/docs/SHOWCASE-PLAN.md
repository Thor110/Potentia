# Showing what Sieve is good for: a plan

Written 27 October 2026, after Edward: "Help me present the project better through the readmes and
help me put together a plan to show off its utility." The READMEs were rewritten the same day; this
is what comes after.

The idea throughout: lead with what a stranger can use and check for themselves in five minutes,
measured on data they can get; link the vision rather than lead with it. Every claim made in public
should have a number behind it and a way to reproduce it.

## 1. Settle what is shown (first)

- **The format is frozen at `sieve-manifest-v31`** (Edward, 27 October 2026: "Yes, it is time I
  stopped pushing more manifest versions"). Nothing new goes into `.sieve` until a change is worth a
  version on its own; packer improvements that need no new format are still free to land. Others
  can only build on a format that holds still.
- **A short specification of `.sieve` v31 on its own**, apart from `SPECIFICATIONS.md`: enough for
  someone to write an independent decoder. The oracle (`reference/sieve_ref.py unpack`) is already a
  second decoder, which is worth saying. *Written 28 October 2026:* `docs/SIEVE-FORMAT.md`.
- **Tidy the repository's front:** the stray `Sieve/lt/` folder (a language-test leftover, committed
  on `d20b2bf`) removed and ignored. *Done 28 October 2026.*
- **A fresh release** whose notes lead with the three uses below, and the benchmark table.

## 2. The benchmark: the strongest single piece of evidence

`tools/bench/compare.py FOLDER --sieve PATH` runs Sieve and the usual compressors on one folder:
size, ratio, time to make, time to unpack, and a check that each gives the folder back. It uses
7-Zip's own program where it finds one (and zstd's and brotli's), else Python packages.
`tools/bench/ladder.py FILE ... --sieve PATH` cuts files to 1 KB, 4 KB, 16 KB and up and sets
Sieve beside 7z at each size, to show where Sieve starts to win for each kind of file.

**Corpora, in this order:**
1. **Sieve's own release folder** (Edward's machine, Windows, with the real 7-Zip): the 7.06:1
   against the others on the same folder.
2. **This repository's `Sieve` source tree** (done 27 October 2026, in `Sieve/README.md`), which
   anyone can check out and repeat.
3. **Public test data**, so the results sit beside published tables: the Silesia corpus (211 MB of
   mixed files) and enwik8 (the first 100 MB of English Wikipedia, used by the Large Text
   Compression Benchmark). *enwik8 done 28 October 2026* (Proof/STATISTICS.md: 21,675,870 bytes,
   7z 26,080,445).
4. **Folders people recognise:** a game's install folder, a program's folder, a photo folder (expect
   little: JPEGs are already compressed until step 3 of the plan in `HANDOFF.md`), a large source
   tree.

**Against:** 7-Zip at `-mx9`, xz `-9e`, zstd 19 and 22 with a long window, brotli 11, zip, and, for
honesty, a context-mixing compressor such as paq8px, Sieve's nearest relative (likely smaller still,
and far slower).

**Report** size, ratio, time to make and to unpack, peak memory, and the machine; and say once that
someone without Sieve also needs the decoder (`sieve-install`, 4.47 MB) the first time. Publish the
tables in `results/`, on the website, and where compression people read (below).

## 3. Extracting archives: the everyday hook

Edward's experience is that Sieve extracts `.iso` files where 7-Zip often fails. Before saying so in
public:
- **Gather a test set** of archives, each one public, named by where to download it and its
  SHA-256, on which 7-Zip (version named) fails and Sieve does not. Linux distribution images and
  old Windows images are the likely places to look.
- **Say what does the work:** Sieve reads ISO, RAR, cab and the others through libarchive; credit it.
- **Record it:** a short screen recording of right-click, Extract, done; and the same file in 7-Zip.

## 4. Demonstrations, each under two minutes

1. **Smaller, and exact:** a folder packed by Sieve and by 7-Zip, sizes side by side; installed back,
   and its files' SHA-256 shown equal to the originals.
2. **Right-click extract** of an archive from the test set above.
3. **A damaged download changes nothing:** one byte of a `.sieve` changed; installing refuses, and
   the folder is untouched.
4. **A web page that proves itself:** Sieve Secure, the tampered picture refused, a changed page
   stopped.
5. **Training anyone can repeat:** `sieve ai-train tests/training_run_v1` on Windows and on Linux,
   the two `result.txt` hashes side by side, identical. The one demonstration nobody else has.
6. **The Gallery:** thirty seconds of the hallway; a sentence warped to its address and found on its
   shelf.

**A tutorial video, the features in one sitting (Edward planned one, 28 October 2026).** A running
order that leads with what a viewer can do, each part short enough to cut out as a clip of its own:
1. *What it is, in one sentence,* over the release page: packs, checks and unpacks files exactly.
2. *Installing Sieve* from `sieve.exe`, and the right-click menu (`install_context.bat`).
3. *Right-click extract* of an `.iso` and a `.rar`.
4. *Packing a folder* in the File Locator: the estimate, the graph of the best so far, Stop; then
   the saved `.sieve` beside a 7z of the same folder, sizes on screen.
5. *Installing it back*, then one byte of the `.sieve` changed and the install refused, the folder
   untouched.
6. *Sieve Secure:* the test site with the extension, the tampered picture refused.
7. *Training repeated:* `sieve ai-train tests/training_run_v1` and its result's hash.
8. *The hallway* last: a sentence warped to its address and found on its shelf, with §0's point
   said once (an address is as long as what it names; the filters and the guided ordering are what
   shorten it).
Say where it is slow (making a `.sieve`) and where it gains little (JPEGs, video), as the posts
will (§6).

## 5. Who to show, and where

| Who | What to show them | Where |
| :--- | :--- | :--- |
| People who care about compression | The benchmark tables, the format specification | encode.su (the compression forum), r/compression |
| Everyday Windows users | Right-click extract; smaller archives | A short video; the release page |
| ML reproducibility and alignment researchers | Bit-exact, replayable training | A short write-up with the commands; the Alignment Forum, ML reproducibility groups |
| Web integrity and security people | Sieve Secure, beside Subresource Integrity | The web README, a short post |
| Developers generally | One narrow claim with its number | "Show HN" |

One audience at a time, each with the one thing that matters to it. The Gallery and the vision are
for those who stay.

## 6. What to have ready before any post

- **One paragraph:** the claim, its number, and the commands to reproduce it.
- **Answers to the first questions:**
  - *Why not zstd or 7-Zip?* Size: the table shows how much smaller, and on what.
  - *Why is it slower?* It tries several ways per file and mixes models.
  - *Is it lossless?* Yes: every file is checked against its SHA-256 before anything is written.
  - *Which systems?* A Windows release; Linux and macOS from source.
  - *What licence?* AGPL v3.
- **Where it loses, said first:** time to make, files already compressed (JPEG, video, most games'
  data), and the Windows-only release.
- **A licence question for Edward:** AGPL keeps many companies from embedding a library. A more
  permissive licence for the decoder and the format specification alone (the encoder staying AGPL)
  would let others read `.sieve` freely. Not a recommendation to change anything, only the question
  adopters will ask.

**Where it stands (28 October 2026).** Edward has joined encode.su, and will post nothing there or
elsewhere until enwik9 and Silesia are measured too, beside enwik8 (Proof/STATISTICS.md). Before
posting there, the
Silesia and enwik8 tables (§2) are what that forum will ask for first, beside paq8px and cmix, and
every public page it may follow a link to should agree with §0 of IDEAS (an address is as long as
what it names), since that is the first thing a reader there will test.

## 7. Order

1. The format frozen; `lt/` removed; the `.sieve` v31 specification written (all three done).
2. The benchmark on the release folder and on Silesia and enwik8 (Windows, with the real 7-Zip).
3. The ISO test set, and the recordings of the demonstrations.
4. The training write-up, run on two machines.
5. A release with all of it, then the posts, one audience at a time.
