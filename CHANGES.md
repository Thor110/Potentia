# Your character is you; the System Viewer back to your dimension; the task list in lists

## Your character in the hallway
- It is no longer drawn ahead of you and to your right: it is you, so it stands where you stand and you do not see it. Its size still sets the hallway's scale (`hallway scale:` unchanged), and its actions still play (idle, walk, run, read).
- Your store's characters still stand ahead on your left, as before.
- The run's output says `your character:` and `your animation:` (they said `character beside you:` and `animation beside you:`); CI step 10 checks the new words.
- The setting's words (`[character] beside_you`, SETTINGS.md), the MANUAL and the `--no-character` help say the same.
- Seeing through its eyes, its arms in view where they reach, is noted in IDEAS (after 19.148), for when the camera starts at its head.

## The System Viewer from your dimension's pause menu
- Esc there now comes back to your dimension (it went to the main menu). Not checked by CI: a screenshot run stops before the way back. Please try it (REVIEW-ROUND item 24).

## Ideas recorded
- IDEAS 19.220: music for your personal dimension, a third space in the Media Player beside WORLD and MENU, kept in the dimension's own file so it travels with it, and loaded for whichever dimension is open.

## REVIEW-ROUND.md
- Item 19 ticked (Personal Dimensions from the hallway and back, as you found it).
- Items 23 (your character not seen in the hallway) and 24 (the System Viewer back to your dimension) added.
- Section 3 rewritten as you asked: the task list as lists by area (P your dimension, R characters and the renderer, M menus and keys, V viewers, F formats and grammars, S new spaces, T testing), each task saying which CI steps check it and where you test it. A batch takes tasks from one list or several and is checked by the steps they name together, not the full CI; the full CI before a release package and after each review.
- This zip is the first such batch: three changes checked by one run of step 10.

## Files to delete by hand
- None.
