"""Validate the isolated Doghouse AI inference environment and packaged weights."""

from __future__ import annotations

import importlib
import json
import sys
from pathlib import Path

import numpy as np
import torch


ROOT = Path(__file__).resolve().parent.parent
CHECKPOINTS = {
    "point_mae": ROOT / "doghouse_ai" / "ckpt-last.pth",
    "doghouse_graph": ROOT / "doghouse_ai" / "checkpoints" / "doghouse_graph_pmae_7_plus_B126302301001.pt",
    "geometry_fallback": ROOT / "doghouse_ai" / "checkpoints" / "doghouse_graph_v1.pt",
    "structure": ROOT / "doghouse_ai" / "structure_s200" / "checkpoints" / "doghouse_graph_s200_structure.pt",
    "similarity_gallery": ROOT / "doghouse_ai" / "checkpoints" / "doghouse_instance_similarity_gallery_7_plus_B126302301001.npz",
}


def main() -> None:
    modules = {}
    for name in ("OCC", "timm", "easydict", "termcolor", "cv2"):
        module = importlib.import_module(name)
        modules[name] = getattr(module, "__version__", "ok")

    missing = [str(path) for path in CHECKPOINTS.values() if not path.is_file()]
    if missing:
        raise FileNotFoundError("Missing packaged model files: " + ", ".join(missing))

    checkpoint_metadata = {}
    for name in ("doghouse_graph", "geometry_fallback", "structure"):
        path = CHECKPOINTS[name]
        checkpoint = torch.load(path, map_location="cpu", weights_only=False)
        checkpoint_metadata[name] = {
            key: checkpoint.get(key)
            for key in ("in_dim", "extra_dim", "hidden_dim", "num_layers", "epochs")
            if key in checkpoint
        }

    gallery = np.load(CHECKPOINTS["similarity_gallery"])
    result = {
        "status": "ok",
        "python": sys.version.split()[0],
        "torch": torch.__version__,
        "cuda_runtime": torch.version.cuda,
        "cuda_available": torch.cuda.is_available(),
        "gpu": torch.cuda.get_device_name(0) if torch.cuda.is_available() else None,
        "modules": modules,
        "checkpoint_metadata": checkpoint_metadata,
        "point_mae_bytes": CHECKPOINTS["point_mae"].stat().st_size,
        "gallery_arrays": sorted(gallery.files),
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
