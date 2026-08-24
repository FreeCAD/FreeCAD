#!/usr/bin/env python3
"""Train face-graph GNN for doghouse detection and AI-driven instance separation."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import numpy as np
import torch
from torch import nn

try:
    from .graph_model import FaceGraphGNN, adjacency_to_edge_index, normalize_face_features
    from .labels import NEGATIVE_ROLES, ROLE_TO_ID
    from .prepare_graph_data import prepare_all
except ImportError:
    from graph_model import FaceGraphGNN, adjacency_to_edge_index, normalize_face_features
    from labels import NEGATIVE_ROLES, ROLE_TO_ID
    from prepare_graph_data import prepare_all


# Semantic ids of explicit hard-negative faces (rib / boundary / protrusion).
# These are the faces the model most often false-positives on, so they get a
# larger loss weight to directly suppress spurious doghouse predictions.
HARD_NEG_SEMANTIC_IDS = frozenset(ROLE_TO_ID[r] for r in NEGATIVE_ROLES)


def node_weights_from_semantic(
    face_semantic: np.ndarray,
    *,
    hard_neg_weight: float,
) -> np.ndarray:
    w = np.ones(len(face_semantic), dtype=np.float32)
    if hard_neg_weight != 1.0:
        mask = np.isin(face_semantic, list(HARD_NEG_SEMANTIC_IDS))
        w[mask] = float(hard_neg_weight)
    return w


def load_graph_npz(path: Path, pmae_dir: Path | None = None) -> dict[str, np.ndarray]:
    data = dict(np.load(path, allow_pickle=True))
    if pmae_dir is not None:
        name = str(data["model_name"][0]) if "model_name" in data else path.stem.replace("_graph", "")
        emb_path = Path(pmae_dir) / f"{name}_pmae_face_emb.npy"
        if emb_path.exists():
            emb = np.load(emb_path).astype(np.float32)
            if emb.shape[0] != data["face_features"].shape[0]:
                raise ValueError(
                    f"pmae emb faces {emb.shape[0]} != face_features {data['face_features'].shape[0]} for {name}"
                )
            data["face_pmae"] = emb
    return data


def graph_tensors(
    data: dict[str, np.ndarray],
    device: torch.device,
    *,
    hard_neg_weight: float = 1.0,
) -> tuple[torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor, torch.Tensor]:
    x = torch.from_numpy(normalize_face_features(data["face_features"])).to(device)
    edge_index = adjacency_to_edge_index(data["adjacency"]).to(device)
    adj = data["adjacency"].astype(np.int64)
    edge_pairs = torch.from_numpy(adj.T.copy()).to(device)
    node_y = torch.from_numpy(data["face_doghouse"].astype(np.float32)).to(device)
    edge_y = torch.from_numpy(data["edge_labels"].astype(np.float32)).to(device)
    node_w = torch.from_numpy(
        node_weights_from_semantic(data["face_semantic"], hard_neg_weight=hard_neg_weight)
    ).to(device)
    sem_y = torch.from_numpy(data["face_semantic"].astype(np.int64)).to(device)
    extra = (
        torch.from_numpy(data["face_pmae"].astype(np.float32)).to(device)
        if "face_pmae" in data
        else None
    )
    return x, edge_index, edge_pairs, node_y, edge_y, node_w, sem_y, extra


NUM_SEMANTIC = len(ROLE_TO_ID)


def pmae_dim(paths: list[Path], pmae_dir: Path | None) -> int:
    if pmae_dir is None:
        return 0
    for path in paths:
        data = load_graph_npz(path, pmae_dir)
        if "face_pmae" in data:
            return int(data["face_pmae"].shape[1])
    return 0


def class_weights_from_npz(paths: list[Path]) -> tuple[float, float]:
    pos = neg = epos = eneg = 0
    for path in paths:
        data = load_graph_npz(path)
        y = data["face_doghouse"]
        pos += int(y.sum())
        neg += int(len(y) - y.sum())
        ey = data["edge_labels"]
        epos += int(ey.sum())
        eneg += int(len(ey) - ey.sum())
    node_pos_weight = max(neg / max(pos, 1), 1.0)
    edge_pos_weight = max(eneg / max(epos, 1), 1.0)
    return node_pos_weight, edge_pos_weight


HOLE_WALL_ID = int(ROLE_TO_ID["hole_wall"])
MOUNT_ID = int(ROLE_TO_ID["mount"])


def hole_wall_pos_weight_from_npz(paths: list[Path]) -> float:
    pos = neg = 0
    for path in paths:
        sem = load_graph_npz(path)["face_semantic"].astype(np.int64)
        y = sem == HOLE_WALL_ID
        pos += int(y.sum())
        neg += int((~y).sum())
    return max(neg / max(pos, 1), 1.0)


def mount_pos_weight_from_npz(paths: list[Path]) -> float:
    pos = neg = 0
    for path in paths:
        sem = load_graph_npz(path)["face_semantic"].astype(np.int64)
        y = sem == MOUNT_ID
        pos += int(y.sum())
        neg += int((~y).sum())
    return max(neg / max(pos, 1), 1.0)


def train_one_model(
    model: FaceGraphGNN,
    paths: list[Path],
    *,
    device: torch.device,
    epochs: int,
    lr: float,
    node_pos_weight: float,
    edge_pos_weight: float,
    edge_loss_weight: float = 1.0,
    hard_neg_weight: float = 1.0,
    semantic_loss_weight: float = 0.0,
    hole_wall_loss_weight: float = 0.0,
    hole_wall_pos_weight: float = 1.0,
    mount_loss_weight: float = 0.0,
    mount_pos_weight: float = 1.0,
    pmae_dir: Path | None = None,
) -> list[float]:
    opt = torch.optim.AdamW(model.parameters(), lr=lr, weight_decay=1e-4)
    # reduction='none' so we can apply per-node hard-negative weights.
    node_loss_fn = nn.BCEWithLogitsLoss(
        pos_weight=torch.tensor([node_pos_weight], device=device),
        reduction="none",
    )
    edge_loss_fn = nn.BCEWithLogitsLoss(
        pos_weight=torch.tensor([edge_pos_weight], device=device),
    )
    semantic_loss_fn = nn.CrossEntropyLoss()
    hole_loss_fn = nn.BCEWithLogitsLoss(
        pos_weight=torch.tensor([hole_wall_pos_weight], device=device),
    )
    mount_loss_fn = nn.BCEWithLogitsLoss(
        pos_weight=torch.tensor([mount_pos_weight], device=device),
    )
    use_semantic = semantic_loss_weight > 0.0 and model.semantic_head is not None
    use_hole = hole_wall_loss_weight > 0.0 and model.hole_wall_head is not None
    use_mount = mount_loss_weight > 0.0 and model.mount_head is not None
    losses: list[float] = []
    for _ in range(epochs):
        model.train()
        total = 0.0
        for path in paths:
            data = load_graph_npz(path, pmae_dir)
            x, edge_index, edge_pairs, node_y, edge_y, node_w, sem_y, extra = graph_tensors(
                data, device, hard_neg_weight=hard_neg_weight,
            )
            node_logits, edge_logits, sem_logits, hole_logits, mount_logits = model.forward_all(
                x, edge_index, edge_pairs, extra
            )
            node_loss = (node_loss_fn(node_logits, node_y) * node_w).sum() / node_w.sum()
            loss = node_loss
            if edge_logits.numel():
                loss = loss + edge_loss_weight * edge_loss_fn(edge_logits, edge_y)
            if use_semantic and sem_logits is not None:
                loss = loss + semantic_loss_weight * semantic_loss_fn(sem_logits, sem_y)
            if use_hole and hole_logits is not None:
                hole_y = (sem_y == HOLE_WALL_ID).float()
                loss = loss + hole_wall_loss_weight * hole_loss_fn(hole_logits, hole_y)
            if use_mount and mount_logits is not None:
                mount_y = (sem_y == MOUNT_ID).float()
                loss = loss + mount_loss_weight * mount_loss_fn(mount_logits, mount_y)
            opt.zero_grad()
            loss.backward()
            opt.step()
            total += float(loss.item())
        losses.append(total / max(len(paths), 1))
    return losses


@torch.no_grad()
def predict_graph(
    model: FaceGraphGNN,
    data: dict[str, np.ndarray],
    device: torch.device,
) -> tuple[np.ndarray, np.ndarray, np.ndarray | None, np.ndarray | None]:
    model.eval()
    x, edge_index, edge_pairs, _, _, _, _, extra = graph_tensors(data, device)
    node_logits, edge_logits, _sem, hole_logits, mount_logits = model.forward_all(
        x, edge_index, edge_pairs, extra
    )
    node_prob = torch.sigmoid(node_logits).cpu().numpy()
    edge_prob = torch.sigmoid(edge_logits).cpu().numpy() if edge_logits.numel() else np.empty(0)
    hole_prob = (
        torch.sigmoid(hole_logits).cpu().numpy() if hole_logits is not None else None
    )
    mount_prob = (
        torch.sigmoid(mount_logits).cpu().numpy() if mount_logits is not None else None
    )
    return node_prob, edge_prob, hole_prob, mount_prob


def instance_metrics(
    pred_instances: list[set[int]],
    gt_instances: list[set[int]],
    *,
    iou_thresh: float = 0.5,
) -> dict[str, float]:
    matched_gt = set()
    matched_pred = set()
    for pi, ps in enumerate(pred_instances):
        best_iou = 0.0
        best_gi = -1
        for gi, gs in enumerate(gt_instances):
            if gi in matched_gt:
                continue
            inter = len(ps & gs)
            if inter == 0:
                continue
            union = len(ps | gs)
            iou = inter / max(union, 1)
            if iou > best_iou:
                best_iou = iou
                best_gi = gi
        if best_iou >= iou_thresh and best_gi >= 0:
            matched_pred.add(pi)
            matched_gt.add(best_gi)
    tp = len(matched_pred)
    fp = len(pred_instances) - tp
    fn = len(gt_instances) - len(matched_gt)
    precision = tp / max(tp + fp, 1)
    recall = tp / max(tp + fn, 1)
    return {
        "tp": tp,
        "fp": fp,
        "fn": fn,
        "precision": precision,
        "recall": recall,
        "pred_instances": len(pred_instances),
        "gt_instances": len(gt_instances),
    }


def face_iou(pred_mask: np.ndarray, gt_mask: np.ndarray) -> float:
    inter = int(np.logical_and(pred_mask, gt_mask).sum())
    union = int(np.logical_or(pred_mask, gt_mask).sum())
    return inter / max(union, 1)


def evaluate_checkpoint(
    checkpoint: Path,
    npz_paths: list[Path],
    *,
    device: torch.device,
    node_threshold: float = 0.5,
    edge_threshold: float = 0.5,
    pmae_dir: Path | None = None,
) -> dict:
    from infer_graph import graph_result_from_arrays

    ckpt = torch.load(checkpoint, map_location=device)
    model = FaceGraphGNN(
        in_dim=int(ckpt["in_dim"]),
        hidden_dim=int(ckpt.get("hidden_dim", 128)),
        num_layers=int(ckpt.get("num_layers", 4)),
        dropout=float(ckpt.get("dropout", 0.2)),
        num_semantic=int(ckpt.get("num_semantic", 0)),
        extra_dim=int(ckpt.get("extra_dim", 0)),
        hole_wall_head=bool(ckpt.get("hole_wall_head", False)),
        mount_head=bool(ckpt.get("mount_head", False)),
    ).to(device)
    model.load_state_dict(ckpt["model_state"])
    model.eval()

    rows = []
    for path in npz_paths:
        data = load_graph_npz(path, pmae_dir)
        node_prob, edge_prob, hole_prob, mount_prob = predict_graph(model, data, device)
        result = graph_result_from_arrays(
            node_prob,
            edge_prob,
            data["adjacency"],
            node_threshold=node_threshold,
            edge_threshold=edge_threshold,
            hole_wall_prob=hole_prob,
            mount_prob=mount_prob,
        )
        pred_mask = np.array([r["doghouse"] for r in result["face_predictions"]], dtype=bool)
        gt_mask = data["face_doghouse"].astype(bool)
        gt_instances = []
        inst_to_faces: dict[int, set[int]] = {}
        for fi, iid in enumerate(data["face_instance"]):
            if int(iid) > 0:
                inst_to_faces.setdefault(int(iid), set()).add(int(fi))
        gt_instances = list(inst_to_faces.values())
        pred_instances = [
            set(ins["faces"]) for ins in result["doghouse_instances"]
        ]
        metrics = instance_metrics(pred_instances, gt_instances)
        metrics["face_iou"] = face_iou(pred_mask, gt_mask)
        metrics["model"] = str(path.stem)
        rows.append(metrics)
    return {"per_model": rows}


def leave_one_out(
    npz_paths: list[Path],
    output_dir: Path,
    *,
    device: torch.device,
    epochs: int,
    lr: float,
    edge_loss_weight: float,
    hard_neg_weight: float = 1.0,
    semantic_loss_weight: float = 0.0,
    hole_wall_loss_weight: float = 0.0,
    mount_loss_weight: float = 0.0,
    pmae_dir: Path | None = None,
    dropout: float = 0.2,
) -> dict:
    output_dir.mkdir(parents=True, exist_ok=True)
    num_semantic = NUM_SEMANTIC if semantic_loss_weight > 0.0 else 0
    use_hole_head = hole_wall_loss_weight > 0.0
    use_mount_head = mount_loss_weight > 0.0
    extra_dim = pmae_dim(npz_paths, pmae_dir)
    all_rows = []
    for holdout in npz_paths:
        train_paths = [p for p in npz_paths if p != holdout]
        node_pw, edge_pw = class_weights_from_npz(train_paths)
        hole_pw = hole_wall_pos_weight_from_npz(train_paths) if use_hole_head else 1.0
        mount_pw = mount_pos_weight_from_npz(train_paths) if use_mount_head else 1.0
        sample = load_graph_npz(train_paths[0])
        in_dim = int(sample["face_features"].shape[1])
        model = FaceGraphGNN(
            in_dim=in_dim,
            num_semantic=num_semantic,
            extra_dim=extra_dim,
            dropout=dropout,
            hole_wall_head=use_hole_head,
            mount_head=use_mount_head,
        ).to(device)
        train_one_model(
            model,
            train_paths,
            device=device,
            epochs=epochs,
            lr=lr,
            node_pos_weight=node_pw,
            edge_pos_weight=edge_pw,
            edge_loss_weight=edge_loss_weight,
            hard_neg_weight=hard_neg_weight,
            semantic_loss_weight=semantic_loss_weight,
            hole_wall_loss_weight=hole_wall_loss_weight,
            hole_wall_pos_weight=hole_pw,
            mount_loss_weight=mount_loss_weight,
            mount_pos_weight=mount_pw,
            pmae_dir=pmae_dir,
        )
        ckpt_path = output_dir / f"loo_{holdout.stem}.pt"
        torch.save(
            {
                "model_state": model.state_dict(),
                "in_dim": in_dim,
                "hidden_dim": 128,
                "num_layers": 4,
                "dropout": float(dropout),
                "num_semantic": num_semantic,
                "extra_dim": extra_dim,
                "hole_wall_head": use_hole_head,
                "mount_head": use_mount_head,
                "holdout": holdout.stem,
            },
            ckpt_path,
        )
        eval_report = evaluate_checkpoint(ckpt_path, [holdout], device=device, pmae_dir=pmae_dir)
        row = eval_report["per_model"][0]
        row["holdout"] = holdout.stem
        all_rows.append(row)
        print(
            f"LOO {holdout.stem}: "
            f"inst P/R={row['precision']:.2f}/{row['recall']:.2f} "
            f"pred/gt={row['pred_instances']}/{row['gt_instances']} "
            f"face_iou={row['face_iou']:.3f}"
        )
    summary = {
        "schema": "doghouse_graph_loo.v1",
        "per_model": all_rows,
        "mean_precision": float(np.mean([r["precision"] for r in all_rows])),
        "mean_recall": float(np.mean([r["recall"] for r in all_rows])),
        "mean_face_iou": float(np.mean([r["face_iou"] for r in all_rows])),
    }
    report_path = output_dir / "loo_report.json"
    report_path.write_text(json.dumps(summary, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"saved LOO report: {report_path}")
    return summary


def train_full(
    npz_paths: list[Path],
    output: Path,
    *,
    device: torch.device,
    epochs: int,
    lr: float,
    edge_loss_weight: float,
    hard_neg_weight: float = 1.0,
    semantic_loss_weight: float = 0.0,
    hole_wall_loss_weight: float = 0.0,
    mount_loss_weight: float = 0.0,
    pmae_dir: Path | None = None,
    dropout: float = 0.2,
) -> Path:
    node_pw, edge_pw = class_weights_from_npz(npz_paths)
    sample = load_graph_npz(npz_paths[0])
    in_dim = int(sample["face_features"].shape[1])
    num_semantic = NUM_SEMANTIC if semantic_loss_weight > 0.0 else 0
    use_hole_head = hole_wall_loss_weight > 0.0
    use_mount_head = mount_loss_weight > 0.0
    hole_pw = hole_wall_pos_weight_from_npz(npz_paths) if use_hole_head else 1.0
    mount_pw = mount_pos_weight_from_npz(npz_paths) if use_mount_head else 1.0
    extra_dim = pmae_dim(npz_paths, pmae_dir)
    model = FaceGraphGNN(
        in_dim=in_dim,
        num_semantic=num_semantic,
        extra_dim=extra_dim,
        dropout=dropout,
        hole_wall_head=use_hole_head,
        mount_head=use_mount_head,
    ).to(device)
    losses = train_one_model(
        model,
        npz_paths,
        device=device,
        epochs=epochs,
        lr=lr,
        node_pos_weight=node_pw,
        edge_pos_weight=edge_pw,
        edge_loss_weight=edge_loss_weight,
        hard_neg_weight=hard_neg_weight,
        semantic_loss_weight=semantic_loss_weight,
        hole_wall_loss_weight=hole_wall_loss_weight,
        hole_wall_pos_weight=hole_pw,
        mount_loss_weight=mount_loss_weight,
        mount_pos_weight=mount_pw,
        pmae_dir=pmae_dir,
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    torch.save(
        {
            "model_state": model.state_dict(),
            "in_dim": in_dim,
            "hidden_dim": 128,
            "num_layers": 4,
            "dropout": float(dropout),
            "num_semantic": num_semantic,
            "extra_dim": extra_dim,
            "hole_wall_head": use_hole_head,
            "mount_head": use_mount_head,
            "epochs": epochs,
            "final_loss": losses[-1] if losses else None,
        },
        output,
    )
    print(f"saved checkpoint: {output} final_loss={losses[-1] if losses else 'n/a'}")
    return output


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", default="doghouse_ai/data/graph_train")
    parser.add_argument("--step-dir", default="../step - 副本2")
    parser.add_argument("--prepare", action="store_true", help="Rebuild graph npz from STEP+annotation")
    parser.add_argument("--loo", action="store_true", help="Run leave-one-out evaluation")
    parser.add_argument("--train-full", action="store_true", help="Train on all models")
    parser.add_argument(
        "--output",
        default="doghouse_ai/checkpoints/doghouse_graph_v1.pt",
    )
    parser.add_argument("--loo-dir", default="doghouse_ai/data/graph_loo")
    parser.add_argument("--epochs", type=int, default=200)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--edge-loss-weight", type=float, default=1.0)
    parser.add_argument(
        "--dropout",
        type=float,
        default=0.2,
        help="FaceGraphGNN layer dropout (written into checkpoint)",
    )
    parser.add_argument(
        "--sample-points-per-face",
        type=int,
        default=64,
        help="Passed to prepare_all when --prepare is set",
    )
    parser.add_argument(
        "--hard-neg-weight",
        type=float,
        default=1.0,
        help="Extra loss weight for hard-negative faces (rib/boundary/protrusion). NOTE: LOO showed >1.0 hurts (relatively downweights easy background -> more spurious FP). Prefer the auxiliary semantic head to exploit negatives. 1.0 = negatives still used as plain doghouse=0 labels",
    )
    parser.add_argument(
        "--semantic-loss-weight",
        type=float,
        default=0.0,
        help="Weight for auxiliary semantic (role) classification head. >0 exploits hard-negative TYPE labels (rib/protrusion/...) as extra supervision. Inference still uses the binary node head",
    )
    parser.add_argument(
        "--hole-wall-loss-weight",
        type=float,
        default=0.0,
        help="Weight for dedicated binary hole_wall recall head. >0 enables hole_wall_head in the checkpoint for AI hole candidates",
    )
    parser.add_argument(
        "--mount-loss-weight",
        type=float,
        default=0.0,
        help="Weight for dedicated binary mount-face head. >0 enables mount_head for AI structure fallback",
    )
    parser.add_argument(
        "--pmae-dir",
        default=None,
        help="Dir of {model}_pmae_face_emb.npy (frozen Point-MAE per-face embeddings). If set, concatenated to face features",
    )
    parser.add_argument("--cpu", action="store_true")
    return parser


def main() -> int:
    parser = build_arg_parser()
    args = parser.parse_args()

    data_dir = Path(args.data_dir)
    if args.prepare:
        prepare_all(
            Path(args.step_dir),
            data_dir,
            sample_points_per_face=args.sample_points_per_face,
        )
    npz_paths = sorted(data_dir.glob("*_graph.npz"))
    if not npz_paths:
        raise FileNotFoundError(f"no graph npz in {data_dir}; run with --prepare")

    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    if args.loo:
        leave_one_out(
            npz_paths,
            Path(args.loo_dir),
            device=device,
            epochs=args.epochs,
            lr=args.lr,
            edge_loss_weight=args.edge_loss_weight,
            hard_neg_weight=args.hard_neg_weight,
            semantic_loss_weight=args.semantic_loss_weight,
            hole_wall_loss_weight=args.hole_wall_loss_weight,
            mount_loss_weight=args.mount_loss_weight,
            pmae_dir=Path(args.pmae_dir) if args.pmae_dir else None,
            dropout=args.dropout,
        )
    if args.train_full:
        train_full(
            npz_paths,
            Path(args.output),
            device=device,
            epochs=args.epochs,
            lr=args.lr,
            edge_loss_weight=args.edge_loss_weight,
            hard_neg_weight=args.hard_neg_weight,
            semantic_loss_weight=args.semantic_loss_weight,
            hole_wall_loss_weight=args.hole_wall_loss_weight,
            mount_loss_weight=args.mount_loss_weight,
            pmae_dir=Path(args.pmae_dir) if args.pmae_dir else None,
            dropout=args.dropout,
        )
    if not args.loo and not args.train_full:
        parser.error("specify --loo and/or --train-full")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
