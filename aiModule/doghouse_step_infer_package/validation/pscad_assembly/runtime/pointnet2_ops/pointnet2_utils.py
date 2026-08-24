"""Pure PyTorch fallback for PointNet++ sampling helpers.

This is slower than the CUDA extension, but it keeps CPU inference usable for
small STEP face-embedding jobs in this package.
"""
from __future__ import annotations

import torch


def furthest_point_sample(points: torch.Tensor, number: int) -> torch.Tensor:
    """Return farthest-point-sampling indices for points shaped [B, N, 3]."""
    if points.ndim != 3:
        raise ValueError(f"points must be [B, N, C], got {tuple(points.shape)}")
    batch, point_count, _channels = points.shape
    if point_count <= 0:
        raise ValueError("points must contain at least one point")
    sample_count = int(number)
    if sample_count <= 0:
        raise ValueError("number must be positive")
    sample_count = min(sample_count, point_count)
    device = points.device
    centroids = torch.zeros(batch, sample_count, dtype=torch.long, device=device)
    distances = torch.full((batch, point_count), float("inf"), dtype=points.dtype, device=device)
    farthest = torch.zeros(batch, dtype=torch.long, device=device)
    batch_indices = torch.arange(batch, dtype=torch.long, device=device)
    for i in range(sample_count):
        centroids[:, i] = farthest
        centroid = points[batch_indices, farthest, :].view(batch, 1, -1)
        dist = torch.sum((points - centroid) ** 2, dim=-1)
        distances = torch.minimum(distances, dist)
        farthest = torch.max(distances, dim=1).indices
    if sample_count < int(number):
        pad = centroids[:, -1:].repeat(1, int(number) - sample_count)
        centroids = torch.cat([centroids, pad], dim=1)
    return centroids


def gather_operation(features: torch.Tensor, idx: torch.Tensor) -> torch.Tensor:
    """Gather channel-first features shaped [B, C, N] with indices [B, S]."""
    if features.ndim != 3:
        raise ValueError(f"features must be [B, C, N], got {tuple(features.shape)}")
    if idx.ndim != 2:
        raise ValueError(f"idx must be [B, S], got {tuple(idx.shape)}")
    idx = idx.to(device=features.device, dtype=torch.long)
    expanded = idx.unsqueeze(1).expand(-1, features.shape[1], -1)
    return torch.gather(features, 2, expanded)
