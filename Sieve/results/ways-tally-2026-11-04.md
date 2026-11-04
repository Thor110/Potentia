# Which ways carried the files: Silesia, Sieve's docs and the web test site (4 November 2026)

Made with Sieve 0.13.77 at v33 on the Linux machine, each Silesia file alone and each folder whole, all with Try every way (`sieve locate PATH --installer OUT --search-per-file 0 --search-every-wave`); then `python tools/ways_tally.py FOLDER --sieve SIEVE`. Every `.sieve` was installed by the tally to measure its files. Proof/STATISTICS.md, *What carried each file*, puts these beside Edward's own benchmark.

14 `.sieve` file(s), 215,106,336 bytes of files in 43,247,842.

| Way | Whose | Files | Their bytes | Own codes | LZMA2 alone (same files) | Saved against LZMA2 |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: |
| mixing, files mixed together as a group (`mixt`) | Sieve's (context mixing) | 42 | 2,689,351 | 1,088,596 | 1,222,601 | 134,005 (11.0%) |
| mixing, alone (`mix4`) | Sieve's (context mixing) | 17 | 211,949,192 | 41,712,112 | 48,460,181 | 6,748,069 (13.9%) |
| LZMA2, in the shared lzma2 stream (`raw`) | 7-Zip's (LZMA2) | 6 | 467,793 |  |  |  |

Streams (shared by their files: one file's share of a solid stream is not recorded):

| Stream | In the `.sieve` | Holding |
| :--- | ---: | ---: |
| rope | 42,800,764 | 42,800,764 |
| lzma2 | 445,395 | 467,793 |

Each `.sieve` (its main way: the one that carried most of its bytes):

| `.sieve` | Version | Entries | Files' bytes | Its bytes | Main way | Own codes | LZMA2 alone (same files) |
| :--- | :--- | ---: | ---: | ---: | :--- | ---: | ---: |
| sieve-docs.sieve | sieve-manifest-v33 | 42 | 2,689,351 | 1,089,194 | `mixt` | 1,088,596 | 1,222,601 |
| silesia-dickens.sieve | sieve-manifest-v33 | 1 | 10,192,446 | 2,205,961 | `mix4` | 2,205,886 | 2,831,149 |
| silesia-mozilla.sieve | sieve-manifest-v33 | 1 | 51,220,480 | 13,318,582 | `mix4` | 13,318,505 | 13,376,180 |
| silesia-mr.sieve | sieve-manifest-v32 | 1 | 9,970,564 | 1,967,650 | `mix4` | 1,967,581 | 2,751,831 |
| silesia-nci.sieve | sieve-manifest-v33 | 1 | 33,553,445 | 1,281,583 | `mix4` | 1,281,517 | 1,449,209 |
| silesia-ooffice.sieve | sieve-manifest-v33 | 1 | 6,152,192 | 1,864,843 | `mix4` | 1,864,765 | 2,427,163 |
| silesia-osdb.sieve | sieve-manifest-v33 | 1 | 10,085,684 | 2,364,331 | `mix4` | 2,364,257 | 2,844,493 |
| silesia-reymont.sieve | sieve-manifest-v33 | 1 | 6,627,202 | 1,089,497 | `mix4` | 1,089,427 | 1,315,531 |
| silesia-samba.sieve | sieve-manifest-v33 | 1 | 21,606,400 | 3,277,403 | `mix4` | 3,277,334 | 3,739,464 |
| silesia-sao.sieve | sieve-manifest-v33 | 1 | 7,251,944 | 4,149,429 | `mix4` | 4,149,347 | 4,425,604 |
| silesia-webster.sieve | sieve-manifest-v33 | 1 | 41,458,703 | 6,226,845 | `mix4` | 6,226,770 | 8,368,609 |
| silesia-x-ray.sieve | sieve-manifest-v33 | 1 | 8,474,240 | 3,614,947 | `mix4` | 3,614,864 | 4,491,203 |
| silesia-xml.sieve | sieve-manifest-v33 | 1 | 5,345,280 | 349,106 | `mix4` | 349,036 | 434,830 |
| test-site.sieve | sieve-manifest-v33 | 11 | 478,405 | 448,471 | `raw` | 2,823 | 4,915 |
