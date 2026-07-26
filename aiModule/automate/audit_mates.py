"""Audit STEP readability and MCF candidate recall on AutoMate mates."""

from __future__ import annotations

import argparse
import collections
import json
import math
import os
import random
import statistics
import sys
import time
from pathlib import Path

import pyarrow.parquet as pq


ROOT = Path(__file__).resolve().parent
DATASET = ROOT / "dataset"
BUILD_DIR = ROOT / "build-ai"
DEFAULT_OUTPUT = ROOT / "audit" / "mates_1000.jsonl"
MATE_TYPE_PERCENTAGES = {
    "FASTENED": 62.1,
    "REVOLUTE": 12.7,
    "PLANAR": 11.8,
    "SLIDER": 5.3,
    "CYLINDRICAL": 5.1,
    "PARALLEL": 1.8,
    "BALL": 0.6,
    "PIN_SLOT": 0.5,
}


def proportional_type_quotas(count):
    """Apply the paper distribution; assign percentage-rounding residue to FASTENED."""
    quotas = {
        mate_type: math.floor(count * percentage / 100.0)
        for mate_type, percentage in MATE_TYPE_PERCENTAGES.items()
    }
    quotas["FASTENED"] += count - sum(quotas.values())
    return quotas


def vec3(values):
    return tuple(float(values[i]) for i in range(3))


def norm(values):
    return math.sqrt(sum(value * value for value in values))


def normalized(values):
    length = norm(values)
    if length < 1.0e-15:
        raise ValueError("zero-length axis")
    return tuple(value / length for value in values)


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def frame_origin_and_z(flat_matrix):
    return (
        (float(flat_matrix[3]), float(flat_matrix[7]), float(flat_matrix[11])),
        normalized((flat_matrix[2], flat_matrix[6], flat_matrix[10])),
    )


def transform_point(flat_matrix, point):
    return tuple(
        sum(float(flat_matrix[row * 4 + col]) * point[col] for col in range(3))
        + float(flat_matrix[row * 4 + 3])
        for row in range(3)
    )


def transform_axis(flat_matrix, axis):
    return normalized(
        tuple(
            sum(float(flat_matrix[row * 4 + col]) * axis[col] for col in range(3))
            for row in range(3)
        )
    )


def candidate_error(target_origin, target_axis, candidate):
    origin, axis = candidate
    axis = normalized(axis)
    signed_dot = max(-1.0, min(1.0, dot(target_axis, axis)))
    angle_deg = math.degrees(math.acos(abs(signed_dot)))
    delta = sub(target_origin, origin)
    axial_offset = dot(delta, axis)
    perpendicular = sub(delta, tuple(axial_offset * value for value in axis))
    line_error = norm(perpendicular)
    origin_error = norm(delta)
    return {
        "angle_deg": angle_deg,
        "line_error_m": line_error,
        "origin_error_m": origin_error,
        "axial_offset_m": axial_offset,
        "axis_flipped": signed_dot < 0.0,
    }


def best_candidate(target_origin, target_axis, candidates):
    best = None
    for index, candidate in enumerate(candidates):
        error = candidate_error(target_origin, target_axis, candidate)
        # Prefer the correct geometric line; axial translation is a separate label.
        key = (error["angle_deg"] / 1.0) ** 2 + (error["line_error_m"] / 1.0e-4) ** 2
        if best is None or key < best[0]:
            best = (key, index, error)
    if best is None:
        raise ValueError("part produced no MCF candidates")
    _, index, error = best
    error["candidate_index"] = index
    return error


class PartCache:
    def __init__(self, part_class, options, step_dir, max_size=256):
        self.part_class = part_class
        self.options = options
        self.step_dir = step_dir
        self.max_size = max_size
        self.values = collections.OrderedDict()

    def load(self, part_id):
        if part_id in self.values:
            self.values.move_to_end(part_id)
            return self.values[part_id]

        path = self.step_dir / f"{part_id}.step"
        if not path.is_file():
            raise FileNotFoundError(path)
        part = self.part_class(str(path), self.options)
        if not part.is_valid:
            raise ValueError(f"invalid STEP: {path.name}")

        raw_candidates = [(vec3(mcf.origin), vec3(mcf.axis)) for mcf in part.default_mcfs]
        candidates = [
            candidate for candidate in raw_candidates
            if all(math.isfinite(value) for vector in candidate for value in vector)
            and norm(candidate[1]) > 1.0e-15
        ]
        value = {
            "faces": len(part.brep.nodes.faces),
            "edges": len(part.brep.nodes.edges),
            "vertices": len(part.brep.nodes.vertices),
            "candidates": candidates,
            "invalid_candidates": len(raw_candidates) - len(candidates),
        }
        self.values[part_id] = value
        if len(self.values) > self.max_size:
            self.values.popitem(last=False)
        return value


def sample_assembly_ids(seed):
    table = pq.read_table(
        DATASET / "assemblies.parquet", columns=["assemblyId", "n_step_mates"]
    )
    ids = [
        assembly_id
        for assembly_id, count in zip(
            table.column("assemblyId").to_pylist(),
            table.column("n_step_mates").to_pylist(),
        )
        if count and count > 0
    ]
    random.Random(seed).shuffle(ids)
    return ids


def percentile(values, fraction):
    if not values:
        return None
    values = sorted(values)
    index = min(len(values) - 1, round((len(values) - 1) * fraction))
    return values[index]


def summarize(rows, requested, elapsed):
    sides = [side for row in rows if row.get("status") == "ok" for side in row["sides"]]
    ok_rows = [row for row in rows if row.get("status") == "ok"]
    failed_rows = [row for row in rows if row.get("status") != "ok"]
    type_counts = collections.Counter(row["mate_type"] for row in ok_rows)
    candidate_counts = [side["candidate_count"] for side in sides]
    axial_offsets = [
        abs(side["axial_offset_m"]) for side in sides
        if math.isfinite(side["axial_offset_m"])
    ]
    invalid_candidate_count = sum(side.get("invalid_candidate_count", 0) for side in sides)
    world_origin_errors = [row["label_world_origin_error_m"] for row in ok_rows]
    world_axis_errors = [row["label_world_axis_error_deg"] for row in ok_rows]
    recall_by_type = {}
    for mate_type in sorted(type_counts):
        type_sides = [
            side for row in ok_rows if row["mate_type"] == mate_type for side in row["sides"]
        ]
        recall_by_type[mate_type] = {
            "mates": type_counts[mate_type],
            "axis_recall_1deg": sum(side["angle_deg"] <= 1.0 for side in type_sides) / len(type_sides),
            "line_recall_0_1mm_1deg": sum(
                side["angle_deg"] <= 1.0 and side["line_error_m"] <= 1.0e-4
                for side in type_sides
            ) / len(type_sides),
            "origin_recall_0_1mm_1deg": sum(
                side["angle_deg"] <= 1.0 and side["origin_error_m"] <= 1.0e-4
                for side in type_sides
            ) / len(type_sides),
        }
    summary = {
        "requested_mates": requested,
        "audited_mates": len(rows),
        "successful_mates": len(ok_rows),
        "failed_mates": len(failed_rows),
        "success_rate": len(ok_rows) / len(rows) if rows else 0.0,
        "audited_sides": len(sides),
        "axis_recall_1deg": sum(side["angle_deg"] <= 1.0 for side in sides) / len(sides) if sides else 0.0,
        "line_recall_0_1mm_1deg": sum(
            side["angle_deg"] <= 1.0 and side["line_error_m"] <= 1.0e-4 for side in sides
        ) / len(sides) if sides else 0.0,
        "origin_recall_0_1mm_1deg": sum(
            side["angle_deg"] <= 1.0 and side["origin_error_m"] <= 1.0e-4 for side in sides
        ) / len(sides) if sides else 0.0,
        "candidate_count_median": statistics.median(candidate_counts) if candidate_counts else None,
        "candidate_count_p95": percentile(candidate_counts, 0.95),
        "invalid_candidate_count": invalid_candidate_count,
        "sides_with_invalid_candidates": sum(
            side.get("invalid_candidate_count", 0) > 0 for side in sides
        ),
        "abs_axial_offset_m_median": statistics.median(axial_offsets) if axial_offsets else None,
        "abs_axial_offset_m_p95": percentile(axial_offsets, 0.95),
        "label_world_origin_error_m_p95": percentile(world_origin_errors, 0.95),
        "label_world_axis_error_deg_p95": percentile(world_axis_errors, 0.95),
        "mate_type_counts": dict(sorted(type_counts.items())),
        "recall_by_type": recall_by_type,
        "failure_counts": dict(sorted(collections.Counter(row.get("error_type") for row in failed_rows).items())),
        "elapsed_seconds": elapsed,
    }
    return summary


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--seed", type=int, default=20260725)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--progress-every", type=int, default=25)
    parser.add_argument(
        "--stratified",
        action="store_true",
        help="sample successful mates according to the paper's mate-type distribution",
    )
    args = parser.parse_args()

    sys.path.insert(0, str(BUILD_DIR))
    dll_dir = None
    if os.name == "nt":
        dll_dir = os.add_dll_directory(str(Path(sys.prefix) / "Library" / "bin"))
    from automate_cpp import Part, PartOptions

    options = PartOptions()
    options.tesselate = False
    options.num_uv_samples = 0
    options.num_random_samples = 0
    options.num_sdf_samples = 0
    options.default_mcfs = True
    options.collect_inferences = False

    cache = PartCache(Part, options, DATASET / "step")
    assembly_ids = sample_assembly_ids(args.seed)
    rng = random.Random(args.seed)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    summary_path = args.output.with_suffix(".summary.json")
    rows = []
    type_quotas = proportional_type_quotas(args.count) if args.stratified else None
    successful_by_type = collections.Counter()
    successful_total = 0
    next_progress = args.progress_every
    start = time.perf_counter()

    with args.output.open("w", encoding="utf-8") as output:
        for assembly_id in assembly_ids:
            if successful_total >= args.count:
                break
            assembly_path = DATASET / "assemblies" / f"{assembly_id}.json"
            try:
                assembly = json.loads(assembly_path.read_text(encoding="utf-8"))
            except Exception as exc:
                row = {
                    "status": "error",
                    "assembly_id": assembly_id,
                    "error_type": type(exc).__name__,
                    "error": str(exc),
                }
                rows.append(row)
                output.write(json.dumps(row) + "\n")
                output.flush()
                continue

            mates = [
                mate for mate in assembly.get("mates", [])
                if mate.get("has_step") and len(mate.get("occurrences", [])) == 2
                and len(mate.get("mcfs", [])) == 2
            ]
            rng.shuffle(mates)
            for mate in mates:
                if successful_total >= args.count:
                    break
                mate_type = mate.get("mateType")
                if type_quotas is not None:
                    if mate_type not in type_quotas:
                        continue
                    if successful_by_type[mate_type] >= type_quotas[mate_type]:
                        continue
                row = {
                    "status": "ok",
                    "assembly_id": assembly_id,
                    "mate_id": mate.get("id"),
                    "mate_type": mate.get("mateType"),
                }
                try:
                    occurrence_indices = mate["occurrences"]
                    occurrences = [assembly["occurrences"][index] for index in occurrence_indices]
                    part_ids = [assembly["parts"][occurrence["part"]]["id"] for occurrence in occurrences]
                    side_rows = []
                    world_origins = []
                    world_axes = []
                    for part_id, occurrence, frame in zip(part_ids, occurrences, mate["mcfs"]):
                        features = cache.load(part_id)
                        target_origin, target_axis = frame_origin_and_z(frame)
                        error = best_candidate(target_origin, target_axis, features["candidates"])
                        error.update({
                            "part_id": part_id,
                            "candidate_count": len(features["candidates"]),
                            "invalid_candidate_count": features["invalid_candidates"],
                            "faces": features["faces"],
                            "edges": features["edges"],
                            "vertices": features["vertices"],
                        })
                        side_rows.append(error)
                        world_origins.append(transform_point(occurrence["transform"], target_origin))
                        world_axes.append(transform_axis(occurrence["transform"], target_axis))
                    row["sides"] = side_rows
                    row["label_world_origin_error_m"] = norm(sub(world_origins[0], world_origins[1]))
                    row["label_world_axis_error_deg"] = math.degrees(
                        math.acos(min(1.0, max(-1.0, abs(dot(world_axes[0], world_axes[1])))))
                    )
                except Exception as exc:
                    row = {
                        "status": "error",
                        "assembly_id": assembly_id,
                        "mate_id": mate.get("id"),
                        "mate_type": mate.get("mateType"),
                        "error_type": type(exc).__name__,
                        "error": str(exc),
                    }

                if row.get("status") == "ok":
                    successful_total += 1
                    successful_by_type[row["mate_type"]] += 1

                rows.append(row)
                output.write(json.dumps(row) + "\n")
                output.flush()
                if successful_total >= next_progress:
                    elapsed = time.perf_counter() - start
                    print(
                        f"successful={successful_total}/{args.count} "
                        f"attempted={len(rows)} elapsed={elapsed:.1f}s "
                        f"types={dict(sorted(successful_by_type.items()))}",
                        flush=True,
                    )
                    next_progress += args.progress_every

    elapsed = time.perf_counter() - start
    summary = summarize(rows, args.count, elapsed)
    summary["sampling"] = "paper_proportional_stratified" if args.stratified else "assembly_random"
    summary["requested_type_quotas"] = type_quotas
    summary["achieved_type_counts"] = dict(sorted(successful_by_type.items()))
    summary_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(json.dumps(summary, indent=2))
    print(f"rows={args.output}")
    print(f"summary={summary_path}")


if __name__ == "__main__":
    main()
