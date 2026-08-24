#!/usr/bin/env python3
"""Minimal PyTorch point-wise doghouse segmentation baseline.

This is intentionally small: it validates the training/inference data pipeline
before switching to Open3D-ML PointNet++/RandLA-Net.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset


class NpzPointDataset(Dataset):
    def __init__(self, npz_paths, num_points=8192):
        self.paths = [Path(p) for p in npz_paths]
        self.num_points = int(num_points)

    def __len__(self):
        return len(self.paths)

    def __getitem__(self, idx):
        data = np.load(self.paths[idx])
        points = data["points"].astype(np.float32)
        features = data["features"].astype(np.float32)
        doghouse = data["doghouse"].astype(np.float32)

        xyz = points.copy()
        center = xyz.mean(axis=0, keepdims=True)
        xyz -= center
        scale = max(float(np.linalg.norm(xyz, axis=1).max()), 1e-6)
        xyz /= scale

        # Normalize scalar feature columns roughly; keep type id / flags as-is.
        f = features.copy()
        f[:, 1] = np.log1p(np.maximum(f[:, 1], 0.0)) / 12.0  # area
        f[:, 2] = f[:, 2] / 50.0  # radius
        f[:, 4] = f[:, 4] / 6.5  # u_range
        f[:, 5] = f[:, 5] / 100.0  # v_range
        f[:, 6:9] = np.log1p(np.maximum(f[:, 6:9], 0.0)) / 8.0  # bbox span

        x = np.concatenate([xyz, f], axis=1)
        n = len(x)
        if n >= self.num_points:
            choice = np.random.choice(n, self.num_points, replace=False)
        else:
            choice = np.random.choice(n, self.num_points, replace=True)
        return torch.from_numpy(x[choice]), torch.from_numpy(doghouse[choice])


class PointMLP(nn.Module):
    def __init__(self, in_dim):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(in_dim, 128),
            nn.BatchNorm1d(128),
            nn.ReLU(inplace=True),
            nn.Linear(128, 128),
            nn.BatchNorm1d(128),
            nn.ReLU(inplace=True),
            nn.Linear(128, 64),
            nn.BatchNorm1d(64),
            nn.ReLU(inplace=True),
            nn.Linear(64, 1),
        )

    def forward(self, x):
        b, n, c = x.shape
        y = self.net(x.reshape(b * n, c))
        return y.reshape(b, n)


def train(args):
    paths = args.npz
    ds = NpzPointDataset(paths, num_points=args.num_points)
    dl = DataLoader(ds, batch_size=args.batch_size, shuffle=True, drop_last=False)
    sample_x, _ = ds[0]

    device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
    model = PointMLP(sample_x.shape[1]).to(device)
    opt = torch.optim.AdamW(model.parameters(), lr=args.lr, weight_decay=1e-4)

    # Class balance from all samples.
    pos = neg = 0
    for p in paths:
        d = np.load(p)
        y = d["doghouse"]
        pos += int(y.sum())
        neg += int(len(y) - y.sum())
    pos_weight = torch.tensor([max(neg / max(pos, 1), 1.0)], device=device)
    loss_fn = nn.BCEWithLogitsLoss(pos_weight=pos_weight)

    for ep in range(1, args.epochs + 1):
        model.train()
        total_loss = 0.0
        total = correct = 0
        for x, y in dl:
            x = x.to(device)
            y = y.to(device)
            logits = model(x)
            loss = loss_fn(logits, y)
            opt.zero_grad()
            loss.backward()
            opt.step()
            total_loss += float(loss.item())
            pred = (torch.sigmoid(logits) >= 0.5).float()
            correct += int((pred == y).sum().item())
            total += int(y.numel())
        if ep == 1 or ep % args.print_every == 0 or ep == args.epochs:
            print(f"epoch {ep:04d} loss={total_loss/len(dl):.4f} acc={correct/max(total,1):.4f}")

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    torch.save(
        {
            "model_state": model.state_dict(),
            "in_dim": int(sample_x.shape[1]),
            "num_points": int(args.num_points),
        },
        output,
    )
    print(f"saved: {output}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--npz", nargs="+", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--epochs", type=int, default=100)
    parser.add_argument("--num-points", type=int, default=8192)
    parser.add_argument("--batch-size", type=int, default=2)
    parser.add_argument("--lr", type=float, default=1e-3)
    parser.add_argument("--print-every", type=int, default=10)
    parser.add_argument("--cpu", action="store_true")
    args = parser.parse_args()
    train(args)


if __name__ == "__main__":
    main()
