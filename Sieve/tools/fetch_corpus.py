#!/usr/bin/env python3
"""Download the Sieve training corpus and check every file against its pinned SHA-256.

    python3 tools/fetch_corpus.py                 # into corpus/gutenberg (git-ignored)
    python3 tools/fetch_corpus.py --dir SOMEWHERE

The corpus is Project Gutenberg's public-domain texts as packaged by NLTK. It is not stored in
the repository; the model built from it is (data/models), pinned by its own SHA-256, and
`sieve train` rebuilds that model byte for byte from these files.
"""
import argparse
import hashlib
import io
import os
import sys
import urllib.request
import zipfile

URL = "https://raw.githubusercontent.com/nltk/nltk_data/gh-pages/packages/corpora/gutenberg.zip"
HERE = os.path.dirname(os.path.abspath(__file__))
MANIFEST = os.path.join(HERE, "..", "data", "models", "corpus", "gutenberg-nltk.tsv")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--dir", default=os.path.join(HERE, "..", "corpus", "gutenberg"))
    p.add_argument("--manifest", default=MANIFEST)
    args = p.parse_args()

    wanted = {}
    for line in open(args.manifest, encoding="utf-8"):
        line = line.rstrip("\r\n")
        if line and not line.startswith("#"):
            name, _enc, _role, sha = line.split("\t")
            wanted[name] = sha

    print("downloading", URL)
    data = urllib.request.urlopen(URL, timeout=120).read()
    os.makedirs(args.dir, exist_ok=True)
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        for info in z.infolist():
            name = os.path.basename(info.filename)
            if name in wanted:
                body = z.read(info)
                got = hashlib.sha256(body).hexdigest()
                if got != wanted[name]:
                    sys.exit(f"{name}: SHA-256 {got} does not match the manifest")
                with open(os.path.join(args.dir, name), "wb") as f:
                    f.write(body)
                wanted.pop(name)
    if wanted:
        sys.exit("missing from the download: " + ", ".join(sorted(wanted)))
    print("corpus ready in", os.path.normpath(args.dir))


if __name__ == "__main__":
    main()
