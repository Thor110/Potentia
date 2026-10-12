# Readable text on the black screens

Against your main plus the earlier zips (navigator.cpp includes nav-random).

## The fix
The address navigator wrote its title, buttons, chosen digits and readout in the line's edge
colour. On lines with black or very dark edges (books, models, tracks, movies) that was black on
black.

It now uses `menu_ink` (dimensions.hpp), the rule the setup menu already uses for line names on
black:
- the edge colour when it reads;
- otherwise the background colour;
- when both are too dark, the background colour lightened halfway to white.

## The same fix on two more black screens
- **The map view** (node_graph.cpp, `draw_graph_view`).
- **The media player** (media_player.cpp).

The pause menu and the item page's SORT and META tabs are left alone. They sit on the line's own
background colour, where the edge colour already reads.

## Checked here (Linux)
- The client builds without warnings.
