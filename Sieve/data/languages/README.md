# The language catalogue: spoken languages, written by hand

Each `.format` file here is one entry of the language catalogue (IDEAS §19.183): a spoken language or
a language family, in the same form as the catalogue of formats (`sieve-format-v1`, SPECIFICATIONS
§12.0m), named by its id. The two catalogues are one web: a language links to the scripts it is
written in (`link written-in script-...`), which are entries of the catalogue of formats, and the
System Viewer opens either one (FORMATS stands at `information`, LANGUAGE at `language`).

Every entry leads up by `link part-of` to `language`, through its family. Its `recognised` line gives
its ISO 639 codes; its `sieve` line says only what Sieve does with it now (for most, that its script
can be taken onto the pages line). `sieve catalogue --check` reads every one.
