"""Vanilla DGCNN (Dynamic Graph CNN) for point cloud feature extraction.

Reference: Wang et al., "Dynamic Graph CNN for Learning on Point Clouds" (TOG 2019).
Used here as a fine-grained local-geometry alternative to Point-MAE's
64-patch tokenization. EdgeConv dynamically builds KNN graphs in the
current feature space, capturing local proportion features that
Point-MAE's coarse grouping misses.

Architecture:
    Input (B, 3, N=1024) → KNN(k=20) in feature space → EdgeConv block
    4 stacked EdgeConv blocks: 64 → 64 → 64 → 128 channels
    Concat multi-scale features: (B, 64*3+128, N) → global max pool → (B, 320)
    MLP head: 320 → 256 (embedding) + 320 → 43 (classifier)
"""
import torch
import torch.nn as nn
import torch.nn.functional as F
from .build import MODELS


def knn(x: torch.Tensor, k: int) -> torch.Tensor:
    """K-nearest neighbors in feature space.

    Args:
        x: (B, C, N) feature tensor
        k: number of neighbors
    Returns:
        idx: (B, N, k) indices of k-NN for each point
    """
    B, C, N = x.shape
    inner = -2 * torch.matmul(x.transpose(2, 1), x)  # (B, N, N)
    xx = (x ** 2).sum(dim=1, keepdim=True)          # (B, 1, N)
    dist = inner + xx + xx.transpose(2, 1)           # (B, N, N)
    _, idx = dist.topk(k=k, dim=-1, largest=False)  # (B, N, k)
    return idx


def gather_features(x: torch.Tensor, idx: torch.Tensor) -> torch.Tensor:
    """Gather neighbor features by index.

    Args:
        x: (B, C, N)
        idx: (B, N, k)
    Returns:
        neighbors: (B, C, N, k)
    """
    B, C, N = x.shape
    k = idx.shape[-1]
    idx_flat = idx.reshape(B, N * k)                      # (B, N*k)
    # expand x to (B, C, N*k) by gathering
    x_flat = x.transpose(1, 2).reshape(B, N, C)            # (B, N, C)
    gathered = torch.gather(
        x_flat, 1,
        idx_flat.unsqueeze(-1).expand(-1, -1, C),         # (B, N*k, C)
    )                                                      # (B, N*k, C)
    return gathered.reshape(B, N, k, C).permute(0, 3, 1, 2)  # (B, C, N, k)


class EdgeConv(nn.Module):
    """Single EdgeConv block: h_ij = MLP([x_i || x_j - x_i]) → max."""
    def __init__(self, in_channels: int, out_channels: int, k: int = 20):
        super().__init__()
        self.k = k
        self.mlp = nn.Sequential(
            nn.Conv2d(in_channels * 2, out_channels, 1, bias=False),
            nn.BatchNorm2d(out_channels),
            nn.LeakyReLU(0.2, inplace=True),
            nn.Conv2d(out_channels, out_channels, 1, bias=False),
            nn.BatchNorm2d(out_channels),
            nn.LeakyReLU(0.2, inplace=True),
        )

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """x: (B, C, N) → (B, out_C, N)"""
        idx = knn(x, self.k)                          # (B, N, k)
        neighbors = gather_features(x, idx)           # (B, C, N, k)
        center = x.unsqueeze(-1).expand_as(neighbors)  # (B, C, N, k)
        edge_feat = torch.cat([center, neighbors - center], dim=1)  # (B, 2C, N, k)
        out = self.mlp(edge_feat)                     # (B, out_C, N, k)
        return out.max(dim=-1)[0]                      # (B, out_C, N)


@MODELS.register_module()
class DGCNN(nn.Module):
    """Vanilla DGCNN with 4 EdgeConv blocks + multi-scale concat + MLP heads.

    Returns a dict similar to PointMAEFinetune so runner code is uniform.
    """
    def __init__(self, config):
        super().__init__()
        self.config = config
        n_classes = config.n_classes
        emb_dim = config.emb_dim
        k = config.k
        self.k = k
        self.emb_dim = emb_dim

        # 4 EdgeConv blocks
        self.conv1 = EdgeConv(3,   64,  k)   # input: xyz
        self.conv2 = EdgeConv(64,  64,  k)
        self.conv3 = EdgeConv(64,  64,  k)
        self.conv4 = EdgeConv(64,  128, k)

        # Multi-scale concat: 64+64+64+128 = 320
        concat_dim = 64 * 3 + 128

        # Embedding head (for retrieval)
        self.emb_head = nn.Sequential(
            nn.Conv1d(concat_dim, 256, 1, bias=False),
            nn.BatchNorm1d(256),
            nn.LeakyReLU(0.2, inplace=True),
            nn.Dropout(0.5),
            nn.Conv1d(256, emb_dim, 1),
        )

        # Classifier head
        self.classifier = nn.Sequential(
            nn.Conv1d(concat_dim, 256, 1, bias=False),
            nn.BatchNorm1d(256),
            nn.LeakyReLU(0.2, inplace=True),
            nn.Dropout(0.5),
            nn.Conv1d(256, 128, 1, bias=False),
            nn.BatchNorm1d(128),
            nn.LeakyReLU(0.2, inplace=True),
            nn.Dropout(0.5),
            nn.Conv1d(128, n_classes, 1),
        )

        # SupCon projection head (128-d like Point-MAE)
        self.proj_head = nn.Sequential(
            nn.Linear(emb_dim, 128, bias=False),
            nn.BatchNorm1d(128),
            nn.ReLU(inplace=True),
            nn.Linear(128, 128, bias=False),
        )

        self.loss_ce = nn.CrossEntropyLoss()
        self.supcon_temperature = 0.07

    def encode(self, pts: torch.Tensor) -> torch.Tensor:
        """pts: (B, 3, N) → multi-scale features (B, 320, N)."""
        x1 = self.conv1(pts)        # (B, 64, N)
        x2 = self.conv2(x1)         # (B, 64, N)
        x3 = self.conv3(x2)         # (B, 64, N)
        x4 = self.conv4(x3)         # (B, 128, N)
        return torch.cat([x1, x2, x3, x4], dim=1)  # (B, 320, N)

    def forward(self, pts: torch.Tensor, labels=None, return_loss: bool = True):
        """pts: (B, 3, N) — uses (B, N, 3) inputs as well via transpose fallback."""
        # Accept (B, N, 3) or (B, 3, N); convert to (B, 3, N)
        if pts.shape[1] == pts.shape[2] == 3:
            pass  # already (B, 3, N)
        elif pts.shape[-1] == 3:
            pts = pts.transpose(1, 2)  # (B, N, 3) → (B, 3, N)
        else:
            raise ValueError(f"Expected (B,3,N) or (B,N,3), got {tuple(pts.shape)}")

        feat = self.encode(pts)                          # (B, 320, N)
        global_feat = feat.max(dim=-1, keepdim=True)[0]  # (B, 320, 1) — for classifier

        emb = self.emb_head(feat)                        # (B, emb_dim, N)
        emb = emb.max(dim=-1)[0]                         # (B, emb_dim) — retrieval feat

        logits = self.classifier(global_feat).squeeze(-1)  # (B, n_classes)

        out = {
            "emb": emb,
            "concat_f": emb,  # for uniform retrieval interface
            "logits": logits,
        }

        if return_loss:
            # CE
            loss_ce = self.loss_ce(logits, labels)
            # SupCon
            emb_norm = F.normalize(emb, p=2, dim=-1)
            proj = self.proj_head(emb_norm)              # (B, 128)
            proj = F.normalize(proj, p=2, dim=-1)
            sim = (proj @ proj.T) / self.supcon_temperature  # (B, B)
            # Mask out self
            mask_self = torch.eye(proj.size(0), device=proj.device, dtype=torch.bool)
            # Positive mask: same label
            if labels is not None:
                pos_mask = labels.unsqueeze(0) == labels.unsqueeze(1)
                pos_mask = pos_mask & ~mask_self
            else:
                pos_mask = ~mask_self
            # SupCon: -log( sum(exp(pos)) / sum(exp(all non-self)) )
            n_pos = pos_mask.sum()
            if n_pos == 0:
                loss_supcon = torch.tensor(0.0, device=emb.device, requires_grad=True)
            else:
                exp_sim = torch.exp(sim) * (~mask_self).float()
                log_prob = sim - torch.log(exp_sim.sum(dim=1, keepdim=True) + 1e-12)
                loss_supcon = -(log_prob[pos_mask].sum() / n_pos)
            out["loss_supcon"] = loss_supcon
            out["loss_ce"] = loss_ce
            out["loss_mae"] = torch.tensor(0.0, device=emb.device)
            out["loss"] = 1.0 * loss_supcon + 1.0 * loss_ce
        return out

    def load_model_from_ckpt(self, ckpt_path: str):
        """Load encoder weights from a saved checkpoint (DGCNN or DGCNN_Pretrain).

        Loads conv1-4 weights from a DGCNN_Pretrain ckpt (skips proj_head),
        or full DGCNN ckpt (skips emb_head/classifier if shape mismatch).
        """
        import logging
        ckpt = torch.load(ckpt_path, map_location="cpu", weights_only=False)
        if "base_model" in ckpt:
            sd = ckpt["base_model"]
        else:
            sd = ckpt
        # Strip "module." prefix
        sd = {k.replace("module.", ""): v for k, v in sd.items()}
        # Only load encoder (conv1-4); skip proj_head/emb_head/classifier
        encoder_sd = {k: v for k, v in sd.items() if k.startswith("conv")}
        incompatible = self.load_state_dict(encoder_sd, strict=False)
        n_loaded = len(encoder_sd)
        n_miss = len(incompatible.missing_keys)
        n_unexp = len(incompatible.unexpected_keys)
        logging.info(f"[DGCNN] Loaded {n_loaded} encoder tensors from {ckpt_path}  "
                     f"missing={n_miss}  unexpected={n_unexp}")
