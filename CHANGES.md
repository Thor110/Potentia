# Changes: shaded characters, and wireframe everywhere when Real Graphics is off

## Characters
- **Shaded by default:** the CHARACTER page and the Character Creator now draw shaded (`[character] drawing = 3`).
- **Your existing settings file:** it still says `drawing = 1` (wireframe). Change it once in the creator, or edit that line.
- **Real Graphics off:** every character is drawn as wireframe, whatever the drawing row says. The row then reads "wireframe (Real Graphics is off)".

## Personal world with Real Graphics off
The whole world is drawn as wireframe:
- a black sky;
- the ground as an open green grid;
- objects as their edges, filled black so they hide what is behind them;
- clouds and characters as outlines.

With Real Graphics on, it looks as before.

## Picture options
`--real-graphics` now also applies to `--menu` pictures, so characters are drawn as the drawing row says.

## IDEAS (planned, not built)
- **§19.193:** a face finder. It removes faces no one can see from placed objects, keeps the original model by its hash, and stores the cut copies in a local cache beside the world.
- **§19.194:** modelling tools inside the dimensions (raising ground, subdividing, extruding, boolean cuts).

## Files to delete by hand
- None.
