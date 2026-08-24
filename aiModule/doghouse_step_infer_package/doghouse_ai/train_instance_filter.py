#!/usr/bin/env python3
"""Train an instance-level doghouse false-positive filter from graph NPZ labels."""

from __future__ import annotations

import argparse
import json
from collections import defaultdict
from pathlib import Path

import numpy as np
import torch
from torch import nn

try:
    from .instance_filter import FEATURE_NAMES, instance_feature_matrix
    from .labels import NEGATIVE_ROLES, ROLE_TO_ID
except ImportError:
    from instance_filter import FEATURE_NAMES, instance_feature_matrix
    from labels import NEGATIVE_ROLES, ROLE_TO_ID


NEGATIVE_IDS = frozenset(ROLE_TO_ID[role] for role in NEGATIVE_ROLES)


def _components(faces: set[int], adjacency: np.ndarray) -> list[list[int]]:
    if not faces:
        return []
    graph: dict[int, set[int]] = defaultdict(set)
    for a, b in adjacency.astype(np.int64):
        a = int(a)
        b = int(b)
        if a in faces and b in faces:
            graph[a].add(b)
            graph[b].add(a)
    seen: set[int] = set()
    comps: list[list[int]] = []
    for start in sorted(faces):
        if start in seen:
            continue
        stack = [start]
        seen.add(start)
        comp: list[int] = []
        while stack:
            cur = stack.pop()
            comp.append(cur)
            for nb in graph.get(cur, ()):
                if nb not in seen:
                    seen.add(nb)
                    stack.append(nb)
        comps.append(sorted(comp))
    return comps


def _result_from_instances(instances: list[list[int]]) -> dict:
    face_predictions = []
    doghouse_instances = []
    for iid, faces in enumerate(instances, 1):
        doghouse_instances.append({"instance_id": iid, "faces": list(faces)})
        for fi in faces:
            face_predictions.append(
                {
                    "face_idx": int(fi),
                    "doghouse": 1,
                    "instance_id": iid,
                    "role": "doghouse",
                    "doghouse_ratio": 1.0,
                }
            )
    return {
        "schema": "doghouse_face_predictions.v1",
        "face_predictions": face_predictions,
        "doghouse_instances": doghouse_instances,
    }


def _instances_from_labels(data: dict) -> dict[int, set[int]]:
    return {
        int(inst["instance_id"]): {int(fi) for fi in inst.get("faces", [])}
        for inst in data.get("doghouse_instances", [])
        if inst.get("faces")
    }


def _instances_from_prediction(data: dict) -> dict[int, set[int]]:
    instances = {
        int(inst["instance_id"]): {int(fi) for fi in inst.get("faces", [])}
        for inst in data.get("doghouse_instances", [])
        if inst.get("faces")
    }
    if instances:
        return instances
    out: dict[int, set[int]] = {}
    for row in data.get("face_predictions", []):
        if int(row.get("doghouse", 0)) <= 0:
            continue
        iid = int(row.get("instance_id", -1))
        if iid > 0:
            out.setdefault(iid, set()).add(int(row["face_idx"]))
    return out


def _iou(a: set[int], b: set[int]) -> float:
    return len(a & b) / max(len(a | b), 1)


def training_examples_from_npz(data: dict[str, np.ndarray]) -> tuple[np.ndarray, np.ndarray]:
    """Return instance features and labels from one labeled graph NPZ."""
    adjacency = np.asarray(data.get("adjacency", np.empty((0, 2))), dtype=np.int64)
    face_instance = np.asarray(data["face_instance"], dtype=np.int64)
    face_doghouse = np.asarray(data["face_doghouse"], dtype=np.int64)
    face_semantic = np.asarray(data["face_semantic"], dtype=np.int64)

    positive_instances = []
    for iid in sorted(int(v) for v in np.unique(face_instance) if int(v) > 0):
        faces = set(np.where((face_instance == iid) & (face_doghouse == 1))[0].astype(int))
        positive_instances.extend(_components(faces, adjacency))

    negative_faces = set(np.where(np.isin(face_semantic, list(NEGATIVE_IDS)))[0].astype(int))
    negative_instances = _components(negative_faces, adjacency)

    rows = []
    labels = []
    if positive_instances:
        x, _ = instance_feature_matrix(_result_from_instances(positive_instances), data)
        rows.append(x)
        labels.append(np.ones(x.shape[0], dtype=np.float32))
    if negative_instances:
        x, _ = instance_feature_matrix(_result_from_instances(negative_instances), data)
        rows.append(x)
        labels.append(np.zeros(x.shape[0], dtype=np.float32))
    if not rows:
        return np.empty((0, len(FEATURE_NAMES)), dtype=np.float32), np.empty(0, dtype=np.float32)
    return np.vstack(rows).astype(np.float32), np.concatenate(labels).astype(np.float32)


def prediction_examples_from_labels(
    prediction: dict,
    labels: dict,
    data: dict[str, np.ndarray],
    *,
    positive_iou: float = 0.5,
) -> tuple[np.ndarray, np.ndarray]:
    """Label predicted components as true/false instances using GT overlap."""
    pred_instances = _instances_from_prediction(prediction)
    gt_instances = _instances_from_labels(labels)
    rows = []
    y = []
    for iid, faces in sorted(pred_instances.items()):
        if not faces:
            continue
        best_iou = max((_iou(faces, gt) for gt in gt_instances.values()), default=0.0)
        result = _result_from_instances([sorted(faces)])
        x, _ = instance_feature_matrix(result, data)
        if x.size:
            rows.append(x[0])
            y.append(1.0 if best_iou >= positive_iou else 0.0)
    if not rows:
        return np.empty((0, len(FEATURE_NAMES)), dtype=np.float32), np.empty(0, dtype=np.float32)
    return np.asarray(rows, dtype=np.float32), np.asarray(y, dtype=np.float32)


def negative_examples_from_failure_report(
    report: dict,
    data: dict[str, np.ndarray],
) -> tuple[np.ndarray, np.ndarray]:
    """Use manually reviewed likely_fragment instances as negative examples."""
    negative_instances = [
        [int(fi) for fi in inst.get("faces", [])]
        for inst in report.get("instances", [])
        if bool(inst.get("likely_fragment")) and inst.get("faces")
    ]
    if not negative_instances:
        return np.empty((0, len(FEATURE_NAMES)), dtype=np.float32), np.empty(0, dtype=np.float32)
    x, _ = instance_feature_matrix(_result_from_instances(negative_instances), data)
    return x.astype(np.float32), np.zeros(x.shape[0], dtype=np.float32)


def fit_logistic_filter(
    features: np.ndarray,
    labels: np.ndarray,
    *,
    epochs: int,
    lr: float,
) -> dict[str, np.ndarray]:
    if features.shape[0] < 2 or len(np.unique(labels)) < 2:
        raise ValueError("instance filter training requires at least one positive and one negative instance")
    mean = features.mean(axis=0).astype(np.float32)
    std = np.maximum(features.std(axis=0), 1e-6).astype(np.float32)
    x = torch.from_numpy(((features - mean) / std).astype(np.float32))
    y = torch.from_numpy(labels.astype(np.float32)).unsqueeze(-1)
    model = nn.Linear(features.shape[1], 1)
    opt = torch.optim.AdamW(model.parameters(), lr=lr, weight_decay=1e-3)
    pos = float(labels.sum())
    neg = float(len(labels) - pos)
    pos_weight = torch.tensor([max(neg / max(pos, 1.0), 1.0)], dtype=torch.float32)
    loss_fn = nn.BCEWithLogitsLoss(pos_weight=pos_weight)
    for _ in range(int(epochs)):
        opt.zero_grad()
        loss = loss_fn(model(x), y)
        loss.backward()
        opt.step()
    with torch.no_grad():
        probs = torch.sigmoid(model(x)).squeeze(-1).numpy()
    return {
        "weights": model.weight.detach().cpu().numpy().reshape(-1).astype(np.float32),
        "bias": model.bias.detach().cpu().numpy().reshape(1).astype(np.float32),
        "mean": mean,
        "std": std,
        "threshold": np.asarray([0.5], dtype=np.float32),
        "train_positive": np.asarray([int(pos)], dtype=np.int64),
        "train_negative": np.asarray([int(neg)], dtype=np.int64),
        "train_score_pos_mean": np.asarray([float(probs[labels == 1].mean())], dtype=np.float32),
        "train_score_neg_mean": np.asarray([float(probs[labels == 0].mean())], dtype=np.float32),
        "feature_names": np.asarray(FEATURE_NAMES),
    }


def train_instance_filter(
    npz_paths: list[Path],
    output: Path,
    *,
    epochs: int,
    lr: float,
    prediction_dirs: list[Path] | None = None,
    label_dir: Path | None = None,
    positive_iou: float = 0.5,
) -> dict[str, np.ndarray]:
    all_x = []
    all_y = []
    for path in npz_paths:
        data = dict(np.load(path, allow_pickle=True))
        x, y = training_examples_from_npz(data)
        if x.size:
            all_x.append(x)
            all_y.append(y)
        if prediction_dirs:
            name = str(data["model_name"][0]) if "model_name" in data else path.stem.replace("_graph", "")
            label_candidates = []
            if label_dir is not None:
                label_candidates = [
                    label_dir / f"{name}_annotation.json",
                    label_dir / f"{name} annotation.json",
                    label_dir / f"{name}.json",
                ]
            label_path = next((p for p in label_candidates if p.exists()), None)
            if label_path is None:
                continue
            labels = json.loads(label_path.read_text(encoding="utf-8"))
            for pred_dir in prediction_dirs:
                pred_path = pred_dir / name / f"{name}_doghouse_pred_faces.json"
                if not pred_path.exists():
                    continue
                prediction = json.loads(pred_path.read_text(encoding="utf-8"))
                x, y = prediction_examples_from_labels(
                    prediction,
                    labels,
                    data,
                    positive_iou=positive_iou,
                )
                if x.size:
                    all_x.append(x)
                    all_y.append(y)
    if not all_x:
        raise ValueError("no doghouse or hard-negative instance examples found")
    features = np.vstack(all_x)
    labels = np.concatenate(all_y)
    model = fit_logistic_filter(features, labels, epochs=epochs, lr=lr)
    output.parent.mkdir(parents=True, exist_ok=True)
    np.savez(output, **model)
    return model


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", default="doghouse_ai/data/graph_train")
    parser.add_argument("--output", default="doghouse_ai/checkpoints/doghouse_instance_filter.npz")
    parser.add_argument("--epochs", type=int, default=800)
    parser.add_argument("--lr", type=float, default=5e-2)
    parser.add_argument(
        "--prediction-dir",
        action="append",
        default=[],
        help="Optional directory containing per-model prediction JSONs; may be repeated to add predicted-component examples",
    )
    parser.add_argument(
        "--label-dir",
        default=None,
        help="Directory containing annotation JSONs for --prediction-dir examples",
    )
    parser.add_argument("--positive-iou", type=float, default=0.5)
    args = parser.parse_args()

    paths = sorted(Path(args.data_dir).glob("*_graph.npz"))
    if not paths:
        raise FileNotFoundError(f"no *_graph.npz files in {args.data_dir}")
    model = train_instance_filter(
        paths,
        Path(args.output),
        epochs=args.epochs,
        lr=args.lr,
        prediction_dirs=[Path(p) for p in args.prediction_dir],
        label_dir=Path(args.label_dir) if args.label_dir else None,
        positive_iou=args.positive_iou,
    )
    print(f"saved instance filter: {args.output}")
    print(f"features: {', '.join(FEATURE_NAMES)}")
    print(f"train positives: {int(model['train_positive'][0])}")
    print(f"train negatives: {int(model['train_negative'][0])}")
    print(f"score pos/neg mean: {float(model['train_score_pos_mean'][0]):.3f}/{float(model['train_score_neg_mean'][0]):.3f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
