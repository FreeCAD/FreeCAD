#!/usr/bin/env python3
"""Run doghouse inference directly from a STEP file."""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
import torch

try:
    from .build_point_dataset import build_dataset
    from .doghouse_assembly_features import export_assembly_colored_step, extract_assembly_features
    from .face_vote import vote_faces
    from .infer_graph import infer_graph_npz
    from .infer_pointnet_torch import _prepare_features
    from .pipeline_defaults import (
        DEFAULT_EDGE_THRESHOLD,
        DEFAULT_MIN_INSTANCE_FACES,
        DEFAULT_NODE_THRESHOLD,
        DEFAULT_PMAE_CKPT,
        DEFAULT_PMAE_FACE_EMB_DIR,
        apply_graph_postprocess,
        checkpoint_needs_pmae,
        resolve_checkpoint,
    )
    from .step_geometry import build_geometry_from_step
    from .train_pointnet_torch import PointMLP
except ImportError:
    from build_point_dataset import build_dataset
    from doghouse_assembly_features import export_assembly_colored_step, extract_assembly_features
    from face_vote import vote_faces
    from infer_graph import infer_graph_npz
    from infer_pointnet_torch import _prepare_features
    from pipeline_defaults import (
        DEFAULT_EDGE_THRESHOLD,
        DEFAULT_MIN_INSTANCE_FACES,
        DEFAULT_NODE_THRESHOLD,
        DEFAULT_PMAE_CKPT,
        DEFAULT_PMAE_FACE_EMB_DIR,
        apply_graph_postprocess,
        checkpoint_needs_pmae,
        resolve_checkpoint,
    )
    from step_geometry import build_geometry_from_step
    from train_pointnet_torch import PointMLP


EMPTY_LABELS = {"schema": "doghouse_instance_labels.v1", "face_labels": []}


def _merge_structure_role_probs(
    base_result: dict,
    structure_result: dict,
    *,
    mount_threshold: float,
    hole_wall_threshold: float,
) -> dict:
    """Attach structure-head probabilities without changing doghouse instances.

    The production doghouse checkpoint remains the source of the doghouse mask
    and connected components. The structure checkpoint only contributes
    mount_prob / hole_wall_prob for assembly fallback.
    """
    structure_by_face = {
        int(row["face_idx"]): row
        for row in structure_result.get("face_predictions", [])
        if "face_idx" in row
    }
    for row in base_result.get("face_predictions", []):
        fi = int(row["face_idx"])
        src = structure_by_face.get(fi, {})
        if "mount_prob" in src:
            mount_p = float(src["mount_prob"])
            row["mount_prob"] = round(mount_p, 6)
            row["mount"] = 1 if mount_p >= float(mount_threshold) else 0
        if "hole_wall_prob" in src:
            hole_p = float(src["hole_wall_prob"])
            row["hole_wall_prob"] = round(hole_p, 6)
            row["hole_wall"] = 1 if hole_p >= float(hole_wall_threshold) else 0
        # Role is only a display/assembly hint after postprocess; keep instance
        # membership from the base checkpoint untouched.
        if int(row.get("doghouse", 0)) > 0:
            if int(row.get("mount", 0)) > 0:
                row["role"] = "mount"
            elif int(row.get("hole_wall", 0)) > 0:
                row["role"] = "hole_wall"
    if "mount_probability" in structure_result:
        base_result["mount_probability"] = structure_result["mount_probability"]
        base_result["mount_threshold"] = float(mount_threshold)
    if "hole_wall_probability" in structure_result:
        base_result["hole_wall_probability"] = structure_result["hole_wall_probability"]
        base_result["hole_wall_threshold"] = float(hole_wall_threshold)
    base_result["structure_checkpoint"] = str(structure_result.get("checkpoint", ""))
    base_result["structure_roles_source"] = "secondary_graph_checkpoint"
    return base_result


def _normalize_unit_sphere(points: np.ndarray) -> np.ndarray:
    pts = points.astype(np.float32).copy()
    center = pts.mean(axis=0, keepdims=True)
    pts -= center
    scale = float(np.linalg.norm(pts, axis=1).max())
    if scale < 1e-6:
        scale = 1.0
    return (pts / scale).astype(np.float32)


def _write_pmae_input(data: dict, input_dir: str | Path, stem: str) -> Path:
    input_dir = Path(input_dir)
    input_dir.mkdir(parents=True, exist_ok=True)
    out = input_dir / f"{stem}_pmae_input.npz"
    np.savez_compressed(
        out,
        points=_normalize_unit_sphere(data["points"]),
        face_idx=data["face_idx"].astype(np.int64),
        num_faces=int(data["face_features"].shape[0]),
    )
    return out


def _attach_pmae_face_embeddings(data: dict, emb_dir: str | Path | None, stem: str) -> Path | None:
    """Load cached Point-MAE per-face embeddings into graph inference data."""
    if emb_dir is None:
        return None
    emb_path = Path(emb_dir) / f"{stem}_pmae_face_emb.npy"
    if not emb_path.exists():
        raise FileNotFoundError(f"PMAE face embedding not found: {emb_path}")
    emb = np.load(emb_path).astype(np.float32)
    expected = int(data["face_features"].shape[0])
    if emb.shape[0] != expected:
        emb_path.unlink(missing_ok=True)
        raise ValueError(
            f"PMAE face embedding count {emb.shape[0]} != num_faces {expected}; "
            f"deleted stale cache, rerun to regenerate: {emb_path}"
        )
    data["face_pmae"] = emb
    return emb_path


def _ensure_pmae_face_embeddings(
    data: dict,
    emb_dir: str | Path | None,
    stem: str,
    pmae_ckpt: str | Path | None,
    pmae_input_dir: str | Path,
    num_group: int,
    group_size: int,
    cpu: bool,
) -> Path | None:
    if emb_dir is None:
        return None
    emb_dir = Path(emb_dir)
    emb_path = emb_dir / f"{stem}_pmae_face_emb.npy"
    if emb_path.exists():
        try:
            cached = np.load(emb_path, mmap_mode="r")
            expected = int(data["face_features"].shape[0])
            if int(cached.shape[0]) != expected:
                print(
                    f"stale PMAE face embedding cache: {emb_path.name} "
                    f"faces={cached.shape[0]} expected={expected}; regenerating"
                )
                emb_path.unlink()
        except Exception:
            emb_path.unlink(missing_ok=True)
    if not emb_path.exists():
        if pmae_ckpt is None:
            raise FileNotFoundError(
                f"PMAE face embedding not found: {emb_path}. "
                "Provide --pmae-ckpt to generate it from the current STEP input."
            )
        input_path = _write_pmae_input(data, pmae_input_dir, stem)
        root_dir = Path(__file__).resolve().parent.parent
        script_path = root_dir / "extract_pmae_face_features.py"
        if not script_path.exists():
            script_path = Path(__file__).resolve().parent / "extract_pmae_face_features.py"
        cmd = [
            sys.executable,
            str(script_path),
            "--ckpt",
            str(pmae_ckpt),
            "--input-dir",
            str(input_path.parent),
            "--output-dir",
            str(emb_dir),
            "--num-group",
            str(num_group),
            "--group-size",
            str(group_size),
        ]
        if cpu:
            cmd.append("--cpu")
        subprocess.run(cmd, check=True, cwd=str(root_dir))
    return _attach_pmae_face_embeddings(data, emb_dir, stem)


def _run_model(npz_path: Path, checkpoint: Path, output_prefix: Path, *, cpu: bool = False):
    device = torch.device("cuda" if torch.cuda.is_available() and not cpu else "cpu")
    ckpt = torch.load(checkpoint, map_location=device)
    model = PointMLP(int(ckpt["in_dim"])).to(device)
    model.load_state_dict(ckpt["model_state"])
    model.eval()

    data = np.load(npz_path)
    x = _prepare_features(data)
    probs = []
    with torch.no_grad():
        for start in range(0, len(x), 65536):
            chunk = torch.from_numpy(x[start:start + 65536]).unsqueeze(0).to(device)
            logits = model(chunk).squeeze(0)
            probs.append(torch.sigmoid(logits).cpu().numpy())
    probs = np.concatenate(probs, axis=0)
    doghouse_pred = (probs >= 0.5).astype(np.int64)
    semantic_pred = doghouse_pred.copy()

    output_prefix.parent.mkdir(parents=True, exist_ok=True)
    np.save(str(output_prefix) + "_doghouse_prob.npy", probs.astype(np.float32))
    np.save(str(output_prefix) + "_doghouse_pred.npy", doghouse_pred)
    np.save(str(output_prefix) + "_semantic_pred.npy", semantic_pred)
    return semantic_pred, doghouse_pred, probs


def infer_step(args) -> dict:
    step_path = Path(args.step)
    out_dir = Path(args.output_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    stem = step_path.stem
    checkpoint = resolve_checkpoint(getattr(args, "checkpoint", None), backbone=args.backbone)

    geometry = build_geometry_from_step(
        step_path,
        sample_points_per_face=args.sample_points_per_face,
    )
    geometry_path = out_dir / f"{stem}_doghouse_infer.json"
    geometry_path.write_text(json.dumps(geometry, ensure_ascii=False, indent=2), encoding="utf-8")

    data = build_dataset(geometry, EMPTY_LABELS)
    npz_path = out_dir / f"{stem}_doghouse_points.npz"
    np.savez_compressed(npz_path, **data)
    pmae_input_dir = getattr(args, "pmae_input_dir", None) or (out_dir / "pmae_input")
    pmae_emb_dir = getattr(args, "pmae_face_emb_dir", None)
    if args.backbone == "graph" and pmae_emb_dir is None and checkpoint_needs_pmae(checkpoint):
        pmae_emb_dir = DEFAULT_PMAE_FACE_EMB_DIR
    pmae_ckpt = getattr(args, "pmae_ckpt", None)
    if args.backbone == "graph" and pmae_ckpt is None:
        pmae_ckpt = DEFAULT_PMAE_CKPT if DEFAULT_PMAE_CKPT.exists() else None
    pmae_emb_path = _ensure_pmae_face_embeddings(
        data,
        pmae_emb_dir,
        stem,
        pmae_ckpt,
        pmae_input_dir,
        int(getattr(args, "pmae_num_group", 256)),
        int(getattr(args, "pmae_group_size", 32)),
        bool(getattr(args, "pmae_cpu", False)),
    )
    if pmae_emb_path is not None:
        np.savez_compressed(npz_path, **data)

    if args.backbone == "graph":
        device = torch.device("cuda" if torch.cuda.is_available() and not args.cpu else "cpu")
        result = infer_graph_npz(
            data,
            checkpoint,
            device=device,
            node_threshold=args.node_threshold,
            edge_threshold=args.edge_threshold,
            min_instance_faces=getattr(args, "min_instance_faces", DEFAULT_MIN_INSTANCE_FACES),
            hole_wall_threshold=float(getattr(args, "hole_wall_threshold", 0.35)),
            mount_threshold=float(getattr(args, "mount_threshold", 0.35)),
        )
        result["source_step"] = str(step_path)
        result["geometry_json"] = str(geometry_path)
        result["npz"] = str(npz_path)
        result["checkpoint"] = str(checkpoint)
        result["backbone"] = "graph"
        if pmae_emb_path is not None:
            result["pmae_face_embeddings"] = str(pmae_emb_path)
        result = apply_graph_postprocess(
            result,
            data,
            instance_sim_gallery=getattr(args, "instance_sim_gallery", None),
            instance_sim_threshold=getattr(args, "instance_sim_threshold", None),
            enable_instance_sim=bool(getattr(args, "instance_sim_filter", True)),
            instance_filter=getattr(args, "instance_filter", None),
            instance_filter_threshold=getattr(args, "instance_filter_threshold", None),
        )
        structure_checkpoint = getattr(args, "structure_checkpoint", None)
        if structure_checkpoint:
            structure_checkpoint = Path(structure_checkpoint)
            structure_result = infer_graph_npz(
                data,
                structure_checkpoint,
                device=device,
                node_threshold=args.node_threshold,
                edge_threshold=args.edge_threshold,
                min_instance_faces=1,
                hole_wall_threshold=float(getattr(args, "hole_wall_threshold", 0.35)),
                mount_threshold=float(getattr(args, "mount_threshold", 0.35)),
            )
            structure_result["checkpoint"] = str(structure_checkpoint)
            result = _merge_structure_role_probs(
                result,
                structure_result,
                mount_threshold=float(getattr(args, "mount_threshold", 0.35)),
                hole_wall_threshold=float(getattr(args, "hole_wall_threshold", 0.35)),
            )
        pred_json = out_dir / f"{stem}_doghouse_pred_faces.json"
        pred_json.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    else:
        pred_prefix = out_dir / f"{stem}_pointmlp"
        semantic_pred, doghouse_pred, probs = _run_model(
            npz_path,
            checkpoint,
            pred_prefix,
            cpu=args.cpu,
        )

        result = vote_faces(
            data["face_idx"],
            semantic_pred,
            doghouse_pred=doghouse_pred,
            adjacency=data["adjacency"],
            threshold=args.threshold,
            min_component_faces=args.min_component_faces,
            close_ratio=args.close_ratio,
            close_iters=args.close_iters,
        )
        result["source_step"] = str(step_path)
        result["geometry_json"] = str(geometry_path)
        result["npz"] = str(npz_path)
        result["checkpoint"] = str(checkpoint)
        result["backbone"] = "pointmlp"
        if pmae_emb_path is not None:
            result["pmae_face_embeddings"] = str(pmae_emb_path)
        result["point_probability"] = {
            "min": round(float(probs.min()), 6),
            "max": round(float(probs.max()), 6),
            "mean": round(float(probs.mean()), 6),
        }
        pred_json = out_dir / f"{stem}_doghouse_pred_faces.json"
        pred_json.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")

    assembly_json = None
    assembly_step = None
    if args.extract_assembly_features or args.assembly_output_step:
        assembly = extract_assembly_features(
            step_path,
            result,
            use_vf2=args.use_vf2,
            vf2_required=args.vf2_required,
            prefer_ai_structure_fallback=bool(getattr(args, "prefer_ai_holes", True)),
            ai_mount_score_threshold=float(getattr(args, "ai_mount_score_threshold", 0.35)),
            ai_hole_score_threshold=float(getattr(args, "ai_hole_score_threshold", 0.35)),
            ai_hole_min_confidence=float(getattr(args, "ai_hole_min_confidence", 0.35)),
            experimental_freeform_endpoint=args.experimental_freeform_endpoint,
        )
        if args.extract_assembly_features:
            assembly_json = out_dir / f"{stem}_doghouse_assembly_features.json"
            assembly_json.write_text(json.dumps(assembly, ensure_ascii=False, indent=2), encoding="utf-8")
            result["assembly_features_json"] = str(assembly_json)
        if args.assembly_output_step:
            assembly_step = Path(args.assembly_output_step)
            export_assembly_colored_step(step_path, assembly, assembly_step)
            result["assembly_colored_step"] = str(assembly_step)

    print(f"step: {step_path}")
    print(f"geometry: {geometry_path}")
    print(f"npz: {npz_path}")
    print(f"prediction: {pred_json}")
    if assembly_json is not None:
        print(f"assembly_features: {assembly_json}")
    if assembly_step is not None:
        print(f"assembly_colored_step: {assembly_step}")
    print(f"backbone: {result.get('backbone', args.backbone)}")
    print(f"faces: {len(result['face_predictions'])}")
    print(f"instances: {len(result['doghouse_instances'])}")
    print(f"doghouse faces: {sum(1 for r in result['face_predictions'] if r['doghouse'])}")
    if "point_probability" in result:
        print(
            "probability: "
            f"min={result['point_probability']['min']} "
            f"max={result['point_probability']['max']} "
            f"mean={result['point_probability']['mean']}"
        )
    if "node_probability" in result:
        print(
            "node_probability: "
            f"min={result['node_probability']['min']} "
            f"max={result['node_probability']['max']} "
            f"mean={result['node_probability']['mean']}"
        )
    return result


def _build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Integrated doghouse pipeline: STEP -> mixed PMAE graph GNN -> "
            "min-instance-faces -> instance-similarity -> optional assembly features"
        ),
    )
    parser.add_argument("--step", required=True)
    parser.add_argument(
        "--checkpoint",
        help="Graph or PointMLP checkpoint; defaults to production mixed PMAE graph weights",
    )
    parser.add_argument(
        "--structure-checkpoint",
        help=(
            "Optional graph checkpoint that supplies mount_prob/hole_wall_prob only. "
            "Doghouse instances still come from --checkpoint."
        ),
    )
    parser.add_argument(
        "--backbone",
        choices=["pointmlp", "graph"],
        default="graph",
        help="graph: integrated production pipeline (default); pointmlp: legacy baseline",
    )
    parser.add_argument(
        "--output-dir",
        default="nearest_hole_tool/doghouse_ai/data/step_infer",
    )
    parser.add_argument("--sample-points-per-face", type=int, default=64)
    parser.add_argument("--threshold", type=float, default=0.5)
    parser.add_argument("--node-threshold", type=float, default=DEFAULT_NODE_THRESHOLD)
    parser.add_argument("--edge-threshold", type=float, default=DEFAULT_EDGE_THRESHOLD)
    parser.add_argument(
        "--min-instance-faces",
        type=int,
        default=DEFAULT_MIN_INSTANCE_FACES,
        help="Remove predicted doghouse instances with fewer faces than this value",
    )
    parser.add_argument(
        "--instance-sim-filter",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Apply production instance-similarity gallery after graph GNN (default: enabled)",
    )
    parser.add_argument(
        "--instance-sim-gallery",
        help="Override production instance-similarity gallery .npz",
    )
    parser.add_argument(
        "--instance-sim-threshold",
        type=float,
        help="Override threshold stored in the instance-similarity gallery",
    )
    parser.add_argument(
        "--instance-filter",
        help="Deprecated legacy instance classifier; prefer --instance-sim-filter",
    )
    parser.add_argument(
        "--instance-filter-threshold",
        type=float,
        help="Override threshold stored in --instance-filter",
    )
    parser.add_argument(
        "--pmae-face-emb-dir",
        default=None,
        help=f"Directory for {{stem}}_pmae_face_emb.npy; graph default: {DEFAULT_PMAE_FACE_EMB_DIR}",
    )
    parser.add_argument(
        "--pmae-ckpt",
        default=None,
        help=f"Point-MAE pretrain checkpoint for on-demand embedding extraction; default: {DEFAULT_PMAE_CKPT}",
    )
    parser.add_argument(
        "--pmae-input-dir",
        help="Directory for generated {step_stem}_pmae_input.npz; defaults to <output-dir>/pmae_input",
    )
    parser.add_argument("--pmae-num-group", type=int, default=256)
    parser.add_argument("--pmae-group-size", type=int, default=32)
    parser.add_argument("--pmae-cpu", action="store_true", help="Run Point-MAE embedding extraction on CPU")
    parser.add_argument(
        "--min-component-faces",
        type=int,
        default=8,
        help="Only removes tiny specks; real instances are recovered by mask closing + mount/hole validation, so this no longer needs per-model tuning",
    )
    parser.add_argument(
        "--close-ratio",
        type=float,
        default=0.66,
        help="Morphological closing: min fraction of neighbours in one component to fill a face (<=0 disables)",
    )
    parser.add_argument("--close-iters", type=int, default=4)
    parser.add_argument("--cpu", action="store_true")
    parser.add_argument(
        "--extract-assembly-features",
        action="store_true",
        help="Also extract mount faces and mounting holes inside predicted doghouses",
    )
    parser.add_argument(
        "--assembly-output-step",
        help="Output colored STEP for mount faces and mounting hole faces; omit for faster JSON-only runs",
    )
    parser.add_argument(
        "--use-vf2",
        action="store_true",
        help="Use local VF2 verification for holes after mount face selection",
    )
    parser.add_argument(
        "--vf2-required",
        action="store_true",
        help="Drop geometric hole groups when local VF2 verification finds none",
    )
    parser.add_argument(
        "--prefer-ai-holes",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="If VF2 lacks reliable analytic round holes, fall back to AI structure mount/hole (default: on)",
    )
    parser.add_argument(
        "--ai-mount-score-threshold",
        type=float,
        default=0.35,
        help="Min mount_prob for AI structure mount candidates",
    )
    parser.add_argument(
        "--ai-hole-score-threshold",
        type=float,
        default=0.35,
        help="Min hole_wall_prob to keep an AI through-hole wall candidate",
    )
    parser.add_argument(
        "--ai-hole-min-confidence",
        type=float,
        default=0.35,
        help="Min AI structure confidence to accept the AI mount/hole fallback",
    )
    parser.add_argument(
        "--hole-wall-threshold",
        type=float,
        default=0.35,
        help="Threshold for marking face role=hole_wall in graph predictions",
    )
    parser.add_argument(
        "--mount-threshold",
        type=float,
        default=0.35,
        help="Threshold for marking face role=mount in graph predictions",
    )
    parser.add_argument(
        "--experimental-freeform-endpoint",
        action=argparse.BooleanOptionalAction,
        default=True,
        help=(
            "Enable freeform oblique-hole endpoint topology rules in assembly extraction "
            "(default: enabled; use --no-experimental-freeform-endpoint for legacy rules)"
        ),
    )
    return parser


def main() -> int:
    parser = _build_arg_parser()
    args = parser.parse_args()
    infer_step(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
