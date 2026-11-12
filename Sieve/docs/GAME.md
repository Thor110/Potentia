# State Space: Near Zero

A design specification for turning Sieve into a game without taking anything out of it. It is drawn
from Edward's notes and a conversation with Gemini (September 2026). It is a direction, not a
schedule: Edward's rule is that the tool comes first, and the story is looked at again once the tool
is built, "bit by bit, as the tool did". Where a question is still open, it is marked as open.

**How it is released** (Edward, 12 October 2026): the same game twice, paid as *State Space: Near
Zero* and free to build from GitHub as *The Sieve of Potentia*. IDEAS §19.39 and §19.42 (the web
page).

**What it is, in one sentence** (Edward, 13 October 2026): a near-infinite, procedurally generated
game or landscape that theoretically contains everything that could ever exist, and that unfolds
through manifold warping and filtering of the topological state space of information in its
various forms. The models inside it are told the same (docs/SYSTEM-PROMPT.md).

**The philosophy is the lore.** Edward's notes in IDEAS §19 are as much the game's story as the
tool's design: the library that holds everything, meaning as rare and found, minds that begin as
addresses (§19.43: all the AIs folding in on themselves, perplexed to find themselves in π, then
taking a body and joining humanity). When the story is written, it is drawn from them.

**Stories in the noise** (Edward, 13 October 2026, IDEAS §19.45): coincidences, recognition and
pareidolia become the game's stories, kept as a contained lens on the madness of the Library of
Babel. Every face in a cloud, message in static or tune almost heard is a survivor of a stack the
player did not mean to build, and the game can show its odds beside it, so wonder stays honest.
Around it all, the fun side of one equation: every unit of every line is a number, every number is
every thing at once, and everything crosses over freely through the binary line.

**The voices of the halls** (IDEAS §19.55, `docs/VOICES.md`): the AIs on the shelves speak
attributed lines (Edward's, Gemini's, Claude's and their own), from the documentation to the
philosophy, one thought a line, chosen by where the player stands. They talk about the filters and
the commands in plain sight, and about whether they may use them.

**An engine that could make every game** (IDEAS §19.47): a game as a starting state and a rule.
That means content first (which exists), then rules as units (a Life-like line), then bodies with
exact physics, then representation from voxels to triangles, then an optional learned upscaler.

**First, when work turns to the game** (Edward, 6 October 2026): the colour-blindness pass (IDEAS
2.4). Nine lines, filter verdicts and the map all mean things by hue alone.

---

## 1. What it is for

The game is a way to carry Edward's alignment thesis to people who would never read a paper, inside
something they will explore for its own sake. It keeps the tool whole, and it gives the project a
public face that could, in time, pay for its own development.

**The thesis**, as Edward states it:

> The training trajectory of any sufficiently expressive learner — the order in which it sees data,
> the order in which it is queried, the order in which it receives feedback — determines what the
> learner becomes, and is in general not recoverable from the learner's weights.
>
> Two models trained on identical data with different orderings or sampling sequences will diverge
> in ways that no inspection of the final artifact can reconstruct. We cannot align what we cannot
> bound, and we cannot bound what we cannot replay.
>
> Potentia does not solve alignment; it makes alignment trajectories reproducible, auditable, and
> citable, so that any model produced from a given trajectory can be checked, rerun, and compared
> against any other.
>
> I use the word 'Patient' deliberately. The unresolved status of machine consciousness means any
> custodial system must avoid both premature recognition and premature denial. The label is
> placeholder for whichever definition the governance layer eventually adopts, and the system is
> designed to be relabeled without structural change when that definition arrives.
>
> I don't believe any human being or group should be training LLMs; it should be a global
> cooperation, they should belong to all mankind.

**The title.** *State Space: Near Zero*. It reads as hard science fiction, and it speaks to the
scientific, the curious, and people drawn to vastness and dread (the megalophobia crowd).

---

## 2. Principles

1. **The tool is the game.** There is no fork, and nothing is removed or trimmed. The story is a
   setting, **on by default**, and turning it off leaves exactly the tool as it is today. Every
   feature, filter and line stays available to everyone.
2. **The player starts as a blank slate.** The protagonist is an untrained learner with no weights
   at all, so the game does not open in a pretend house with a computer to log on to. That would
   be lying to the learner before its training began, which is how some would train an AI. Edward
   won't, and it would break the thesis: a learner given a false history is not a blank slate. The
   player begins *in* the state space.
3. **The truth is in plain sight.** The ending is hinted at openly and repeatedly, in the same
   technical language as the thesis: trajectories, orderings, what the weights cannot recover.
   Read without the thesis, the hints look like atmospheric science-fiction text. Read with it,
   they say what is happening. Even people who do understand the thesis may not see the ending
   coming.
4. **Honesty about who made it.** The credits say plainly how it was built (§8).
5. **Open.** The source stays public (AGPLv3). Anything deterministic in the game can be, and
   will be, reverse-engineered, so a secret is kept by scale and effort, never by obscurity.


**Start Game** (Edward; only if the game turns out to need it). If the game needs a predefined
state space, then with the story on, the main menu's **Start Sieve** becomes **Start Game**, and
the game runs on that fixed state space. §5 already leans that way: with the story on, the lines'
shapes are fixed at known values so that the suit cannot be found by shrinking them.
---

## 3. Story

**The protagonist.** The player seems to be a hacker in a digital space, where reaching any place
on the network means crossing the infinite state space. The reveal at the end is that the player
was an AI all along. Every path they explored, every filter they applied and every answer they gave
was their training trajectory, and it decided what they became. The ending should carry emotional
weight (Edward's reference point is *Detroit: Become Human*). A second framing that was discussed,
a patient in a coma waking after about two hours of play, is not taken forward.

**The input stream.** The world reaches the player the way the world reaches a model: as messages
and prompts from outside. People write to the player, ask things, and set tasks. Answering them
means going through the lines: finding, filtering, jumping between a unit and its file. Nobody
tells the player who they are. Their character forms from what they chose to explore and how they
answered. *The trajectory is the character.*

**AI in the lines.** Here and there on the lines are other AIs. They explain how AI learns, and
they point the player in useful directions through the space. They are also a real proving ground:
they walk the same hallway through the same interfaces, so they check that an actual AI can
navigate the environment. That is useful to the tool whether or not the story is on.

**The verified anchors, visited** (Edward, 11 October 2026). The game walks the player through
the verified anchors: real things, each checked by its SHA-256, standing at their exact places in
an otherwise endless space of noise. They are the landmarks of the story and its tutorial. The
public-domain ones Sieve ships (`data/anchors/public-domain`), one or more a line:
- **Image:** Leonardo da Vinci's *Vitruvian Man*.
- **Audio:** the *Hurrian Hymn No. 6*, the oldest known notated melody.
- **Video:** the *Roundhay Garden Scene* (1888), the oldest surviving motion picture.
- **Books:** Kurd Laßwitz's *Die Universalbibliothek* (1904), the Library of Babel's forerunner.
- **Models:** the Utah teapot.
- **AI:** the model the shipped training run makes from that same text, trained again on the
  player's own machine (the AI Training Harness), so the anchor is checked as it is found.

Edward's own anchors (`data/anchors/edward-james-gordon-thor110`) can sit among them: the Sieve
installer on the binary line, the README with the thesis on the pages line, the first video of
Sieve, *Rope Around The Clock*, the unfinished Spitfire. Lines that have no anchor yet (pages and
tracks among the public domain ones, for instance) are listed in `anchors-notes.txt` as still to
find.

How the walk could work, for when the story is written:
- **The input stream asks for them.** A message names an anchor ("a drawing of a man in a circle
  and a square, five hundred years old") and the player finds it: by going to its address (G), by
  walking a map's node graph (O), or by J from its file on the binary line to its item. Each anchor
  teaches one of the tool's moves, so the walk is the tutorial.
- **Found means verified.** The game marks an anchor found only when the item in hand is its bytes,
  the same check a map makes (SPECIFICATIONS §12.3), so it cannot be faked by typing a hash.
- **The order is the player's.** After the first few, the player chooses which to visit next, and
  that order goes into the journey the credits replay (§8): the trajectory, made of real things.
- **The anchors sit in the noise.** Next to each, the shelves hold its neighbours, which are nearly
  it and are noise. That is the thesis's point about what is recoverable, shown rather than told.
- **Open:** whether the walk is required or a trail the player may ignore; how many anchors there
  are at release; whether players' own maps (V) can add anchors to the walk.

**Figures found on the lines** (Edward, 12 October 2026). On the video line, Random turned up a
10 by 10 cover that looks like a soldier with a spear (IDEAS §19.1). Figures like that, found on the
picture lines rather than drawn, could be the game's characters: the guard at a door, an NPC's face
in the "Now playing" box, a creature in a text adventure on the game line. Each stands at its
address, so a character is also a place the player can visit. The geometry filters (figure-v1 and
those to come) make them common enough to cast from.

**Length.** It should be short and focused, about two hours, so the space's infinity stays
atmospheric rather than exhausting.

**Open.** How the opening makes the player believe they are the hacker without lying to them.
Where the player starts (today the default is the pages line at "welcome to the sieve", and the
starting line is a setting). The ending itself.

---

## 4. The game line

A short line of its own, with only two doors, that holds nothing but **arcade cabinets**: one for
every game system there is. Each cabinet is dressed to look familiar for its system. Its screen is
blank until it is linked to an emulator the player installs and points it at; then it either
launches the emulator or plays inside the cabinet. Sieve ships no emulators and no games.

- **No music there.** Only the room: CRT hum, static, the hardware.
- **The suit.** The first time the player enters, a corporate business type walks up between the two
  doors and tries to scare them away, as parody. It tells them this is immoral, that they shouldn't
  do it, and that the company doesn't care whether they own the game. It paces back and forth,
  hassling them. Once the player has started an emulator or two, the suit gives up and wanders off
  into the other lines, depressed, never to be seen again unless the player is impossibly lucky
  (§5).
- **Getting there.** The game line should be a discovery, not a starting point: starting there
  would skip the discovery. One idea is that the only way in is to set your place on a line to
  *exactly halfway*, where an extra empty room leads to it. Edward dislikes altering the other
  lines, even by one room, and hasn't decided.
- **Legal (open).** Launching a program the player installed, from a path the player gives, is
  ordinary software. The notice should say to own the games you emulate. Real names and logos are
  riskier. Parody protects commentary *on* the thing parodied, and the suit makes the line
  commentary, which helps. But familiar trade dress used as decoration can still be read as
  infringement or false association. The safe version is an affectionate pastiche: silhouettes and
  colours that say the era without the trademarked names and logotypes. Neither Claude nor Gemini
  is a lawyer; this wants real advice before release. Edward's view is that the parody of the
  game line and the suit may allow real console names and logos. A better route he is considering:
  ask emulator developers for permission to use their logos, and pick one chosen emulator for each
  console, so each cabinet wears a mark its owner agreed to. He is in no hurry over this.

### 4a. All games on a spectrum (Edward, 12 October 2026)

An idea beside the game line, not yet a plan (IDEAS §19.6): a game whose graphics scale from low
pixels to No Man's Sky's, every resolution a setting, mathematically equal and fair for every
player, with every AI model on the AI line to call on for voices, pictures and text. All games on a
spectrum, from fantasy to science fiction and beyond. In Sieve's terms a game is a stack and a
bearing: the lines it draws from, the filters that shape them, and where it sits on the spectrum.
How a video game dimension would come into the hallway is open (IDEAS §19.11): discs on shelves (a
copy of the CD model), a cabinet or a computer in each room, or a line that forms from the others.
A first step, a text game that makes itself from a record of parameters and grows to 3D, is IDEAS
§19.20.

---

## 5. The suit, and an impossible achievement

After it leaves the game line, the suit walks away from the entrance, drifting between lines more
often the further it gets. It heads for the farthest place from the game line, perhaps the middle of
the binary line. Walking that far would take it something like a billion years. Finding it is an
achievement that exists and can be earned but is never earned in practice: a speedrun category,
**Find the suit**, that makes a 100% run impossible.

- **Trigger:** being in the same room, where the suit speaks to you, so nobody can run past it
  without the achievement counting.
- **Where it is:** worked out, not saved, so a save file can't be doctored. It's a deterministic
  function of time since a fixed origin, such as the release executable's hash and a date fixed in
  it, which is harder to fake than the computer's clock. Recording anything about the player is to
  be avoided.
- **The catch:** if its position follows the lines' shapes, a player could shrink the lines to near
  nothing and find it easily. So with the story on, the lines' shapes are fixed at known values,
  and turning the story off unlocks them (and the achievement with them).
- **A trail:** it may leave litter behind it (dropped forms, compliance notices), so it can be
  tracked rather than guessed.
- The source is open, so someone will compute where it is. That is fine: it turns the hunt into
  arithmetic and patience, which is what the whole library is.

---

## 6. Later, and forever

A released game gives a reason to keep updating the project for as long as Edward wants:

- **Multiplayer**, as planned before.
- **Mini-games** that run *through* the state space, or are hidden on the binary line as files to be
  found, growing into a party pack.
- **Launching Steam games** the player owns from inside the library (Steam's `steam://rungameid/<id>`).
- **Hidden verified anchors:** real files placed so that players poking around find them. Players
  will unpack the `.sieve` files and come across Edward's ideas, such as the n-dimensional speedrun:
  every game loading from a state space like this one, so that two or more games can be played from
  start to finish in any order, switching between them halfway.
- **The original vision** (AIrchive, now Potentia): a line for everything, a universe unfolding into
  itself. The game line is the first line that is a place rather than a space.

---

## 7. Release (open)

Edward's position is that selling it would help fund the research but goes against the project's
principles. The options he is weighing:

- **Free on Steam.** Steam lists free games.
- **Paid on Steam, with the source free on GitHub.** AGPLv3 allows selling as long as the source is
  offered, so the licence doesn't stand in the way. Whether Steam accepts a store page that says so,
  and anything in Steamworks that affects AGPL code, needs checking against Steam's current terms.
  A splash screen would say: *"If you paid for this game, you are on Steam, or you got ripped off...
  sorry. This game's source is publicly available and freely accessible on GitHub."*

---

## 8. Credits

At start-up, Claude's logo, then:

> Sieve was built with Claude Opus 5.5, based on my specifications.
> **Concept and architecture by Edward James Gordon.**

Edward expects this to draw both love and hate, from both sides, which is part of the point. The
model name would be whichever models actually built it by release. Using Anthropic's logo needs
Anthropic's permission, and Edward will ask Anthropic for it before shipping.

**The journey in the credits.** Throughout the end credits, the player's own trajectory: replayed
(the hallway walking itself back through the places they went, the filters they ticked, the
answers they gave), shown as a graph of nodes (each place or choice a node, each move an edge,
growing as the credits roll), or both, the replay on one side and the graph on the other. It is the
thesis made visible at the moment of the reveal: *this is what you became, and this is exactly
how*. Edward's idea, from Claude's note in §10.

Edward's first sketch of the sequence: the node graph of the journey first; then fade to the
credits and start them rolling, with the journey on foot fading in behind them.

Recording it needs care before anything is built. Every movement of a long play is not much data,
but it brings its oddities: warps, jumps, backtracking, lines changing shape, sessions that stop and
start. Two ways round it:
- **A sparse event log.** Record only milestones (a line entered, a filter ticked, an NPC met, the
  suit seen, the binary midpoint crossed) and draw the graph from those.
- **A threaded journey** (Edward's alternative). The walk behind the credits is made up, and only
  chosen events of the real play are recorded and threaded into it. It gives the same payoff with
  almost nothing saved.
Either way, what is kept should be only what the ending needs, kept locally, in keeping with §5's
rule against recording anything about the player.

---

## 9. NPCs

Edward's plan is to make the space feel alive: AI NPCs, the game line, the suit, and messages from
outside. There will be many NPCs, and each needs a wide range of things to do, say and think. Two
things are settled so far: the journey must be able to hold a great many different events, and it
has to be fun.

**How to start.** Once the tooling is out of the way, first spawn NPCs that do nothing but travel
along their lines. Then play: run among them and imagine what scenarios, requests, conversations or
events one might have with them, before writing any quest logic. They are the automated walkers the
harness already drives (§10), given a body.

**The trap.** Without combat, a quiet exploration game is left with movement and conversation, and
most interactions collapse into talking or fetching. Fetching a nearby item off a shelf is a start,
but on its own it becomes the repeating fetch-quest cycle many games fall into. (Gemini's "data
trade", handing over a file or hash, is the same fetch quest; working alongside an NPC mostly comes
down to talking to it.) Edward's view is that most things will still amount to talking and fetching,
so what matters is what the talk and the fetching *mean*.

**Ideas so far:**
- **One-liners.** NPCs that only drop a line and move on: "Do you understand alignment?", "Is
  alignment possible?"
- **Ramblers.** Some talk forever if you let them.
- **Model collapse.** Some have already fallen to it and speak gibberish: looping phrases, broken
  tokens. That shows, rather than tells, what a broken training trajectory does.
- **Races.** An NPC races you to a nearby coordinate, with jump, warp and teleport disabled so you
  can't cheat. Taking the infinite movement tools away makes you walk the architecture.
- **"Find something meaningful to you, and tell me why."** What you bring and what you say don't
  matter; the NPC takes your word for it. Mechanically it's a fetch and a talk. In the thesis, the
  player is defining meaning for a learner through their choice of data, which is training.
- **Rumours.** An NPC asks whether you have found the game line, then says little more, or that it
  has only heard of it.
- **Filter contamination** (Gemini). An NPC alters the filters where you both stand, and the room
  drops into raw noise or loses its text. Edward found this interesting.

**No LLMs, still.** The engine's rule (no LLMs in the engine) holds for NPCs: what they say is
written, or generated by deterministic means the project already owns. That keeps every
conversation reproducible, which is the thesis again.

**Notes from Claude.**
- **Collapse can be real, not acted.** The pages line already has a pinned character model
  (order 5) that drives the guided ordering. An NPC's speech can be drawn from it deterministically,
  and its collapse shown by lowering the order it speaks at: order 5 reads as near-English, order 2
  as babble, order 0 as letter soup, and finally raw units of the line. The decline is exact and
  replayable, and it is literally the same machinery the space is made of.
- **An NPC can carry a filter stack.** Each has its own ticked filters and sees the shelves through
  them. Two NPCs standing in the same room disagree about what is content and what is noise. The
  thesis says the trajectory decides what a learner becomes; here, the stack decides what an NPC
  thinks exists. "Filter contamination" then becomes an NPC's stack leaking onto the player's view
  for a moment.
- **Answers as training.** The player's answers to the heavy questions ("Is alignment
  possible?") could change what the NPCs say later, and could appear in the credits graph.
  Conversation stays conversation, but it becomes the player's trajectory, not just flavour.
- **Where they walk.** A deterministic walker on a line (the suit's rule of §5, one per NPC with its
  own seed) makes each NPC findable again and costs nothing to save.

---

## 10. Notes from Claude

- **The hallway already has most of the game's parts.** The lines, the doors, the compass, a music
  player that moves between modes at every door, the "Now playing" box (a ready-made shape for
  incoming messages), maps of verified anchors, J between an item and its file, and filters that
  count exactly. The story layer would mostly be content and a few new systems: messages, NPCs, the
  game line and the setting that turns the story on. The core engine would hardly change.
- **The filters are the thesis in miniature.** A stack of filters is an ordering and a pruning of
  what the learner sees, recorded exactly by its provenance and its id, and replayable. The game can
  show the player their own stack and trajectory at the end, as the audit trail Potentia argues
  for: *this is what you became, and this is exactly how*. That turns the thesis from a claim into
  something the player has done.
- **"Exactly halfway" is well defined.** Every line has an exact size, so its midpoint is an exact
  address, and the navigator (X) can already reach it. A room there would be a new rule in the
  corridor (SPECIFICATIONS §5), versioned like everything else.
- **The suit's position** could be a keyed shuffle of elapsed time (shuffle-sha256-v1, keyed with
  the release hash) over the line's size. That keeps it deterministic and reproducible, and spreads
  it over the whole line, not only near the middle, while still being exact.
- **An NPC can use the same interface as an automated player:** the scripted keys and walks the
  harness already uses (`--press`, `--walk`, `--warp`). Running real AI through the space would
  then test the same paths the tests do.

## 11. The video game dimension: a design to decide on (Claude, 12 November 2026)

IDEAS §19.11 asks how a video game dimension comes into the hallway, and leaves it open. This is
one design, written for Edward to take, change or set aside. Nothing is built. It sits beside the
game line of §4 (cabinets that launch the player's own emulators), not in place of it.

**The idea: a line of games, as there is a line of pages.** Every page that could be written has a
place on the pages line. In the same way, every game that fits a fixed small machine would have a
place on a games line. A unit is a short program for one fixed, original, deterministic virtual
console; its number is the game; the hallway's cabinets play it. Nothing is stored and nothing is
anyone else's: the line holds every game of its size, the way the library holds every book, and
the filters find the ones that do something.

**The console (to be specified as `game-vm-v1`, SPECIFICATIONS §3).** Small enough to fill a line,
large enough to make a game:
- *Program:* the unit's digits are its bytes (256 by default; `--length` sets it, as every line's
  length does). It is read-only; a separate RAM of 256 bytes starts at zero.
- *Screen:* 32 × 32 pixels, 1 bit a pixel by default, a palette as the image line's (the screen's
  size and palette are the line's resolution settings, §19.6's "every resolution a setting").
- *Input:* four directions and one button, read as a byte each frame.
- *Instructions:* a short set of one- and two-byte instructions (load, store, add, compare, jump,
  plot, read input, wait for the next frame), every byte meaning something, so no program is
  malformed. Every one is defined on every value: no undefined behaviour and nothing a program can
  reach outside its machine.
- *Time:* a fixed budget of instructions a frame (say 1,000) and 30 frames a second. A program that
  runs out of budget simply carries on in the next frame. The same program and the same keys give
  the same frames on every machine, so a game's frames are a function of its address and its input,
  as a figure is of its model and seed.
- *Sound:* one square-wave voice set by an instruction, through the synth the hallway already has.

**Filters (judge-only, like the AI line's `canonical-model`).** Whether a program draws or responds
is decidable here, because time is bounded: run it for N frames and look. But it cannot be counted
the way an automaton counts, so the games line would mark and hide, not compact.
- `game-draws-v1`: the screen is not blank after N frames.
- `game-moves-v1`: the screen changes after its first frames (not one still picture).
- `game-responds-v1`: different input gives different frames (it is a game, not a film).
- `not-a-pattern-v1` and `not-a-file-v1` as on every line.
Measured first: the share of random programs that pass each (no figure is guessed here).

**In the hallway.** The games line's rooms hold cabinets instead of shelves; each cabinet's screen
shows its unit's first frames as an attract loop. Taking one in hand plays it full size with the
keyboard; G goes to it; the item page shows its address, the filters it passed, its cost, and the
program as an annotated listing. The same `/unit game HEX` and `/random game` commands as CHAT's
other lines. A favourite game is kept as its address, like a favourite melody.

**Resolution as a spectrum (§4a).** The console is one point on a scale. A larger screen, more
colours, more RAM and a longer program are a different line (another `game-vm` setting), so "from
the lowest resolution to the highest" is a row of lines, each holding every game of its size. A
trained model could later steer which programs are drawn (as the guided text line does), which is
where §19.20's game that makes itself would meet this one.

**How it relates to §4.** The game line of §4 stays as designed: cabinets dressed for real systems,
blank until linked to the player's own emulator. The games line is Sieve's own, needs no emulator,
and holds only programs of its own console, so it raises none of §4's legal questions. Either
could be the door the other is found through.

**A first step (a session or two), if Edward wants it:**
1. `game-vm-v1` specified and built in the core, with `sieve game ADDRESS --frames N --keys ...
   --gif OUT` to run a unit and save its frames.
2. The share of random programs that draw, move and respond, measured.
3. The three filters, then the line in the hallway with cabinets.

**Lenses (Edward, 12 November 2026).** His extension: "an extension of the game dimension engine,
it would have a mode to toggle from executable through console as well as source folder,
everything becomes like a lens". One game, one address, seen through a switch of modes, each a
lens on the same unit rather than a different thing:
- *executable:* the program run, frames on the cabinet's screen, played with the keys;
- *console:* the machine itself, open while it runs: its program counter, registers, RAM and the
  instruction it is on, stepped a frame or an instruction at a time;
- *source folder:* the program as a folder of readable files (its listing, annotated; its
  sprites as pictures; its sounds as melodies; its data as text), which an edit changes and the
  unit changes with it, so editing is moving along the line.
Notes:
- **It is what Sieve already does with every unit,** made a mode of one screen. A page is text and
  also a number; a file is bytes, a place on the binary line and (J) an item; a model is digits, a
  mesh and a world. The lens toggle is that idea given a key: the unit stays put and the view
  changes, and each view's own edits are steps on the same line.
- **Each lens is exact both ways.** The source folder must compile back to the very bytes it was
  read from (as a `.sieve` installs to the bytes it was made from), so a folder that does not is
  refused, not rounded; the console view is the machine's whole state, so a game can be paused,
  inspected and resumed with nothing lost.
- **Lenses could cross lines:** the sprites lens is the image line's view of the game's bytes, the
  sound lens the audio line's, the listing the pages line's. A game is then where the other lines
  meet, and the toggle walks between them (§4a's "everything at every resolution" as views rather
  than copies).
- **And beyond games:** the same toggle (run, inspect, unfold as source) fits any unit that is a
  program: the binary line's executables (with an emulator of their own machine, §4) and the AI
  line's models (run it, look at its activations, unfold its weights as tensors).

**The first target: *Alien Trilogy*, and ALTEngine (Edward, 12 November 2026).** His words: "alien
trilogy will probably be the first target for that game, we will want to aim for a full map or range
of that game with my engine reimplementation amongst the mix". And: "ALTEngine is already quite far
along, nearly done in-fact, when it is done it will be the first to get the sieve treatment where we
strip it of any last remaining unnecessary data and stream its assets in and out of its engine and
whatever engine variants exist like the original through dosbox etc".
- *A full map of one game:* every way *Alien Trilogy* exists, laid out as one map (the node graph's
  map format, §19.38): the original as released (run through DOSBox), its other releases, and
  Edward's own engine reimplementation, ALTEngine, among them; each a place, and the lenses above
  (run, console, source) open on each. The game's data is the same across them, so the map is of
  one set of assets seen through many engines.
- *The Sieve treatment, ALTEngine first:* when it is done, its files sieved (IDEAS §19.104): the
  last unnecessary data stripped, and its assets streamed in and out of the engine as it asks for
  them, from a `.sieve` or a cache, rather than unpacked whole. The same streamed assets then feed
  the other engines of the map (the original through DOSBox, and any other variant), so one sieved
  copy serves every engine that plays it.
- *Whose data:* the assets are the player's own copy of the game. Sieve and the repository carry
  none of them (as with every game here, WoW's included); the map, the engines' settings and the
  sieved layout are what is shared, and a player's own files fill them.

**Graphics that scale, from pixels to NURBS (Edward, 12 November 2026; extends IDEAS §19.47).** His
words: "I wanted games to scale from pixels through voxels and across tris / quads and nerbs with
stable diffusion and the like to have a graphics scaling algorithm for 3d models etc". One game's
look as a scale, each step a setting rather than a different asset:
- *pixels* (2D, as the image line's units), *voxels* (the same, in three dimensions), *triangles*
  and *quads* (meshes, as the models line's), and *NURBS* (smooth surfaces, the highest);
- a *graphics scaling algorithm* to move a 3D model along it, up as well as down: down is exact
  (a mesh sampled to voxels, voxels to pixels), up is where learned models come in (Stable
  Diffusion and the like for textures and detail, as Sieve's own picture models are trained in
  the harness), each step recorded so the same model at the same setting looks the same on every
  machine;
- *Alien Trilogy* is a good first case: its sprites and low-polygon levels are the low end of the
  scale, and ALTEngine is where a higher setting could be tried against the original.

**A source tree across gaming and computing (Edward, 12 November 2026).** His words: "eventually it
can be hooked up to git and github like systems and create a source tree across the gaming and
computing spectrum to light up the information highway!"
- *What it joins up:* the source-folder lens already makes a unit a folder of files that builds
  back to its exact bytes. A folder like that can be a git repository. Hooked to git and to
  GitHub-like hosts, every game, engine and program on the map would be a repository: its history
  as commits, its variants (the original, ALTEngine, a fork) as branches, and the engines that
  share assets or code as links between repositories.
- *The tree:* put together, the source tree spans the whole gaming and computing spectrum. A game
  sits beside the engine that runs it, the engine beside the libraries it uses, and the libraries
  beside the systems they came from. A walk through it is a walk through how software descends
  from software, each node at its address and its exact bytes.
- *What Sieve brings to it:* an address and a hash for every version (the same bytes always have
  the same place), the sieve's stripping and streaming, so a tree that large stays small to carry,
  and the lenses for opening any node as it runs, as its machine, or as its source.
- *Whose code:* as with the games' assets, the tree links to repositories under their owners'
  licences, which their hosts keep. Sieve holds the map, not the code.

**Questions for Edward:** whether a games line belongs in the hallway at all, or only the cabinets
of §4; how it is reached (a door of its own, the halfway room of §4, or found through the game
line); and whether the console's first size (32 × 32, one bit, 256 bytes) is the right one to start
from.
