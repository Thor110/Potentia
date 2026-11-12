# Changes since b87b9b4 "sieve state space delta save preservation infinite raid system" (12 November 2026)

Three commits (8a15014 to 9432455): the RAID's authorship secured in its licence; the infinite RAID, stage 2; one idea noted.

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

## Ideas
- **Sieve/docs/IDEAS.md §19.178:** the RAID's lockdown against spam before stage 3 shares storage between machines: one character per verified identity, each dimension's own limits, and what is still to decide (how identity is verified while kept private, the quotas, what happens over a limit).
