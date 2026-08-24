#!/usr/bin/env python3
"""Inference for face-graph GNN doghouse detection and AI instance separation."""

from __future__ import annotations

import argparse
import json
from collections import defaultdict
from pathlib import Path

import numpy as np
import torch

try:
    from .graph_model import FaceGraphGNN, adjacency_to_edge_index, normalize_face_features
    from .pipeline_defaults import (
        DEFAULT_EDGE_THRESHOLD,
        DEFAULT_MIN_INSTANCE_FACES,
        DEFAULT_NODE_THRESHOLD,
        apply_graph_postprocess,
        resolve_checkpoint,
    )
except ImportError:
    from graph_model import FaceGraphGNN, adjacency_to_edge_index, normalize_face_features
    from pipeline_defaults import (
        DEFAULT_EDGE_THRESHOLD,
        DEFAULT_MIN_INSTANCE_FACES,
        DEFAULT_NODE_THRESHOLD,
        apply_graph_postprocess,
        resolve_checkpoint,
    )


def connected_components(
    dog_faces: set[int],
    adjacency: np.ndarray,
    edge_prob: np.ndarray,
    *,
    edge_threshold: float = 0.5,
) -> dict[int, int]:
    """Split doghouse faces into instances using only AI-positive edges."""
    if not dog_faces:
        return {}
    graph: dict[int, set[int]] = defaultdict(set)
    for ei, (a, b) in enumerate(adjacency):
        a = int(a)
        b = int(b)
        if a not in dog_faces or b not in dog_faces:
            continue
        if edge_prob.size and float(edge_prob[ei]) < edge_threshold:
            continue
        graph[a].add(b)
        graph[b].add(a)

    seen: set[int] = set()
    comps: list[list[int]] = []
    for start in sorted(dog_faces):
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

    comps.sort(key=lambda c: (-len(c), c[0]))
    out: dict[int, int] = {}
    for iid, comp in enumerate(comps, 1):
        for fi in comp:
            out[int(fi)] = iid
    return out


def graph_result_from_arrays(
    node_prob: np.ndarray,
    edge_prob: np.ndarray,
    adjacency: np.ndarray,
    *,
    node_threshold: float = 0.5,
    edge_threshold: float = 0.5,
    min_instance_faces: int = 1,
    hole_wall_prob: np.ndarray | None = None,
    hole_wall_threshold: float = 0.35,
    mount_prob: np.ndarray | None = None,
    mount_threshold: float = 0.35,
) -> dict:
    """Build doghouse_face_predictions.v1 result from raw probabilities."""
    num_faces = len(node_prob)
    dog_faces = {
        fi for fi in range(num_faces) if float(node_prob[fi]) >= node_threshold
    }
    component_ids = connected_components(
        dog_faces,
        adjacency,
        edge_prob,
        edge_threshold=edge_threshold,
    )

    rows = []
    for fi in range(num_faces):
        is_dog = fi in dog_faces and fi in component_ids
        iid = int(component_ids.get(fi, -1)) if is_dog else -1
        hole_p = float(hole_wall_prob[fi]) if hole_wall_prob is not None else None
        mount_p = float(mount_prob[fi]) if mount_prob is not None else None
        is_hole = bool(hole_p is not None and hole_p >= hole_wall_threshold)
        is_mount = bool(mount_p is not None and mount_p >= mount_threshold)
        if is_mount and is_dog:
            role = "mount"
        elif is_hole and is_dog:
            role = "hole_wall"
        elif is_dog:
            role = "doghouse"
        else:
            role = "background"
        row = {
            "face_idx": int(fi),
            "doghouse": 1 if is_dog else 0,
            "instance_id": iid,
            "role": role,
            "doghouse_ratio": round(float(node_prob[fi]), 6),
        }
        if hole_p is not None:
            row["hole_wall_prob"] = round(hole_p, 6)
            row["hole_wall"] = 1 if is_hole else 0
        if mount_p is not None:
            row["mount_prob"] = round(mount_p, 6)
            row["mount"] = 1 if is_mount else 0
        rows.append(row)

    instances: dict[int, set[int]] = defaultdict(set)
    for row in rows:
        if row["doghouse"]:
            instances[int(row["instance_id"])].add(int(row["face_idx"]))

    removed_instances = []
    min_instance_faces = max(int(min_instance_faces), 1)
    if min_instance_faces > 1:
        kept_instances: dict[int, set[int]] = {}
        old_to_new: dict[int, int] = {}
        for iid, faces in sorted(instances.items()):
            if iid <= 0:
                continue
            if len(faces) < min_instance_faces:
                removed_instances.append({"instance_id": int(iid), "faces": sorted(faces)})
            else:
                new_iid = len(kept_instances) + 1
                kept_instances[new_iid] = faces
                old_to_new[int(iid)] = new_iid
        removed_ids = {int(inst["instance_id"]) for inst in removed_instances}
        for row in rows:
            iid = int(row.get("instance_id", -1))
            if iid in removed_ids:
                row["doghouse"] = 0
                row["instance_id"] = -1
                row["role"] = "background"
                row["small_instance_removed"] = True
            elif iid in old_to_new:
                row["instance_id"] = old_to_new[iid]
        instances = kept_instances

    out = {
        "schema": "doghouse_face_predictions.v1",
        "backbone": "face_graph_gnn",
        "face_predictions": rows,
        "doghouse_instances": [
            {"instance_id": int(iid), "faces": sorted(faces)}
            for iid, faces in sorted(instances.items())
            if iid > 0
        ],
        "node_probability": {
            "min": round(float(node_prob.min()), 6),
            "max": round(float(node_prob.max()), 6),
            "mean": round(float(node_prob.mean()), 6),
        },
        "edge_probability": {
            "min": round(float(edge_prob.min()), 6) if edge_prob.size else 0.0,
            "max": round(float(edge_prob.max()), 6) if edge_prob.size else 0.0,
            "mean": round(float(edge_prob.mean()), 6) if edge_prob.size else 0.0,
        },
        "removed_small_doghouse_instances": removed_instances,
        "min_instance_faces": min_instance_faces,
    }
    if hole_wall_prob is not None:
        out["hole_wall_probability"] = {
            "min": round(float(hole_wall_prob.min()), 6),
            "max": round(float(hole_wall_prob.max()), 6),
            "mean": round(float(hole_wall_prob.mean()), 6),
        }
        out["hole_wall_threshold"] = float(hole_wall_threshold)
    if mount_prob is not None:
        out["mount_probability"] = {
            "min": round(float(mount_prob.min()), 6),
            "max": round(float(mount_prob.max()), 6),
            "mean": round(float(mount_prob.mean()), 6),
        }
        out["mount_threshold"] = float(mount_threshold)
    return out


def infer_graph_npz(
    data: dict[str, np.ndarray],
    checkpoint: Path,
    *,
    device: torch.device | None = None,
    node_threshold: float = 0.5,
    edge_threshold: float = 0.5,
    min_instance_faces: int = 1,
    hole_wall_threshold: float = 0.35,
    mount_threshold: float = 0.35,
) -> dict:
    if device is None:
        device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    ckpt = torch.load(checkpoint, map_location=device)
    extra_dim = int(ckpt.get("extra_dim", 0))
    model = FaceGraphGNN(
        in_dim=int(ckpt["in_dim"]),
        hidden_dim=int(ckpt.get("hidden_dim", 128)),
        num_layers=int(ckpt.get("num_layers", 4)),
        dropout=float(ckpt.get("dropout", 0.2)),
        num_semantic=int(ckpt.get("num_semantic", 0)),
        extra_dim=extra_dim,
        hole_wall_head=bool(ckpt.get("hole_wall_head", False)),
        mount_head=bool(ckpt.get("mount_head", False)),
    ).to(device)
    model.load_state_dict(ckpt["model_state"])
    model.eval()

    x = torch.from_numpy(normalize_face_features(data["face_features"])).to(device)
    edge_index = adjacency_to_edge_index(data["adjacency"]).to(device)
    adj = data["adjacency"].astype(np.int64)
    edge_pairs = torch.from_numpy(adj.T.copy()).to(device)
    extra = None
    if extra_dim > 0:
        if "face_pmae" not in data:
            raise ValueError(
                "checkpoint expects Point-MAE face embeddings (extra_dim>0) but npz has no 'face_pmae'"
            )
        extra = torch.from_numpy(data["face_pmae"].astype(np.float32)).to(device)
    with torch.no_grad():
        node_logits, edge_logits, _sem, hole_logits, mount_logits = model.forward_all(
            x, edge_index, edge_pairs, extra
        )
        node_prob = torch.sigmoid(node_logits).cpu().numpy()
        edge_prob = (
            torch.sigmoid(edge_logits).cpu().numpy()
            if edge_logits.numel()
            else np.empty(0, dtype=np.float32)
        )
        hole_prob = (
            torch.sigmoid(hole_logits).cpu().numpy()
            if hole_logits is not None
            else None
        )
        mount_p = (
            torch.sigmoid(mount_logits).cpu().numpy()
            if mount_logits is not None
            else None
        )
    return graph_result_from_arrays(
        node_prob,
        edge_prob,
        data["adjacency"],
        node_threshold=node_threshold,
        edge_threshold=edge_threshold,
        min_instance_faces=min_instance_faces,
        hole_wall_prob=hole_prob,
        hole_wall_threshold=hole_wall_threshold,
        mount_prob=mount_p,
        mount_threshold=mount_threshold,
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Graph GNN doghouse inference with integrated production post-processing",
    )
    parser.add_argument("--npz", required=True)
    parser.add_argument(
        "--checkpoint",
        help="Graph checkpoint; defaults to production mixed PMAE weights",
    )
    parser.add_argument("--output", required=True)
    parser.add_argument("--node-threshold", type=float, default=DEFAULT_NODE_THRESHOLD)
    parser.add_argument("--edge-threshold", type=float, default=DEFAULT_EDGE_THRESHOLD)
    parser.add_argument(
        "--min-instance-faces",
        type=int,
        default=DEFAULT_MIN_INSTANCE_FACES,
        help="Remove predicted doghouse instances with fewer faces than this value",
    )
    parser.add_argument(
        "--instance-sim-filter",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Apply production instance-similarity gallery (default: enabled)",
    )
    parser.add_argument(
        "--instance-sim-gallery",
        help="Override production instance-similarity gallery .npz",
    )
    parser.add_argument(
        "--instance-sim-threshold",
        type=float,
        help="Override threshold stored in the instance-similarity gallery",
    )
    parser.add_argument(
        "--instance-filter",
        help="Deprecated legacy instance classifier; prefer --instance-sim-filter",
    )
    parser.add_argument(
        "--instance-filter-threshold",
        type=float,
        help="Override threshold stored in --instance-filter",
    )
    parser.add_argument("--cpu", action="store_true")
    args = parser.parse_args()

    data = dict(np.load(args.npz, allow_pickle=True))
    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    checkpoint = resolve_checkpoint(args.checkpoint, backbone="graph")
    result = infer_graph_npz(
        data,
        checkpoint,
        device=device,
        node_threshold=args.node_threshold,
        edge_threshold=args.edge_threshold,
        min_instance_faces=args.min_instance_faces,
    )
    result = apply_graph_postprocess(
        result,
        data,
        instance_sim_gallery=args.instance_sim_gallery,
        instance_sim_threshold=args.instance_sim_threshold,
        enable_instance_sim=bool(args.instance_sim_filter),
        instance_filter=args.instance_filter,
        instance_filter_threshold=args.instance_filter_threshold,
    )
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"saved: {output}")
    print(f"instances: {len(result['doghouse_instances'])}")
    print(f"doghouse faces: {sum(1 for r in result['face_predictions'] if r['doghouse'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
