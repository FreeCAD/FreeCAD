#!/usr/bin/env python3
"""Lightweight instance-level false-positive filter for doghouse predictions."""

from __future__ import annotations

import copy
import json
from pathlib import Path

import numpy as np


FEATURE_NAMES = (
    "face_count",
    "area_sum",
    "area_mean",
    "area_max",
    "prob_mean",
    "prob_min",
    "span_max",
    "span_mean",
)


def _instance_faces(result: dict) -> list[tuple[int, list[int]]]:
    instances = result.get("doghouse_instances", [])
    out: list[tuple[int, list[int]]] = []
    for inst in instances:
        iid = int(inst["instance_id"])
        faces = sorted(int(fi) for fi in inst.get("faces", []))
        if faces:
            out.append((iid, faces))
    return sorted(out, key=lambda row: row[0])


def instance_feature_matrix(
    result: dict,
    data: dict[str, np.ndarray],
) -> tuple[np.ndarray, list[int]]:
    """Build fixed-size aggregate features for each predicted doghouse instance."""
    face_features = np.asarray(data["face_features"], dtype=np.float32)
    prob_by_face = {
        int(row["face_idx"]): float(row.get("doghouse_ratio", row.get("probability", 0.0)))
        for row in result.get("face_predictions", [])
    }

    rows: list[list[float]] = []
    ids: list[int] = []
    for iid, faces in _instance_faces(result):
        f = face_features[np.asarray(faces, dtype=np.int64)]
        areas = f[:, 1] if f.shape[1] > 1 else np.zeros(len(faces), dtype=np.float32)
        spans = f[:, 6:9] if f.shape[1] >= 9 else np.zeros((len(faces), 3), dtype=np.float32)
        probs = np.asarray([prob_by_face.get(fi, 0.0) for fi in faces], dtype=np.float32)
        rows.append(
            [
                float(len(faces)),
                float(areas.sum()),
                float(areas.mean()) if areas.size else 0.0,
                float(areas.max()) if areas.size else 0.0,
                float(probs.mean()) if probs.size else 0.0,
                float(probs.min()) if probs.size else 0.0,
                float(spans.max()) if spans.size else 0.0,
                float(spans.mean()) if spans.size else 0.0,
            ]
        )
        ids.append(iid)
    if not rows:
        return np.empty((0, len(FEATURE_NAMES)), dtype=np.float32), []
    return np.asarray(rows, dtype=np.float32), ids


def load_instance_filter(path: str | Path) -> dict[str, np.ndarray | float]:
    obj = dict(np.load(path, allow_pickle=True))
    threshold = float(np.asarray(obj.get("threshold", 0.5)).reshape(-1)[0])
    return {
        "weights": np.asarray(obj["weights"], dtype=np.float32).reshape(-1),
        "bias": float(np.asarray(obj.get("bias", 0.0)).reshape(-1)[0]),
        "mean": np.asarray(obj.get("mean", np.zeros(len(FEATURE_NAMES))), dtype=np.float32),
        "std": np.asarray(obj.get("std", np.ones(len(FEATURE_NAMES))), dtype=np.float32),
        "threshold": threshold,
    }


def score_instances(
    features: np.ndarray,
    model: dict[str, np.ndarray | float],
) -> np.ndarray:
    if features.size == 0:
        return np.empty(0, dtype=np.float32)
    weights = np.asarray(model["weights"], dtype=np.float32).reshape(-1)
    if weights.shape[0] != features.shape[1]:
        raise ValueError(f"instance filter weight dim {weights.shape[0]} != feature dim {features.shape[1]}")
    mean = np.asarray(model["mean"], dtype=np.float32).reshape(-1)
    std = np.asarray(model["std"], dtype=np.float32).reshape(-1)
    z = (features - mean) / np.maximum(std, 1e-6)
    logits = z @ weights + float(model["bias"])
    return (1.0 / (1.0 + np.exp(-logits))).astype(np.float32)


def apply_instance_filter(
    result: dict,
    data: dict[str, np.ndarray],
    filter_path: str | Path,
    *,
    threshold: float | None = None,
) -> dict:
    """Remove predicted instances whose learned instance score is below threshold."""
    model = load_instance_filter(filter_path)
    keep_threshold = float(model["threshold"] if threshold is None else threshold)
    features, ids = instance_feature_matrix(result, data)
    scores = score_instances(features, model)
    score_by_id = {iid: float(score) for iid, score in zip(ids, scores)}
    keep_ids = {iid for iid, score in score_by_id.items() if score >= keep_threshold}

    filtered = copy.deepcopy(result)
    rejected = []
    kept_instances = []
    for inst in filtered.get("doghouse_instances", []):
        iid = int(inst["instance_id"])
        score = score_by_id.get(iid, 0.0)
        inst["instance_filter_score"] = round(score, 6)
        if iid in keep_ids:
            kept_instances.append(inst)
        else:
            rejected.append(inst)
    filtered["doghouse_instances"] = kept_instances
    filtered["rejected_doghouse_instances"] = rejected
    filtered["instance_filter"] = {
        "path": str(filter_path),
        "threshold": round(keep_threshold, 6),
        "feature_names": list(FEATURE_NAMES),
    }

    removed_ids = {int(inst["instance_id"]) for inst in rejected}
    for row in filtered.get("face_predictions", []):
        if int(row.get("instance_id", -1)) in removed_ids:
            row["doghouse"] = 0
            row["instance_id"] = -1
            row["role"] = "background"
            row["instance_filter_rejected"] = True
    return filtered


def main() -> int:
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--prediction", required=True)
    parser.add_argument("--npz", required=True)
    parser.add_argument("--filter", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--threshold", type=float)
    args = parser.parse_args()

    result = json.loads(Path(args.prediction).read_text(encoding="utf-8"))
    data = dict(np.load(args.npz, allow_pickle=True))
    filtered = apply_instance_filter(result, data, args.filter, threshold=args.threshold)
    out = Path(args.output)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(filtered, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"saved: {out}")
    print(f"kept instances: {len(filtered['doghouse_instances'])}")
    print(f"rejected instances: {len(filtered.get('rejected_doghouse_instances', []))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
