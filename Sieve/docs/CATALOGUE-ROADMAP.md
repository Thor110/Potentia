# The catalogue of formats: a roadmap

Edward (12 November 2026, IDEAS §19.165): a roadmap "for tracking and recording all known and
recorded formats, from file formats out through to language and even pictograph or emoji like
language", into "a web or database of documentation and file format documentation, translation
metrics and recorded stats, pros, cons, etc for all things", where "all information links to all
other information one way or another, like the wiki principle where all pages lead to a certain place
or space"; recorded "dynamically and then procedurally ... across all dimensions of all things".

This is the plan. Stages 1 and 2 are built, and stages 3 and 4 in a first form (below); the rest is not. It is in the catalogue
of dimensions as `formats` (`sieve dimensions formats`). Every figure the catalogue holds will be
measured or sourced, as everywhere in Sieve; a pro or con with neither is not recorded.

## What counts as a format

A format is any agreed way of writing something down so that it can be read back. That covers:

- **File formats:** PNG, FLAC, glTF, `.sieve`, a game's WAD.
- **Encodings and containers:** UTF-8, base64, ZIP, Matroska.
- **Scripts and writing systems:** Latin, Devanagari, Han characters, Egyptian hieroglyphs, braille,
  SignWriting.
- **Languages:** English, Welsh, Esperanto, programming languages; dialects as parts of their language.
- **Notations:** musical notation, MIDI, chemical formulas, mathematical notation, DNA's bases.
- **Pictograph and emoji sets:** emoji, road signs, ideograms.

They differ in what reads them (a program, a person, both), but each has the same things worth
recording, so one catalogue holds them all.

## Already here

Sieve already records a good deal of this, in several places:

- `sieve/filekind.hpp`: the file kinds Sieve recognises from their first bytes (`file-kinds-v1`).
- docs/FORMATS.md and docs/FORMATS-PLAN.md: every format the packer handles, its route under each
  method, and what was measured.
- `sieve/importers.hpp`: the model importers (`.obj`, `.gltf`, `.glb`), each with what it carries and
  what it leaves out (`sieve import --formats`).
- `sieve/alphabet.hpp`: the built-in alphabets and the named Unicode blocks (scripts, emoji,
  pictographs, hieroglyphs), 211 listed by `sieve alphabets`, `dna4` among them.
- The AI line's corpora and the language notes of IDEAS §19.22 and §19.59.
- `sieve/dimensions.hpp`: the catalogue of dimensions, whose links already form a small web.

The first stage gathers these into one place rather than starting again.

## One entry

Each format is one entry, a small text record in the house style (one field a line), named by a
stable id (`png`, `utf-8`, `script-latin`, `lang-cym`, `emoji`) and checked by its SHA-256, as a
character is. A changed entry is a new version; the old one stays readable (§19.137). The fields:

| Field | What it holds |
| :--- | :--- |
| `id`, `names` | the stable id; the names it goes by, in any language |
| `kind` | file, encoding, container, script, language, notation, pictograph set |
| `recognised` | how a program tells it: signatures, extensions, media types; for a script, its Unicode ranges; for a language, its ISO 639 code |
| `documents` | where it is specified: standards, specifications, grammars, with their licences |
| `sieve` | what Sieve does with it: its route in the packer, its importer, the line or alphabet it lands on, or none yet |
| `stats` | measured figures, each with the date, the machine and the results file it came from |
| `translations` | to and from which other entries, and what is lost each way, measured: lossless, lossy with a stated error, or a translation score |
| `pros`, `cons` | each with its source or its measurement |
| `links` | typed links to other entries: *contains*, *encodes*, *written in*, *derived from*, *supersedes*, *translates to*, *part of* |
| `roof` | the most it can hold, as §19.159 asks of every vessel |

## The wiki principle, made checkable

Every entry links to others, and following links upward from any entry reaches one root, the entry
`information`. The catalogue can check this for itself: an entry with no path to the root has not
been linked yet. A test will walk the links and name any such entry, as the unit tests already check
that every link in the dimensions catalogue names a dimension that exists. `sieve catalogue path A B`
will show the shortest chain of links between two entries (PNG to DEFLATE to compression to
information, and back down to Welsh).

## Stages

1. **(Built 12 November 2026: `sieve catalogue`, core/include/sieve/catalogue.hpp. Gathered from the code itself
   rather than by hand, so it cannot fall behind it: 166 entries, 26 file kinds with their signatures,
   3 importers, 6 alphabets, 102 Unicode blocks, 8 of Sieve's own formats, under 21 general entries;
   every one reaches `information`, checked by `sieve catalogue --check` and the unit tests. Not yet:
   the packer's routes per format and the measured figures in each entry.)** **Gather what Sieve knows
   (by hand, from its own sources).** Turn the lists above into entries: the
   file kinds, the packer's formats and routes, the importers, the alphabets and blocks, `dna4`. Each
   entry's `sieve` field is filled from the code, and its `stats` from the results files that already
   exist. Add `information` and the first layer of general entries under it: compression, text, image,
   sound, language, writing.
2. **(Built 12 November 2026: SPECIFICATIONS §12.0m; `read_format_entry` checks every entry; entries
   written by hand in `data/catalogue/` join the code's, the first four being UTF-8, base64, English and
   staff notation; `--search`; each entry shows the entries beneath it; `--check` and the unit tests
   read every entry back.)** **The record and the command.** `sieve-format-v1` as a written specification (SPECIFICATIONS), a
   reader in the core, and `sieve catalogue` to list, show, search by any field, follow links, and find
   paths. Unit tests: every entry reads back, every link resolves, every entry reaches the root.
3. **(Built 12 November 2026, as a first form: the FORMATS page (at first called CATALOGUE) of the setup menu's right-hand panel,
   the entry you stand at with its doors in a ring round it, walked by Left/Right, Enter, Backspace and
   Home; L takes an alphabet or a Unicode block onto the pages line. Not yet: the catalogue as a
   corridor of the 3D hallway itself, and files' and models' entries leading onto their lines.)**
   **In the hallway.** The catalogue as a dimension: each entry a door, its links the corridors, the
   formats Sieve can open leading straight onto their lines (a PNG's entry onto the image line, a
   script's onto the pages line over its alphabet, `dna4` onto DNA).
4. **(Built 12 November 2026, as a first form: `sieve catalogue --measure FILE|FOLDER` packs each file
   alone as Sieve does, against LZMA2 (7z's method) and deflate, with the times to make and to install,
   and installs it back to check the round trip; `sieve import` records what each import kept against
   what the file held. Each is a `sieve-format-stat-v1` line (SPECIFICATIONS §12.0p) in the person's
   stats folder, outside the repository, naming no file; `sieve catalogue ID`, `--measured` and the
   SYSTEMS page add them up. Not yet: the hallway's own packing and imports adding theirs, and figures
   gathered from many machines.)** **Measured, as it is used.** Each time Sieve packs, imports or reads a format, its figures can be
   added to that entry's `stats` (size against 7z, time to make and to install, what an importer
   kept), so the catalogue fills from use rather than from claims. Translation metrics come the same
   way: a round trip measured (PNG to its pixels and back: lossless; glTF to a held model: 8 of N
   vertices kept).
5. **Grown procedurally, from open registries.** Importers for public registries, each stating its
   licence and its roof, and each entry it adds marked with where it came from. Candidates: the UK
   National Archives' PRONOM (file format signatures), the Library of Congress's format descriptions,
   IANA's media types, the Unicode Character Database and CLDR (scripts, emoji, locale data), ISO 639
   (language codes), Glottolog and WALS (languages and their features), and Wikidata. Each licence is
   to be checked, and the choice to take one in is Edward's, before anything is copied into the
   repository.
6. **Languages, scripts and pictographs in depth.** Beyond the codes: each script's alphabet as a
   Sieve alphabet (many already are), each language's corpus where one can be had openly (§19.59), and
   translation metrics between languages measured with a stated method rather than asserted.
7. **The far end.** The catalogue joined to the research dimension (§19.162), so that a finding about
   a format links to it, and to the law dimension (§19.164) for the rules that govern formats
   (standards, patents, licences). Every format any dimension uses has an entry, and every entry is a
   door.

## Rules kept throughout

- No figure without a measurement or a source; no pro or con without either.
- Licences recorded per entry and per source; nothing copied in that its licence does not allow.
- Game and personal data never enter the catalogue (the player's own files are theirs); a game
  format's entry describes the format, not anyone's files.
- Old entries stay readable; a change is a new version.
