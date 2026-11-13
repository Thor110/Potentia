# Changes: favourites, in every dimension and in one list (IDEAS §19.190)

## In the hallway
- **H** with an item in hand keeps it as a favourite of your personal dimension; H again lets it go. The item page's key line now shows "H: favourite".
- A page with no title is named by its own text.
- Files and models' weights can't be favourites yet; the hallway says so if you try.

## What's stored, beside the settings file
- **`favourites/SHA.item`** (`sieve-favourite-v1`) holds the line, the line's shape, the title and the item's digits. It's a reference, never a copy.
  - SHA is the SHA-256 of the shape and the digits, so the same item kept twice is one favourite.
  - A file changed by hand no longer matches its name, so it isn't read.
- **`worlds/personal.favourites`** (`sieve-favourites-v1`) is your dimension's list, one SHA per line, in the order kept.
  - It's a separate file, so keeping a favourite never rewrites the world file.

## In your dimension
- The catalogue (Tab) lists your favourites after Sieve's shelf items.
- A placed favourite stands as its line's shelf item, written as `place fav:SHA ...` in a v2 world.
- **O** on it opens the hallway on its line with the item in your hand.
  - This works when the line is shaped as it was when you kept it. If not, it tells you how the line was shaped.

## One list of them all
- **Main menu > Personal Dimensions > Favourites** lists every favourite: each dimension's, plus the Media Player's melodies.
- Each row says where it's kept and which line it's from.
- The list is worked out each time it opens, never stored, so it can't drift from the dimensions' own lists.

## Other
- `--open-favourite SHA` opens a favourite (kept beside `--settings`) for a picture.
- Catalogue entries `sieve-favourite-v1` and `sieve-favourites-v1`. The catalogue now has 203 entries, and "document" has 9 doors.
- Docs:
  - SPECIFICATIONS §12.0r gives both formats;
  - MANUAL covers favourites;
  - IDEAS §19.190 is marked built.
- CI step 10 checks:
  - keeping a favourite;
  - its file and title;
  - placing and opening it in your dimension;
  - the combined list;
  - opening it in the hallway;
  - letting it go.

## Not done yet
- Each favourite's own cover on its shelf item.
- Files and models' weights as favourites.

## Files to delete by hand
- None.
