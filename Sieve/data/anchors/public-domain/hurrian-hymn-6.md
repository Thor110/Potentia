# Hurrian Hymn No. 6 (h.6), the Hymn to Nikkal

Clay tablet RS 15.30 + 15.49 + 17.387, Ugarit, about 1400 BCE (National Museum of Damascus).
Information sourced from https://virtual-museum-syria.org/damascus/56-tablet-musical-staff/ and
then reproduced under Sieve architecture. The tablet is about 3,400 years old and in the public domain.
This reading of it is our own.

## The tablet's notation
Interval names, each followed by a number. "uš-ta-ma-a-ri" isn't an interval name, so it's left out.
"1 + 1" is read as 2.

    qáb-li-te 3  ir-bu-te 1  qáb-li-te 3  ša-aḫ-ri 1  i-šar-te 10  uš-ta-ma-a-ri
    ti-ti-mi-šar-te 2  zi-ir-te 1  ša-aḫ-ri 2  ša-aš-ša-te 2  ir-bu-te 2
    um-bu-be 1  ša-aš-ša-te 2  ir-bu-te 1 + 1  na-ad-qáb-li 1  ti-tar-qáb-li 1  ti-ti-mi-šar-te 4
    zi-ir-te 1  ša-aḫ-ri 2  ša-aš-ša-te 4  ir-bu-te 1  na-ad-qáb-li 1  ša-aḫ-ri 1  ša-aš-ša-te 4
    ša-aḫ-ri 1  ša-aš-ša-te 2  ša-aḫ-ri 1  ša-aš-ša-te 2  ir-bu-te 2  ki-it-me 2
    qáb-li-te 3  ki-it-me 1  qáb-li-te 4  ki-it-me 1  qáb-li-te 2

## How it was read
- **Each name is a pair of strings** on the lyre, from the Akkadian string-pair list:

  | Name | Akkadian | Strings |
  | :--- | :--- | :--- |
  | qáb-li-te | qablītum | 5–2 |
  | ir-bu-te | rebûttum | 2–7 |
  | ša-aḫ-ri | šērum | 7–5 |
  | i-šar-te | išartum | 2–6 |
  | ti-ti-mi-šar-te | titur išartim | 3–5 |
  | zi-ir-te | zerdum | 4–6 |
  | ša-aš-ša-te | šalšatum | 1–6 |
  | um-bu-be | embūbum | 3–7 |
  | na-ad-qáb-li | nīd qablim | 4–1 |
  | ti-tar-qáb-li | titur qablītim | 2–4 |
  | ki-it-me | kitmum | 6–3 |

- **The number is how many times the pair is played.**
- **The tuning is nīd qablim**, as the colophon says. Strings 1 to 7 are C4 D4 E4 F4 G4 A4 B4.
  In this tuning 4–1 is a perfect fourth, and the one tritone is 7–4 (pītum). The hymn never uses
  pītum, or 1–5.
- **Two voices.** Voice 1 is the higher string of each pair and voice 2 the lower.
- **Rhythm.** The tablet gives none, so every pair is a quarter note.

## In Sieve
- **Line:** audio, `--note-set notes2 --voices 2 --length 73` (C3–C6, all eight durations).
- **Space:** `notes2/C3-C6/seEqQhHw/V2/L146/key=sieve/feistel-sha256-v1`.
- **Size:** 73 pairs per voice, one unit.
- **Positional address:**
  `09de94ed626d3e821bee71e9b6113e8a8b65111d9e583c0f76dacdee020e0d21352973478b69a13cae4daada744fb1c902a8e912ba2a49232cf813e1ca6db1dbea9d368d971a1036b142f15975b37562c34dc723f69d426e5093b30d62ed0be37d6c79fc1a9b44df15b0968aa8767f74fc67c1102342cf63c4a5910dc7b6f6e8e6605ee0dda8e4427fc3d0d783d5fddf54c6cc1bf8258b`
- **Files:**
  - `hurrian-hymn-6.notes` is the notation that was warped (SHA-256 e3c10a4ad915ab455b031a45171bbbd2be58a93294e0ca51ccbe2166b3da6bc0).
  - `hurrian-hymn-6.mid` was saved by `sieve read` from that address (SHA-256 2b11a436d4ea36e4a9e50d83724bf237a5d06929d37c56141855e1ec97991c9f).

Reproduce:

    sieve warp --line audio --note-set notes2 --voices 2 --length 73 --file hurrian-hymn-6.notes
