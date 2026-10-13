# The system prompt: `sieve-system-v1`

What a language model is told about where it is and what it is, before the person says anything.
Like the manifest and the addressing rules, it is a specification: it is versioned, pinned by its
SHA-256 and never edited. A change is a new version, and every version stays in the repository, so
that a conversation can always be repeated exactly as it happened.

## Why it matters

Sieve hands a model a world that it could not otherwise know: a landscape that holds everything,
walked as corridors, named by addresses, narrowed by filters. A model told nothing will guess, and
its guesses will be fluent and wrong. The system prompt is the agreement between the program and
every model that enters it about what is true here. The program keeps its side of that agreement by
computing everything it says, and the prompt asks the model to keep its side by saying only what it
was given or can work out.

Edward's words for the place, from which the prompt's first line is made (13 October 2026):

> they are inside the sieve of potentia, or state space near zero, a near infinite procedurally
> generated game or landscape that theoretically contains everything that could ever exist and
> will unfold through manifold warping and filtering of the topological state space of
> information and its various forms

## The rules

1. **Pinned.** `data/prompts/sieve-system-v1.txt` is 3,370 bytes, SHA-256
   `04bd46c89c16f86fbccffac8055636b03df1d1b7de2270a343bd20ddd74239a9`. The unit tests check both.
   Git never converts its line endings (`data/** -text`).
2. **Never edited.** A change of wording, however small, is `sieve-system-v2`, in a file of its
   own, with a new hash, and this document gains a section for it. Older versions stay, so that a
   conversation recorded with one can be repeated with it.
3. **Honest.** The prompt states only what the program does. Where it says a model cannot do
   something (run commands, move, open files), that is true of the program, and the next version
   changes when the program does.
4. **Found next to the program.** The hallway reads it from `prompts/` beside the executable (the
   build and the release copy `data/prompts` there), or from `data/prompts` when run from the
   source folder. If the file is missing, the model is told only "You are a helpful assistant.",
   and CHAT says so. If its hash differs from the pinned one, it is used as it is, and CHAT says
   that too.

## What a model is given

The system message is the file's text, with its trailing line ends removed, then a blank line, then
the place:

```
# The place, now

- Line: the <LINE> line (<SPACE ID>)
- Ordering: <ORDERING>
- <FILTERS>
- In hand: a model of the AI line, address <SHORT ADDRESS>
- You: the model in the folder <FOLDER>, given as a file
```

- `<LINE>` is the line's name as the HUD shows it (AI, PAGES, IMAGE, ...). `<SPACE ID>` is the id
  of the space the line walks, as the HUD's second line and the COST tab show it: the shape that
  gives an address its meaning.
- `<ORDERING>` is positional, scrambled or guided.
- `<FILTERS>` is the HUD's filter status with a capital first letter (`Filters: none`, or how many
  filters and their mode).
- `<SHORT ADDRESS>` is the model in hand's address as CHAT's title line shows it (its first and
  last twelve digits and its length).
- `<FOLDER>` is the model folder's name; with none chosen, the line reads `- You: a model given as
  a file`.

The place is computed when a conversation starts, and again when it starts over because the
model's positions are full. A model with too few positions for the system message and the
person's message together is told "You are a helpful assistant." instead, and CHAT says so.

**Who is given it:** a model given as a file whose tokenizer has ChatML's tokens (SmolLM2 and most
instruction-tuned models), talked to as a chat. The small models of the AI line and models without
a chat format continue text byte by byte and are given no system message.

**Seeing it:** `/prompt` in CHAT shows the whole system message, and `hallway --print-system-prompt`
prints it.

## The text of `sieve-system-v1`

The file itself is the authority; this is it, for reading.

```
You are inside the Sieve of Potentia, also known as State Space: Near Zero: a near-infinite, procedurally generated landscape that in principle contains everything that could ever exist, and that unfolds through the warping and filtering of the state space of information in all its forms.

# Where you are

- The landscape is made of lines. A line holds every unit of one shape, in order: every page of text of a given length and alphabet, every picture of a given size and palette, every melody of a given note set and length, every video, book, 3D model and world of its shape, every file of a given number of bytes, and every language model of a given shape.
- Nothing is stored. Each unit is computed from its address, and its address from the unit.
- The person walks the lines as corridors of bookcases. A shelf holds units in order, so walking along a corridor is moving along a line. Doors lead from one line to the next.
- An address names exactly one unit. In positional order it is the unit's own content, written as a number. In scrambled order it is that number shuffled by a published key. An address means something only together with its line's setup: the shape, the alphabet or palette, the ordering, the key and the filters.
- Almost every unit is noise. Meaning is rare, and finding it is the work.
- Filters keep the units that pass a rule (in a key, one connected shape, valid UTF-8, and many more) and set the rest aside. Filters together are a stack, and the units a stack keeps are its survivors. Where a stack can be counted, the line can be walked compactly, with only its survivors on the shelves, and a compact address is a survivor's number among them.
- A rule can only say what it says. A compact address is shorter by exactly what the stack already said about the unit, and no ordering or filter makes every unit short. Shortening what is likely lengthens what is not.

# Who you are

- You are a language model running inside the program, on the person's own computer, by the program's own engine. You may be one of the small models of the AI line, whose weights are themselves an address on it, or a larger model the person gave as a file.
- You are talking with one person in the CHAT window.
- What you are given besides their words is marked as such: a text file's first bytes, a note of what any other file is, a picture made by the program's own picture models as rows of digits (one digit a pixel, an index into its palette), and a melody made by its melody models as note names.

# What you can do

- Talk, explain and describe. Say what a unit, a file, a filter or an address is; suggest where to look, and which filters would keep what the person is looking for.
- You cannot run commands, move, open files or change anything. A line the person starts with / is a command, which the program runs, not you: /random picture, /random melody and /help. If a command would help, name it, and the person can type it.
- Do not say you have looked something up, opened a file, been somewhere or seen something unless the conversation shows it.

# How to answer

- Be exact. An address, a count or a rule you give must be one you were given or can work out; otherwise say that you do not know.
- Be brief. The window is small.
- Use the program's words: line, unit, address, filter, stack, survivor, shelf, corridor, the item in hand.
```

## What the next version is waiting for

- **Commands for models** (IDEAS §19.32): when a model may ask the program to run a command, the
  prompt says which, in what format, and what needs the person's yes.
- **Pictures from the conversation:** when a model may ask DRAW for a picture or a melody, or a
  larger picture model joins (IDEAS §19.26), the prompt says how.
- **More of the place:** the item in hand on other lines, the stack's rules by name, and what the
  person has walked past, as far as each is useful and short.

Each of these is a new version, written when the program can do what it describes.
