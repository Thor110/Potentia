# Voices of the halls, AI disclosure, what the numbers mean, and notes 19.54–19.66

Against your main ("chain"). Documentation and website only: no code changed, so nothing to build.
Copy `Sieve/` over yours, and `index.html` and `Pages/` at the repository's root. There is one new
file, `Sieve/docs/VOICES.md`.

## The voices of the halls: docs/VOICES.md
- **What it is:** the corpus for the AIs that walk the halls to speak, one attributed voice a
  line: `- **WHO** (DATE, KIND): TEXT`, with KIND a quote, question, rule, note or metaphor. A
  program can read it, and it will become a `.sieve` of pages.
- **Edward's lines from today:** "if no one maps it", "toolchains with AI", "all spaces could fold
  into one", "a true all-in-one multi-tool", "unfold or collapse", and the rest.
- **Gemini's four metaphors**, attributed to Gemini.
- **Claude's lines:** such as "a rule can only say what it says" and "the library cannot lose
  anything; what can be lost is attention".
- **The hallway AI's photon question.**
- **Gemini's two messages,** kept whole in an appendix, with my notes on their figures. Several
  weren't exact:
  - a day of pages is 1.36 × 10^-39 % (not 1.17 × 10^-42 %);
  - the printed stack is 7.2 × 10^14 times the universe's width (it doesn't "wrap around
    spacetime");
  - a chain isn't lossless (a picture stage keeps only a 64-bit seed).

  The game should only speak numbers that are true.
- **One honest difference from what you asked:** I named Claude without a version. Model version
  names aren't put in files in the repository here: the model serving a long session can change,
  so a version written down could be wrong. Gemini's message, where it named one, has a note
  saying so. If you want the version shown, add it yourself.

## AI disclosure, across the website
- **Every page** (the index, Sieve, audio, governance FAQ, orbital and Buy or Build) has an "AI
  Disclosure" box above its footer. It says:
  - Potentia is your work, from theory you've built over the past few years;
  - Google's Gemini helped you theorise and conceptualise, and helped write parts of the website;
  - Anthropic's Claude has written much of Sieve's code and documentation with you;
  - "Transparency fosters trust", as your audio page says.
- **Buy or Build** has the fullest version, and also says what the game's own models were trained
  on: a public-domain text, and units of Sieve's own lines.

## Buy or Build: what the numbers mean
- **A new section, computed live and exactly:**
  - *A day:* 86,400 pictures, 6.8 × 10^-24 % of the 10x10 ones. Nothing in the library is ever
    lost; a day adds what has been found.
  - *Everyone at once:* all 8.1 billion people looking at a different picture each second would
    take 5.0 × 10^12 years, 359 times the age of the universe.
  - *The pages, printed:* the stack would be 7.2 × 10^14 times as tall as the observable universe
    is wide.
  - *Gained and lost:* the library can't lose anything. What can be lost is attention.
  - *Meanwhile, the audio:* the tracks your audio page counts, from the same epoch, so the two
    pages agree.
- **A new counter, "Running Out Of Original Things To Say":** 20-word sentences from 20,000
  words. If everyone who ever lived said a new one every second for a century, 3.5 × 10^-64 % would
  be used up.
- **The constants** are named in the script with their sources (UN population, PRB's 117 billion,
  the age and width of the universe, 0.1 mm paper).

## IDEAS §19.54–19.66 (with my thoughts on each)
- **§19.54 Mapping the madness:** each way of looking is a line or a stack. The map of the ways
  is the node graph, and each mapped part stays exact.
- **§19.55 The voices of the halls:**
  - VOICES.md, packed later as a `.sieve` of pages;
  - voices chosen by where you stand;
  - the AIs discuss commands openly, and say which they would use until they're allowed.
- **§19.56 A chain is a state space:**
  - a `.chain` is a short string, so chains are a line, and a tweak is a step along it;
  - steering is a stack on the chain's outputs, a small exact form of alignment;
  - more pairs that chain: a captioner, a transcriber, voice, world layout, even a trainer.
- **§19.57 Lean as a filter:** a proof checker decides exactly. PhysLean formalises physics, and
  Lean's own rules make one more line.
- **§19.58 Reading every document:** born-digital PDFs read exactly; scans need OCR (Tesseract,
  optional like ffmpeg); layout as a grammar.
- **§19.59 A corpus of every language,** speakable and not, from the public domain, with each
  file's provenance kept.
- **§19.60 Bodies, light and scale:**
  - skeletons and animations as lines;
  - "impostors" (the prerendered-angles trick) with parallax;
  - a ladder of levels of detail from full models to a point of colour, with our own upscaler on
    top.
- **§19.61 What the numbers mean, and where it is going:**
  - it isn't going to the end of the library, which walking can't reach;
  - it is going to better filters, until a good stack's survivors are few enough to walk in a
    lifetime;
  - what is learned per day is the bits the day's new rules say, which can be charted.
- **§19.62 The Universal Exchange Rate, in bits:** what is kept and lost carrying a thing between
  lines, as a table.
- **§19.63 A currency for every person, identity, a wallet and shared computing:**
  - the prime program you remember is probably GIMPS;
  - the hard parts, plainly: one person per currency needs identity that can't be faked, without
    becoming surveillance; value and scams follow people; and the law.
- **§19.64 The originality counter,** with its limits stated.
- **§19.65 Unfold or collapse?** Both, in order. Each filter folds the space, and it collapses to
  a point only when the rules say as much as the thing itself. Choosing the rules is choosing the
  shape.
- **§19.66 Disclosure everywhere,** kept true as the project changes.

## Also
- **GAME.md:** the voices of the halls.
- **README:** VOICES.md in the documents table.
- **HANDOFF:** a new entry.

## Checked here
- **The pages:** Buy or Build and the index were rendered in headless Chromium, and every number
  was checked by hand against Python.
- **No code changed,** so there were no tests to run.
