# Keys registry, map zoom, viewer-first opening

## Every key in one registry (IDEAS 19.199, first step)
- New `client/keys.hpp` / `client/keys.cpp`: each system declares its actions (section, id, name, default keys) and asks the registry, rather than testing key codes itself.
- The personal dimension's 28 actions are on it, in four sections: moving, the catalogue, holding, the map.
- **Settings > Controls > Keys** lists every action by section. Enter, then a key, changes one (Shift/Ctrl/Alt kept); Backspace restores its default; Esc cancels.
- Changes are kept in the settings file under `[keys]` (`personal.holding/pick_up = F`). Footers and the aiming hint show the keys as set.

## Map zoom
- `+` and `-` zoom the personal dimension's map (five steps, each halving what it shows, centred on you). The width shown is written under the map.

## Items open in the viewer made for them (IDEAS 19.200)
- New **3D** view: models and worlds open on it (arrows or WASD turn it); the .obj text and the picture are tabs.
- O on a favourite in your dimension opens it in hand **and** in its viewer.
- Models, worlds and books can now be favourites, kept by their place on the line (`index` in the `.item` file). One kept from a compacted line opens with the filters off.
- A file has no viewer of its own yet: it is shown as hex, the viewer says a viewer for its kind needs making, and the kind is noted once in `viewers-needed.txt` beside the settings file.
- Fixed: keeping a model from a compacted line failed with "digit vector has the wrong length" (a blank-titled model was being named by page text).

## Ideas recorded
- IDEAS 19.200: the viewer rule.
- IDEAS 19.201: a documentation viewer on the main menu, laid out as a categorised help library (idea only).
- IDEAS 19.202: screenshots of every screen and feature made by a tool, version-tagged, remade only where their inputs changed; changes sorted by the testing they need (unit, CI, user, ...) (idea only).

## CI (step 10)
- Map zoom; the Keys screen's read-aloud row; rebinding pick up to F and using it.
- A model favourite kept from the compacted line, then opened on its 3D view; a file's missing viewer flagged and noted.

## Files to delete by hand
- None.
