#!/usr/bin/env python3
"""Checks the committed conformance vectors against the Python oracle, as CI does.

Each vector file in tests/ was written by reference/sieve_ref.py from the specification. This runs
every such command again and compares its output with the file, byte for byte (line endings aside,
since a Windows console writes CRLF). A difference means the oracle and the committed file disagree:
never "fix" either to make it pass without knowing which one the specification supports.

  python tools/check_vectors.py            every check
  python tools/check_vectors.py image      only the checks whose command or file names contain "image"
"""
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REFERENCE = os.path.normpath(os.path.join(HERE, "..", "reference"))
TESTS = os.path.normpath(os.path.join(HERE, "..", "tests"))

# (the oracle's command and its arguments, the vector file it must reproduce), as CI checks them.
CHECKS = [
    (["vectors"], "vectors_v1.tsv"),
    (["biguint-vectors"], "vectors_biguint_v1.tsv"),
    (["biguint-large-vectors"], "vectors_biguint_large_v1.tsv"),
    (["digit-vectors"], "vectors_digits_v1.tsv"),
    (["canon-vectors"], "vectors_canon.tsv"),
    (["bytes-vectors"], "vectors_bytes_v1.tsv"),
    (["alphabet-vectors"], "vectors_alphabets_v1.tsv"),
    (["model-vectors"], "vectors_models_v1.tsv"),
    (["image-vectors"], "vectors_image_v1.tsv"),
    (["image-v2-vectors"], "vectors_image_v2.tsv"),
    (["cipher-vectors"], "vectors_cipher_v1.tsv"),
    (["guided-vectors"], "vectors_guided_v1.tsv"),
    (["filter-vectors"], "vectors_filters_v1.tsv"),
    (["compact-vectors"], "vectors_compact_v1.tsv"),
    (["cross-vectors"], "vectors_cross_v1.tsv"),
    (["titled-vectors"], "vectors_titled_v1.tsv"),
    (["book-vectors"], "vectors_books_v1.tsv"),
    (["book-filter-vectors"], "vectors_book_filters_v1.tsv"),
    (["binary-vectors"], "vectors_binary_v1.tsv"),
    (["kind-vectors"], "vectors_kinds_v1.tsv"),
    (["chunk-vectors"], "vectors_chunks_v1.tsv"),
    (["written-vectors"], "vectors_written_v1.tsv"),
    (["utf8-vectors"], "vectors_utf8_v1.tsv"),
    (["notes2-vectors"], "vectors_notes2_v1.tsv"),
    (["notes3-vectors"], "vectors_notes3_v1.tsv"),
    (["pcm-vectors"], "vectors_pcm_v1.tsv"),
    (["sound-vectors"], "vectors_sound_v1.tsv"),
    (["composition-vectors"], "vectors_compositions_v1.tsv"),
    (["picture-vectors"], "vectors_picture_v1.tsv"),
    (["figure-vectors"], "vectors_figure_v1.tsv"),
    (["world-vectors"], "vectors_worlds_v1.tsv"),
    (["ai-vectors"], "vectors_ai_v1.tsv"),
    (["ai-filter-vectors"], "vectors_ai_filters_v1.tsv"),
    (["ai-run-filter-vectors"], "vectors_ai_run_filters_v1.tsv"),
    (["train-vectors", "--corpus", "../tests/training_corpus.txt"], "vectors_training_v1.tsv"),
    (["train-vectors", "--corpus", "../tests/training_corpus.txt", "--v2"], "vectors_training_v2.tsv"),
    (["diffusion-vectors"], "vectors_diffusion_v1.tsv"),
]


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else ""
    env = dict(os.environ, PYTHONIOENCODING="utf-8")
    failed, ran = [], 0
    for args, name in CHECKS:
        if only and only not in " ".join(args) and only not in name:
            continue
        ran += 1
        start = time.time()
        out = subprocess.run([sys.executable, "-I", "sieve_ref.py"] + args, cwd=REFERENCE, env=env, capture_output=True)
        with open(os.path.join(TESTS, name), "rb") as f:
            want = f.read()
        got = out.stdout.replace(b"\r\n", b"\n")
        ok = out.returncode == 0 and got == want.replace(b"\r\n", b"\n")
        print(f"{'ok  ' if ok else 'FAIL'}  {name:34} {time.time() - start:7.1f} s", flush=True)
        if not ok:
            failed.append(name)
            if out.returncode != 0:
                print(out.stderr.decode("utf-8", "replace")[-2000:], file=sys.stderr)
    print(f"\n{ran - len(failed)} of {ran} vector files reproduced by the oracle" + ("" if not failed else ": FAILED " + ", ".join(failed)))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
