"""Filter successful audit rows whose required B-Rep graph cache is missing."""

from __future__ import annotations

import argparse
import collections
import json
import os
from pathlib import Path

from preprocess_parts import graph_path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--audit", type=Path, required=True)
    parser.add_argument("--cache-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise RuntimeError(f"Output already exists: {args.output}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    temporary = args.output.with_suffix(args.output.suffix + f".{os.getpid()}.tmp")
    source_rows = written_rows = removed_rows = 0
    status_counts = collections.Counter()
    missing_parts = collections.Counter()
    try:
        with args.audit.open(encoding="utf-8") as source, temporary.open(
            "w", encoding="utf-8", newline="\n"
        ) as destination:
            for line in source:
                source_rows += 1
                row = json.loads(line)
                status_counts[row.get("status")] += 1
                missing = []
                if row.get("status") == "ok":
                    missing = [
                        side["part_id"] for side in row.get("sides", [])
                        if not graph_path(args.cache_dir, side["part_id"]).is_file()
                    ]
                if missing:
                    removed_rows += 1
                    missing_parts.update(missing)
                    continue
                destination.write(line if line.endswith("\n") else line + "\n")
                written_rows += 1
        os.replace(temporary, args.output)
    finally:
        if temporary.exists():
            temporary.unlink()
    summary = {
        "schema_version": 1,
        "source_audit": str(args.audit),
        "cache_root": str(args.cache_dir),
        "source_rows": source_rows,
        "written_rows": written_rows,
        "removed_rows": removed_rows,
        "source_status_counts": dict(status_counts),
        "usable_successful_mates": status_counts.get("ok", 0) - removed_rows,
        "missing_parts": dict(sorted(missing_parts.items())),
    }
    summary_path = args.output.with_suffix(".summary.json")
    summary_path.write_text(json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
