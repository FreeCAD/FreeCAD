"""Split verified Location selection examples without document or part leakage."""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path

from build_training_index import SPLIT_RATIOS, assign_grouped_splits, atomic_write_jsonl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--examples-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=20260725)
    args = parser.parse_args()
    if args.output_dir.exists() and any(args.output_dir.iterdir()):
        raise RuntimeError(f"Output directory is not empty: {args.output_dir}")
    source_summary = json.loads(
        (args.examples_dir / "summary.json").read_text(encoding="utf-8")
    )
    source_verification = json.loads(
        (args.examples_dir / "verification.json").read_text(encoding="utf-8")
    )
    if source_verification.get("status") != "ok":
        raise ValueError("Location selection examples have not passed verification")
    rows = []
    with (args.examples_dir / "examples.jsonl").open(encoding="utf-8") as stream:
        for line in stream:
            row = json.loads(line)
            row["document_id"] = row["assembly_id"].split("_")[0]
            rows.append(row)
    splits, group_count = assign_grouped_splits(rows, SPLIT_RATIOS, args.seed)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for split, split_rows in splits.items():
        atomic_write_jsonl(args.output_dir / f"{split}.jsonl", split_rows)

    document_sets = {
        split: {row["document_id"] for row in split_rows}
        for split, split_rows in splits.items()
    }
    part_sets = {
        split: {side["part_id"] for row in split_rows for side in row["sides"]}
        for split, split_rows in splits.items()
    }
    document_leakage = {
        f"{left}_{right}": sorted(document_sets[left] & document_sets[right])
        for left in document_sets
        for right in document_sets
        if left < right
    }
    part_leakage = {
        f"{left}_{right}": sorted(part_sets[left] & part_sets[right])
        for left in part_sets
        for right in part_sets
        if left < right
    }
    summary = {
        "schema_version": 1,
        "seed": args.seed,
        "source_examples": str(args.examples_dir),
        "cache_root": source_summary["cache_root"],
        "examples": len(rows),
        "split_counts": {split: len(split_rows) for split, split_rows in splits.items()},
        "split_candidate_pairs": {
            split: sum(int(row["candidate_pair_count"]) for row in split_rows)
            for split, split_rows in splits.items()
        },
        "split_mate_type_counts": {
            split: dict(sorted(collections.Counter(row["mate_type"] for row in split_rows).items()))
            for split, split_rows in splits.items()
        },
        "split_group_count": group_count,
        "document_counts": {split: len(values) for split, values in document_sets.items()},
        "part_counts": {split: len(values) for split, values in part_sets.items()},
        "document_leakage": document_leakage,
        "part_leakage": part_leakage,
    }
    (args.output_dir / "summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(summary, ensure_ascii=False, indent=2))
    if any(document_leakage.values()) or any(part_leakage.values()):
        raise SystemExit("document or part leakage detected")


if __name__ == "__main__":
    main()
