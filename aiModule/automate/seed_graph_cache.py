"""Seed a compatible graph cache with hard links from an existing cache."""

from __future__ import annotations

import argparse
import os
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--destination", type=Path, required=True)
    args = parser.parse_args()
    linked = 0
    existing = 0
    failed = 0
    source_parts = args.source / "parts"
    destination_parts = args.destination / "parts"
    for source in source_parts.rglob("*.pt"):
        relative = source.relative_to(source_parts)
        destination = destination_parts / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        if destination.exists():
            existing += 1
            continue
        try:
            os.link(source, destination)
            linked += 1
        except OSError as exc:
            failed += 1
            print(f"link_failed={source} error={exc}", flush=True)
    print(f"linked={linked} existing={existing} failed={failed}")
    if failed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
