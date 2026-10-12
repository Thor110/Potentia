# CHAT as a tab, and five new notes

Against your main ("chat window"). Copy `Sieve/` over yours. There are no new files.

## CHAT is a tab of the hand panel
- **Where it is:** with a model of the AI line in hand, the tabs are ITEM, COST, SORT (META where
  a map has notes on it) and CHAT, last.
- **Opening it:** Enter opens CHAT, and so does C when it comes round to it.
- **While CHAT shows,** the keys type into its box. Esc goes back to ITEM, where C goes round the
  other tabs as before.
- **Unchanged:** everything in it works as before (files, figures, the cards' buttons, the keys).
  It is drawn in the hand panel instead of over the whole screen.
- **The compass** is no longer drawn over the tab: it covered the SEND button.

## IDEAS: new notes (each with *Notes.*)
- **§19.21 Conversations among the pages and books:**
  - filters for a conversation's surface rules, which are shapes on the page an automaton can
    count: alternating turns, turn marks, a question followed by an answer, bounded turns;
  - what those filters cannot see: meaning, which is a model's judgement;
  - why it would matter: training material for the harness's chat models.
- **§19.22 An unfoldable language network:**
  - models as nodes and relations as edges, in the maps that already exist;
  - unfolding as zoom: a node opens into a network of its own and closes back to one address;
  - translation as two runs on the same texts, joined by an edge;
  - the caution that relations multiply faster than anyone can look at them.
- **§19.23 Limits of reduction, measured and kept:**
  - the exact bounds: log2(N/K) bits saved on each survivor, paid for by the rest and by the
    stack's description;
  - the "surface" as a front of description bits against what a stack keeps;
  - warping (the orderings) moves length between units and creates none; a guided line's floor is
    its model's entropy;
  - a ledger of arrangements kept with the releases, proposed as a round to build.
- **§19.24 The other side of the library is addressing, not a trick:**
  - a shared prediction is a library agreed in advance;
  - its misses steer the choice of filters, as tailoring does;
  - "usefulness" as a measured percentage: random survivors of a stack, judged, with the ones that
    pass listed as demonstration addresses.
- **§19.25 A model's probabilities over every line:**
  - a guided ordering is already one model's probability laid over a line;
  - a model at temperature 0, or with a fixed seed, is a function, which is what lets it take part
    in an address;
  - many models combined are again one nameable model;
  - format conversion and decompilation as modelling, as good as the models that do not exist yet.
- **§19.13:** now notes that the window is the CHAT tab.

## README and HANDOFF
- **README:** the AI line's paragraph says CHAT is a tab.
- **HANDOFF:** a new entry.

## Checked here
- **Unit tests:** 76,335 checks, 0 failures (once, as this machine has no hardware SHA-256; about twice that on yours).
- **The chat lines in CI,** run on the tab:
  - a file given, two figures drawn and a message sent;
  - Ctrl+G on a figure;
  - Ctrl+J on a text file;
  - Esc back to ITEM, then C round the tabs;
  - the AI line's `--talk`.
