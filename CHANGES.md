# The web dictionary, measured (3 November 2026)

Nothing in Sieve itself changes in this update. It adds a measurement and the script that made it.

- `Sieve/results/web-dictionary-2026-11-03.md`: dictionaries learnt from MDN's CC0 example
  repositories and from MIT/BSD web libraries on npm, weighed against brotli on pages none of them
  were learnt from. The tests were 258 pages from 2014, your 11 site pages, and MDN's learning-area.
  Mixing primed on the MDN 64 KiB dictionary came out 10.6% under brotli on the 2014 pages and
  18.1% under on yours, and smaller than brotli on every one of the 423 pages.
- `Sieve/tools/measure_web_dictionary.py`: the script, to repeat it.

Building it needs a new format (mixing primed on a registered dictionary). A decision is waiting
for you on whether that goes in v33 or v34.
