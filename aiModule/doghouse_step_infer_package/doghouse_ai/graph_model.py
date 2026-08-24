#!/usr/bin/env python3
"""Face-graph GNN with node (doghouse) and edge (same-instance) heads."""

from __future__ import annotations

import numpy as np
import torch
from torch import nn


def normalize_face_features(face_features: np.ndarray) -> np.ndarray:
    """Apply the same rough scaling used for point features."""
    f = face_features.astype(np.float32).copy()
    f[:, 1] = np.log1p(np.maximum(f[:, 1], 0.0)) / 12.0  # area
    f[:, 2] = f[:, 2] / 50.0  # radius
    f[:, 4] = f[:, 4] / 6.5  # u_range
    f[:, 5] = f[:, 5] / 100.0  # v_range
    f[:, 6:9] = np.log1p(np.maximum(f[:, 6:9], 0.0)) / 8.0  # bbox span
    return f


def adjacency_to_edge_index(adjacency: np.ndarray) -> torch.Tensor:
    """Return [2, E] edge index with both directions for undirected graphs."""
    if len(adjacency) == 0:
        return torch.empty((2, 0), dtype=torch.long)
    a = torch.from_numpy(adjacency[:, 0].astype(np.int64))
    b = torch.from_numpy(adjacency[:, 1].astype(np.int64))
    return torch.stack([torch.cat([a, b]), torch.cat([b, a])], dim=0)


class GraphSAGELayer(nn.Module):
    def __init__(self, in_dim: int, out_dim: int, *, dropout: float = 0.2):
        super().__init__()
        self.lin_self = nn.Linear(in_dim, out_dim)
        self.lin_neigh = nn.Linear(in_dim, out_dim)
        self.norm = nn.LayerNorm(out_dim)
        self.dropout = nn.Dropout(dropout)

    def forward(self, x: torch.Tensor, edge_index: torch.Tensor) -> torch.Tensor:
        if edge_index.numel() == 0:
            out = self.lin_self(x)
        else:
            src, dst = edge_index
            agg = torch.zeros_like(x)
            agg.index_add_(0, dst, x[src])
            deg = torch.zeros(x.shape[0], device=x.device, dtype=x.dtype)
            deg.index_add_(0, dst, torch.ones(dst.shape[0], device=x.device, dtype=x.dtype))
            deg = deg.clamp_min(1.0).unsqueeze(-1)
            neigh_mean = agg / deg
            out = self.lin_self(x) + self.lin_neigh(neigh_mean)
        out = self.norm(out)
        out = torch.relu(out)
        return self.dropout(out)


class FaceGraphGNN(nn.Module):
    def __init__(
        self,
        in_dim: int,
        hidden_dim: int = 128,
        num_layers: int = 4,
        dropout: float = 0.2,
        num_semantic: int = 0,
        extra_dim: int = 0,
        extra_proj_dim: int = 32,
        hole_wall_head: bool = False,
        mount_head: bool = False,
    ):
        super().__init__()
        self.num_semantic = int(num_semantic)
        self.extra_dim = int(extra_dim)
        self.hole_wall_head_enabled = bool(hole_wall_head)
        self.mount_head_enabled = bool(mount_head)
        # Optional frozen Point-MAE per-face embedding, projected to a small dim
        # before being concatenated to the geometric face features. Keeping the
        # projection small guards against overfitting with few labeled models.
        if self.extra_dim > 0:
            self.extra_proj = nn.Sequential(
                nn.Linear(self.extra_dim, extra_proj_dim),
                nn.LayerNorm(extra_proj_dim),
                nn.ReLU(inplace=True),
            )
            input_dim = in_dim + extra_proj_dim
        else:
            self.extra_proj = None
            input_dim = in_dim
        self.input = nn.Sequential(
            nn.Linear(input_dim, hidden_dim),
            nn.LayerNorm(hidden_dim),
            nn.ReLU(inplace=True),
        )
        self.layers = nn.ModuleList(
            GraphSAGELayer(hidden_dim, hidden_dim, dropout=dropout)
            for _ in range(num_layers)
        )
        self.node_head = nn.Sequential(
            nn.Linear(hidden_dim, hidden_dim // 2),
            nn.ReLU(inplace=True),
            nn.Linear(hidden_dim // 2, 1),
        )
        self.edge_head = nn.Sequential(
            nn.Linear(hidden_dim * 2, hidden_dim),
            nn.ReLU(inplace=True),
            nn.Linear(hidden_dim, 1),
        )
        # Auxiliary semantic head: predicts the annotated role (background /
        # doghouse / mount / hole_wall / rib / protrusion / ...). This exploits
        # the hard-negative *type* labels so the shared encoder learns to
        # separate rib/protrusion/... as distinct classes, improving doghouse
        # discrimination without distorting the binary node-loss balance.
        # Auxiliary only: inference still uses node_head for the doghouse mask.
        if self.num_semantic > 0:
            self.semantic_head = nn.Sequential(
                nn.Linear(hidden_dim, hidden_dim // 2),
                nn.ReLU(inplace=True),
                nn.Linear(hidden_dim // 2, self.num_semantic),
            )
        else:
            self.semantic_head = None
        # Dedicated binary head for fastener hole-wall recall. Kept separate from
        # the multi-class semantic head so doghouse training is not disturbed.
        if self.hole_wall_head_enabled:
            self.hole_wall_head = nn.Sequential(
                nn.Linear(hidden_dim, hidden_dim // 2),
                nn.ReLU(inplace=True),
                nn.Linear(hidden_dim // 2, 1),
            )
        else:
            self.hole_wall_head = None
        # Dedicated binary head for mount-face structure role (outer fastener plane).
        if self.mount_head_enabled:
            self.mount_head = nn.Sequential(
                nn.Linear(hidden_dim, hidden_dim // 2),
                nn.ReLU(inplace=True),
                nn.Linear(hidden_dim // 2, 1),
            )
        else:
            self.mount_head = None

    def encode(
        self,
        x: torch.Tensor,
        edge_index: torch.Tensor,
        extra: torch.Tensor | None = None,
    ) -> torch.Tensor:
        if self.extra_proj is not None and extra is not None:
            x = torch.cat([x, self.extra_proj(extra)], dim=-1)
        h = self.input(x)
        for layer in self.layers:
            h = layer(h, edge_index)
        return h

    def semantic_logits(
        self,
        x: torch.Tensor,
        edge_index: torch.Tensor,
        extra: torch.Tensor | None = None,
    ) -> torch.Tensor | None:
        if self.semantic_head is None:
            return None
        return self.semantic_head(self.encode(x, edge_index, extra))

    def forward(
        self,
        x: torch.Tensor,
        edge_index: torch.Tensor,
        edge_pairs: torch.Tensor,
        extra: torch.Tensor | None = None,
    ) -> tuple[torch.Tensor, torch.Tensor]:
        node_logits, edge_logits, _semantic, _hole, _mount = self.forward_all(
            x, edge_index, edge_pairs, extra
        )
        return node_logits, edge_logits

    def forward_all(
        self,
        x: torch.Tensor,
        edge_index: torch.Tensor,
        edge_pairs: torch.Tensor,
        extra: torch.Tensor | None = None,
    ) -> tuple[
        torch.Tensor,
        torch.Tensor,
        torch.Tensor | None,
        torch.Tensor | None,
        torch.Tensor | None,
    ]:
        """Single-encode forward: node, edge, optional semantic/hole-wall/mount."""
        h = self.encode(x, edge_index, extra)
        node_logits = self.node_head(h).squeeze(-1)
        src = edge_pairs[0]
        dst = edge_pairs[1]
        pair_feat = torch.cat([h[src] + h[dst], torch.abs(h[src] - h[dst])], dim=-1)
        edge_logits = self.edge_head(pair_feat).squeeze(-1)
        semantic_logits = self.semantic_head(h) if self.semantic_head is not None else None
        hole_logits = (
            self.hole_wall_head(h).squeeze(-1) if self.hole_wall_head is not None else None
        )
        mount_logits = (
            self.mount_head(h).squeeze(-1) if self.mount_head is not None else None
        )
        return node_logits, edge_logits, semantic_logits, hole_logits, mount_logits
