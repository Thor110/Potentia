# T1: CI only where a change reaches, and the screenshots kept as data (IDEAS 19.202, first step)

## Only the steps a change reaches
- **`ci/run.sh --changed`** runs only the CI steps the uncommitted changes reach. **`--changed-since REF`** does the same for everything changed since a commit (for example `origin/main`). **`--dry`** shows what they reach without running anything. `ci\run.bat --changed` works too.
- **Which steps:** each changed file is shown with the steps it reaches and why. The map is `ci/reach.txt`, a list of patterns kept as data:
  - the hallway's sources (and the data only it reads) reach steps 03, 04, 10 and 11, every step that runs the hallway;
  - the unit tests reach 01;
  - the reference oracle reaches the steps that run it;
  - documents, results and the Python tools the CI does not run reach none;
  - the CI's runner reaches every step, and a step's own script reaches that step.
- **Also:** a file named in a step's commands reaches that step (a fixture, or a tool the step runs: `docs/MANUAL.md` reaches 10 and 11, which pack it).
- **Missing rules are safe:** a file that matches no rule reaches every step, so a rule left out costs a longer run, never a missed check. The core library, the command line and the build files fall through to every step on purpose.
- **The help check** is run for every change.

## The screenshots as data
- **`tools/shots.txt`** lists 19 shots, each with the files it is made from and the hallway's arguments for it:
  - the main menu, the setup menu, the CHARACTER page, YOURS;
  - the Character Creator's four drawings and its tilt;
  - the hallway (wireframe, Real Graphics, its pause menu);
  - your dimension (wireframe, Real Graphics, its catalogue, its pause menu);
  - Settings > Keys, Replays, and a file in the viewer.
- **`tools/screenshots.py --hallway build/hallway --out shots/`** makes them as `NAME-vVERSION.png` and keeps `shots.tsv` beside them: each shot's version, and the SHA-256 of its inputs and of its picture.
  - A shot whose inputs have not changed is not made again (`--all` makes every one).
  - One made again with the same picture keeps its old version tag.
  - Nothing personal is read: its own HOME, settings, and seed 99's character.
- **Tried:** the first run made all 19. A second run made none, as nothing had changed. With `--all`, 15 came out the same picture. The other 4 differ from run to run by a mark drawn from the clock (the setup menu, the CHARACTER page, YOURS, the hallway's pause menu), so they are remade whenever they are asked for.
- **Still to come:** the website reading the listing (task 32).

## Checked
- `ci/run.sh --changed --dry` and `--changed-since HEAD~1 --dry` show the files and steps as above.
- `ci/run.sh 02` still runs as before.

## REVIEW-ROUND.md
- T1 ticked (in the last zip).

## Files to delete by hand
- None.
