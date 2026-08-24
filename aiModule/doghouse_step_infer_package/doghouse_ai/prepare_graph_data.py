#!/usr/bin/env python3
"""Prepare per-model face-graph training data from STEP + annotation JSON."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np

try:
    from .build_point_dataset import build_dataset, load_json
    from .step_geometry import build_geometry_from_step
except ImportError:
    from build_point_dataset import build_dataset, load_json
    from step_geometry import build_geometry_from_step


def _find_label_for_step(step_path: Path) -> Path | None:
    """Locate the annotation JSON for a STEP file using common naming schemes."""
    stem = step_path.stem
    candidates = [
        step_path.with_name(f"{stem}_annotation.json"),
        step_path.with_name(f"{stem} annotation.json"),
        step_path.with_name(f"{stem}.json"),
    ]
    for cand in candidates:
        if cand.exists():
            return cand
    return None


def discover_pairs(step_dir: Path) -> list[tuple[str, str]]:
    """Auto-discover (STEP, annotation) pairs by scanning the directory.

    Robust to annotation files being renamed (e.g. adding an _annotation
    suffix): every STEP that has a matching doghouse_instance_labels.v1 JSON
    is included, so no per-file path needs hardcoding.
    """
    pairs: list[tuple[str, str]] = []
    for step_path in sorted(step_dir.glob("*.step")):
        label_path = _find_label_for_step(step_path)
        if label_path is None:
            continue
        try:
            with open(label_path, encoding="utf-8") as f:
                schema = json.load(f).get("schema")
        except (json.JSONDecodeError, OSError):
            continue
        if schema != "doghouse_instance_labels.v1":
            continue
        pairs.append((step_path.name, label_path.name))
    return pairs


def build_edge_labels(
    face_instance: np.ndarray,
    adjacency: np.ndarray,
) -> np.ndarray:
    """Return 0/1 edge labels: 1 iff both endpoints share the same positive instance id."""
    if len(adjacency) == 0:
        return np.empty(0, dtype=np.float32)
    a = adjacency[:, 0].astype(np.int64)
    b = adjacency[:, 1].astype(np.int64)
    ia = face_instance[a]
    ib = face_instance[b]
    return ((ia > 0) & (ia == ib)).astype(np.float32)


def prepare_one(
    step_path: Path,
    label_path: Path,
    *,
    sample_points_per_face: int = 64,
) -> dict[str, np.ndarray]:
    geometry = build_geometry_from_step(
        step_path,
        sample_points_per_face=sample_points_per_face,
    )
    labels = load_json(label_path)
    if labels.get("schema") != "doghouse_instance_labels.v1":
        raise ValueError(f"unexpected label schema: {labels.get('schema')}")
    data = build_dataset(geometry, labels)
    data["edge_labels"] = build_edge_labels(data["face_instance"], data["adjacency"])
    data["model_name"] = np.asarray([step_path.stem], dtype=str)
    return data


def prepare_all(
    step_dir: Path,
    output_dir: Path,
    pairs: list[tuple[str, str]] | None = None,
    *,
    sample_points_per_face: int = 64,
) -> list[Path]:
    output_dir.mkdir(parents=True, exist_ok=True)
    pairs = pairs or discover_pairs(step_dir)
    if not pairs:
        raise FileNotFoundError(
            f"no (STEP, doghouse_instance_labels.v1 JSON) pairs found in {step_dir}"
        )
    saved: list[Path] = []
    for step_name, label_name in pairs:
        step_path = step_dir / step_name
        label_path = step_dir / label_name
        if not step_path.exists():
            raise FileNotFoundError(step_path)
        if not label_path.exists():
            raise FileNotFoundError(label_path)
        data = prepare_one(
            step_path,
            label_path,
            sample_points_per_face=sample_points_per_face,
        )
        out = output_dir / f"{step_path.stem}_graph.npz"
        np.savez_compressed(out, **data)
        saved.append(out)
        pos_edges = int(data["edge_labels"].sum())
        print(
            f"saved: {out.name} "
            f"faces={data['face_features'].shape[0]} "
            f"edges={data['adjacency'].shape[0]} "
            f"doghouse_faces={int(data['face_doghouse'].sum())} "
            f"pos_edges={pos_edges}"
        )
    return saved


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--step-dir",
        default="../step - 副本2",
        help="Directory containing STEP files and annotation JSON",
    )
    parser.add_argument(
        "--output-dir",
        default="doghouse_ai/data/graph_train",
    )
    parser.add_argument("--sample-points-per-face", type=int, default=64)
    args = parser.parse_args()
    prepare_all(
        Path(args.step_dir),
        Path(args.output_dir),
        sample_points_per_face=args.sample_points_per_face,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
