#!/usr/bin/env python3
"""Default model resources for the embedded Doghouse inference runtime."""

from __future__ import annotations

import warnings
from pathlib import Path


MODULE_DIR = Path(__file__).resolve().parents[2]
RESOURCE_DIR = MODULE_DIR / "resources"

PRODUCTION_GRAPH_CHECKPOINT = (
    RESOURCE_DIR / "checkpoints" / "doghouse_graph_pmae_7_plus_B126302301001.pt"
)
PRODUCTION_STRUCTURE_CHECKPOINT = (
    RESOURCE_DIR / "checkpoints" / "doghouse_graph_s200_structure.pt"
)
PRODUCTION_INSTANCE_SIM_GALLERY = (
    RESOURCE_DIR
    / "galleries"
    / "doghouse_instance_similarity_gallery_7_plus_B126302301001.npz"
)
DEFAULT_PMAE_CKPT = RESOURCE_DIR / "checkpoints" / "ckpt-last.pth"

DEFAULT_MIN_INSTANCE_FACES = 2
DEFAULT_NODE_THRESHOLD = 0.5
DEFAULT_EDGE_THRESHOLD = 0.3


def checkpoint_needs_pmae(checkpoint: Path) -> bool:
    import torch

    ckpt = torch.load(checkpoint, map_location="cpu")
    return int(ckpt.get("extra_dim", 0)) > 0


def resolve_checkpoint(path: str | Path | None) -> Path:
    return Path(path) if path else PRODUCTION_GRAPH_CHECKPOINT


def resolve_instance_sim_gallery(path: str | Path | None) -> Path | None:
    return Path(path) if path else PRODUCTION_INSTANCE_SIM_GALLERY


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
    """Apply the production instance-similarity postprocessor."""
    if instance_filter:
        warnings.warn(
            "Legacy instance filters are not included in the embedded runtime",
            RuntimeWarning,
            stacklevel=2,
        )

    gallery_path = resolve_instance_sim_gallery(instance_sim_gallery) if enable_instance_sim else None
    if gallery_path is not None and gallery_path.exists():
        from .instance_similarity import apply_instance_similarity_filter

        result = apply_instance_similarity_filter(
            result,
            data,
            gallery_path,
            threshold=instance_sim_threshold,
        )
    elif gallery_path is not None:
        result.setdefault("pipeline_warnings", []).append(
            f"instance similarity gallery not found: {gallery_path}"
        )
    else:
        result["instance_similarity_filter"] = {"enabled": False}

    result["pipeline"] = {
        "backbone": "face_graph_gnn",
        "min_instance_faces": int(result.get("min_instance_faces", DEFAULT_MIN_INSTANCE_FACES)),
        "instance_similarity_enabled": bool(
            enable_instance_sim and gallery_path and gallery_path.exists()
        ),
        "instance_similarity_gallery": str(gallery_path) if gallery_path else None,
    }
    return result
