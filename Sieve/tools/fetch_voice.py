#!/usr/bin/env python3
"""Fetch LJ Speech's voice (public domain) and check every clip against its pinned SHA-256.

    python3 tools/fetch_voice.py                  # the anchors' hour, as WAVs, into corpus/voice
    python3 tools/fetch_voice.py --all            # every clip of the shard (1,638, about 3 hours)
    python3 tools/fetch_voice.py --dir SOMEWHERE

The anchors (data/anchors/public-domain/voice/lj-speech) hold the first hour of LJ Speech 1.1 as
FLAC: 547 clips, LJ001-0001 to the last that brings them to an hour, with their transcripts. This
script makes the same clips again as the WAV files LJ Speech itself has, byte for byte (their
SHA-256 in the anchors' manifest.tsv), from the first of the dataset's Parquet shards on Hugging
Face, itself pinned by its SHA-256. With --all it writes every clip of that shard, the anchors'
and the rest, with a metadata.csv of their transcripts, for training on more than the hour.

It needs pyarrow (pip install pyarrow) to read the shard.
"""
import argparse
import hashlib
import os
import sys
import urllib.request

URL = ("https://huggingface.co/datasets/keithito/lj_speech/resolve/"
       "refs%2Fconvert%2Fparquet/main/train/0000.parquet")
SHARD_SHA256 = "18c8a13f9a8975a1c22546d931ee30b7852f091f2aed75e766ee3289b396d6d8"
HERE = os.path.dirname(os.path.abspath(__file__))
ANCHORS = os.path.join(HERE, "..", "data", "anchors", "public-domain", "voice", "lj-speech")


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--dir", default=os.path.join(HERE, "..", "corpus", "voice", "lj-speech"))
    p.add_argument("--shard", help="the shard already downloaded (else it is fetched)")
    p.add_argument("--all", action="store_true", help="every clip of the shard, not only the anchors' hour")
    args = p.parse_args()
    try:
        import pyarrow.parquet as pq
    except ImportError:
        sys.exit("fetch_voice: needs pyarrow (pip install pyarrow)")

    wanted = {}
    for line in open(os.path.join(ANCHORS, "manifest.tsv"), encoding="utf-8"):
        line = line.rstrip("\r\n")
        if line and not line.startswith("#"):
            clip, _samples, sha = line.split("\t")
            wanted[clip] = sha

    shard = args.shard
    if not shard:
        os.makedirs(args.dir, exist_ok=True)
        shard = os.path.join(args.dir, "0000.parquet")
        if not os.path.exists(shard):
            print("downloading", URL, "(about 480 MB)")
            urllib.request.urlretrieve(URL, shard)
    h = hashlib.sha256()
    with open(shard, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    if h.hexdigest() != SHARD_SHA256:
        sys.exit(f"fetch_voice: {shard} is not the pinned shard (SHA-256 {h.hexdigest()})")

    os.makedirs(args.dir, exist_ok=True)
    f = pq.ParquetFile(shard)
    written, bad, transcripts = 0, 0, []
    for g in range(f.num_row_groups):
        for row in f.read_row_group(g, columns=["id", "audio", "text", "normalized_text"]).to_pylist():
            clip = row["id"]
            if clip not in wanted and not args.all:
                continue
            data = row["audio"]["bytes"]
            if clip in wanted and hashlib.sha256(data).hexdigest() != wanted[clip]:
                print("MISMATCH", clip)
                bad += 1
                continue
            with open(os.path.join(args.dir, clip + ".wav"), "wb") as out:
                out.write(data)
            transcripts.append((clip, row["text"], row["normalized_text"]))
            written += 1
    transcripts.sort()
    with open(os.path.join(args.dir, "metadata.csv"), "w", encoding="utf-8", newline="\n") as m:
        for clip, text, normalized in transcripts:
            m.write(f"{clip}|{text}|{normalized}\n")
    missing = [c for c in wanted if not os.path.exists(os.path.join(args.dir, c + ".wav"))]
    print(f"{written} clip(s) written to {args.dir}; {bad} mismatched; {len(missing)} of the anchors' missing")
    sys.exit(1 if bad or missing else 0)


if __name__ == "__main__":
    main()
