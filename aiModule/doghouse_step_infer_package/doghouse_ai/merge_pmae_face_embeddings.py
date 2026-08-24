#!/usr/bin/env python3
"""Merge cached Point-MAE per-face embeddings into doghouse graph/inference NPZ files."""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np


def merge_pmae_face_embeddings(npz_path: str | Path, emb_path: str | Path, output_path: str | Path) -> Path:
    npz_path = Path(npz_path)
    emb_path = Path(emb_path)
    output_path = Path(output_path)

    data = dict(np.load(npz_path, allow_pickle=True))
    emb = np.load(emb_path).astype(np.float32)
    expected = int(data["face_features"].shape[0])
    if emb.shape[0] != expected:
        raise ValueError(f"PMAE face embedding count {emb.shape[0]} != num_faces {expected}: {emb_path}")

    data["face_pmae"] = emb
    output_path.parent.mkdir(parents=True, exist_ok=True)
    np.savez_compressed(output_path, **data)
    return output_path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--npz", required=True, help="Input doghouse *_points.npz or *_graph.npz")
    parser.add_argument("--emb", required=True, help="Input {model}_pmae_face_emb.npy")
    parser.add_argument("--output", required=True, help="Output NPZ containing face_pmae")
    args = parser.parse_args()

    output = merge_pmae_face_embeddings(args.npz, args.emb, args.output)
    print(f"saved: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
