"""Verify split counts, unique samples, candidates, and leakage for Location."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--index-dir", type=Path, required=True)
    args = parser.parse_args()
    summary = json.loads((args.index_dir / "summary.json").read_text(encoding="utf-8"))
    seen = set()
    failures = []
    counts = {}
    documents = {}
    parts = {}
    for split in ("train", "validation", "test"):
        split_count = 0
        documents[split] = set()
        parts[split] = set()
        with (args.index_dir / f"{split}.jsonl").open(encoding="utf-8") as stream:
            for line_number, line in enumerate(stream, 1):
                row = json.loads(line)
                split_count += 1
                sample_id = row["sample_id"]
                if sample_id in seen:
                    failures.append(f"duplicate sample {sample_id}")
                seen.add(sample_id)
                documents[split].add(row["document_id"])
                parts[split].update(side["part_id"] for side in row["sides"])
                local_a = row["sides"][0]["local_mcfs"]
                local_b = row["sides"][1]["local_mcfs"]
                if len(local_a) * len(local_b) != row["candidate_pair_count"]:
                    failures.append(f"pair count mismatch {split}:{line_number}")
                if not row["positive_pairs"]:
                    failures.append(f"empty positives {split}:{line_number}")
        counts[split] = split_count
    for left in documents:
        for right in documents:
            if left < right and documents[left] & documents[right]:
                failures.append(f"document leakage {left}:{right}")
            if left < right and parts[left] & parts[right]:
                failures.append(f"part leakage {left}:{right}")
    if counts != summary["split_counts"]:
        failures.append(f"split counts mismatch {counts} != {summary['split_counts']}")
    if len(seen) != summary["examples"]:
        failures.append("unique sample total mismatch")
    result = {
        "status": "ok" if not failures else "error",
        "examples": len(seen),
        "split_counts": counts,
        "document_leakage": 0 if not any("document leakage" in x for x in failures) else 1,
        "part_leakage": 0 if not any("part leakage" in x for x in failures) else 1,
        "failures": failures[:100],
    }
    destination = args.index_dir / "verification.json"
    destination.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False, indent=2))
    print(f"verification={destination}")
    if result["status"] != "ok":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
