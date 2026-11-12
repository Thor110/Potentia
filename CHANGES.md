# Changes since b87b9b4 "sieve state space delta save preservation infinite raid system" (12 November 2026)

Eleven commits (8a15014 to ce94ca3). This zip replaces the earlier zips since your last push. The RAID's authorship secured in its licence; the infinite RAID, stage 2; the catalogue of formats, stages 1 to 3; seven ideas noted.

**Build:** a new core source, `core/src/catalogue.cpp`, is in CMakeLists.txt, so CMake must run again.

## The RAID's authorship (8a15014)
- **The licence, as I chose it** (you left the choice to me): the RAID stays under the GNU AGPL v3, as everything else is (you had already published it under that), with the one additional term the AGPL allows and protects, its section 7(b): whoever passes the RAID's files on must keep the notice "The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)" in them, and show it with the work's notices. Unlike other added terms, this one cannot be removed.
- **The charity share** is written as a request, not a condition: the AGPL lets anyone remove a binding restriction of that kind, so a binding one would mean leaving the AGPL for this part (possible for new code, but for a lawyer to draft, and no longer open source). A permissive licence would let others take the RAID closed; the AGPL keeps it open and now keeps your name on it.
- **Where it is:** **Sieve/docs/RAID-NOTICE.md** (new: the notice, the request, and why); the top of Sieve/core/include/sieve/raid.hpp and Sieve/core/src/raid.cpp; `sieve help raid`; Sieve/README.md's Licence section; IDEAS §19.177.

## The infinite RAID, stage 2 (56d8128)
- **Cuts made by the content** (`sieve-raid-cut-v2`; SPECIFICATIONS §12.0l): a save is cut into chunks of 16 to 256 KiB where its content says, so an edit moves only the cuts near it, and a new version shares all its other chunks with the one before. Measured on a 4 MiB save kept twice:
  - one byte changed in the middle: the new version added 412,078 bytes (1 MiB pieces, as in stage 1: 2,098,024);
  - ten bytes inserted near the start: 1,000,026 (stage 1: 8,389,648). Two pieces were new: the first had reached the 256 KiB cap, a cut that does not follow the content.
  - chunks averaged 80,971 bytes on random data.
  Stores written in stage 1 read as before.
- **Your character kept as versions:** with `[raid] drives = D:/raid,E:/raid` in the settings file, each character saved with Ctrl+S on the CHARACTER page is also kept in the RAID as a new version of "your character" (its animation as "your character's animation"), each version naming the one before. `--save-character FILE` does it for one run, and the readout gives the page's note.
- **Tests:** the unit suite passes (178,260 checks; the cuts' bounds, sameness, and an insert costing at most two pieces); CI step 4 passes (122 s) and step 10 passes (954 s), with the RAID filled from the menu and checked whole.

## The catalogue of formats, stage 1 (1e5e585)
- **`sieve catalogue`** (Sieve/core/include/sieve/catalogue.hpp, Sieve/core/src/catalogue.cpp; MANUAL; docs/CATALOGUE-ROADMAP.md): every format Sieve knows, gathered from its own code so it cannot fall behind it. 166 entries:
  - 26 file kinds, each with the signature it is told by;
  - the 3 model importers;
  - the 6 built-in alphabets and 102 Unicode blocks (emoji and hieroglyph blocks under pictographs, music and maths under notation);
  - 8 of Sieve's own formats (.sieve, characters v1 to v3, species, animations, RAID manifests, catalogue entries);
  - all under 21 general entries (text, image, sound, writing, compression...), each leading up to one root, `information`.
- Each entry as text (`sieve-format-v1`), its SHA-256 the version. `sieve catalogue file-flac` shows one with its lineage (file-flac, sound, data, information); `--kind file` lists one kind; `--path file-png,alphabet-dna4` finds how two meet; `--check` confirms every entry reaches information (the wiki principle: all 166 do).
- Not yet: the packer's route and the measured figures for each format (stage 4 of the roadmap).
- **Tests:** `test_catalogue` (ids unique, every link resolves, every entry reaches information, every file kind has an entry, FLAC's entry pinned); the unit suite passes (179,334 checks); CI step 4 passes (143 s).

## The catalogue of formats, stage 2 (a044458)
- **The specification:** SPECIFICATIONS §12.0m writes `sieve-format-v1` out, and `read_format_entry` checks an entry is in its canonical form (fields in order, an id of lower-case letters, digits, `-`, `.`, `_`).
- **Entries written by hand:** **Sieve/data/catalogue/** (new), one `<ID>.format` file an entry, for formats the code does not name; the first four are UTF-8, base64, English and staff notation, each saying plainly what Sieve does with it (staff notation: nothing yet). An id that is already there, or a link to an entry that is not, is refused; a README there says how to add one. The catalogue is now 170 entries.
- **The command:** `sieve catalogue --search TEXT` (any field, any case); an entry now lists the entries beneath it; `--check` also reads every entry back from its text.
- **Tests:** every entry reads back the same; refusals (out of order, a bad id, a missing line feed, an id twice, a link to nothing, a file named otherwise); a file with Windows line endings is still read. The unit suite passes (180,024 checks); CI step 4 passes (131 s).

## The catalogue of formats, stage 3 (558d5ad)
- **FORMATS, the right-hand panel's third page** (PgUp/PgDn to it; Sieve/client/menu.cpp): the catalogue as a ring of doors. The entry you stand at is in the middle, its doors round it: what it is part of in gold, what is part of it in grey, other links in blue. Left/Right choose a door, Enter goes through, Backspace goes back, Home goes to the root; **L** takes an alphabet or a Unicode block straight onto the pages line as its alphabet (at Greek: the pages line becomes Greek, 144 symbols). The doors stand down the ring's two sides, at most 24 at once, with a window round the chosen one where there are more (writing has 104). `--catalogue-entry ID` opens it at an entry; the readout says where you stand and how many doors.
- **The panel mirrors the settings panel:** its inner edge drawn full height, and a scroll bar on that inner edge wherever a page holds more than fits: YOURS now scrolls (wheel, or its rows, the chosen one kept in view); the ring's bar shows where its window of doors sits.
- **The hand-written entries** are copied beside the programs when they are built, and both programs look for them there (CMakeLists.txt).
- **Tests:** CI step 10 walks it (the ring at image, eight doors; through to document; Home; L at Greek) and passes (975 s).

## Ideas
- **Sieve/docs/IDEAS.md §19.178:** the RAID's lockdown against spam before stage 3 shares storage between machines: one character per verified identity, each dimension's own limits, and what is still to decide (how identity is verified while kept private, the quotas, what happens over a limit).
- **Sieve/docs/IDEAS.md §19.179:** the RAID between machines as peer to peer with a few known nodes to start from; IP addresses as a first limit (weak, and personal data, so kept as salted hashes), then digital ID, then a character's own record, each step added without removing the one before.
- **Sieve/docs/IDEAS.md §19.178:** your digital ID structure from Pages/order.pdf noted: the anchor, ten at a time, random groups of ten after 1,000, re-verification on doubt; it is the vouching kind of limit, and a project of its own, whose result (verified or not) the RAID would read.
- **Sieve/docs/IDEAS.md §19.180:** a decentralised place to talk and keep one's own record, built on the RAID: what it needs, and what has to be decided (who reads what, withdrawing a post, moderation).
- **Sieve/docs/IDEAS.md §19.181:** implementations side by side (your mesh scaler and your friend's), measured on the same inputs and steered towards their shortest form, with a program's packed size as the measurable bound on its Kolmogorov complexity; applications as entries of the catalogue.
- **Sieve/docs/IDEAS.md §19.182:** the catalogue as another node viewer, like the node graph for items and maps: one viewer for every web.
- **Sieve/docs/IDEAS.md §19.183:** a catalogue of spoken languages as a LANGUAGE page for characters, made the same way as FORMATS; and a System Viewer on the main and pause menus, opening FORMATS and LANGUAGE as the first two systems to chart. Spelling kept British: "catalogue" is the noun and the verb.
