#!/usr/bin/env python3
"""Shared production defaults for the integrated doghouse pipeline."""

from __future__ import annotations

import warnings
from pathlib import Path

DOGHOUSE_AI_DIR = Path(__file__).resolve().parent

PRODUCTION_GRAPH_CHECKPOINT = (
    DOGHOUSE_AI_DIR / "checkpoints" / "doghouse_graph_pmae_7_plus_B126302301001.pt"
)
PRODUCTION_INSTANCE_SIM_GALLERY = (
    DOGHOUSE_AI_DIR / "checkpoints" / "doghouse_instance_similarity_gallery_7_plus_B126302301001.npz"
)
LEGACY_GRAPH_CHECKPOINT = DOGHOUSE_AI_DIR / "checkpoints" / "doghouse_graph_v1.pt"
DEFAULT_PMAE_FACE_EMB_DIR = DOGHOUSE_AI_DIR / "pmae_face_emb"
DEFAULT_PMAE_CKPT = DOGHOUSE_AI_DIR / "ckpt-last.pth"

DEFAULT_BACKBONE = "graph"
DEFAULT_MIN_INSTANCE_FACES = 2
DEFAULT_NODE_THRESHOLD = 0.5
DEFAULT_EDGE_THRESHOLD = 0.3


def checkpoint_needs_pmae(checkpoint: Path) -> bool:
    import torch

    ckpt = torch.load(checkpoint, map_location="cpu")
    return int(ckpt.get("extra_dim", 0)) > 0


def resolve_checkpoint(path: str | Path | None, *, backbone: str) -> Path:
    if path:
        return Path(path)
    if backbone == "graph":
        return PRODUCTION_GRAPH_CHECKPOINT
    raise ValueError("--checkpoint is required when --backbone pointmlp")


def resolve_instance_sim_gallery(path: str | Path | None) -> Path | None:
    if path:
        return Path(path)
    return PRODUCTION_INSTANCE_SIM_GALLERY


def apply_graph_postprocess(
    result: dict,
    data: dict,
    *,
    instance_sim_gallery: str | Path | None,
    instance_sim_threshold: float | None = None,
    enable_instance_sim: bool = True,
    instance_filter: str | Path | None = None,
    instance_filter_threshold: float | None = None,
) -> dict:
    """Apply integrated graph post-processing: optional legacy filter, then similarity gallery."""
    if instance_filter:
        warnings.warn(
            "--instance-filter is deprecated; production pipeline uses --instance-sim-gallery",
            DeprecationWarning,
            stacklevel=2,
        )
        try:
            from .instance_filter import apply_instance_filter
        except ImportError:
            from instance_filter import apply_instance_filter
        result = apply_instance_filter(
            result,
            data,
            instance_filter,
            threshold=instance_filter_threshold,
        )

    gallery_path = resolve_instance_sim_gallery(instance_sim_gallery) if enable_instance_sim else None
    if gallery_path is not None:
        if gallery_path.exists():
            try:
                from .instance_similarity import apply_instance_similarity_filter
            except ImportError:
                from instance_similarity import apply_instance_similarity_filter
            result = apply_instance_similarity_filter(
                result,
                data,
                gallery_path,
                threshold=instance_sim_threshold,
            )
        else:
            result.setdefault("pipeline_warnings", []).append(
                f"instance similarity gallery not found: {gallery_path}"
            )
    elif not enable_instance_sim:
        result["instance_similarity_filter"] = {"enabled": False}

    result["pipeline"] = {
        "backbone": "face_graph_gnn",
        "min_instance_faces": int(result.get("min_instance_faces", DEFAULT_MIN_INSTANCE_FACES)),
        "instance_similarity_enabled": bool(enable_instance_sim and gallery_path and gallery_path.exists()),
        "instance_similarity_gallery": str(gallery_path) if gallery_path else None,
    }
    return result
