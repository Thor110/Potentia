# Ideas implemented

Entries of [IDEAS.md](../IDEAS.md) that are done: built, answered or superseded, with nothing left open. They are moved here whole, as they were written, so IDEAS.md holds what is still to do while the record of why each thing was built is kept. IDEAS.md keeps a one-line pointer at each old number, so every "IDEAS §N" reference elsewhere still leads here.

Edward, 12 November 2026: "move ones we have worked through across into a document that isn't front and centre all the time … a work done and an ideas implemented plus deprecated ideas document or set of documents, everything is a range and a lens and everything fits on a chart across time."

How an entry is chosen: every idea in it is marked *done* in [IDEAS-TRIAGE.md](../IDEAS-TRIAGE.md), none only partly built, and its text names nothing still to do. Entries done in part stay in IDEAS.md until the rest is. The first move, 12 November 2026: 39 entries.

The section headings are IDEAS.md's own.

### 2. Defects and corrections

**2.1 Entropy is measured against the wrong space.** The filters score a unit against a fixed
symbol count, but the alphabet is user-chosen and runs from 27 symbols to the whole of Unicode. A
page drawn from `lower27` judged against a 21-bit code point space gives a meaningless reading.
Every alphabet needs its own entropy baseline. A correctness bug, not a feature.
- *Fixed, 24 October 2026 (step 3 of the plan):* `symbol-entropy-v2` takes its bounds in
  thousandths of the most a unit of the line can have, log2 min(symbols, length), so one setting
  means the same on every alphabet. Measured on Don Quixote (`pg996.txt`) in lower27 pages of
  1,000: English 826 to 877 thousandths (median 851), random letters 992 to 996; pages of 100,
  700 to 885 and 920 to 960. The default top, 925, lies between them at both lengths. v1 stays,
  retired as before, and says it is replaced by v2.

**2.2 Padding is counted as content.** `model-information` and `symbol-entropy` count trailing
padding spaces as real characters, so a padded page scores as *more* text-like than its text alone.
- *Fixed, 24 October 2026:* `symbol-entropy-v2` and `model-information-v2` judge a unit that ends
  in two or more SPACEs on what comes before them, as `words-v2` does; a unit of padding alone
  fails. A page of six letters of noise padded to 32 with SPACEs passed `model-information-v1`
  and fails v2 (unit tests). Both are in the oracle, with 288 verdict vectors and 24 ranks (v2 in
  black and white).

**2.3 The cased dictionary lets short gibberish through.** Folding one cased list to lower case
also folds SCOWL's abbreviations and chemical symbols, so "zn" and "tb" now pass. Fix: a version
that keeps proper names and drops short all-caps tokens.
- *Fixed, 24 October 2026:* SCOWL writes the symbols in mixed case ("Zn", "Tb", "pH"), so dropping
  all-caps tokens alone would miss them. The `-names-v2` lists (`scowl-en-35-names-v2`, `-60-`,
  `-80-`) leave out, from each pinned `-names` file, every entry of two or more letters written all
  in capitals (OK, IBM, NASCAR: 45 in the 60 list) and every entry of one or two letters whose
  lower case is not in the plain list of its size (Zn, Tb, Hz, pH, Ed, Jo: 59), but "a" and "I";
  names of three letters or more stay. Made by `tools/build_dictionary.py --names-v2`, registered
  beside the old lists, which are unchanged. With scowl-en-60-names-v2, `sieve sift` counts 28
  times fewer pages of 64 letters that are all words (10^-38.49 of the space against 10^-37.03),
  and at 2 letters the plain list's 73 (134 before). In Don Quixote, no whole word was lost (only
  pieces of accented words split by the counting: "Señor" as "se"). Made the default the same
  day (Edward: "We should probably just make a function-words-v2 so as to maintain the backwards
  compatibility we have built in so far"): `scowl-en-60-names` stays registered, so a stack made
  with it reproduces by naming it, and `function-words-v1` keeps its own copy of that list;
  `function-words-v2` is the same rules from the new list, so it implies `words-v2` with the
  default again. Measured: 10^-19.08 of the pages of 32 letters against v1's 10^-18.33, and the
  same verdict on each of the 100 held-out sentences and their 100 shuffles.

### 4. Manifests, installs and anchors

**4.1 A directory walker that emits a manifest.** Point the CLI at a folder; it walks the tree,
records the structure, filenames and sizes, and gets each file's key. Then it takes the key of the
manifest itself. Buildable now, useful now, and it is the honest core of the genesis-key idea:
one canonical, self-verifying description of a tree. Add total size to the manifest so an install
can refuse before it starts.
*Built:* `sieve locate` (§4.9, SPECIFICATIONS §12.2); every install checks the tree's SHA-256 before
it writes anything.

**4.5 Real file types out of the CLI.** Already partly there: `read --out` writes `.png` for image
and video, `.mid` for audio, `.txt` for text, and `mesh --out` writes `.obj`. Worth finishing —
`.wav`, and a frame sequence rather than frames side by side — so any address can be handed to any
other program.
*Since:* `.wav` is written for sound (`pcm`), and a video's frames go to any format ffmpeg writes
(`sieve help`).

### 5. The hallway's shape

**5.3 Signs above the portals.** Neon-style lettering in the destination line's colour above each
doorway. Small, and it fixes real navigation confusion.

**5.5 Accelerating scroll.** The wheel already moves by tiles; give it an acceleration curve so a
fast spin compresses distance, which is the only way browsing stays possible once the fork count is
unbounded.
*Built (6 November 2026):* a notch within 60 ms of the one before, the same way, adds to a run;
every two notches of a run double how far a notch goes, up to 1,024 tiles; a pause or a turn back
starts again at one tile a notch (client/hallway.cpp).

**5.8a The node graph, first version: built** (27 September 2026, SPECIFICATIONS §12.3). Maps
of verified anchors, one at a time, in 3D, and the item page's SORT tab. Edward's notes on where
it goes next: maps shipped with a release or offered as optional downloads ("if you want map X,
you get it"); relations beyond `contains` (versions, patches); nodes for items on the other lines;
and anchors feeding the filters, so verified real content helps build heuristics that find
meaningful content. A graph of the whole state space was considered and left out: it would be a
tangle of noise, at the cost of a new limit in the setup menu. It can be added later if wanted.
On directions like "anchor X, 1,000 rooms right, item 128": they cost as many bits as the
distance, so they only reach near an anchor (in positional order, files that differ from it only
in their last few bytes). Edges that are patches between versions are where a map saves real
data.

**5.11a The address navigator.** *Built (X in the hallway, `client/navigator.cpp`).* Walking and
the jump keys only ever reach the right-hand end of an address. The navigator shows the whole of
it, a hex digit at a time, full screen and scrolling; any digit can be chosen with the keys or the
pointer and turned with Up/Down, the wheel or its arrows, carrying as counting does and wrapping
round the line. Possible next steps: a preview of the item at the address being turned, and a
decimal view beside the hex. *Decimal built (6 November 2026):* the place in decimal under the
bearing, with its number of digits, its first and last digits where it is wider than the screen;
worked out when the address changes, for addresses of up to 4,096 hex digits.

### 6. Limits and settings

**6.1–6.4 Limits and reset.** *Built.* `FIND MY LIMITS` in the setup menu sets every line to the
largest shape this machine can open; `RESET EVERY SHAPE` puts them all back. Two things bound a
shape and both are checked: the length of one address, which is a single number held in memory and
whose arithmetic grows with the square of its length (a 256th of installed memory), and the length
of one *unit*, because the hallway caches a few thousand whole units and that is usually what runs
a machine out of memory first (a quarter of installed memory across four thousand of them). Both
are closed-form from the shape, so the search generates nothing. A red line names any line that is
over, rather than a general refusal. Pages and image grow in step, because the books line is made
of both and has to fit too.

Neither number is a limit of the design. They describe the machine of the day, and a bigger one
finds bigger numbers with the same arithmetic.

**6.5 Global parameters, and entries per shelf.** *Built.* Items per wall is a setting (a power of
two from 2 to 4096, default 128), under GLOBAL in the setup menu. It turned out not to need
versioning: it changes no address, only the (tile, slot) coordinate an address is shown at, so
"tile N" is a display coordinate tied to the setting, and the address is what gets written down.

### 7. Features from earlier in the conversation, still standing

**7.5 Punctuation filters for `babel29` and `ascii95`.** *Built: `sentence-shape-v1` and
`babel-punctuation-v1` (FILTER-PLUGINS §19).* Punctuation follows a word and is followed
by a space; for `ascii95`, a capital after a full stop. Countable with the word trie that already
exists. This matters: `lower27` throws away case and punctuation, so the books line currently holds
word streams, not sentences.

**7.12 Reels — video as a composition, not a line.** *Built as the movies line (§13): a cover, a
title and N units of video, `composition-v1`; the video line stays, as the units.* F frames of W×H is the same set of
possibilities as F image units in a row, so the video line adds bundling and no content. A reel is
frame rate, image shape, a list of frame addresses, audio shape, a list of audio addresses. A still
scene stores one address and a count; filters work per frame on the image line; and one mechanism
covers book, album, comic and reel so no further category is ever needed. The honest cost:
frame-to-frame coherence moves up a level — a given reel can be judged, but every possible reel
cannot be counted, because exact temporal counting needs the whole previous frame as state.

### 16. Edward's list of 10 October 2026, evaluated

**16.2 The File Locator from the main menu.** Show "Go to it" only when the locator is open in
the world, and add "File Locator" to the main menu, under Settings and above the Filter Designer.
*Evaluation.* Straightforward. From the main menu there is no hallway to walk in, so Go to it is
hidden there and everything else (choose, weigh, tailor, save, install) works as it does now. One
point to decide: "Use the tailored filters" from the main menu would save the filters for the next
hallway built rather than build one at once. Verdict: build. *Built (10 October 2026):* the
locator is its own class, opened from either place; from the main menu, Use the tailored filters
saves them for the next hallway.

**16.4 A file type filter on the binary line.** Filter the binary dimension by file types, read
from their headers.
*Evaluation.* Built: `binary-kind-v1` keeps (or excludes) the files of chosen kinds, read from their
own first bytes by the `file-kinds-v1` table (PNG, ZIP, MID, EXE, ...), counted and ranked exactly,
so the line compacts under it. Its `kinds` setting takes one kind or a group (signed, text,
signed-or-text, unknown, empty, any). What it lacks is a **set** of kinds (PNG and JPG together,
say). Verdict: extend the setting to a list.
*Built (7 November 2026):* `kinds` also takes several kinds apart by commas (`png,jpg`), the files
of any of them; an empty or unknown part is refused. The manifest is unchanged: the setting was
already text.

**16.5 "Calculating Dimensions" where it is seen.** Move it to the top middle of the setup menu, in
red, and grey out ENTER THE HALLWAY while it is calculating.
*Evaluation.* Straightforward: the status line under the budget bars moves and turns red, and ENTER
is shown greyed (and refuses) while any line is being counted, as Go to it is greyed in the
locator when it cannot go. Verdict: build. *Built (10 October 2026):* "Calculating
Dimensions..." in red, top middle, always the same words, size and place; which lines are still being
counted stays under the bars.

**16.6 A slide-in window for a line's settings,** to free the background for new dimensions.
*Evaluation.* The setup menu shows every setting at once (the budget model needs to see them
change together). A slide-in panel, one dimension's settings at a time, frees the screen for more
dimensions and suits a growing list. Verdict: build when the dimensions outgrow the screen, which
worlds and an AI dimension would bring about. *Built (10 October 2026), ahead of those dimensions, as Edward
asked:* the whole list, GLOBAL down to ENTER THE HALLWAY, slides in from the left over where it
stood (Tab, or the SETTINGS tab at the left edge); while it is out the map takes the width; and
while the dimensions are being calculated it slides out by itself so the bars can be watched,
coming back when that is done. *Then (10 October 2026):* the settings above the three actions
scroll, so more of them, and more dimensions, fit.

**16.9 A demonstration: the first item at every degree of every dimension, as one `.sieve`.** For
each dimension and each whole degree from 0 to 359, the item a bearing of that many degrees lands on
(the unit at `floor(N * d / 360)` of the line's `N`), saved as the file F saves, all in one folder
and made into Sieve instructions (v4). Even if the files are noise, it proves the system end to end:
the installer installs them, each checked against its SHA-256, and each one, opened with J or reached
by its bearing (X), is that item again.
*Evaluation.* Straightforward with what exists (bearings, F's files, `--installer`), and a good CI
check besides: the bearings and the files agree on every machine. The files are about the size of the
lines' units (a page 32 characters, a picture a small PNG, a file on the binary line up to its
length), so the instructions are small. Verdict: build, as `sieve sample --degrees` or a script.
*Built (10 October 2026)* in the hallway, so every dimension's file is the hallway's own:
`hallway --sample-degrees DIR`. The bearing names the first unit at or past it (the navigator's
rule). Each item is a file named by its title (Edward: the reconstructed item is a file with its
title as its name; the pictures are for looks), pictures one pixel a pixel, as J reads them. At the
default shapes: 3,600 items of ten dimensions (worlds included), 1,055,836 bytes, as a `.sieve` of
436,788 bytes. CI makes it twice and
installs it, and checks a bearing typed in the navigator against it. What it showed: at whole
degrees the units repeat (every 40 or 72 degrees); on the lines without a cover the titles tell the
items apart, and on those with one (books, audio, tracks, video, movies, worlds) the cover leads
the address, so the titles repeat as well (45 pairs a line) and " (2)" tells the files apart. Each file
is named, under its line, by three values (the line with its stack, its ordering, its number),
which is what the next manifest and map schema should carry instead of its bytes (§16.3).

### 18. Edward's ideas of 11 October 2026

**18.2 A language filter, or layers, ordering models outwards (Edward, 11 October 2026).** His
words: we will need a language filter or a layering system, to order models outwards into the
infinite expanse, since the program could in theory be cranked up high enough to match real data.

*Evaluation.* The AI line grows as fast as its shape does (Micro is 2^32768 models; 8 layers of
width 1024, 2^537 million), and almost every model babbles, so something has to say which come
first. Three ways, the first two buildable now:
- **A language filter, exactly.** What a model says is floating point, but `training-v1`'s forward
  pass is integer arithmetic, every rounding pinned (SPECIFICATIONS §12.0e). So a model's score on
  a pinned text (bits a byte, as the harness measures) is exact on every machine, and a filter can
  keep the models that predict that text better than a threshold: a soft filter, versioned with
  its text's SHA-256. It judges only: counting the models below a score is as hard as the line is
  large. On the shelves it would mark or hide the babblers, and it gives the layers below a
  measure.
- **Layers as shells around anchors.** A trained model is an anchor: a model that means
  something, on the record (its run says how it came to be). Not a shorter name: the shipped run
  is 14.5 KB, its model's address 4 KB of digits; its worth is that it marks where language is on
  the line. The models a few digits away from it are its neighbours, and they mostly say nearly what it says (positional neighbours already do). Layer
  `k` is every model `k` digit changes from the anchor: `C(W, k) (2^B - 1)^k` of them, counted and
  ranked exactly, so "the anchor, then layer 1, then layer 2" is an ordering outwards with
  addresses, as near-anchor filters were planned for the other lines. Several anchors (several
  runs) give several centres, and a model is named by its nearest anchor and its layer: the more
  anchors, the more of the expanse lies near one, which is the agreed-search-vectors argument
  (§16) on this line.
- **Matching real data, cranked up.** At real sizes the anchors are what matter: a model the size
  of a published one stands on the line at its weights' address, and its run (its recipe, its
  data's manifest and its order) is how anyone checks it came from that data. Getting there needs the line to
  grow (aispace-v2, real tokens, §15), training to keep its speed, and the result to keep only the
  model's hash past a size (§15's "the result's size").
- **Verdict:** build the language filter on `training-v1`'s forward pass (exact, judged), and the
  layers as shells around anchors (exact, counted and ranked), the harness's runs being the
  anchors; the second is the one that orders the expanse outwards.
- *Built (11 October 2026), SPECIFICATIONS §12.0f:* `language-v1` and `near-model-v1`, each naming
  a training run; the top bar names the language. At Micro, two digits round the shipped run's
  model is 7,548,948,481 models, each named in nine hex digits, and walking them the shelves are
  full of models writing German-looking words. Ordering outwards is the `changes` setting: each
  step up adds the next layer.

### 19. Edward's ideas of 12 October 2026

**19.3 How a published multimodal model sees a picture.** Edward asked how Claude perceives a
picture sent to it. As far as is published for models of its kind (§18.5): the picture is cut into
patches, a vision encoder turns each patch into a vector, and those vectors join the text the model
reads. It is not given the exact pixels one by one, so small details (a single cell of a 10 by 10
cover, seen at an angle on a shelf) can be misread. A byte model on the AI line is the opposite
case: it reads every pixel exactly, as a byte, but with no sense of the row above. That is why
`figure-v1` decides on the cells themselves.

**19.10 Testing in fewer, larger rounds (Edward, 12 October 2026).** Edward builds both
configurations and runs the whole suite (`out\build\x64-Release\sieve_tests.exe tests`) for every
package. To spend less of his time: packages grouped by category (a round of filters, a round of
the AI line, a round of the hallway) and delivered less often; the full suite run here before each;
and a way for the suite to run one area at a time (a test name or category on its command line), so
a feature can be checked on its own while the full run is kept for the end of a round.
*Built (6 November 2026):* `sieve_tests tests --only NAME[,NAME...]` runs only the tests whose name
holds one of them, and `--list` names the 80; `--only mix4,rope_wave` is 242 checks in about 4 s here.

**19.12 A metrics list, made with every release (Edward, 12 October 2026).** Edward wants an
automated list, made each build and issued as a text file with the installer, of how much each
line's filters remove, one line a dimension:

    Binary  : 99.9954%                ( Kept : 10^-4.34    )
    Images  : 99.5%                   ( Kept : 10^-2.32    )
    Pages   : 99.999999999999...%     ( Kept : 10^-18.32   )
    ...
    AI      : 99.999999999999...%     ( Kept : 10^-9854.27 )

*Design.*
- **What exists:** the setup menu already works each line's figure out (`Menu::stack_job`,
  `worlds_stack_info`, `parts_stack_info` and `filtered_text` in client/menu.cpp: the same
  "99.5%  (kept 10^-2.32)" text), and `tools/survey` counts filters over a grid of settings. Neither
  writes this list, and the release does not run either.
- **One tally, shared:** the menu's per-line tally moves out of the client into the shared library
  (tools/cli, beside build_stack), as `line_tally(line, settings, shapes)` giving the kept and
  removed shares, the survivors' log, and whether it is exact, estimated or not countable. The
  menu and the new command both call it, so the list and the menu can never disagree.
- **`sieve metrics [--filters PATH] [line options] [--out FILE] [--tsv FILE]`:** every line, in
  door order, at its shape (the defaults, or the options given), under the stack the settings file
  ticks. It prints the list as above (the percentages as the menu writes them, estimates marked
  `~`, lines whose stack cannot be counted saying why), and with `--tsv` the same as a table
  (line, shape, stack id, kept log10, exact or estimated) for keeping a history.
- **Which stack a release measures:** nothing is ticked by default, so a release needs a stack of
  its own. Proposed: a reference settings file shipped and versioned in data/
  (`data/metrics/reference-filters.ini`), its SHA-256 printed at the top of the list, so the
  figures from one release to the next are comparable and anyone can reproduce them. Your own
  settings give your own list (`sieve metrics --filters sieve-filters.ini`).
- **In the release:** make_release.py runs the release's own `sieve metrics` on the reference file
  and writes `metrics.txt` beside the installer, listed in SHA256SUMS.txt and in the notes draft.
  Not a default build step: counting every line takes a while (the AI line and the books' parts
  longest), so a build stays fast; a CMake target `metrics` runs it on demand.
- **Checked:** a unit test that the list's figures for a small stack equal the menu's tally and
  the counts the oracle gives.
- *Built (12 October 2026), as `hallway --metrics`:* rather than moving the menu's tally into the
  tool, the hallway counts the list itself, with no window, through the setup menu's own tallies
  (`Menu::metrics_text`), so the list and the menu are one code and cannot differ. Edward chose a
  reference file; it ticks each line's hard filters that count together, so the release's figures
  say what Sieve removes before any taste is applied (your own file gives your own list). The whole
  list takes about two seconds. make_release writes `metrics.txt` (in SHA256SUMS.txt and the
  notes); CI checks the binary and models lines' figures. A `sieve metrics` in the tool waits on the
  tally moving into the shared library.

**19.18 Every menu reaches every system, in one order (Edward, 12 October 2026).** The main menu
and the pause menu should always offer the same systems in the same order, unless one is unique
to one of them (the navigator and the node graph need a hallway walked). The reason: a system
reachable only from one place cuts off whoever entered the other way, and interchangeable menus
make the systems easier to travel between, more versatile and more usable.
- *Built (12 October 2026):* the main menu is Start, File Locator, Media Player, Filter Designer,
  AI Training Harness, Settings, Exit; the pause menu is Resume, Restart, Node Graph Viewer,
  Navigation System, then the same systems in the same order. The main menu's File Locator has
  Go to it again (the hallway is built and walks to the file; a file longer than the binary line
  still asks twice), and its Media Player builds the hallway with the player open over it.

**19.42 The web page's Build or Buy (Edward, 13 October 2026).** Built: `Pages/release.html`,
linked from the index as "Build or Buy (Free If You Build It)".
- **What is on it:** the two names, the three steps to build it, a PayPal link to buy or support
  it, links to the current release and the repository, and "When It Is Ready" as the expected
  release.
- **The counters,** in the spirit of the orbital and audio pages, all exact (JavaScript BigInt):
  - every 10x10 black-and-white picture, looked at one a second since the 24th of April 2026
    (finished in the year 4.0 × 10^22);
  - then every 32-character page (2.0 × 10^38);
  - then every model of the AI line's default shape (4.5 × 10^9856).
- **Later:** storefronts in place of PayPal, and the build video.

**19.44 Keeping the interface even (Edward, 13 October 2026).** His description of the interface
as it should stay: even and distinguished. Rows line up, controls in a row share their height, a
symbol sits in the exact middle of its button, and the corridors' wireframe style carries into
the panels.
- *Built:* CHAT's row. The box, +, DRAW and SEND share their top and bottom edges, and the + is
  drawn as two bars to the pixel rather than as a letter of the font.
- *Kept as a rule* in HANDOFF's house style, for every panel to come.

**19.61 What the numbers mean (Edward, 13 October 2026).** His questions: how would these numbers
translate to anyone in the real world? What do they mean for the information, how long will it
take to get somewhere, and where is it all going? It will never finish, but how much is learned or
lost per day? He wants lists of the strange ways a human can quantify these things, kept as
statistics, ideas and filters for the AIs to talk about from every perspective.
- *Built, on the Build or Buy page,* computed live and exactly:
  - *a day:* 86,400 pictures, 6.8 × 10^-24 % of the 10x10 ones;
  - *everyone at once:* all 8.1 billion people would take 5.0 × 10^12 years, 359 times the age of
    the universe;
  - *the pages printed:* a stack 7.2 × 10^14 times the width of the observable universe;
  - *gained and lost:* the library loses nothing, and what can be lost is attention;
  - *the audio page's tracks,* from the same epoch, so the two pages agree.
- **Where it is going:** not to the end of the library, which is unreachable by walking (that is
  what the numbers say). It goes to better filters. Each rule found makes the meaningful part of
  a line findable, and that part can be small enough to finish: the survivors of a good stack can
  be few enough to walk in a lifetime. Progress is measured in what the rules have narrowed, not in
  units visited.
- **What is learned per day:** the bits that the day's new rules say (a filter that keeps one unit
  in 2^k has said k bits). That can be counted and charted (§19.50).
- **The list grows:** each new measure is a line in VOICES.md and, where exact, a counter on the
  page.

**19.64 Running out of original things to say (Edward, 13 October 2026).** Some people fear that
if everything is put into grammar, there will be no room left for anyone to say anything original.
He doubts it, and wants a counter.
- *Built, on the Build or Buy page:*
  - *the space:* sentences of 20 words from a 20,000-word vocabulary, 20,000^20 of them;
  - *the speakers:* every person who has ever lived (about 117 billion), each saying a new one
    every second for a hundred years;
  - *the share used:* 3.5 × 10^-64 %.
- *Its limits:* most of those sentences are nonsense, so the count of meaningful ones is far
  smaller. It is still astronomically larger than anything that can be used up, and longer
  sentences multiply it again. The counter measures how small the share is; it does not prove
  originality endless. That rests on the fact that a sentence can always be longer.

**19.66 Disclosure of AI, everywhere (Edward, 13 October 2026).** His request: an AI honesty
disclosure across the whole website, as the audio page has, and fullest on the Build or Buy page.
It should say that Gemini helped to theorise and conceptualise (the website, many concepts and
ideas), that Claude built with him, and that he has been building the theory and ideas himself
over the past few years.
- *Built:* a disclosure box on every page, and a fuller one on Build or Buy. It says what the
  game's own models are trained on: a public-domain text, and Sieve's own lines.
- *Kept true:* as the project changes, the disclosure changes with it. A new contributor, human or
  model, is added when they contribute.

**19.67 Folding everything (Edward, 13 October 2026).** His words: they (the filters, rules and
ideas) are pretty much all intended to be automata, and many are or can be turned into them. So
it could fold in on all information that could ever exist, through all the light humanity will be
able to capture in the time the universe has, against the fading horizon of the observable
universe and the unobservable universe beyond it. That makes another set of numbers to weigh on
the website. The aim is to fold everything. "There are so many ways to map the data. We'll get
there." (VOICES.md).

*Notes.*
- **Why folding is the only way:**
  - *Writing everything out is impossible:* the universe can hold about 10^90 bits (Lloyd, 2002),
    and the AI line's models alone, written out, would take 10^9868 bits.
  - *Folding works:* a rule that can be run (an automaton, a line's definition, a stack) holds a
    whole set in a few bits. The AI line's definition is one line of SPECIFICATIONS.
  - *So "fold everything" means:* hold everything as rules, and unfold only the part looked at.
    That is what Sieve already does, line by line.
- **The light and the horizon:**
  - *Every photon that reaches us is finite in number:* the afterglow of the Big Bang alone is
    about 1.5 × 10^89 photons in the observable universe.
  - *The reachable part shrinks:* only about 4.6% of what we can see could ever be reached by
    anything sent today, and that share falls every second (the Orbital Simulation page's
    figures).

  So what can ever be captured is bounded, while the lines are not. The rules are what reach past
  that bound: a rule written here describes what no telescope will ever see.
- **Built, on the Build or Buy page:** "Folding Everything".
  - *Its counter:* bits written out against one line folded.
  - *Its list:* what the universe can hold, the light there is, the receding horizon, the stars
    crossing out of reach this year (the Orbital Simulation page's constants and formula, so the
    pages agree), and what fits and what folds.
- **Also on the website:** the AI disclosure is now at the top of every page, and on Build or Buy
  it comes before the buttons to buy or download, which are now at the bottom (Edward: it always
  has to be front and centre).

**19.68 The voices on the website (Edward, 13 October 2026).** His idea: the site can read out a
random voice from VOICES.md, at the very bottom of Build or Buy, under the donate button, as
another quirk of the site.
- *Built:* the page fetches `docs/VOICES.md` and shows one voice at random, with who said it and
  when, changing every 12 seconds. A "Read aloud" button speaks each voice with the browser's own
  speech synthesis, once a visitor turns it on (browsers do not let a page speak unasked). The page
  reads the file itself, so a voice added there is on the site with no change to the page. Where
  the file cannot be read (a page opened from disk), the box stays hidden.

**19.69 Build or Buy, and every build on the website (Edward, 13 October 2026).** His words: the
website should only ever read as build or buy, and not try to sell, which is why the buy button
went to the bottom, as in the honesty disclosure he drafted with the audio page. All the metrics
and statistics recorded by builds can go on the site too, some as counters and some fixed until
new figures come in. All of it has to be kept in sync, and eventually it may all translate across
every programming language, interface, address and taxonomy.
- *Built:*
  - *The name and the order:* the page is "Build or Buy" everywhere. Its buttons run Build, then
    Download, then Buy, at the bottom. The line at the top says the game is the same either way and
    that nothing on the page will try to talk anyone into buying it.
  - *"Every Build, On The Record":*
    - a counter of the time since the last recorded build;
    - the latest build's figures, fixed until the next build: unit checks, oracle vector files,
      specification sections, commands, source files and lines, ideas and voices;
    - what the reference filters keep on each line, when a hallway was built;
    - a chart for each measure, a dot a build, joined by lines.
- *Kept in sync:* there is one source, `metrics/index.tsv`, written by `tools/log_metrics.py` from
  the records. The page reads it, and the start room's wall will read the same file. Each measure is
  a named column, so a change of meaning is a new column, never a new meaning for an old one.
- *Honest by construction:*
  - a record names its commit, with `+changes` when it was made before committing;
  - it names the kind of machine (system, release and processor family), not which machine;
  - nothing is logged from a run with a failure.
- *Translating across everything:* every figure is a named, counted quantity in a plain table. A
  program in any language can read it, and an interface in any language can label it. That is the
  first step towards every address and taxonomy being one map (§19.54).

**19.78 Every figure with its inverse (Edward, 13 October 2026).** His words: "Running Out Of
Original Things To Say" says how many are used up, but has no counter beside it saying how many
are left. Always try to give the inverse of a metric, where and when it can be given, to put it in
perspective.
- *The rule:* a count of what is done comes with what is left, and a share with its complement.
  Where the inverse cannot be known (no honest total to take from), the figure says so, and no
  total is made up.
- *Built on the website:*
  - **Expected Completion:** the pictures looked at, and the pictures still to go, both as counts
    and as shares (1.267 × 10^30 left, 99.99999999999999999999882...%).
  - **And Then The Pages / The Language Models:** none read yet, so all of them still to read,
    100%.
  - **Running Out Of Original Things To Say:** the share used up, and the sentences still unsaid
    (1.048 × 10^86, 100% less the share used).
  - **Folding Everything:** the bits written out, and the rule that holds them: under 10^4 bits,
    less than 1 part in 10^9864 of them.
  - **The Orbital Simulation:** 95.4% unreachable, and 4.6% still reachable.
- *Not given an inverse, and why:*
  - **The audio page's tracks:** there is no total of tracks that could be made to take them from.
    The honest inverse would be the tracks not made, which needs the number that were made.
  - **The time since the last build:** a time has no "left", until there is a plan to measure
    against.
- *Next:* the game's figures (the setup menu's counts, the HUD's position along a line) get the
  same pairing: how far along, and how far still to go. *Built for the position (6 November
  2026):* the HUD, the book in hand, the shelf's verdict line, the navigator and the position
  message each say how far along the line and how far still to go ("X% along, Y% to go").
  *Built for the setup menu's counts (7 November 2026):* a stack's survivors line adds how long
  walking them at one a second would take ("at one a second, ~10^38.3 years to walk them"), the
  same reckoning as the main menu's estimate (§19.49).

**19.93 The `.map` format folded into `.sieve` (Edward, 22 October 2026; built the same day).**
His thought: update the `.map` format, or replace it with `.sieve` and deprecate it, the
differences once needed for the 3D node graph perhaps being redundant now.
- *What a map has that instructions lack* (SPECIFICATIONS §12.3): files **named but not held**
  (a size and SHA-256, a claim that the bytes exist somewhere), **edges** with a relation
  (`contains`, `anchor`, any word), **metadata** on a node, a **seal**, and **held** files whose
  bytes it carries as they are.
- *What the node graph needs from it:* none of that is 3D. A map stores no position; the graph
  lays it out afresh each time (force-directed, starting from each node's id), so nothing of the
  3D view is in the file, and that difference is already gone.
- *What instructions have that a map lacks:* every way of packing (LZMA2 as now chosen, the
  x86 stream, the rope, pieces, items, archives remade), the binary head, and installing.
- *Why it was set aside before (11 October 2026, §16.3):* every `.sieve` would have had to allow
  for entries only maps need. Since v16 that no longer costs anything: the head's flags byte has
  two bits free (64, 128), so a map's parts would be present only where a flag says so, and
  ordinary instructions would be the same bytes as now. (Both have been used since: 64 for the graph part
  in v18, 128 for the body as text in v20. The flags byte is now full; see NEXT-VERSION.md.)
- *Recommended:* one format. A `.sieve` whose head may flag a **graph part** (named-only entries,
  edges, metadata, a seal), its held files carried and packed as any instructions' files are; a
  new manifest version, refused by older readers. The node graph opens `.sieve` files as maps;
  `sieve install` puts back the held files and lists the named-only ones (it cannot make them);
  `sieve map` writes `.sieve`. The `.map` formats (v1, v2) stay readable, with a conversion
  (`sieve map OLD.map --to NEW.sieve`), and nothing writes them any more. The DOT and GraphML
  exports stay. Held anchors gain what installers gain (§16.3: a map of texts or programs).
- *To weigh first:* the release map is sealed and its SHA-256 published, so it would be
  republished as a `.sieve` with a new hash; "This installation" is made fresh each time and
  would simply be made as a `.sieve`. Edward to decide before it is built.
- *Edward's decision (22 October 2026):* build it; the release's map becomes `source.sieve`
  rather than `sieve.map`, and any `.sieve` opens as a node graph, perhaps without notes.
- *Built (22 October 2026):* `sieve-manifest-v18` (SPECIFICATIONS §12.2, §12.3). The graph part
  is the map's own text, every line behind `g<TAB>`, after the entries; head byte 18, flag 64.
  The held files are the instructions' files, packed as any are; the named-only nodes are in the
  text alone, and `sieve install` says how many it does not carry. `sieve map` writes `.sieve`
  and refuses `--out NAME.map`; older `.map` files are read, and `--out NAME.sieve` converts one
  (the conversion is `--out`, not the `--to` first suggested above). The hallway lists `.sieve`
  and `.map` in `maps/`, saves every change as `.sieve`, and shows instructions without a graph
  part as the map of their tree, read-only. `make_release` publishes `source.sieve`, checked by
  installing the source back from it. The oracle reads v18 (`unpack`, `graph`). Measured: a map of
  a 6-byte and a 3,000-byte file, named only, 271 bytes; the same two named and a 200,000-byte file
  of noise held, sealed, 200,353 bytes.

**19.96 Every dimension at every degree, and the starting page, as `.sieve` (Edward, 22 October
2026; measured).** His ask: the sample of every dimension at each whole degree (§16.9,
`hallway --sample-degrees`), "which should be tiny", and "welcome to the sieve" ("1 - 0 - 112",
§9) as `.sieve` files. Made here with the newest version (`sieve locate … --items`), each
installed back byte for byte:
- *The sample* (3,960 files, 13,321,756 bytes): 8,784 bytes (the hallway's own, v4, 482,495; 7z
  22,961). 1,368 files are items, their places 1,492,272 bytes carried in 1,293: a whole degree's
  place is a fraction of the line with a short repeating expansion, so the places repeat, and
  LZMA2 takes the repeats. Each dimension alone, its `.sieve` against its 7z: AI 1,531 / 6,756;
  audio 755 / 1,326; binary 1,044 / 1,548; books 993 / 1,649; image 811 / 3,144; models 1,123 /
  1,591; movies 1,216 / 2,758; pages 893 / 1,378; tracks 801 / 1,400; video 661 / 2,314; worlds
  910 / 3,366.
- *The starting page* (32 bytes as the file the hallway saves): 162 bytes (v4 196), the page an
  item of the pages line, its place 20 bytes; the rest is the instructions' head, the tree's
  SHA-256 (32 bytes) and the file's name. "1 - 0 - 112" is the idealised form (§9) and is not a
  format.
- The hallway's `--sample-degrees` still writes v4; the CI checks that the newest made from the
  sample is smaller than it, so changing it waits for a check of its own. *(Done 27 October 2026:
  it writes the newest, and the CI's check compares v5 with `--v4`.)* Made again that day, both
  still v20 (items and LZMA2 are all they use): the sample 7,887 bytes (8,784 before), the
  starting page 159 (162), each installed back byte for byte; Proof/ holds them.

**19.100 "Create .sieve" in the right-click menu (Edward, 24 October 2026; noted, not to be built
yet).** "I was thinking about adding create .sieve to the context menu, but that brings with it a
barrel of issues, such as needing the full file locator and the potential amount of time it might
take to sieve a file." Extracting is quick and its time is known (decoding is measured on every
install); making is neither: the rope's search, LZMA2's passes and cutting into pieces take from
seconds to many minutes (a 40 MB tar: §19.97), and the File Locator is what shows that time, its
estimate, the best so far, and Stop and Force stop. So it waits until making is understood and
faster:
- *Measured first:* how long making takes, per stage and per kind of file, on a fixed set of files
  (the varied set, the 40 MB tar, the release folder, Edward's JPEGs), and how far the estimate is
  from the time taken (15 to 20% low when last measured: §19.97).
- *Made faster:* threads (§19.99), with results that do not depend on how many.
- *Then the entry itself:* likely a small window of the File Locator's (progress, estimate, best
  so far, Stop) rather than the command line, with a time budget chosen beforehand, as the
  locator's search time is; one file or a selection, made beside them as `NAME.sieve`.
- *Decided (Edward, 29 October 2026):* the entry opens the File Locator itself, as the hallway
  starts by default (its usual window and settings; to be tuned later): `hallway --create PATH
  [--out PATH.sieve]` opens the locator alone, with no hallway behind it, reads what was chosen
  (several files chosen: one window, one `.sieve` of them), and saves `NAME.sieve` beside it.
  Done, it says so for a moment and closes, with an exit code a script can read; stopped or failed,
  it stays open, saying why. Everything the locator shows comes with it: the estimate, the graph
  of each best so far, Stop and Force stop, Keep a log.
- *The menu (Edward):* the same `Sieve` submenu as Extract's (`context/install_context.bat`), with
  **Create .sieve**, and **Other compressed formats** listing the formats Sieve can write, to
  mirror the ones it extracts: gz, xz, lzma, bz2, zst, lz4 and br by Sieve's own compressors (the
  ones it remakes archives with), and 7z, zip, tar, cpio, iso, ar and warc by libarchive (linked
  already, its writers built with it: checked 29 October 2026, unused so far). Not rar (no free compressor
  exists, and unRAR's licence forbids writing one), nor lz, cab or lha (no writer at hand). Those
  are made at once, at each one's strongest preset, with no search, so they need no window beyond a
  line of progress.
- *When:* after making is faster and the estimate is checked against Edward's logs (HANDOFF's plan
  of 29 October 2026, items 1 and 4).
- *Built (6 November 2026):* `hallway --create PATH [--out FILE]` and the "Create .sieve" entry on
  every file and folder (`context/install_context.bat`), as decided: the File Locator alone, live
  while it works, NAME.sieve saved beside it, closed with 0; stopped or failed, open and saying why
  (1). A folder of the test fixture 2.2 s, MANUAL.md 3.2 s, each installing back the same. Several
  chosen (Edward: "treat the multiple selected files as a directory"), the same day: the folder
  they are in, holding just them (walk_folder's `only`), saved as FOLDER.sieve; Explorer starts a
  program for each, and the first gathers the others' paths (a lock in the temporary folder for
  that folder, half a second without a new one). Three files and a folder chosen: one .sieve of
  the four, the file not chosen left out.
- *Built, the other formats (6 November 2026):* `sieve compress PATH... --format F` and a
  "Compress to" submenu on every file and folder (on its own, as "Create .sieve" is): zip, 7z, xz,
  gz, bz2, zst, lz4, br, lzma and tar, at the settings the comparison table measures (zip deflate 9,
  7z LZMA2 9e solid, xz -9e, gzip -9, bzip2 -9, zstd -19 --long=27, lz4 -12, brotli -q 11 -w 24),
  each made in memory, read back and compared with what went in before it is written. A folder or
  several files go into a stream format as a tar first. tar and zip by libarchive's writers; 7z
  written by Sieve itself (libarchive's 7z writer has no LZMA2 without liblzma built into it, which
  Sieve's libarchive is not) and read back by the installer's own 7z reader; the streams by the
  libraries Sieve already remakes them with. Not cpio, iso, ar or warc (no one asks for them by a
  menu), nor rar, lz, cab or lha, as above. Each read by 7-Zip, tar, unzip, xz, gzip, bzip2 and
  Python's modules here, names with accents included. IDEAS.md (520,976 bytes) alone: gz 195,506
  bytes, zip 195,667, xz 167,464, lzma 167,402, 7z 167,518, zst 170,227, br 162,994, bz2 152,916,
  lz4 225,407; each under a second. Several chosen are gathered as for "Create .sieve" (the
  gathering moved to tools/cli/gather.cpp, for both). Everything is read into memory.

**19.107 Edward's notes of 29 October 2026 (built).**
- *The window's title* is "The Sieve of Potentia".
- *Choosing a folder* on Windows opens Explorer's own picker (`IFileOpenDialog`, folders only),
  with an address bar and the places pane, instead of SDL's tree of folders (`SHBrowseForFolder`);
  where it cannot be had, SDL's as before. Elsewhere SDL's, which is the system's own.
- *An installer program from a `.sieve`:* the File Locator's "Installer program from a .sieve" (M)
  asks for a `.sieve`, then where to save, and writes `sieve-install` with it attached, without
  installing it first.
- *The installer's address bar* is a text field: a cursor, selection by mouse (a double click takes
  it all) and Shift with the arrows, Ctrl+A, Ctrl+C, Ctrl+X, Ctrl+V, Home and End.
- *The installer reads its file on a thread of its own,* so its window answers while a large one is
  read ("Reading ...").

**19.109 A look round, 31 October 2026 (three ideas measured; nothing built).** Edward: "give the
ideas and current work a look over ... Even come up with your own ideas if you wish". The plan is
[ROADMAP.md](../ROADMAP.md). Its part D holds the new ideas, three of them measured:
- **A PNG's rows carried unfiltered under temporary:** PNG's filters are sums and differences modulo
  256, so the original comes back byte for byte by construction. It is about 22% to 41% smaller at
  worst on the two PNGs written by other programs, and larger on small and flat pictures, so it is
  weighed. The tables are in `results/png-unfiltered-2026-10-31.md`.
- **Mixing tried on an `stb` PNG's pixels:** hallway-compact's pixels were carried by LZMA2 in
  15,957 bytes, where mixing as a picture gives 14,059. No version is needed.
- **Names equal but for case** (found by CI step 11's failure on Windows): installing checks that
  no file is already there before writing. Two names in one `.sieve` that differ only in case both
  pass, and on Windows or macOS the second is written over the first. Read from the code, not tried
  on Windows.
- *Worked through the same day* (Edward: "Work through all of that"): all three were built, the
  first two behind the draft.
  - The PNG rows unfiltered (`skr`) came to 23% to 40% smaller on the five PNGs other programs
    wrote, each back byte for byte.
  - Mixing with the recipe's shape took hallway-compact from 16,058 bytes to 12,772, and the 18
    hallway screenshots together from 245,100 to 211,937.
  - The installers now refuse a pair of names that are one where case is ignored.
  - The book cover, first counted among other programs' PNGs, was Sieve's own (stb): corrected in
    the results.

**19.124 A box for zip and 7z's comparison (Edward, 6 November 2026; built the same day).**
Edward: "Another checkbox for disabling/enabling the basic zip/7zip comparisons to be added in
the future, on by default." *What it would save:* the File Locator works out, for every folder or
file it reads, what zip (deflate at level 9, each file on its own) and 7z (LZMA2 at preset 9
extreme, everything as one stream) make of it, and then what they make of the Sieve instructions
themselves. Measured on the six GIFs (9.6 MB): zip 0.19 s and 7z 3.5 s on the files, which since
6 November run beside the packing rather than before it, so they no longer lengthen it where the
packing takes as long; and 2.7 s on the instructions (8.4 MB), after the packing, which still
does. On `docs/` (2.7 MB of text) neither came to anything measurable. So the box would save the
instructions' comparison outright, and the files' comparison's processor time (energy, and cores
the packing could have had on a folder whose packing is quick), not its wait. *How:* a "Compare
with zip and 7z" box beside More formats, ticked by default and kept in the settings like it
([locator] compare); unticked, the table shows Sieve's figures alone, and More formats, which only
adds to the comparison, is greyed. *Worth keeping in mind:* the comparison is how the File
Locator shows what Sieve gained, so the default stays on, as Edward said. *Built (6 November 2026):*
"Compare with zip and 7z" (Z, `[locator] compare`, ticked by default); unticked, the six GIFs 25.7 s
to 21.6 s in the File Locator, the table Sieve's figures alone and More formats greyed.

**19.125 Every compressed format is a possible bomb (Edward, 6 November 2026: "in theory can't any
compressed format become a zip bomb for sieve ... I don't know if it can really be avoided that well").**
In theory, yes: a few kilobytes of any compressed format can say, or make, gigabytes. It cannot be
prevented; it can be met before the memory runs out, which is what Sieve does.
- *Installing a `.sieve`:* the declared total of every file, and of what each remade archive is made
  from, against half the memory available, before anything is made; each stream's size read from its
  chunks before it is decoded; pictures against the viewer's limits (since 4 November 2026).
- *Packing, single streams:* xz, gzip, bzip2, zstd, lz4, brotli, lzma and lzip contents, and zip's
  members, read only while they stay under the memory free; past it the file is carried as it is,
  not opened.
- *Closed today (6 November 2026):*
  - **RAR, cab, ISO, LHA, cpio, ar, WARC and tar** (libarchive): every member read so far, together,
    under the same limit (it had none);
  - **7z**: the LZMA SDK unpacks a solid block whole into one buffer of the size its header
    declares, so each block's declared size (and, where every file is held in memory, the files'
    total beside the largest block) is checked against the memory available before anything is
    unpacked. A 7z of 16 GiB of zeros (3.6 MB) is now refused by Extract at once ("the 7z's blocks
    declare 32768 MiB to unpack into memory, more than the 15534 MiB available": the files and their
    block, held together) and carried as it is by the packer (4.2 s, 323 MiB at most); a 114-byte 7z
    declaring 2^50 bytes is in CI step 11;
  - **the limit asked from 16 MiB** of one read, and at every 16 MiB after (it was asked only from
    256 MiB, so a folder of many archives each under that went unchecked). What earlier archives
    made is already out of the memory free, so many bombs in one folder, or one inside another,
    meet the same limit together.
- *Not tested with a real RAR, cab or LHA bomb:* there is no maker for those formats here; the
  libarchive limit is the same code as the others'.
