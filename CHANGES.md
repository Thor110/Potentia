# Changes: the first search's time, and the dialogs in fullscreen (19 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files.

## Your question: does the time setting reach the first search?

It didn't. The search row only appeared once a file had been read, so the first search always ran
with no limit. The rope also searched each file twice: once for the weighing and once for the
instructions. Now:

- **The search row shows before anything is chosen** (and after a failed read), so the first
  search can have its time.
- **The time is kept in `sieve-hallway.ini`**, in a new `[locator]` section: `search_seconds`
  (0 for no limit) and `search_for` (`file` or `run`). Both File Locators (the main menu's and the
  world's) start from it, and changing it saves it.
- **The rope's search is done once per file per run.** The weighing and the instructions share
  the answer. On the mixed test folder both together took 5.5 s, where each took about 5 s
  before.

## The install dialogs: locking up, and minimising at every step

I couldn't reproduce this here (there are no Windows dialogs on Linux), so I've changed the three
things most likely to cause it. Please check on Windows:

1. **The window no longer minimises when a dialog takes focus** (an SDL hint, set where the window
   is made).
2. **Exclusive fullscreen switches to the desktop's fullscreen while a dialog is open,** then
   switches back once the last dialog closes. An exclusive window can sit over the dialog, and
   since the dialog is modal the hallway then looks locked.
3. **The installer's first dialog ("which instructions") no longer brings the window forward when
   it closes.** The folder dialog follows straight after, and bringing the window forward could
   put it over that folder dialog.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,158 checks, 0 failures.
- The CI lines for the rope, the File Locator and the settings file. The settings check now also
  covers `[locator]`, and an older file keeps its values.
- Screenshots of the search row with nothing chosen and with a saved time loaded.

**Docs:** README (the search row), HANDOFF.
