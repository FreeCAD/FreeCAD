"""Pure-PyTorch drop-in replacement for knn_cuda.

Replaces `from knn_cuda import KNN` with a native PyTorch implementation.
Used when the compiled knn_cuda package is unavailable.

Interface matches the original KNN class:
    knn = KNN(k=K, transpose_mode=True)
    dist, idx = knn(x, y)   # x: (B, N, C) or (B, C, N), y: (B, G, C) or (B, C, G)
                             # returns dist/idx: (B, G, K)
"""
import torch
import torch.nn as nn


class KNN(nn.Module):
    def __init__(self, k: int, transpose_mode: bool = True):
        super().__init__()
        self.k = k
        self.transpose_mode = transpose_mode

    def forward(self, x: torch.Tensor, y: torch.Tensor):
        if self.transpose_mode:
            # x: (B, N, C), y: (B, G, C)
            pass
        else:
            # x: (B, C, N), y: (B, C, G) -> transpose to (B, N, C), (B, G, C)
            x = x.transpose(1, 2)
            y = y.transpose(1, 2)

        # x: (B, N, C), y: (B, G, C)
        # Pairwise squared L2 distance via: ||a-b||^2 = ||a||^2 + ||b||^2 - 2*a·b
        x_sq = (x ** 2).sum(dim=-1, keepdim=True)     # (B, N, 1)
        y_sq = (y ** 2).sum(dim=-1, keepdim=True)     # (B, G, 1)
        inner = torch.matmul(y, x.transpose(1, 2))    # (B, G, N)
        dist = y_sq + x_sq.transpose(1, 2) - 2 * inner  # (B, G, N)

        # Top-K smallest distances
        if self.k >= dist.shape[-1]:
            idx = torch.arange(dist.shape[-1], device=dist.device).view(1, 1, -1).expand(
                dist.shape[0], dist.shape[1], -1
            )
            return dist, idx

        topk = dist.topk(self.k, dim=-1, largest=False)
        return topk.values, topk.indices
