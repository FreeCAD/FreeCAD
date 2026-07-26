"""Verify immutable artifacts recorded by a frozen training run."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir", nargs="?", type=Path, default=ROOT / "runs/mate_pair_v1")
    args = parser.parse_args()
    manifest = json.loads((args.run_dir / "FROZEN.json").read_text(encoding="utf-8"))
    if manifest["status"] != "frozen":
        raise AssertionError("run status is not frozen")
    for name, expected in manifest["sha256"].items():
        actual = sha256(args.run_dir / name)
        if actual != expected:
            raise AssertionError(f"hash mismatch for {name}: {actual} != {expected}")
        print(f"sha256_ok={name}")
    epoch_count = sum(1 for line in (args.run_dir / "metrics.jsonl").open(encoding="utf-8") if line.strip())
    if epoch_count != manifest["training"]["epochs"]:
        raise AssertionError(f"epoch count mismatch: {epoch_count}")
    print(f"epochs_ok={epoch_count}")
    print("frozen_run_check=OK")


if __name__ == "__main__":
    main()

