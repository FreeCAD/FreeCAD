"""Validate that dataset mate frames can be matched to AutoMate MCF candidates."""

from __future__ import annotations

import argparse
import json
import math
import os
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
BUILD_DIR = ROOT / ("build-ai" if sys.version_info[:2] == (3, 10) else "build-msvc-clean")
DATASET_DIR = ROOT / "dataset"


def vector(values):
    return tuple(float(value) for value in values)


def norm(values):
    return math.sqrt(sum(value * value for value in values))


def normalized(values):
    length = norm(values)
    return tuple(value / length for value in values)


def distance(a, b):
    return norm(tuple(x - y for x, y in zip(a, b)))


def axis_error_degrees(a, b):
    a = normalized(a)
    b = normalized(b)
    # A mating axis is treated as an unoriented line for candidate matching.
    cosine = min(1.0, max(-1.0, abs(sum(x * y for x, y in zip(a, b)))))
    return math.degrees(math.acos(cosine))


def point_to_axis_errors(point, axis_origin, axis):
    axis = normalized(axis)
    delta = tuple(value - origin for value, origin in zip(point, axis_origin))
    axial = sum(value * direction for value, direction in zip(delta, axis))
    perpendicular = tuple(
        value - axial * direction for value, direction in zip(delta, axis)
    )
    return axial, norm(perpendicular)


def frame_origin_and_z(flat_matrix):
    # Dataset matrices are flattened row-major homogeneous transforms.
    origin = vector((flat_matrix[3], flat_matrix[7], flat_matrix[11]))
    z_axis = vector((flat_matrix[2], flat_matrix[6], flat_matrix[10]))
    return origin, z_axis


def best_candidate(candidates, target_origin, target_axis):
    best = None
    for index, candidate in enumerate(candidates):
        origin = vector(candidate.origin)
        axis = vector(candidate.axis)
        position_error = distance(origin, target_origin)
        angle_error = axis_error_degrees(axis, target_axis)
        rank_key = (position_error, angle_error)
        if best is None or rank_key < best[0]:
            best = (rank_key, index, origin, axis)
    return best


def find_sample():
    assemblies_dir = DATASET_DIR / "assemblies"
    step_dir = DATASET_DIR / "step"
    for assembly_path in sorted(assemblies_dir.glob("*.json")):
        assembly = json.loads(assembly_path.read_text(encoding="utf-8"))
        for mate in assembly.get("mates", []):
            if not mate.get("has_step") or len(mate.get("occurrences", [])) != 2:
                continue
            occurrence_indices = mate["occurrences"]
            occurrences = [assembly["occurrences"][index] for index in occurrence_indices]
            part_ids = [assembly["parts"][occurrence["part"]]["id"] for occurrence in occurrences]
            step_paths = [step_dir / f"{part_id}.step" for part_id in part_ids]
            if all(path.is_file() for path in step_paths):
                return assembly_path, mate, step_paths
    raise RuntimeError("No mate with two available STEP files was found.")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--max-mcfs", type=int, default=0,
                        help="limit candidates per part; zero means all")
    args = parser.parse_args()

    sys.path.insert(0, str(BUILD_DIR))
    dll_dir = None
    if os.name == "nt":
        dll_dir = os.add_dll_directory(str(Path(sys.prefix) / "Library" / "bin"))
    from automate_cpp import Part, PartOptions

    assembly_path, mate, step_paths = find_sample()
    print(f"assembly={assembly_path.name}")
    print(f"mate={mate['name']} type={mate['mateType']}")

    options = PartOptions()
    options.default_mcfs = True
    for side, (step_path, matrix) in enumerate(zip(step_paths, mate["mcfs"])):
        part = Part(str(step_path), options)
        if not part.is_valid:
            raise RuntimeError(f"Could not load {step_path}")
        candidates = part.default_mcfs
        if args.max_mcfs > 0:
            candidates = candidates[: args.max_mcfs]
        target_origin, target_axis = frame_origin_and_z(matrix)
        best = best_candidate(candidates, target_origin, target_axis)
        (position_error, angle_error), index, origin, axis = best
        axial_error, line_error = point_to_axis_errors(target_origin, origin, axis)
        print(
            f"side={side} part={step_path.name} faces={len(part.brep.nodes.faces)} "
            f"mcfs={len(part.default_mcfs)}"
        )
        print(f"  target origin={target_origin} z={target_axis}")
        print(f"  best index={index} origin={origin} axis={axis}")
        print(f"  position_error_m={position_error:.9g} angle_error_deg={angle_error:.6g}")
        print(f"  axial_offset_m={axial_error:.9g} point_to_axis_m={line_error:.9g}")


if __name__ == "__main__":
    main()
