#!/usr/bin/env python3
"""Build an instance-level doghouse similarity gallery."""

from __future__ import annotations

import argparse
from pathlib import Path

try:
    from .instance_similarity import build_instance_gallery, save_similarity_gallery
except ImportError:
    from instance_similarity import build_instance_gallery, save_similarity_gallery


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--data-dir", default="doghouse_ai/data/graph_train")
    parser.add_argument("--output", default="doghouse_ai/checkpoints/doghouse_instance_similarity_gallery.npz")
    parser.add_argument(
        "--prediction-dir",
        action="append",
        default=[],
        help="Optional directory containing per-model prediction JSONs; may be repeated to add extra-component negatives",
    )
    parser.add_argument(
        "--positive-prediction-dir",
        action="append",
        default=[],
        help="Prediction dirs whose components covering GT (cover>=--positive-cover) are added as positive morphologies",
    )
    parser.add_argument("--label-dir", help="Directory containing annotation JSONs for prediction IoU matching")
    parser.add_argument("--extra-iou", type=float, default=0.2)
    parser.add_argument(
        "--positive-cover",
        type=float,
        default=0.8,
        help="Min GT face cover for --positive-prediction-dir components to become positive prototypes",
    )
    parser.add_argument("--threshold", type=float, default=0.0)
    args = parser.parse_args()

    paths = sorted(Path(args.data_dir).glob("*_graph.npz"))
    if not paths:
        raise FileNotFoundError(f"no *_graph.npz files in {args.data_dir}")

    gallery = build_instance_gallery(
        paths,
        prediction_dirs=[Path(p) for p in args.prediction_dir],
        positive_prediction_dirs=[Path(p) for p in args.positive_prediction_dir],
        label_dir=Path(args.label_dir) if args.label_dir else None,
        extra_iou=args.extra_iou,
        positive_cover=args.positive_cover,
        threshold=args.threshold,
    )
    save_similarity_gallery(args.output, gallery)
    print(f"saved instance similarity gallery: {args.output}")
    print(f"positive prototypes: {int(gallery['train_positive'][0])}")
    print(f"negative prototypes: {int(gallery['train_negative'][0])}")
    print(f"numeric features: {len(gallery['numeric_feature_names'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
