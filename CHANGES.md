# Fixes from your review (14 November 2026), and version 0.13.79

## Version
- **0.13.79** in every place the last bump changed: CMakeLists.txt, vcpkg.json, the browser extension's manifest, web/README.md's hello example, make_release.py's and SIEVE-INSTALL-USAGE.md's examples, and HANDOFF.md's note.

## Replays
- The Replays screen has a few words under its heading: press F12 in the hallway to record a replay (F12 again stops it); every replay is listed there (Enter plays one, Space caches it). Any screen can now have such words (`intro.<screen>` in the language file).

## The viewer's keys
- The keys at the bottom wrap onto more lines, upward, broken only between keys (`wrap_keys` in hallway.hpp: one way for every key list), so none is left partly on a line.
- The 3D view has its own key line (arrows / WASD turn it).

## Personal Dimensions from the hallway's pause menu
- It opened the setup menu (its old route, from before your dimension had a screen of its own). It now opens the main menu's **Personal Dimensions** screen over the hallway; Esc there, or from your dimension, comes back to the hallway, paused. A favourite opened in your dimension (O) goes into this hallway's hand where it is of this line, else the hallway is built on its line.
- Screenshot runs now say where a press asks to go (`goes to: Personal Dimensions`), so CI checks it.

## A pause menu in your dimension
- **Esc** in your dimension opens its pause menu (Esc again resumes), from one list of every pause menu's rows (`client/pause_rows.hpp`), each row saying where it is shown. The hallway's pause menu reads the same list (its rows unchanged).
- Your dimension's leaves out the hallway's own tools (Restart Sieve, the Node Graph Viewer, the Navigation System) and adds **Leave Your Dimension**. **Personal Dimensions > Features > The hallway's tools in its pause menu** (off at first; `[personal] hallway_tools`) puts them in: each goes into the hallway and opens there.
- Settings, Personal Dimensions and the Filter Designer open over your dimension and come back to it; the File Locator, the AI Training Harness and the Encryption Engine come back to it too, by the main menu's way; the Media Player and the System Viewer go where they go from the main menu. From the hallway's pause menu, a system's row in your dimension is chosen on the hallway's pause menu.
- A row can have its own words in one place (`pause.<id>.personal`), and the help under the list wraps onto two lines.

## Depth in your dimension
- Faces popped in and out (the bookcase's long boards worst): every face was put in order by its middle's depth, which large and small faces side by side defeat. The objects are now drawn with a test of depth at every pixel: each face filled into a layer that keeps how near each pixel's face is, the layer then laid over the ground. In the wireframe, faces are filled black and their edges drawn only where nothing nearer covers them. The ground is drawn as before.
- Not timed here: please watch the frame rate.

## Favourites' pictures in your dimension
- A favourite placed in your dimension now shows its picture on its front, where the hallway puts it on the shelf (its line's face rectangle). The hallway draws it as it draws the item on its shelf, with its settings as they are, and keeps it beside the favourite as `favourites/SHA.png` when the favourite is kept (H), and again whenever it is opened there.

## Ideas recorded
- IDEAS 19.196: its first step built (the pause menus as data).
- IDEAS 19.204: a catalogue of personal dimensions to start from (templates: yours, others', groups', games').
- IDEAS 19.205: faces drawn from both sides (a two-sided flag; pages first).
- IDEAS 19.206: an interface designer (every menu laid out by the person, over the menu itself).
- IDEAS 19.207: one renderer for every 3D space (the hallway's MeshBatch, for your dimension and the Character Creator too): next.
- IDEAS 19.208: O in your dimension opens the hallway's own viewer in place, with a small "calculating..." by the crosshair.
- IDEAS 19.209: the author's tools, the first a ground shaper (key 1 out, # away), and who may use them.
- IDEAS 19.210: saves across the RAID sieved, split with parity and shared in small parts within limits, before machines can find one another; its formats fixed first, as there is no going back.
- IDEAS 19.211, and the ROADMAP's rules: decentralised, no server needed ever; servers only host dimensions; authorship shown by the creator's own key and the item's SHA-256, checkable anywhere.

## CI (step 10)
- The pause menu's Personal Dimensions goes to the Personal Dimensions screen.
- A kept favourite has its picture (a PNG 128 pixels wide).
- Esc in your dimension opens its pause menu with its default rows; a system's row goes to it; the Features toggle is kept as `[personal] hallway_tools = on` and adds the hallway's tools.

## REVIEW-ROUND.md
- Kept as one list, unticked as things change: items 7 and 10 unticked (their drawing changed), items 16 to 21 added for these fixes, and a **Recheck everything** box at the top: ticked, the next zip unticks every box for a fresh round.

## Files to delete by hand
- None.
