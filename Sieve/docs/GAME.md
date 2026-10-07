# State Space: Near Zero

A design specification for turning Sieve into a game without taking anything out of it. It is drawn
from Edward's notes and a conversation with Gemini (September 2026). It is a direction, not a
schedule: Edward's rule is that the tool comes first, and the story is looked at again once the tool
is built, "bit by bit, as the tool did". Where a question is still open, it is marked as open.

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
