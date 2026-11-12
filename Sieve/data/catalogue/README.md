# The catalogue of formats: entries written by hand

Each `.format` file here is one entry of the catalogue of formats (`sieve catalogue`; IDEAS §19.165;
docs/CATALOGUE-ROADMAP.md), written by hand: a format the code does not name for itself, such as a
language, an encoding or a notation. The file is named by the entry's id, and its text is exactly
`sieve-format-v1` as SPECIFICATIONS §12.0m writes it out; `sieve catalogue --check` reads every one,
and refuses one that is not in its canonical form, that repeats an id already in the catalogue, or
that links to an entry that is not there.

Every entry needs a `link part-of` chain up to `information`, through the general entries
(`sieve catalogue --kind general` lists them). Add only what is true: where Sieve does nothing with a
format, its `sieve` line says so.
