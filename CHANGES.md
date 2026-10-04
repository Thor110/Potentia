# Item memory, measured world graphics, view distance, the widest picture and close-ups (relative to origin/main e1be880)

To apply it, do one of these from the Potentia repository root:
- unzip `files/` over the repository;
- or run `git apply view-and-graphics.patch`.

Two files are new: `Sieve/client/gpu_memory.hpp` and `Sieve/client/gpu_memory.cpp`.

## D. The item cache's share of memory is a setting
**The new row.** GLOBAL has a new row, ITEM MEMORY: the share of installed memory the items around
you may take.
- **Values:** 25% at first, 5% to 90%, in steps of 5. PgUp/PgDn step by 25.
- **Saved as:** `item_memory_pct` in [world]. On the command line, `--item-memory PCT`.
- **What follows it:** the item cache and the "too large" address limit.
- **The row shows** the share and how much memory it comes to here.

**The rows below it.** The line rows moved down one (`kFirstLineRow` is 16). CI moved with them,
and CI now checks that the new row saves.

## A. The world's graphics, measured rather than assumed
**What changed.** The graphics bar used to count a fixed 512 MB for everything but the item
pictures. Every texture the program makes now goes through `gpu::create` and `gpu::destroy`
(`client/gpu_memory.hpp`). These keep a running count of the bytes, split into the world and the
item pictures.

**How the world's figure is worked out.** It is the renderer's three frames, plus the larger of
two figures:
- **What the settings will make:**
  - Real Graphics' frame;
  - the door portals;
  - the seven door signs;
  - two picture-sized frames.
- **What the world holds now**, from the running count.

**Where the sizes come from.** The portal grain and the sign size now live in `world.hpp`. Both the
hallway and the menu read them from there.

**The result.** About 37 MB at 1080p, where 512 MB was counted before. So the display cache and
FIND MY LIMITS have about 475 MB more graphics memory to use.

## B. View distance (Settings > Graphics)
There are two new rows, after the FPS counter (screenshot: `view-distance-rows.png`).

| Row | What it sets | At first | Range | Command line |
| :--- | :--- | :--- | :--- | :--- |
| **View Distance** | Rooms drawn and kept either side of you, as many behind as ahead | 7 either side (it was 7 ahead, 6 behind) | 2 to 64 | `--view-rooms N` |
| **Picture Distance** | Rooms either side of yours whose items get pictures of their own | 1 | 0 to 8 | `--picture-rooms N` |

**Picture Distance's limit.** It never goes past the View Distance, and its help line says
so.

**Where they're saved.** Both are saved in [graphics].

**What follows them.**
- The constants they replace are gone: `kCacheBack`, `kCacheAhead`, `kTilesKept` and `kFaceRooms`.
  One `view_rooms()` in `menu.hpp` now serves both directions.
- At the default, 15 rooms are kept rather than 14: the one more behind you.
- The item cache follows them, and so does the line cache's estimate.
- So does the render window.
- FIND MY LIMITS' display cache follows them too.
- The setup menu's display cache row names how many rooms with pictures it's counting.

**Checked in CI.** A new CI check sets View Distance to 8 and Picture Distance to 3, then checks
that both save.

## C. The widest picture follows the display cache
**What changed.** `kMaxDisplayPx` (1,024 px) is gone. Displays still widen for their letters, but
only up to the widest power of two at which every picture of the rooms with pictures fits the
display cache. They are also never wider than the renderer's widest texture.

**The display size setting comes first.** The new limit only holds back the letters' widening. It
never pulls a display below the display size setting.

**Example: 4,000-character pages.** At this length the letters ask for 1,024 px (screenshots:
`long-pages-64MB-cache.png` and `long-pages-1024MB-cache.png`).

| Display cache | Before | Now |
| :--- | :--- | :--- |
| 64 MB (the default) | 1,024 px, about ten items with pictures, the rest stand-ins | 128 px, every item in the three rooms pictured |
| 1,024 MB | 1,024 px, about 170 items with pictures | 512 px, every item pictured |

Close-ups still draw the items nearest you wide.

**Other limits that went with it.**
- **The display size row and the close-up row** now go up to the renderer's widest texture. They
  stopped at 1,024 before.
- **The display cache** now goes as far as the graphics memory has room for. It stopped at 4 GB
  before, in the menu, the settings file and the hallway.

**FIND MY LIMITS and the display cache row.**
- FIND MY LIMITS sizes the cache for the widths the letters ask for, so a bigger cache is what
  brings back the wide pictures.
- The display cache row's "rooms: N MB" shows the same figure: what the letters ask for, which the
  current cache may hold back.

## E. Close-ups follow the screen and the graphics memory
**The size: "screen" by default.** The close-up display size now starts at **screen**: your
screen's width, rounded up to a power of two.
- **What that comes to:** 2,048 px at 1920 × 1080 and 4,096 px at 4K, where 1,024 was the default.
- **Why the screen:** a close-up is drawn at the width the item shows on screen, so pixels past the
  screen's own width are never seen.
- **The row:** it goes off, screen, then 256 px up to the renderer's widest texture.
- **The setting:** saved as `close-up = screen`. `--close-up screen` or `--close-up PX`.

**How many: what the memory holds.** They were 24, whatever the memory. Now as many are kept as fit,
at the close-up size, in the graphics memory left after the world and the display cache.
- There's at least one, and never more than a room's items.
- The hallway is now told the Graphics Memory setting, and counts the world's textures as they are.

**What the graphics bar counts.** Kept that way, close-ups would show as filling all the leftover
memory, so the bar counts what the screen can actually show of them.
- **Why it's bounded:** each close-up is drawn under twice as wide as it shows, and the items on
  screen don't cover each other. So together they come to under four screens, and twice that
  allows for the ones kept a couple of seconds after you look away.
- **What that comes to:** 63 MB at 1920 × 1080, and 253 MB at 4K.
- **The row reads,** for example, `screen 2048 px, up to 128 items (39 MB)`.

**The display cache and FIND MY LIMITS.** When they size the display cache, they keep room for one
close-up.

**Readability.** Standing at a shelf, the pages in front of you are redrawn sharp. A page reads
once it shows wide enough for its letters at 8 px. That's about 660 px on screen for 4,000
characters, so readable up close at 1080p. Longer pages need the item in hand, scrolled: point 3,
next.

**Cost on the software renderer.** It samples the textures on the processor. At a shelf of
4,000-character pages at 1280 × 720, 2,048 px close-ups take about 27 ms a frame, against 23 ms at
1,024 px. A graphics card hardly notices the difference, and 1,024 can still be chosen.

## Checked
- **Tests:** `tests/test_core.cpp`: 56,274 checks over both passes, 0 failures.
- **CI, run locally:** all these steps pass:
  - same addresses;
  - filters judge, count and rank the same;
  - models and the setup menu (with the item memory check);
  - the bytes256 line;
  - books;
  - the whole hallway step (with the new view distance check).
- **Close-ups:** the hallway runs with `--close-up` set to screen, 4096, 512 and 0. Standing at a
  shelf, the pages in front are drawn sharp.
- **Hallway bench** (software renderer, 60 frames):

| Settings | Time per frame |
| :--- | :--- |
| 4,000-character pages | 17.8 ms |
| 4,000-character pages, 1,024 MB cache | 21.3 ms |
| 4,000-character pages, View 3, Pictures 5 (held to 3) | 16.5 ms |
| Default settings, 7 either side | 16.6 ms |
| View Distance 20 either side | 16.8 ms |
