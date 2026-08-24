#!/usr/bin/env python3
"""Sweep node/edge thresholds on leave-one-out checkpoints (AI-decision only)."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import torch

try:
    from .graph_model import FaceGraphGNN, adjacency_to_edge_index, normalize_face_features
    from .infer_graph import graph_result_from_arrays
    from .train_graph import face_iou, instance_metrics, load_graph_npz
except ImportError:
    from graph_model import FaceGraphGNN, adjacency_to_edge_index, normalize_face_features
    from infer_graph import graph_result_from_arrays
    from train_graph import face_iou, instance_metrics, load_graph_npz


def _probs(model, data, device):
    x = torch.from_numpy(normalize_face_features(data["face_features"])).to(device)
    edge_index = adjacency_to_edge_index(data["adjacency"]).to(device)
    adj = data["adjacency"].astype(np.int64)
    edge_pairs = torch.from_numpy(adj.T.copy()).to(device)
    with torch.no_grad():
        nl, el = model(x, edge_index, edge_pairs)
        npb = torch.sigmoid(nl).cpu().numpy()
        epb = torch.sigmoid(el).cpu().numpy() if el.numel() else np.empty(0, dtype=np.float32)
    return npb, epb


def _gt_instances(data):
    inst: dict[int, set[int]] = {}
    for fi, iid in enumerate(data["face_instance"]):
        if int(iid) > 0:
            inst.setdefault(int(iid), set()).add(int(fi))
    return list(inst.values())


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", default="doghouse_ai/data/graph_train")
    parser.add_argument("--loo-dir", default="doghouse_ai/data/graph_loo")
    parser.add_argument("--node-thresholds", default="0.4,0.5,0.6")
    parser.add_argument("--edge-thresholds", default="0.2,0.3,0.4,0.5")
    parser.add_argument("--cpu", action="store_true")
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    data_dir = Path(args.data_dir)
    loo_dir = Path(args.loo_dir)
    node_ths = [float(x) for x in args.node_thresholds.split(",")]
    edge_ths = [float(x) for x in args.edge_thresholds.split(",")]

    npz_paths = sorted(data_dir.glob("*_graph.npz"))
    # Load each holdout model + matching LOO checkpoint once.
    loaded = []
    for path in npz_paths:
        ckpt_path = loo_dir / f"loo_{path.stem}.pt"
        if not ckpt_path.exists():
            print(f"skip (no ckpt): {path.stem}")
            continue
        ckpt = torch.load(ckpt_path, map_location=device)
        model = FaceGraphGNN(
            in_dim=int(ckpt["in_dim"]),
            hidden_dim=int(ckpt.get("hidden_dim", 128)),
            num_layers=int(ckpt.get("num_layers", 4)),
            dropout=float(ckpt.get("dropout", 0.2)),
            num_semantic=int(ckpt.get("num_semantic", 0)),
            hole_wall_head=bool(ckpt.get("hole_wall_head", False)),
            mount_head=bool(ckpt.get("mount_head", False)),
        ).to(device)
        model.load_state_dict(ckpt["model_state"])
        model.eval()
        data = load_graph_npz(path)
        npb, epb = _probs(model, data, device)
        loaded.append((path.stem, data, npb, epb, _gt_instances(data)))

    print(f"{'node':>5} {'edge':>5} {'meanP':>7} {'meanR':>7} {'meanIoU':>8}  per-model pred/gt")
    best = None
    for nt in node_ths:
        for et in edge_ths:
            ps, rs, ious, detail = [], [], [], []
            for stem, data, npb, epb, gts in loaded:
                result = graph_result_from_arrays(
                    npb, epb, data["adjacency"],
                    node_threshold=nt, edge_threshold=et,
                )
                preds = [set(i["faces"]) for i in result["doghouse_instances"]]
                m = instance_metrics(preds, gts)
                pmask = np.array([r["doghouse"] for r in result["face_predictions"]], dtype=bool)
                iou = face_iou(pmask, data["face_doghouse"].astype(bool))
                ps.append(m["precision"]); rs.append(m["recall"]); ious.append(iou)
                detail.append(f"{len(preds)}/{len(gts)}")
            mp, mr, mi = float(np.mean(ps)), float(np.mean(rs)), float(np.mean(ious))
            f1 = 2 * mp * mr / max(mp + mr, 1e-9)
            print(f"{nt:>5} {et:>5} {mp:>7.3f} {mr:>7.3f} {mi:>8.3f}  {' '.join(detail)}")
            if best is None or f1 > best[0]:
                best = (f1, nt, et, mp, mr, mi)
    if best:
        print(
            f"\nbest F1={best[0]:.3f} at node={best[1]} edge={best[2]} "
            f"(P={best[3]:.3f} R={best[4]:.3f} IoU={best[5]:.3f})"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
