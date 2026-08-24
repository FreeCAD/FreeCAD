#!/usr/bin/env python3
"""Command-line entry point for Doghouse inference from geometry JSON."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from doghouse_ai.inference import infer_geometry_data


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Run Doghouse inference from doghouse_inference_geometry.v1 JSON",
    )
    parser.add_argument("--geometry", required=True, help="Input geometry JSON")
    parser.add_argument("--output-dir", required=True, help="Inference output directory")
    parser.add_argument("--checkpoint", help="Override the production graph checkpoint")
    parser.add_argument("--structure-checkpoint", help="Override the structure checkpoint")
    parser.add_argument("--pmae-checkpoint", help="Override the Point-MAE checkpoint")
    parser.add_argument("--instance-sim-gallery", help="Override the similarity gallery")
    parser.add_argument("--instance-sim-threshold", type=float)
    parser.add_argument(
        "--no-instance-similarity",
        action="store_true",
        help="Disable the production instance-similarity filter",
    )
    parser.add_argument("--node-threshold", type=float, default=0.5)
    parser.add_argument("--edge-threshold", type=float, default=0.3)
    parser.add_argument("--min-instance-faces", type=int, default=2)
    parser.add_argument("--hole-wall-threshold", type=float, default=0.35)
    parser.add_argument("--mount-threshold", type=float, default=0.35)
    parser.add_argument("--pmae-num-group", type=int, default=256)
    parser.add_argument("--pmae-group-size", type=int, default=32)
    parser.add_argument("--cpu", action="store_true")
    return parser


def main() -> int:
    args = _build_parser().parse_args()
    geometry_path = Path(args.geometry)
    geometry = json.loads(geometry_path.read_text(encoding="utf-8"))
    output_dir = Path(args.output_dir)
    result = infer_geometry_data(
        geometry,
        output_dir=output_dir,
        stem=geometry_path.stem,
        checkpoint=args.checkpoint,
        structure_checkpoint=args.structure_checkpoint,
        pmae_checkpoint=args.pmae_checkpoint,
        instance_sim_gallery=args.instance_sim_gallery,
        instance_sim_threshold=args.instance_sim_threshold,
        enable_instance_similarity=not args.no_instance_similarity,
        node_threshold=args.node_threshold,
        edge_threshold=args.edge_threshold,
        min_instance_faces=args.min_instance_faces,
        hole_wall_threshold=args.hole_wall_threshold,
        mount_threshold=args.mount_threshold,
        pmae_num_group=args.pmae_num_group,
        pmae_group_size=args.pmae_group_size,
        cpu=args.cpu,
    )
    result["source_geometry"] = str(geometry_path)
    result_path = output_dir / f"{geometry_path.stem}_doghouse_result.json"
    result_path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(result_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
