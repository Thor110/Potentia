# Sieve's voice: LJ Speech, as units

What Sieve's own voice (`tools/cli/voice.hpp`, `sieve speak`, the hallway's Text to Voice) needs
besides its recordings, which are the anchors' hour of LJ Speech
(`data/anchors/public-domain/voice/lj-speech/clips`, public domain).

- `cmudict.dict`: the Carnegie Mellon University Pronouncing Dictionary, unchanged, as kept at
  github.com/cmusphinx/cmudict on 9 November 2026 (SHA-256
  `81917843c7f44ce2b094ac63873c2c7a4cf802040792c455ba3ca406891c3d22`). Its licence is
  `cmudict-LICENSE.txt` beside it (BSD-style: keep the notice), and in `third_party_licenses/cmudict`.
- `alignment.tsv`: where each speech sound lies in each clip (clip, phone, start and end in samples),
  made by `tools/voice/align_voice.py` with espeak-ng as a build-time tool; 503 of the 547 clips.

The build copies this folder beside the programs as `voice/lj-speech`; the clips are found in the
anchors above a build inside the repository, or in a `clips` folder here. How it was made and how
well it is understood: `results/voice-units-2026-11-09.md`.
