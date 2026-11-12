# System Viewer and panel fixes

- **The System Viewer is now a view of its own.** It opens from the main menu or the pause menu and fills the whole window. It shows the catalogue's trees as a ring of doors, with the same keys as before; the mouse wheel turns the ring, and Esc goes back to wherever it was opened from. It no longer opens the setup menu's sliding panel. `--system-viewer ID` opens it at an entry, for screenshots.
- **The right-hand panel now mirrors the left.** It has no frame and no white line down its side. The map squashes beside it, as it does beside the settings, and its scroll bars are on its left edge.
- **Only one panel is open at a time.** Opening the right-hand panel sends the settings out. Esc on it brings the settings back if they were open before. Tab brings the settings back instead and closes the right-hand panel.
- **CI step 10** checks the viewer and the one-panel rule. The screenshot report now says whether the settings panel is in or out.
- **Docs:** MANUAL, ROADMAP (step 2 marked done) and IDEAS updated.

Checked: CI step 10 passed (802 s), help check passed. No files to delete.
