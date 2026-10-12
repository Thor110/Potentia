# Manifest v5, stage 1: items carried as their places

Apply after ai-language.zip, nav-random.zip and judge-only.zip, in that order. The shared files
here carry their changes too.

Your choice was stage 1: `sieve install` and the hallway install v5, and the small sieve-install
stays small.

## The format (SPECIFICATIONS §12.2)
v4, plus:

```
line <K> <space id> <options>     each of Sieve's lines the files use, once
tree <sha256>                     one SHA-256 over the whole tree
f	<size>	-	item <K> <address>	<path>
```

- **Items:** a file that is exactly an item of a line is carried as its line and its positional
  address there, and made again on installing. That means a page as its text, a picture or film
  as its PNG, or a melody as its MIDI file.
- **Lines** are written with the options that make them, and checked by their space id. A change
  to a line's defaults can never quietly make another file.
- **Hashes:** an item's own SHA-256 is "-" (`--item-hashes` keeps it). The tree's SHA-256 covers
  every file. Files carried as bytes keep their own hashes and their v4 ways.

## Using it
- `sieve locate FOLDER --installer OUT.sieve --items` makes it.
- `sieve install` and the hallway install it.
- **The small sieve-install refuses it, and says why:** "these Sieve instructions carry items of
  Sieve's lines (...): open them with Sieve (sieve install, or the hallway's File Locator), which
  makes them again". With `--yes` it now prints its reason when it fails.

## Measured: the degree sample
3,960 files, 1,368 of them items (pages, pictures, films, melodies). 443,969 bytes, against
v4's 482,491 (8% smaller). Installed, every file is the same.

## Not yet (stage 2)
- **Ranges** ("bearings 0 to 359"). Most of what's left is the listing: paths, and the other
  files' hashes.
- **Items of the other lines** (books, tracks, movies, models, worlds, AI). Their file-making
  code is in the hallway (item_save.cpp), so it has to move into a shared library first.
- **The File Locator making v5.** It makes v4 today; it installs v5.

## Checked here (Linux)
- **The degree sample as v5:** installed with `sieve install`, `diff -r` identical.
- **The oracle's unpack** reads v5 from the definition. It makes the pages itself, takes PNG and
  MIDI items from a folder, and checks the tree hash: identical.
- **The small installer** refuses with its message.
- **v4 and v3:** still round-trip through `sieve install`, sieve-install, the oracle, and the v4
  degree sample.
- **CI lines** for all of the above.
- **Not run:** the full unit suite. The core change is one field on FilterSpec; the rest is CLI
  and client.
