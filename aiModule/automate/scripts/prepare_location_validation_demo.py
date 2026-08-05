"""Prepare five no-leak validation pairs (ten unique STEP parts) for FreeCAD."""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TARGETS = [
    ("FASTENED", 25, 150),
    ("CYLINDRICAL", 151, 500),
    ("PLANAR", 501, 1000),
    ("REVOLUTE", 1001, 3000),
    ("SLIDER", 151, 1000),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--index-dir", type=Path, default=ROOT / "dataset/training/location_paper_full")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dataset/demo/location_validation_10_parts")
    args = parser.parse_args()
    if args.output_dir.exists() and any(args.output_dir.iterdir()):
        raise RuntimeError(f"Output directory is not empty: {args.output_dir}")
    rows = [json.loads(line) for line in (args.index_dir / "validation.jsonl").open(encoding="utf-8")]
    used_parts, selected = set(), []
    for mate_type, minimum, maximum in TARGETS:
        candidates = [
            row for row in rows
            if row["mate_type"] == mate_type
            and minimum <= int(row["candidate_pair_count"]) <= maximum
            and not ({side["part_id"] for side in row["sides"]} & used_parts)
            and row["sides"][0]["part_id"] != row["sides"][1]["part_id"]
        ]
        candidates.sort(key=lambda row: row["sample_id"])
        if not candidates:
            raise RuntimeError(f"No unique validation example for {mate_type}")
        row = candidates[len(candidates) // 2]
        selected.append(row)
        used_parts.update(side["part_id"] for side in row["sides"])
    if len(used_parts) != 10:
        raise AssertionError(f"Expected ten unique parts, found {len(used_parts)}")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    manifest = {"schema_version": 1, "source_split": "validation", "pairs": []}
    for pair_index, row in enumerate(selected, 1):
        item = {
            "pair": pair_index, "sample_id": row["sample_id"],
            "mate_type": row["mate_type"], "candidate_pair_count": row["candidate_pair_count"],
            "positive_pairs": row["positive_pairs"], "parts": [],
        }
        for side_name, side in zip(("A", "B"), row["sides"]):
            source = ROOT / "dataset/step" / f"{side['part_id']}.step"
            destination_name = f"pair_{pair_index:02d}_{side_name}.step"
            if not source.is_file(): raise FileNotFoundError(source)
            shutil.copy2(source, args.output_dir / destination_name)
            item["parts"].append({
                "side": side_name, "part_id": side["part_id"], "file": destination_name,
                "selected_face_index": side["selected_face"],
                "selected_face_name": f"Face{int(side['selected_face']) + 1}",
                "local_mcf_count": len(side["local_mcfs"]),
            })
        manifest["pairs"].append(item)
    (args.output_dir / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(manifest, ensure_ascii=False, indent=2))
    print(f"demo={args.output_dir}")


if __name__ == "__main__": main()
