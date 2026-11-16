# The fractal index where the line starts; ideas of 16 November recorded

- client/app_main.cpp: a listed line whose first book is its index (the FRACTALS line, unless the known
  fractals are excluded) starts at the index, not halfway along. The index was there (address 0), but
  halfway along the line is some 10^400 rulesets from it since X4. Excluded, there is no index and it
  starts halfway along as before. A door into the line still keeps your angle round the loop; Home goes
  to the index from anywhere on it.
- ci/steps/10-hallway.sh: two checks, the line starting at the index, and halfway along when excluded.
- data/help/fractal-dimension.txt: says so.
- docs/IDEAS.md: 19.236 to 19.243, your ideas in your words (CI steps split into 1a, 1b; tilings as a
  dimension; a large model by its shape and an existing model plugged in; a cellular automata dimension
  for the video line; Replays' name and the main menu's shape; device rooms between the hallway's rooms;
  vaulting from binary to binary; and the index fix).

Checked: the two new CI lines pass (and the first fails against the old start); check_help and
check_library pass.
