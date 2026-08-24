#!/usr/bin/env python3
"""Build doghouse AI point dataset from CAD geometry JSON + label JSON.

Inputs:
  - geometry JSON: exported by doghouse_inference_export.FCMacro
  - label JSON: exported by doghouse_instance_label_tool.FCMacro

Output .npz fields:
  points:        [N, 3] xyz
  features:      [N, F] per-point numeric features
  face_idx:      [N] source CAD face index
  local_face_idx:[N] per-model source CAD face index
  model_idx:     [N] source model index
  semantic:      [N] role id (background/doghouse/mount/...)
  doghouse:      [N] 0/1 instance mask
  instance_id:   [N] doghouse instance id, -1 background
  face_features: [num_faces, F_face]
  face_semantic: [num_faces]
  face_instance: [num_faces]
  adjacency:     [E, 2]
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np

try:
    from .labels import FACE_TYPE_TO_ID, INTERNAL_HARD_NEGATIVE_ROLES, NEGATIVE_ROLES, ROLE_TO_ID
except ImportError:
    from labels import FACE_TYPE_TO_ID, INTERNAL_HARD_NEGATIVE_ROLES, NEGATIVE_ROLES, ROLE_TO_ID


def load_json(path: str | Path) -> dict:
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def _label_maps(label_json: dict, num_faces: int):
    face_semantic = np.zeros(num_faces, dtype=np.int64)
    face_instance = np.full(num_faces, -1, dtype=np.int64)
    face_doghouse = np.zeros(num_faces, dtype=np.int64)

    negative_overrides = {}
    for row in label_json.get("face_labels", []):
        idx = int(row["face_idx"])
        if not (0 <= idx < num_faces):
            continue
        role = str(row.get("role", "background"))
        face_semantic[idx] = ROLE_TO_ID.get(role, 0)
        if role in NEGATIVE_ROLES:
            negative_overrides[idx] = role
            face_instance[idx] = -1
            face_doghouse[idx] = 0
            continue
        iid = int(row.get("instance_id", -1))
        face_instance[idx] = iid
        if role in INTERNAL_HARD_NEGATIVE_ROLES and iid > 0:
            face_doghouse[idx] = 1
        else:
            face_doghouse[idx] = int(row.get("doghouse", 1 if iid > 0 else 0))

    for idx, role in negative_overrides.items():
        face_semantic[idx] = ROLE_TO_ID.get(role, 0)
        face_instance[idx] = -1
        face_doghouse[idx] = 0

    # If face_labels is absent, fall back to doghouse_instances fields.
    if not label_json.get("face_labels"):
        for inst in label_json.get("doghouse_instances", []):
            iid = int(inst["instance_id"])
            for idx in inst.get("faces", []):
                face_doghouse[int(idx)] = 1
                face_instance[int(idx)] = iid
                face_semantic[int(idx)] = ROLE_TO_ID["doghouse"]
            role_fields = {
                "mount_faces": "mount",
                "hole_wall_faces": "hole_wall",
                "hole_bottom_faces": "hole_bottom",
                "transition_faces": "transition",
                "root_boundary_faces": "root_boundary",
            }
            for field, role in role_fields.items():
                for idx in inst.get(field, []):
                    face_semantic[int(idx)] = ROLE_TO_ID[role]
                    face_instance[int(idx)] = iid
                    face_doghouse[int(idx)] = 1

            internal_hard_negative_fields = {
                "non_hole_cylinder_faces": "non_hole_cylinder",
                "non_hole_fillet_faces": "non_hole_fillet",
            }
            for field, role in internal_hard_negative_fields.items():
                for idx in inst.get(field, []):
                    idx = int(idx)
                    if 0 <= idx < num_faces:
                        face_semantic[idx] = ROLE_TO_ID[role]
                        face_instance[idx] = iid
                        face_doghouse[idx] = 1

        for role, indices in label_json.get("hard_negative_faces", {}).items():
            role_id = ROLE_TO_ID.get(str(role))
            if role_id is None:
                continue
            for idx in indices:
                idx = int(idx)
                if 0 <= idx < num_faces:
                    face_semantic[idx] = role_id
                    face_instance[idx] = -1
                    face_doghouse[idx] = 0

    return face_semantic, face_instance, face_doghouse


def _face_numeric_features(face: dict) -> list[float]:
    bbox = face.get("bbox") or {}
    lo = np.asarray(bbox.get("min", [0.0, 0.0, 0.0]), dtype=float)
    hi = np.asarray(bbox.get("max", [0.0, 0.0, 0.0]), dtype=float)
    span = hi - lo
    normal = face.get("normal")
    if normal is None:
        normal = [0.0, 0.0, 0.0]
    return [
        float(FACE_TYPE_TO_ID.get(str(face.get("face_type", "other")), 0)),
        float(face.get("area", 0.0)),
        float(face.get("radius", 0.0)),
        float(face.get("has_radius", 0)),
        float(face.get("u_range", 0.0)),
        float(face.get("v_range", 0.0)),
        float(span[0]),
        float(span[1]),
        float(span[2]),
        float(normal[0]),
        float(normal[1]),
        float(normal[2]),
    ]


def build_dataset(
    geometry_json: dict,
    label_json: dict,
    *,
    model_idx: int = 0,
) -> dict[str, np.ndarray]:
    faces = geometry_json.get("faces", [])
    num_faces = int(geometry_json.get("num_faces", len(faces)))
    if len(faces) != num_faces:
        raise ValueError(f"geometry faces length {len(faces)} != num_faces {num_faces}")

    face_semantic, face_instance, face_doghouse = _label_maps(label_json, num_faces)

    face_features = np.asarray(
        [_face_numeric_features(face) for face in faces],
        dtype=np.float32,
    )

    points = []
    features = []
    face_idx = []
    local_face_idx = []
    model_indices = []
    semantic = []
    doghouse = []
    instance_id = []

    for face in faces:
        idx = int(face["face_idx"])
        pts = face.get("sample_points") or []
        if not pts:
            # Keep one centroid point so face-only exports are still usable.
            pts = [face.get("centroid", [0.0, 0.0, 0.0])]
        ffeat = face_features[idx]
        for pt in pts:
            points.append(pt)
            features.append(ffeat)
            face_idx.append(idx)
            local_face_idx.append(idx)
            model_indices.append(int(model_idx))
            semantic.append(face_semantic[idx])
            doghouse.append(face_doghouse[idx])
            instance_id.append(face_instance[idx])

    adjacency = np.asarray(
        [
            [int(edge["a"]), int(edge["b"])]
            for edge in geometry_json.get("adjacency_edges", [])
        ],
        dtype=np.int64,
    )
    if adjacency.size == 0:
        adjacency = adjacency.reshape(0, 2)

    return {
        "points": np.asarray(points, dtype=np.float32),
        "features": np.asarray(features, dtype=np.float32),
        "face_idx": np.asarray(face_idx, dtype=np.int64),
        "local_face_idx": np.asarray(local_face_idx, dtype=np.int64),
        "model_idx": np.asarray(model_indices, dtype=np.int64),
        "semantic": np.asarray(semantic, dtype=np.int64),
        "doghouse": np.asarray(doghouse, dtype=np.int64),
        "instance_id": np.asarray(instance_id, dtype=np.int64),
        "face_features": face_features,
        "face_semantic": face_semantic,
        "face_doghouse": face_doghouse,
        "face_instance": face_instance,
        "face_model_idx": np.full(num_faces, int(model_idx), dtype=np.int64),
        "adjacency": adjacency,
    }


def build_multi_dataset(
    datasets: list[dict[str, np.ndarray]],
    *,
    model_names: list[str] | None = None,
) -> dict[str, np.ndarray]:
    """Combine per-model datasets while offsetting face indices and adjacency."""
    if not datasets:
        raise ValueError("at least one dataset is required")

    point_keys = [
        "points",
        "features",
        "local_face_idx",
        "model_idx",
        "semantic",
        "doghouse",
        "instance_id",
    ]
    face_keys = [
        "face_features",
        "face_semantic",
        "face_doghouse",
        "face_instance",
        "face_model_idx",
    ]
    out = {key: [] for key in point_keys + face_keys}
    out["face_idx"] = []
    out["adjacency"] = []
    face_offsets = []

    face_offset = 0
    for data in datasets:
        num_faces = int(data["face_features"].shape[0])
        face_offsets.append(face_offset)

        for key in point_keys:
            out[key].append(data[key])
        out["face_idx"].append(data["face_idx"] + face_offset)
        for key in face_keys:
            out[key].append(data[key])

        adjacency = data["adjacency"]
        if len(adjacency):
            out["adjacency"].append(adjacency + face_offset)
        face_offset += num_faces

    merged = {}
    for key, chunks in out.items():
        if key == "adjacency" and not chunks:
            merged[key] = np.empty((0, 2), dtype=np.int64)
        else:
            merged[key] = np.concatenate(chunks, axis=0)
    merged["face_offsets"] = np.asarray(face_offsets, dtype=np.int64)
    if model_names is None:
        model_names = [str(i) for i in range(len(datasets))]
    merged["model_names"] = np.asarray(model_names, dtype=str)
    return merged


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--geometry-json", nargs="+", required=True)
    parser.add_argument("--label-json", nargs="+", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    if len(args.geometry_json) != len(args.label_json):
        raise ValueError("--geometry-json and --label-json counts must match")

    datasets = []
    model_names = []
    for model_idx, (geometry_path, label_path) in enumerate(
        zip(args.geometry_json, args.label_json)
    ):
        geometry = load_json(geometry_path)
        labels = load_json(label_path)
        if geometry.get("schema") != "doghouse_inference_geometry.v1":
            raise ValueError(f"unexpected geometry schema: {geometry.get('schema')}")
        if labels.get("schema") != "doghouse_instance_labels.v1":
            raise ValueError(f"unexpected label schema: {labels.get('schema')}")
        datasets.append(build_dataset(geometry, labels, model_idx=model_idx))
        model_names.append(Path(geometry_path).stem)

    data = datasets[0] if len(datasets) == 1 else build_multi_dataset(
        datasets, model_names=model_names,
    )
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(output, **data)

    print(f"saved: {output}")
    print(f"points: {data['points'].shape}")
    print(f"features: {data['features'].shape}")
    print(f"faces: {data['face_features'].shape}")
    print(f"adjacency: {data['adjacency'].shape}")
    print(f"doghouse points: {int(data['doghouse'].sum())}")
    print(f"instances: {sorted(set(int(x) for x in data['instance_id'] if int(x) > 0))}")
    print(f"models: {len(datasets)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
