# The viewer's views: the picture, the cover and the title, and saving each (relative to origin/main 74d66a5)

To apply it, do one of these from the Potentia repository root:
- unzip `files/` over the repository;
- or run `git apply viewer-views.patch`.

No new files.

## Views
The viewer opens on the thing itself, as before. A row of buttons along the top now shows what else
was made of it on the way to the shelf. Click a button, or press **Tab** (**Shift+Tab** goes back).

| Line | The thing itself | Other views |
| :--- | :--- | :--- |
| Pages | TEXT | PICTURE, TITLE |
| Image | PIXELS | PICTURE, TITLE |
| Video | FRAMES | PICTURE, COVER, TITLE |
| Audio | NOTES | PICTURE, COVER, TITLE |
| Books | PAGE | PICTURE, COVER, TITLE |
| Models | .OBJ | PICTURE, TITLE |
| Binary | HEX | PICTURE, TITLE |

TITLE appears when the item has a title. A book always has one: its title page.

- **PICTURE:** the picture on the item, drawn again by the same code that draws it on the shelf.
  - **How large:** as wide as its letters need, or as wide as your screen (rounded up to a power of
    two), whichever is wider. It is held to what fits the display cache.
  - **Example:** a 4,000-character page on a 1280-wide screen is drawn at 2048 × 2926.
  - **Where it's drawn:** in the background, so a long page doesn't hold up the window. The view
    says "drawing the picture..." until it arrives.
- **COVER:** a book's, a track's or a film's cover, at its own pixels.
- **TITLE:** the title laid out as a page. A book's whole title page is long enough to need this.

Each view zooms and scrolls like the rest of the viewer. A book's page still turns with N and B.

## Saving from any view
**F**, or the button at the right of the row, saves what is shown. The button names what it will do.

| View | F saves |
| :--- | :--- |
| The thing itself | The item, as F on the item page saves it: text, PNG, MIDI, .obj, or the file's bytes |
| PICTURE | The picture on the item, as a PNG at one pixel a pixel (`-picture.png`) |
| COVER | The cover, as a PNG (`-cover.png`) |
| TITLE | The title as text (`-title.txt`) |

**The file name** is the item's own, with the suffix above. The save dialog is the same one F
already uses.

**`--save-view PATH`** saves what the viewer shows without the dialog, for scripts and CI.

## Also changed
- **The book's heading** now shows the start of its title and PAGE n OF m. It was blank.
- **`display_text_here()`** is split out of `display_px_here()`, so the viewer sizes the picture as
  the shelf does.

## Checked
- **Every line:** each opens and cycles through its views (screenshots in the zip).
- **Saving:** done from each kind of view.
  - A page's picture: a 2048 × 2926 PNG.
  - A book's cover: a 10 × 10 PNG.
  - A book's title: text.
  - The thing itself: the item's own text, as before.
- **CI, run locally:** the hallway, models and books steps pass. The hallway step has new checks: it
  saves a page's picture and a book's cover as PNGs and a book's title as text, all through
  `--save-view`. The click check now expects "viewing its TEXT".
- **Not run this round:** the core tests. The core is unchanged.
