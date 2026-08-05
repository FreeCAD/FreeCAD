"""Prepare one real ten-part validation assembly as nine incremental mates."""

from __future__ import annotations

import argparse
import heapq
import json
import shutil
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
ASSEMBLY_ID = "5973e481db748c664bbedd44_69bfacdd8fd1d5675f02ee02_2414089ea7b87c714eaba2ae_default"
ROOT_PART = "5973e481db748c664bbedd44_69bfacdd8fd1d5675f02ee02_071dbce6e7961a7c6059052b_default_jjgui"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--index-dir", type=Path, default=ROOT / "dataset/training/location_paper_full")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dataset/demo/location_validation_sequence_10")
    args = parser.parse_args()
    if args.output_dir.exists() and any(args.output_dir.iterdir()):
        raise RuntimeError(f"Output directory is not empty: {args.output_dir}")
    rows = [json.loads(line) for line in (args.index_dir / "validation.jsonl").open(encoding="utf-8")]
    rows = [row for row in rows if row["assembly_id"] == ASSEMBLY_ID]
    adjacency = defaultdict(list)
    for row_index, row in enumerate(rows):
        left, right = (side["part_id"] for side in row["sides"])
        adjacency[left].append((int(row["candidate_pair_count"]), right, row_index))
        adjacency[right].append((int(row["candidate_pair_count"]), left, row_index))
    if ROOT_PART not in adjacency: raise RuntimeError("Configured root part is absent")
    assembled, steps, frontier = {ROOT_PART}, [], []
    for cost, neighbor, row_index in adjacency[ROOT_PART]:
        heapq.heappush(frontier, (cost, ROOT_PART, neighbor, row_index))
    while frontier and len(assembled) < 10:
        _, existing, new_part, row_index = heapq.heappop(frontier)
        if new_part in assembled: continue
        row = rows[row_index]
        original_left = row["sides"][0]["part_id"]
        if original_left == existing:
            sides = row["sides"]
            positives = row["positive_pairs"]
        else:
            sides = [row["sides"][1], row["sides"][0]]
            positives = [[right, left] for left, right in row["positive_pairs"]]
        steps.append({"row": row, "sides": sides, "positive_pairs": positives,
                      "existing": existing, "new": new_part})
        assembled.add(new_part)
        for cost, neighbor, next_row in adjacency[new_part]:
            if neighbor not in assembled:
                heapq.heappush(frontier, (cost, new_part, neighbor, next_row))
    if len(assembled) != 10 or len(steps) != 9:
        raise RuntimeError(f"Could not build a ten-part sequence: {len(assembled)} parts")

    ordered_parts = [ROOT_PART] + [step["new"] for step in steps]
    part_number = {part_id: index for index, part_id in enumerate(ordered_parts, 1)}
    args.output_dir.mkdir(parents=True, exist_ok=True)
    parts = []
    for part_id in ordered_parts:
        number = part_number[part_id]
        filename = f"part_{number:02d}.step"
        source = ROOT / "dataset/step" / f"{part_id}.step"
        if not source.is_file(): raise FileNotFoundError(source)
        shutil.copy2(source, args.output_dir / filename)
        parts.append({"number": number, "part_id": part_id, "file": filename,
                      "role": "base" if number == 1 else "added"})
    manifest_steps = []
    for step_number, step in enumerate(steps, 1):
        row, sides = step["row"], step["sides"]
        manifest_steps.append({
            "step": step_number, "sample_id": row["sample_id"],
            "mate_type": row["mate_type"], "candidate_pair_count": row["candidate_pair_count"],
            "positive_pairs": step["positive_pairs"],
            "a": {"part_number": part_number[step["existing"]], "part_id": step["existing"],
                  "selected_face_index": sides[0]["selected_face"],
                  "selected_face_name": f"Face{int(sides[0]['selected_face']) + 1}",
                  "local_mcf_count": len(sides[0]["local_mcfs"])},
            "b": {"part_number": part_number[step["new"]], "part_id": step["new"],
                  "selected_face_index": sides[1]["selected_face"],
                  "selected_face_name": f"Face{int(sides[1]['selected_face']) + 1}",
                  "local_mcf_count": len(sides[1]["local_mcfs"])},
        })
    manifest = {"schema_version": 1, "source_split": "validation",
                "assembly_id": ASSEMBLY_ID, "parts": parts, "steps": manifest_steps}
    (args.output_dir / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(manifest, ensure_ascii=False, indent=2))
    print(f"sequence={args.output_dir}")


if __name__ == "__main__": main()
