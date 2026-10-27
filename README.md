# Potentia

*Exact, verifiable records of human work, and of how AI models are made.*

Potentia is a long-term project to preserve what people make, and the AI models trained on it, in a
form anyone can check down to the last byte. Its working part today is **[Sieve](Sieve/README.md)**:
tools for packing, checking and unpacking files exactly, and for exploring every possible text,
picture and melody as a place with an address.

**Concept and architecture by Edward James Gordon.**

## What you can do with it today

| | |
| :--- | :--- |
| **Pack a folder smaller than 7-Zip, and get it back exactly** | Sieve's own release, 44,647,549 bytes of programs and data, packs to a 6,319,561-byte `.sieve` (7.06:1), and installs back byte for byte. [How it compares](Sieve/README.md#smaller-archives-that-come-back-exactly) |
| **Extract almost any archive from the right-click menu** | `.zip`, `.7z`, `.rar`, `.tar`, `.gz`, `.xz`, `.bz2`, `.zst`, `.cab`, `.iso` and more, on Windows, with every file of a `.sieve` checked before it is written. |
| **Install only what was meant** | Every file is checked against its SHA-256 before anything is written, so a damaged or incomplete download changes nothing on your disk. |
| **Publish web pages that prove themselves** | A browser extension shows a site's files only when each is exactly the file its link vouches for. [Sieve Secure](Pages/secure.html) |
| **Train a model that anyone can train again, to the bit** | A small language model trained in pinned integer arithmetic, with a record of everything it read and in what order: retraining it on any machine gives the same model. |
| **Walk the Gallery of Babel** | Every possible page, picture, melody, film, 3D model and file, each at exactly one address, laid out as a corridor you can walk and search with filters. |

## Try it

1. Download `sieve.exe` from the [latest release](../../releases/latest) (64-bit Windows 10 and 11)
   and run it: it installs Sieve into a folder you choose.
2. For the right-click menu, run `context\install_context.bat` in that folder. Right-click any
   archive and choose **Sieve**.
3. Start `hallway.exe` to walk the Gallery of Babel, or open a command prompt in `tools\` and run
   `sieve help`.

On Linux and macOS, [build it from the source](Sieve/README.md#build-it).

## Why it exists

The training trajectory of any sufficiently expressive learner (the order in which it sees data,
is queried and receives feedback) determines what the learner becomes, and is in general not
recoverable from its weights. *We cannot align what we cannot bound, and we cannot bound what we
cannot replay.* Potentia does not solve alignment; it makes training trajectories reproducible,
auditable and citable, so that any model made from one can be checked, rerun and compared.

The same exactness serves the archive: a **Museum of Human Creations**, each work identified by
its bytes, and the **Gallery of Babel** around it, every possible work at one address, searched
outwards from the real ones with filters that set noise aside without ever visiting it.

The full argument, the evidence behind it, and the design of the Gallery's filtration layers are
in **[VISION.md](VISION.md)**.

## Where it stands

**Built (Sieve):** exact addresses for every line (text, pictures, audio, video, books, tracks,
movies, 3D models, worlds, language models and files); a versioned filtration stack with exact
survivor counts; maps of verified anchors; the walkable hallway; `.sieve` instructions and
installers; the right-click menu; the web extension and site builder; training recorded so it can
be replayed exactly.

**Next:** benchmarks against the usual compressors on public test data, and the persistent
environment, the Museum and the reconstruction tools set out in the vision.

**Roadmap:**
1. ✅ The coordinate system (Sieve: exact addressing for every line).
2. ✅ A walkable prototype of the Gallery (Sieve's hallway).
3. ✅ Maps of verified anchors, the foundation of the Museum.
4. ✅ Training recorded and replayed exactly (`sieve ai-train`).
5. Enumerate all known base AI models.
6. Build out the filtration stack: word pairs, grammar, per-line prediction models.
7. Populate the early Museum of Human Creations.
8. Create the agent sandbox environment.
9. Develop reconstruction workflows.

## Learn more

- **[Sieve's README](Sieve/README.md)**: what Sieve does, with commands and measurements.
- **[VISION.md](VISION.md)**: why Potentia exists and what it is meant to become.
- **[Sieve Secure](Pages/secure.html)**: the web extension, with a live test site.
- **[Previous](Previous/)**: earlier versions of these documents.
- A controlled study of how language models invent things that do not exist:
  [Non-Existent-Data-LLM-Stress-Testing](https://github.com/thor110/Non-Existent-Data-LLM-Stress-Testing).

## Contributing

Open an issue, submit a pull request, or join the discussion on
[Discord](https://discord.gg/HPDty4kDCq). Useful contributions include building maps of real
works, proposing filters, and running Sieve's benchmark on your own folders. Please say who made a
contribution: a person, an AI under human direction, or an AI on its own.

If you know of unarchived AI models, please open an issue or get in touch.

## Licence

GNU AGPL v3.0.

## Acknowledgments

Special thanks to Llama 3.3-307B-Instruct for early refinement of the concept
([conversation logs](https://hf.co/chat/r/1gJTQ7w?leafId=21fc542d-b68e-42d4-8a9e-723e0d0bef63)).
The idea was refined further in discussions with GPT5 and Gemini3. Sieve was built with Claude
Opus 5.5, based on my specifications.

## Changelog

1. Initial commit
2. README updates
3. Minor fixes
4. Architectural vision expanded (training, preservation, reconstruction)
5. Search space redefined
6. Single page search space specifications added
7. Deterministic filtration methods defined
8. Sieve created
9. Sieve updated
10. Sieve released
11. Sieve build release script updated
12. Sieve highlight fixes and video window size
13. README rewritten against the Sieve release; the original moved to Previous/READMEv1.md
14. README rewritten around what works today; the vision moved whole to VISION.md; the previous
    README kept as Previous/READMEv2.md
