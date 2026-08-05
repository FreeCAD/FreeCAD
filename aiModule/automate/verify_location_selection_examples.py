"""Verify face-conditioned Location examples against cached B-Rep graphs."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from build_location_selection_examples import GraphCache, local_mcfs_for_face


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--examples-dir", type=Path, required=True)
    args = parser.parse_args()
    summary = json.loads((args.examples_dir / "summary.json").read_text(encoding="utf-8"))
    cache = GraphCache(summary["cache_root"])
    max_pairs = int(summary["thresholds"]["max_pairs"])
    failures = []
    sample_ids = set()
    count = 0
    with (args.examples_dir / "examples.jsonl").open(encoding="utf-8") as stream:
        for count, line in enumerate(stream, 1):
            row = json.loads(line)
            try:
                if row["sample_id"] in sample_ids:
                    raise ValueError("duplicate_sample_id")
                sample_ids.add(row["sample_id"])
                side_a, side_b = row["sides"]
                _, graph_a = cache.load(side_a["part_id"])
                _, graph_b = cache.load(side_b["part_id"])
                expected_a = local_mcfs_for_face(graph_a, int(side_a["selected_face"]))
                expected_b = local_mcfs_for_face(graph_b, int(side_b["selected_face"]))
                if side_a["local_mcfs"] != expected_a or side_b["local_mcfs"] != expected_b:
                    raise ValueError("local_mcf_mismatch")
                pair_count = len(expected_a) * len(expected_b)
                if pair_count != int(row["candidate_pair_count"]):
                    raise ValueError("candidate_pair_count_mismatch")
                if not 0 < pair_count <= max_pairs:
                    raise ValueError("candidate_pair_count_out_of_range")
                local_a = set(expected_a)
                local_b = set(expected_b)
                positives = {tuple(pair) for pair in row["positive_pairs"]}
                if not positives:
                    raise ValueError("empty_positive_pairs")
                if any(a not in local_a or b not in local_b for a, b in positives):
                    raise ValueError("positive_outside_candidates")
            except Exception as exc:
                failures.append(
                    {
                        "line": count,
                        "sample_id": row.get("sample_id"),
                        "error": str(exc),
                    }
                )
    result = {
        "status": "ok" if not failures and count == summary["accepted"] else "error",
        "examples": count,
        "expected_examples": summary["accepted"],
        "unique_sample_ids": len(sample_ids),
        "failures": len(failures),
        "failure_details": failures[:100],
    }
    destination = args.examples_dir / "verification.json"
    destination.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False, indent=2))
    print(f"verification={destination}")
    if result["status"] != "ok":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
