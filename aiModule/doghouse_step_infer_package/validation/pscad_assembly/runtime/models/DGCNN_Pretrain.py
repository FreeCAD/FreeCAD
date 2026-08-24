"""DGCNN encoder for PointContrast-style self-supervised pre-training.

Reference: Xie et al., "PointContrast: Unsupervised Pre-training for 3D Point
Cloud Understanding" (ICCV 2021).

Architecture:
    Input (B, 3, N=1024) → 4 EdgeConv blocks (64→64→64→128) → concat (B, 320, N)
    Per-point projection head: 320 → 128 (L2-normalized for contrastive loss)

The per-point 128-d features are used for point-level InfoNCE between two
augmented views of the same point cloud. After pretraining, the encoder
is warm-started into DGCNN for downstream FT (SupCon + CE).
"""
import torch
import torch.nn as nn
import torch.nn.functional as F
from .DGCNN import knn, gather_features, EdgeConv
from .build import MODELS


@MODELS.register_module()
class DGCNN_Pretrain(nn.Module):
    """DGCNN encoder + per-point projection head for PointContrast pretrain.

    Returns per-point (B, N, 128) L2-normalized features.
    """

    def __init__(self, config):
        super().__init__()
        self.config = config
        proj_dim = config.get("proj_dim", 128)
        k = config.get("k", 20)

        # 4 EdgeConv blocks (same as DGCNN)
        self.conv1 = EdgeConv(3,   64,  k)
        self.conv2 = EdgeConv(64,  64,  k)
        self.conv3 = EdgeConv(64,  64,  k)
        self.conv4 = EdgeConv(64,  128, k)

        concat_dim = 64 * 3 + 128  # 320

        # Per-point projection head (320 → proj_dim), like PointContrast
        self.proj_head = nn.Sequential(
            nn.Conv1d(concat_dim, 256, 1, bias=False),
            nn.BatchNorm1d(256),
            nn.LeakyReLU(0.2, inplace=True),
            nn.Conv1d(256, proj_dim, 1),
        )

    def encode(self, pts: torch.Tensor) -> torch.Tensor:
        """pts: (B, 3, N) → multi-scale features (B, 320, N)."""
        x1 = self.conv1(pts)
        x2 = self.conv2(x1)
        x3 = self.conv3(x2)
        x4 = self.conv4(x3)
        return torch.cat([x1, x2, x3, x4], dim=1)

    def forward(self, pts: torch.Tensor) -> torch.Tensor:
        """pts: (B, 3, N) or (B, N, 3) → per-point (B, proj_dim, N) L2-normed."""
        if pts.shape[1] == 3 and pts.shape[2] != 3:
            pass  # (B, 3, N)
        elif pts.shape[-1] == 3:
            pts = pts.transpose(1, 2)  # (B, N, 3) → (B, 3, N)
        else:
            raise ValueError(f"Expected (B,3,N) or (B,N,3), got {tuple(pts.shape)}")

        feat = self.encode(pts)                       # (B, 320, N)
        proj = self.proj_head(feat)                   # (B, proj_dim, N)
        proj = F.normalize(proj, p=2, dim=1)          # L2-normalize per point
        return proj                                  # (B, proj_dim, N)

    def load_model_from_ckpt(self, ckpt_path: str):
        """Load encoder weights from a saved DGCNN_Pretrain or DGCNN checkpoint."""
        import logging
        ckpt = torch.load(ckpt_path, map_location="cpu", weights_only=False)
        # ckpt may be from runner_pretrain (DGCNN_Pretrain raw) or runner_finetune (DGCNN wrapped)
        if "base_model" in ckpt:
            sd = ckpt["base_model"]
        else:
            sd = ckpt
        sd = {k.replace("module.", "").replace("base_model.", ""): v
              for k, v in sd.items()}
        # Only load encoder weights (conv1-4 + proj_head)
        # If from DGCNN_Pretrain: keys are conv1.mlp.0.weight etc
        # If from DGCNN: keys are conv1.mlp.0.weight + emb_head.* + classifier.* (skip latter)
        missing, unexpected = self.load_state_dict(sd, strict=False)
        if missing:
            logging.info(f"[DGCNN_Pretrain] Missing keys (random init): {len(missing)}")
        if unexpected:
            logging.info(f"[DGCNN_Pretrain] Unexpected keys (skipped): {len(unexpected)}")
        logging.info(f"[DGCNN_Pretrain] Loaded encoder from {ckpt_path}")
