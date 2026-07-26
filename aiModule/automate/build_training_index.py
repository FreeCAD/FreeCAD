"""Create leakage-safe train/validation/test indexes from audited AutoMate mates."""

from __future__ import annotations

import argparse
import collections
import hashlib
import json
import math
import os
import random
from pathlib import Path


ROOT = Path(__file__).resolve().parent
DEFAULT_AUDIT = ROOT / "audit" / "mates_1000.jsonl"
DEFAULT_CACHE = ROOT / "dataset" / "cache" / "brep_graph_v1"
DEFAULT_OUTPUT = ROOT / "dataset" / "training" / "index_v1"
SPLIT_RATIOS = {"train": 0.8, "validation": 0.1, "test": 0.1}


def cache_path(cache_root, part_id):
    digest = hashlib.sha1(part_id.encode("utf-8")).hexdigest()
    return cache_root / "parts" / digest[:2] / f"{part_id}.pt"


def document_id(assembly_id):
    return assembly_id.split("_", 1)[0]


def read_manifest(cache_root):
    values = {}
    path = cache_root / "manifest.jsonl"
    with path.open("r", encoding="utf-8") as stream:
        for line in stream:
            row = json.loads(line)
            if row.get("status") == "built":
                values[row["part_id"]] = row
    return values


def atomic_write_jsonl(path, rows):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + f".{os.getpid()}.tmp")
    try:
        with temporary.open("w", encoding="utf-8") as stream:
            for row in rows:
                stream.write(json.dumps(row, separators=(",", ":")) + "\n")
        os.replace(temporary, path)
    finally:
        if temporary.exists():
            temporary.unlink()


def prepare_rows(audit_path, cache_root, manifest, max_angle_deg, max_line_error_m):
    accepted = []
    rejected = []
    mate_types = set()

    with audit_path.open("r", encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, 1):
            audit = json.loads(line)
            rejection = None
            if audit.get("status") != "ok":
                rejection = "audit_error"
            elif len(audit.get("sides", [])) != 2:
                rejection = "invalid_side_count"
            else:
                for side in audit["sides"]:
                    part_id = side["part_id"]
                    entry = manifest.get(part_id)
                    if entry is None or not cache_path(cache_root, part_id).is_file():
                        rejection = "missing_cache"
                        break
                    if not 0 <= int(side["candidate_index"]) < int(entry["mcfs"]):
                        rejection = "candidate_out_of_range"
                        break
                    if side["angle_deg"] > max_angle_deg:
                        rejection = "axis_not_recalled"
                        break
                    if side["line_error_m"] > max_line_error_m:
                        rejection = "axis_line_not_recalled"
                        break

            if rejection is not None:
                rejected.append({
                    "line": line_number,
                    "assembly_id": audit.get("assembly_id"),
                    "mate_id": audit.get("mate_id"),
                    "mate_type": audit.get("mate_type"),
                    "reason": rejection,
                })
                continue

            mate_type = audit["mate_type"]
            mate_types.add(mate_type)
            sides = []
            for side in audit["sides"]:
                part_id = side["part_id"]
                path = cache_path(cache_root, part_id)
                sides.append({
                    "part_id": part_id,
                    "cache": path.relative_to(cache_root).as_posix(),
                    "candidate_index": int(side["candidate_index"]),
                    "axial_offset_m": float(side["axial_offset_m"]),
                    "axis_flipped": bool(side["axis_flipped"]),
                    "candidate_count": int(side["candidate_count"]),
                })
            accepted.append({
                "sample_id": f"{audit['assembly_id']}:{audit['mate_id']}",
                "document_id": document_id(audit["assembly_id"]),
                "assembly_id": audit["assembly_id"],
                "mate_id": audit["mate_id"],
                "mate_type": mate_type,
                "sides": sides,
                "quality": {
                    "label_world_origin_error_m": float(audit["label_world_origin_error_m"]),
                    "label_world_axis_error_deg": float(audit["label_world_axis_error_deg"]),
                    "side_angle_error_deg": [float(side["angle_deg"]) for side in audit["sides"]],
                    "side_line_error_m": [float(side["line_error_m"]) for side in audit["sides"]],
                },
            })

    mate_type_to_id = {name: index for index, name in enumerate(sorted(mate_types))}
    for row in accepted:
        row["mate_type_id"] = mate_type_to_id[row["mate_type"]]
    return accepted, rejected, mate_type_to_id


def assign_grouped_splits(rows, ratios, seed):
    parent = {}

    def find(value):
        parent.setdefault(value, value)
        if parent[value] != value:
            parent[value] = find(parent[value])
        return parent[value]

    def union(left, right):
        left_root = find(left)
        right_root = find(right)
        if left_root != right_root:
            parent[max(left_root, right_root)] = min(left_root, right_root)

    part_owner = {}
    for row in rows:
        doc = row["document_id"]
        find(doc)
        for side in row["sides"]:
            part_id = side["part_id"]
            if part_id in part_owner:
                union(doc, part_owner[part_id])
            else:
                part_owner[part_id] = doc

    groups = collections.defaultdict(list)
    for row in rows:
        group_id = find(row["document_id"])
        row["split_group_id"] = group_id
        groups[group_id].append(row)

    global_types = collections.Counter(row["mate_type"] for row in rows)
    targets_total = {split: len(rows) * ratio for split, ratio in ratios.items()}
    targets_type = {
        split: {mate_type: count * ratios[split] for mate_type, count in global_types.items()}
        for split in ratios
    }
    assigned = {split: [] for split in ratios}
    assigned_types = {split: collections.Counter() for split in ratios}

    def stable_noise(group_id):
        digest = hashlib.sha1(f"{seed}:{group_id}".encode()).digest()
        return int.from_bytes(digest[:8], "big")

    # Place groups containing rare classes and large groups first.
    ordered = sorted(
        groups.items(),
        key=lambda item: (
            -sum(len(item[1]) / global_types[row_type] for row_type in set(r["mate_type"] for r in item[1])),
            -len(item[1]),
            stable_noise(item[0]),
        ),
    )

    for group_id, group_rows in ordered:
        group_types = collections.Counter(row["mate_type"] for row in group_rows)
        best = None
        for split in ratios:
            hypothetical_total = {name: len(assigned[name]) for name in ratios}
            hypothetical_total[split] += len(group_rows)
            cost = sum(
                ((hypothetical_total[name] - targets_total[name]) / max(targets_total[name], 1.0)) ** 2
                for name in ratios
            )
            for name in ratios:
                for mate_type in global_types:
                    value = assigned_types[name][mate_type]
                    if name == split:
                        value += group_types[mate_type]
                    target = targets_type[name][mate_type]
                    cost += 0.5 * ((value - target) / max(target, 1.0)) ** 2
            candidate = (cost, stable_noise(f"{group_id}:{split}"), split)
            if best is None or candidate < best:
                best = candidate
        split = best[2]
        assigned[split].extend(group_rows)
        assigned_types[split].update(group_types)

    rng = random.Random(seed)
    for rows_in_split in assigned.values():
        rng.shuffle(rows_in_split)
    return assigned, len(groups)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--audit", type=Path, default=DEFAULT_AUDIT)
    parser.add_argument("--cache-dir", type=Path, default=DEFAULT_CACHE)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--seed", type=int, default=20260725)
    parser.add_argument("--max-angle-deg", type=float, default=1.0)
    parser.add_argument("--max-line-error-m", type=float, default=1.0e-4)
    args = parser.parse_args()

    manifest = read_manifest(args.cache_dir)
    accepted, rejected, mate_type_to_id = prepare_rows(
        args.audit, args.cache_dir, manifest, args.max_angle_deg, args.max_line_error_m
    )
    splits, split_group_count = assign_grouped_splits(accepted, SPLIT_RATIOS, args.seed)

    type_counts = {}
    documents = {}
    for split, rows in splits.items():
        atomic_write_jsonl(args.output_dir / f"{split}.jsonl", rows)
        type_counts[split] = dict(sorted(collections.Counter(row["mate_type"] for row in rows).items()))
        documents[split] = sorted(set(row["document_id"] for row in rows))
    atomic_write_jsonl(args.output_dir / "rejected.jsonl", rejected)

    document_sets = {split: set(values) for split, values in documents.items()}
    leakage = {
        f"{left}_{right}": sorted(document_sets[left] & document_sets[right])
        for left in document_sets for right in document_sets if left < right
    }
    class_counts = collections.Counter(row["mate_type"] for row in accepted)
    part_sets = {
        split: {side["part_id"] for row in rows for side in row["sides"]}
        for split, rows in splits.items()
    }
    part_leakage = {
        f"{left}_{right}": sorted(part_sets[left] & part_sets[right])
        for left in part_sets for right in part_sets if left < right
    }
    class_weights = {
        mate_type: len(accepted) / (len(class_counts) * count)
        for mate_type, count in sorted(class_counts.items())
    }
    summary = {
        "schema_version": 1,
        "seed": args.seed,
        "source_audit": str(args.audit),
        "cache_root": str(args.cache_dir),
        "accepted": len(accepted),
        "rejected": len(rejected),
        "rejection_counts": dict(sorted(collections.Counter(row["reason"] for row in rejected).items())),
        "mate_type_to_id": mate_type_to_id,
        "class_weights": class_weights,
        "split_counts": {split: len(rows) for split, rows in splits.items()},
        "split_type_counts": type_counts,
        "split_document_counts": {split: len(values) for split, values in documents.items()},
        "split_group_count": split_group_count,
        "document_leakage": leakage,
        "part_leakage": part_leakage,
        "thresholds": {
            "max_angle_deg": args.max_angle_deg,
            "max_line_error_m": args.max_line_error_m,
        },
    }
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))
    if any(leakage.values()) or any(part_leakage.values()):
        raise SystemExit("document or part leakage detected")


if __name__ == "__main__":
    main()
