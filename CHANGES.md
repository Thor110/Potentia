# Item models for worlds and AI; entering a world, noted

Against `feea76d` (panel scroll).

## Item models
- **Worlds:** already had its own copy of the models line's crate (`book-worlds.obj` and
  `book-worlds.mtl`) and a `[worlds]` section in `faces.ini`, from the worlds batch.
- **AI:** `book-ai.obj` and `book-ai.mtl`, copies of the crate, and an `[ai]` section in
  `faces.ini`. Nothing loads them until the AI dimension exists.
- **The line's id:** I assumed `ai`. If the AI dimension takes another id, the two files, their
  `mtllib` line and the section are renamed with it.
- **The models line has no other models of its own:** its hallway, shelves and markers use the
  templates, so the crate is the only thing to copy.
- **`templates/README.md`:** worlds and AI added to the table of per-line copies, and to the list
  of item models already there.

## IDEAS §14: entering a world, and a games dimension
- **Your idea:**
  - an ENTER button on worlds, to fly around a world in ghost mode;
  - a far-future games dimension built from worlds, models, UVs (algorithm plus delta) and code
    manifests that compile themselves or are already compiled, tested first with Sieve itself;
  - translation layers that reduce games to their smallest footprint.
- **Weighed:**
  - Ghost mode is near, since a world's mesh is built already.
  - A real game's level needs later world-space versions: placement off the grid, scale, textures
    and more models.
  - Code costs about its compressed size, and "compiles itself" needs a reproducible build.
  - The footprint falls where assets are generated or close to it.
- **Already there:** UVs and the game engine (§14), the game-format parsers (§7.1), and manifests
  (§4, §16.3). The section points to them.

## Docs
- **HANDOFF:** a short entry.

## Checked here
- **The copies:** the crate's geometry is identical to `book-models.obj`, line for line, apart from
  the header.
- **Not checked in the hallway:** there is no AI line to load `book-ai.obj` yet.
