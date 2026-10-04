# The item viewer: anything in hand, zoomed and scrolled both ways (relative to origin/main eb9d0bb)

To apply it, do one of these from the Potentia repository root:
- unzip `files/` over the repository;
- or run `git apply item-viewer.patch`.

One file is new: `Sieve/client/viewer.cpp`.

## What it does
The item page draws the thing at the page's size, so a long page stops at the foot of the panel and
a picture is only as large as the panel. Now **Z**, or a **click on the thing**, opens it over the
whole window, as large as you like.

| What is in hand | What opens |
| :--- | :--- |
| A page | All of it, a character to a cell, laid out like the picture on the item, only larger. Letters smaller than the **letters on items** size are drawn as bars, as on the item. |
| A picture | Its pixels, fitted to the window at first, then down to one to one or as large as you like. |
| A film | The same, playing. **Space** stops it, and **N** and **B** step a frame. |
| A book | The open page. **N** and **B** turn the pages here too. |
| A track | Its notes. |
| A model | Its `.obj` text. |
| A file | All of it, as a hex dump. |

**The click.** A click on the thing itself opens it.
- With the mouse free (Tab), that's where the pointer is.
- With the mouse held for turning, it's the crosshair in the middle of the screen.
- A click anywhere else puts the thing back, as before. So does a click on the other tabs.

## Controls
| Input | Action |
| :--- | :--- |
| Wheel | Scroll down and up |
| Shift + wheel, or a sideways wheel | Scroll across |
| Ctrl + wheel | Zoom about the pointer |
| Drag | Move it |
| Arrows or WASD | Move a tenth of a screen |
| PgUp / PgDn | Move a screen |
| Home / End | The top and the foot |
| + / - | Zoom (Shift: twice as far) |
| 0 | Fit it to the window |
| 1 | One to one |
| N / B | A book's page, or a film's frame |
| Space | Play or stop a film |
| Esc / Z | Close it, with the thing still in your hands |

**What the window shows.**
- **The heading:** what is open, the zoom, the frame, and the rows in view.
- **The foot:** the keys.
- **Scroll bars:** down the right and along the foot, where it doesn't all fit.

## Speed
- **Only what is in view is drawn.**
  - Text is drawn a row at a time.
  - A picture is sampled into one texture the size of the window whenever it moves, so the work is
    the same whatever the picture's size or the zoom.
- **The corridor isn't drawn while the viewer covers it.**
- **Measured on the software renderer:** a 30,000-character page in the viewer draws at
  16.7 ms a frame, at 200%, one to one and fitted. The corridor with the item page took
  36 ms a frame.

## Also changed
- **The item page's key hints** now include `Z / click it: view`. Putting it back is E or Esc.
  `msg.holding` no longer mentions a click.
- **`--press Click`:** a left click where the crosshair is, so scripts and CI can click.

## Checked
- **Every line in the hallway:** a long page, a picture, a film, a track, a book, a model and a file
  all open in the viewer (screenshots in the zip).
- **On a 4,000-character page:** zooming, End, fitting and the bars all work.
- **CI:** new checks in the hallway step, and that step passes locally, as do the models and
  books steps.
  - A click on a 4,000-character page in hand opens it in the viewer. It is laid out in 52 columns
    of 77 rows, which is 416 × 770 at one to one.
  - Z then Shift+= gives 400%.
  - Esc closes the viewer with the page still in hand.
- **Not run this round:** the core tests. The core is unchanged.
