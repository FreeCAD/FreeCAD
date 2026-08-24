#!/usr/bin/env python3
"""Export per-model point clouds (unit-sphere normalized) for Point-MAE feature extraction.

Bridge step 1 of the Point-MAE hybrid pipeline (runs on any machine, no GPU):
reads the face-graph training npz (which already carries per-point ``points`` +
``face_idx``) and writes a compact ``{model_name}_pmae_input.npz`` per model:

    points:    [N, 3] float32, normalized to the unit sphere (centered, scaled)
    face_idx:  [N]    int64,   source CAD face index for each point
    num_faces: scalar int

These files are copied to the GPU/Linux machine and consumed by
``extract_pmae_face_features.py`` there (Point-MAE needs CUDA ops).
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np


def normalize_unit_sphere(points: np.ndarray) -> np.ndarray:
    pts = points.astype(np.float32).copy()
    center = pts.mean(axis=0, keepdims=True)
    pts -= center
    scale = float(np.linalg.norm(pts, axis=1).max())
    if scale < 1e-6:
        scale = 1.0
    return (pts / scale).astype(np.float32)


def export_one(graph_npz: Path, output_dir: Path) -> Path:
    data = np.load(graph_npz, allow_pickle=True)
    points = data["points"]
    face_idx = data["face_idx"].astype(np.int64)
    num_faces = int(data["face_features"].shape[0])
    name = str(data["model_name"][0]) if "model_name" in data.files else graph_npz.stem.replace("_graph", "")

    norm_points = normalize_unit_sphere(points)
    out = output_dir / f"{name}_pmae_input.npz"
    output_dir.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(out, points=norm_points, face_idx=face_idx, num_faces=num_faces)
    print(f"saved: {out.name} points={norm_points.shape} num_faces={num_faces}")
    return out


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--graph-dir", default="doghouse_ai/data/graph_train")
    parser.add_argument("--output-dir", default="doghouse_ai/data/pmae_input")
    args = parser.parse_args()

    graph_dir = Path(args.graph_dir)
    npz_paths = sorted(graph_dir.glob("*_graph.npz"))
    if not npz_paths:
        raise FileNotFoundError(f"no *_graph.npz in {graph_dir}")
    for path in npz_paths:
        export_one(path, Path(args.output_dir))
    print(f"done: {len(npz_paths)} models -> {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
