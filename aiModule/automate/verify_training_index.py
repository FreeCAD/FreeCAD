"""Validate training index references and labels."""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent
DEFAULT_INDEX = ROOT / "dataset" / "training" / "index_v1"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--index-dir", type=Path, default=DEFAULT_INDEX)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    summary = json.loads((args.index_dir / "summary.json").read_text(encoding="utf-8"))
    cache_root = Path(summary["cache_root"])
    sample_ids = set()
    documents = {}
    parts = {}
    counts = {}
    graph_cache = {}

    for split in ("train", "validation", "test"):
        rows = [json.loads(line) for line in (args.index_dir / f"{split}.jsonl").open(encoding="utf-8")]
        counts[split] = len(rows)
        documents[split] = {row["document_id"] for row in rows}
        parts[split] = {side["part_id"] for row in rows for side in row["sides"]}
        for row in rows:
            if row["sample_id"] in sample_ids:
                raise ValueError(f"duplicate sample {row['sample_id']}")
            sample_ids.add(row["sample_id"])
            if row["mate_type_id"] != summary["mate_type_to_id"][row["mate_type"]]:
                raise ValueError("mate type id mismatch")
            for side in row["sides"]:
                if not math.isfinite(side["axial_offset_m"]):
                    raise ValueError("non-finite axial offset")
                cache_path = cache_root / side["cache"]
                if cache_path not in graph_cache:
                    graph_cache[cache_path] = torch.load(
                        cache_path, map_location="cpu", weights_only=False
                    )["graph"]
                graph = graph_cache[cache_path]
                index = side["candidate_index"]
                if not 0 <= index < graph.mcfs.shape[0]:
                    raise ValueError("candidate index out of range")
                if not torch.isfinite(graph.mcfs[index]).all():
                    raise ValueError("selected MCF is not finite")

    pairs = (("train", "validation"), ("train", "test"), ("validation", "test"))
    for left, right in pairs:
        if documents[left] & documents[right]:
            raise ValueError(f"document leakage: {left}/{right}")
        if parts[left] & parts[right]:
            raise ValueError(f"part leakage: {left}/{right}")

    rejected_count = sum(
        1 for line in (args.index_dir / "rejected.jsonl").open(encoding="utf-8")
        if line.strip()
    )
    if len(sample_ids) != summary["accepted"]:
        raise ValueError("accepted count does not match split files")
    if rejected_count != summary["rejected"]:
        raise ValueError("rejected count does not match rejected.jsonl")

    result = {
        "status": "ok",
        "samples": counts,
        "unique_samples": len(sample_ids),
        "rejected_samples": rejected_count,
        "source_total": len(sample_ids) + rejected_count,
        "documents": {key: len(value) for key, value in documents.items()},
        "parts": {key: len(value) for key, value in parts.items()},
        "document_leakage": 0,
        "part_leakage": 0,
    }
    output = args.output or args.index_dir / "verification.json"
    output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    print(f"verification={output}")


if __name__ == "__main__":
    main()
