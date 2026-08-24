#!/usr/bin/env python3
"""Run the minimal PyTorch doghouse point model on one .npz sample."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import torch

try:
    from .train_pointnet_torch import PointMLP
except ImportError:
    from train_pointnet_torch import PointMLP


def _prepare_features(data):
    points = data["points"].astype(np.float32)
    features = data["features"].astype(np.float32)

    xyz = points.copy()
    center = xyz.mean(axis=0, keepdims=True)
    xyz -= center
    scale = max(float(np.linalg.norm(xyz, axis=1).max()), 1e-6)
    xyz /= scale

    f = features.copy()
    f[:, 1] = np.log1p(np.maximum(f[:, 1], 0.0)) / 12.0
    f[:, 2] = f[:, 2] / 50.0
    f[:, 4] = f[:, 4] / 6.5
    f[:, 5] = f[:, 5] / 100.0
    f[:, 6:9] = np.log1p(np.maximum(f[:, 6:9], 0.0)) / 8.0
    return np.concatenate([xyz, f], axis=1).astype(np.float32)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--npz", required=True)
    parser.add_argument("--checkpoint", required=True)
    parser.add_argument("--output-prefix", required=True)
    parser.add_argument("--batch-points", type=int, default=65536)
    parser.add_argument("--cpu", action="store_true")
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    ckpt = torch.load(args.checkpoint, map_location=device)
    model = PointMLP(int(ckpt["in_dim"])).to(device)
    model.load_state_dict(ckpt["model_state"])
    model.eval()

    data = np.load(args.npz)
    x = _prepare_features(data)
    probs = []
    with torch.no_grad():
        for start in range(0, len(x), args.batch_points):
            chunk = torch.from_numpy(x[start:start + args.batch_points]).unsqueeze(0).to(device)
            logits = model(chunk).squeeze(0)
            probs.append(torch.sigmoid(logits).cpu().numpy())
    probs = np.concatenate(probs, axis=0)
    pred = (probs >= 0.5).astype(np.int64)

    prefix = Path(args.output_prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    np.save(str(prefix) + "_doghouse_prob.npy", probs.astype(np.float32))
    np.save(str(prefix) + "_doghouse_pred.npy", pred)
    # Binary model only predicts background/doghouse semantic ids.
    semantic = pred.copy()
    np.save(str(prefix) + "_semantic_pred.npy", semantic)
    print(f"saved: {prefix}_*.npy")
    print(f"points={len(pred)} doghouse_pred={int(pred.sum())}")


if __name__ == "__main__":
    main()
