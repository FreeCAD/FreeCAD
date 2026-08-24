#!/usr/bin/env python3
"""Extract frozen Point-MAE per-face embeddings for the doghouse face-graph GNN.

RUN THIS ON THE GPU/LINUX MACHINE (needs CUDA: knn_cuda, misc.fps).

Bridge step 2 of the Point-MAE hybrid pipeline. Input files are produced by
``export_pointcloud_for_pmae.py`` on any machine:

    {model}_pmae_input.npz : points[N,3] (unit-sphere), face_idx[N], num_faces

For each model:
    1. Group (FPS centers + KNN neighborhoods) -> Encoder -> transformer blocks.
    2. Fetch layers [3, 7, 11], LayerNorm each, concat -> [G, 1152] group features.
    3. Parameter-free inverse-distance interpolation from the G centers to every
       point (no random seg-head weights involved) -> [N, 1152] per-point.
    4. Mean-pool by face_idx -> [num_faces, 1152] per-face embedding.
    5. Save {model}_pmae_face_emb.npy.

Optional multi-view point dropout (``--point-dropout-views > 1``): randomly drop
points each view (keeping >=1 point per face), extract face embeddings, then
mean-pool across views.

Only the ABC-pretrained parts are used (Encoder + pos_embed + blocks + norm);
the checkpoint is loaded read-only and never trained here.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
import torch
from torch import nn

# Allow `from models.Point_MAE import ...` when launched from doghouse_ai/.
_ROOT = Path(__file__).resolve().parents[1]
if str(_ROOT) not in sys.path:
    sys.path.insert(0, str(_ROOT))


def apply_point_dropout(
    points: np.ndarray,
    face_idx: np.ndarray,
    *,
    max_ratio: float,
    rng: np.random.Generator,
) -> tuple[np.ndarray, np.ndarray]:
    """Randomly drop points up to ``max_ratio``, keeping >=1 point per face."""
    points = np.asarray(points, dtype=np.float32)
    face_idx = np.asarray(face_idx, dtype=np.int64)
    n = int(points.shape[0])
    if n == 0 or max_ratio <= 0.0:
        return points.copy(), face_idx.copy()

    ratio = float(rng.random() * max_ratio)
    drop = rng.random(n) < ratio
    keep = ~drop
    # Ensure every face retains at least one point.
    for face in np.unique(face_idx):
        face_mask = face_idx == face
        if not np.any(keep[face_mask]):
            idxs = np.flatnonzero(face_mask)
            keep[int(rng.choice(idxs))] = True
    return points[keep], face_idx[keep]


def mean_stack_embeddings(views: list[np.ndarray]) -> np.ndarray:
    if not views:
        raise ValueError("no embeddings to average")
    stacked = np.stack([np.asarray(v, dtype=np.float32) for v in views], axis=0)
    return stacked.mean(axis=0).astype(np.float32)


try:
    from models.Point_MAE import Encoder, Group, TransformerEncoder
except ModuleNotFoundError:  # pragma: no cover - unit tests only need helpers
    Encoder = Group = TransformerEncoder = None  # type: ignore


class PMAEEncoder(nn.Module):
    """Mirror of MaskTransformer's frozen submodules for weight loading."""

    def __init__(self, trans_dim: int = 384, depth: int = 12, num_heads: int = 6, drop_path_rate: float = 0.1):
        super().__init__()
        self.encoder = Encoder(encoder_channel=trans_dim)
        self.pos_embed = nn.Sequential(
            nn.Linear(3, 128),
            nn.GELU(),
            nn.Linear(128, trans_dim),
        )
        dpr = [x.item() for x in torch.linspace(0, drop_path_rate, depth)]
        self.blocks = TransformerEncoder(
            embed_dim=trans_dim, depth=depth, drop_path_rate=dpr, num_heads=num_heads,
        )
        self.norm = nn.LayerNorm(trans_dim)

    @torch.no_grad()
    def group_features(self, neighborhood, center, fetch=(3, 7, 11)) -> torch.Tensor:
        tokens = self.encoder(neighborhood)
        pos = self.pos_embed(center)
        x = tokens
        feats = []
        for i, block in enumerate(self.blocks.blocks):
            x = block(x + pos)
            if i in fetch:
                feats.append(self.norm(x))
        return torch.cat(feats, dim=-1)


def load_pretrained(ckpt_path: Path, model: PMAEEncoder) -> None:
    ckpt = torch.load(ckpt_path, map_location="cpu")
    state = ckpt.get("base_model", ckpt.get("model", ckpt))
    cleaned = {}
    for k, v in state.items():
        kk = k.replace("module.", "")
        if kk.startswith("MAE_encoder."):
            cleaned[kk[len("MAE_encoder."):]] = v
        elif kk.startswith(("encoder.", "pos_embed.", "blocks.", "norm.")):
            cleaned[kk] = v
    incompatible = model.load_state_dict(cleaned, strict=False)
    print(
        f"loaded pretrained: missing={len(incompatible.missing_keys)} "
        f"unexpected={len(incompatible.unexpected_keys)}"
    )


@torch.no_grad()
def interpolate_to_points(
    points: torch.Tensor, centers: torch.Tensor, group_feat: torch.Tensor, k: int = 3
) -> torch.Tensor:
    dist = torch.cdist(points, centers)
    knn_d, knn_i = torch.topk(dist, k=min(k, centers.shape[0]), dim=1, largest=False)
    w = 1.0 / (knn_d + 1e-8)
    w = w / w.sum(dim=1, keepdim=True)
    gathered = group_feat[knn_i]
    return (gathered * w.unsqueeze(-1)).sum(dim=1)


@torch.no_grad()
def pool_by_face(point_feat: torch.Tensor, face_idx: torch.Tensor, num_faces: int) -> torch.Tensor:
    feat_dim = point_feat.shape[1]
    out = torch.zeros(num_faces, feat_dim, device=point_feat.device)
    cnt = torch.zeros(num_faces, 1, device=point_feat.device)
    out.index_add_(0, face_idx, point_feat)
    cnt.index_add_(0, face_idx, torch.ones(point_feat.shape[0], 1, device=point_feat.device))
    return out / cnt.clamp_min(1.0)


@torch.no_grad()
def extract_face_embedding(
    model: PMAEEncoder,
    group_divider: Group,
    points_np: np.ndarray,
    face_idx_np: np.ndarray,
    num_faces: int,
    device,
) -> np.ndarray:
    points = torch.from_numpy(np.asarray(points_np, dtype=np.float32)).to(device)
    face_idx = torch.from_numpy(np.asarray(face_idx_np, dtype=np.int64)).to(device)
    n = points.shape[0]
    max_pts = 8192
    if n > max_pts:
        sel = torch.randperm(n, device=device)[:max_pts]
        grp_pts = points[sel].unsqueeze(0)
    else:
        grp_pts = points.unsqueeze(0)
    neighborhood, center = group_divider(grp_pts)
    group_feat = model.group_features(neighborhood, center)
    per_point = interpolate_to_points(points, center[0], group_feat[0])
    per_face = pool_by_face(per_point, face_idx, num_faces)
    return per_face.cpu().numpy().astype(np.float32)


def process_model(
    model: PMAEEncoder,
    group_divider: Group,
    npz_path: Path,
    output_dir: Path,
    device,
    *,
    point_dropout_views: int = 1,
    point_dropout_max_ratio: float = 0.4,
    seed: int = 0,
) -> Path:
    data = np.load(npz_path)
    points = data["points"].astype(np.float32)
    face_idx = data["face_idx"].astype(np.int64)
    num_faces = int(data["num_faces"])
    views = max(int(point_dropout_views), 1)
    rng = np.random.default_rng(seed)

    emb_views: list[np.ndarray] = []
    for view_i in range(views):
        if views == 1:
            pts_v, faces_v = points, face_idx
        else:
            pts_v, faces_v = apply_point_dropout(
                points,
                face_idx,
                max_ratio=point_dropout_max_ratio,
                rng=rng,
            )
        emb_views.append(
            extract_face_embedding(model, group_divider, pts_v, faces_v, num_faces, device)
        )
    per_face = mean_stack_embeddings(emb_views) if views > 1 else emb_views[0]

    name = npz_path.stem.replace("_pmae_input", "")
    output_dir.mkdir(parents=True, exist_ok=True)
    out = output_dir / f"{name}_pmae_face_emb.npy"
    np.save(out, per_face.astype(np.float32))
    print(f"saved: {out.name} shape={tuple(per_face.shape)} views={views}")
    return out


def main() -> int:
    if Encoder is None:
        raise ModuleNotFoundError(
            "models.Point_MAE not found; run from Point-MAE root or ensure it is on PYTHONPATH"
        )
    parser = argparse.ArgumentParser()
    parser.add_argument("--ckpt", required=True, help="ABC pretrain checkpoint (ckpt-last.pth)")
    parser.add_argument("--input-dir", required=True, help="dir of *_pmae_input.npz")
    parser.add_argument("--output-dir", required=True)
    parser.add_argument(
        "--num-group",
        type=int,
        default=256,
        help="FPS centers for extraction (finer than pretrain's 64; weights are token-count agnostic)",
    )
    parser.add_argument("--group-size", type=int, default=32)
    parser.add_argument("--cpu", action="store_true")
    parser.add_argument(
        "--point-dropout-views",
        type=int,
        default=1,
        help="If >1, extract multiple point-dropout views and mean-pool face embeddings",
    )
    parser.add_argument(
        "--point-dropout-max-ratio",
        type=float,
        default=0.4,
        help="Max random drop ratio per view when --point-dropout-views > 1",
    )
    parser.add_argument("--seed", type=int, default=0)
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    model = PMAEEncoder().to(device).eval()
    load_pretrained(Path(args.ckpt), model)
    group_divider = Group(num_group=args.num_group, group_size=args.group_size).to(device)

    input_dir = Path(args.input_dir)
    npz_paths = sorted(input_dir.glob("*_pmae_input.npz"))
    if not npz_paths:
        raise FileNotFoundError(f"no *_pmae_input.npz in {input_dir}")
    for i, path in enumerate(npz_paths):
        process_model(
            model,
            group_divider,
            path,
            Path(args.output_dir),
            device,
            point_dropout_views=args.point_dropout_views,
            point_dropout_max_ratio=args.point_dropout_max_ratio,
            seed=args.seed + i,
        )
    print(f"done: {len(npz_paths)} models -> {args.output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
