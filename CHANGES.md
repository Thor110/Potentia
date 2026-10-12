# CHAT cards show what a file is (with the chat fixes)

Against your main ("chat tab"). This zip holds everything since then: last round's fixes, which
are not on main yet, and this round. Copy `Sieve/` over yours. There are no new files.

## Thumbnails and previews on file cards (client/chat.cpp)
- **Pictures:** a PNG, JPG, GIF or BMP shows its picture, small (at most 96 pixels on its longer
  side).
- **Videos:** an MP4, AVI or animated WEBP shows its first frame, read by ffmpeg when it is there.
  Without ffmpeg it shows its first bytes instead.
- **Texts:** a text shows its first four lines, in their own characters (»«, ß and ü come through).
  A line that isn't UTF-8, or is cut in the middle of a character, is drawn byte by byte, as the
  model's replies are.
- **Anything else:** its first 24 bytes in hex (a MIDI file starts `4d 54 68 64`).
- **How it is made:**
  - It is worked out once, when the file is given, on the same worker as the weighing.
  - The texture is made the first time the card is drawn and let go with the conversation.
  - Pictures pass the vault, as every picture shown does.
- **Scripted runs:** `--chat-attach` takes several files at once, apart by `;`.

## Last round's fixes (in this zip too)
- **CHAT's clicks:** they land where you see them (`SDL_RenderCoordinatesFromWindow`).
- **Shadowed names:** renamed in music.cpp and the other 9 files.
- **The figures run's pictures:** CI checks that they are drawn again from the filters.
- **A slip of mine:** last round's hallway CI lines ran on a hallway binary built before those
  edits. A second build folder I made, sharing its SDL download, had broken the client build, and
  my error filter hid the failure. I rebuilt it and ran them again on the current code: they
  pass, so the fixes stand as delivered. I also re-read the music.cpp rename line by line, because
  screenshot runs don't write the music settings file.

## IDEAS §19.26: bigger models, and ComfyUI
- **Two kinds of outside help, kept apart:**
  - format tools such as ffmpeg (optional, as now);
  - generators such as ComfyUI and big language models. Their output is a file, and a file is
    already on the lines, so it is named and checked by its hash. Sieve needn't trust that the
    generator is deterministic.
- **A ComfyUI bridge** through its local API (`/prompt`, `/history`, `/view`):
  - it would sit in CHAT as a picture model;
  - the workflow, seed and SHA-256 are kept with each card;
  - it needs a small localhost-only HTTP client of our own, off unless a setting names the server.
- **The order proposed:**
  1. published models in CHAT through Sieve's own engine (self-contained);
  2. the ComfyUI bridge (optional, like ffmpeg);
  3. quantised weights for bigger language models on the CPU.

## Checked here
- **Unit tests:** 76,334 checks, 0 failures (run once here, as this machine has no hardware
  SHA-256).
- **CI lines,** run on the rebuilt hallway:
  - the chat lines, including the new cards line;
  - `--talk`;
  - the harness reproducing its shipped run;
  - the figures pictures.
- **With ffmpeg:** the MP4 thumbnail was checked here with a static ffmpeg.
