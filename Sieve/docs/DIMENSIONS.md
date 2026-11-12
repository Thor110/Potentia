# Dimensions

Every dimension of Sieve, whether it is built or not, is written down in one place: the catalogue in
`core/src/dimensions.cpp`. `sieve dimensions` prints it, and `sieve dimensions ID` shows one dimension
with the dimensions that link to it. This page explains the catalogue; the catalogue itself is the
source, so when the two differ, the code is right and this page is out of date (IDEAS §19.162).

## What a dimension is

A dimension is a state space with addresses of its own. Each entry in the catalogue says four things:

- **What one address is:** a unit of text, a mesh, a character's seed and settings, a sequence of bases.
- **Its roof:** the most it holds as built (or as planned). Every vessel states its upper end, and a
  richer original is carried at the most the roof allows, with the rest named rather than dropped
  silently (§19.159).
- **Whether it is built:** *built*, *partly built*, or *planned*.
- **Its links:** the dimensions it leads to. The links are the lattice's highways (§19.145): DNA to
  research, characters to species to DNA, every line to the binary line beneath it.

A new dimension is added to the catalogue first, with its links, so the map is never behind the work.
The unit tests check that every link names a dimension that exists, that every dimension is linked from at least one other, and that following links from any dimension reaches the pages line: the wiki principle of IDEAS §19.165, held for the dimensions themselves.

## The dimensions, by kind

**The hallway's eleven lines** (SPECIFICATIONS §3), all built: pages, image, audio, video; the books,
tracks and movies composed of them; models, worlds, AI and binary.

**Built since, from 12 November 2026:**

- *Characters:* a seed and settings that unfold a tree of spheres, each sphere holding a model
  (§19.147-19.159; SPECIFICATIONS §12.0j).
- *Species:* a character's settings with the seed left free (§19.152).
- *Personal dimensions* (partly built): your store of characters, read back as a line of your own
  (§19.154, §19.157). Next: a dimension as a list of its characters' ids, with an id of its own, so
  that all personal dimensions form one state space (§19.161).
- *Importers* (partly built): existing files brought onto a line. `.obj`, `.gltf` and `.glb` come in
  as held models (`sieve import`, and Ctrl+I on the CHARACTER page); a glTF's first skin comes in as a
  skeleton (`sieve-character-v3`, Ctrl+K) and its animations onto the animation line.
- *Animation* (built, §19.170): `sieve-animation-v1`, each bone's turn from rest per frame at 30 a
  second, 256 steps a number, so every animation is an address; played on any character. The roof:
  64 bones, 300 frames, turns only.
- *DNA* (partly built): a sequence of the four bases is a unit of the pages line over the `dna4`
  alphabet (`a c g t`; capitals read as these, anything else dropped). Try
  `sieve warp --alphabet dna4 --length 12 GATTACA`. A reader of FASTA files, which would skip the
  header lines, is not built yet.

**Planned**, each where it was first written about:

- *Research* (§19.162): findings, each linked to what it rests on; it grows while research is open,
  and once everything known is catalogued it pops into a hallway of links between all knowledge.
- *Languages* (§19.22, §19.59, §19.152), *emotion* (§19.144), *theoretical law* (§19.164), *board
  games* (§19.171: boards, pieces, rules and artwork across genres and franchises).
- *Formats* (§19.165, partly built): every format in the widest sense, from file formats through
  scripts and languages to emoji, as one web of entries that all lead to one root. Stage 1 is
  `sieve catalogue` (166 entries gathered from the code); the plan is
  [CATALOGUE-ROADMAP.md](CATALOGUE-ROADMAP.md).
- *Simulations* (§19.166): every world to visit, catalogued and reachable.
- *Games and their generators* (§19.104, §19.161, §19.163): each generator (procedural worlds,
  random dungeons) a sub state space of seeds and settings.
- *Other people's* and *group* dimensions (§19.157): these wait on sharing between machines.

## Formats

The importers are one layer (`core/include/sieve/importers.hpp`): each reads one format into the same
plain mesh and says what else the file held. `sieve import --formats` lists them, each with what it
carries and what it leaves out. The next ones, in order: glTF's skins and animations onto an
animation line; then FBX and the old game formats of `engine/` (§19.159).
