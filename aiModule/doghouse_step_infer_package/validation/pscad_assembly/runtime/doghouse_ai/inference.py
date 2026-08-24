#!/usr/bin/env python3
"""Production Doghouse inference from an in-memory geometry document."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import numpy as np
import torch

from .build_point_dataset import build_dataset
from .infer_graph import infer_graph_npz
from .pipeline_defaults import (
    DEFAULT_EDGE_THRESHOLD,
    DEFAULT_MIN_INSTANCE_FACES,
    DEFAULT_NODE_THRESHOLD,
    DEFAULT_PMAE_CKPT,
    PRODUCTION_STRUCTURE_CHECKPOINT,
    apply_graph_postprocess,
    checkpoint_needs_pmae,
    resolve_checkpoint,
)


EMPTY_LABELS = {"schema": "doghouse_instance_labels.v1", "face_labels": []}


def _normalize_unit_sphere(points: np.ndarray) -> np.ndarray:
    points = points.astype(np.float32).copy()
    points -= points.mean(axis=0, keepdims=True)
    scale = float(np.linalg.norm(points, axis=1).max())
    return (points / max(scale, 1.0e-6)).astype(np.float32)


def _generate_pmae_embeddings(
    data: dict,
    output_dir: Path,
    stem: str,
    checkpoint: Path,
    *,
    cpu: bool,
    num_group: int,
    group_size: int,
) -> Path:
    input_dir = output_dir / "pmae_input"
    embedding_dir = output_dir / "pmae_face_emb"
    input_dir.mkdir(parents=True, exist_ok=True)
    embedding_dir.mkdir(parents=True, exist_ok=True)
    input_path = input_dir / f"{stem}_pmae_input.npz"
    np.savez_compressed(
        input_path,
        points=_normalize_unit_sphere(data["points"]),
        face_idx=data["face_idx"].astype(np.int64),
        num_faces=int(data["face_features"].shape[0]),
    )

    python_dir = Path(__file__).resolve().parents[1]
    command = [
        sys.executable,
        str(python_dir / "extract_pmae_face_features.py"),
        "--ckpt",
        str(checkpoint),
        "--input-dir",
        str(input_dir),
        "--output-dir",
        str(embedding_dir),
        "--num-group",
        str(num_group),
        "--group-size",
        str(group_size),
    ]
    if cpu:
        command.append("--cpu")
    subprocess.run(command, check=True, cwd=python_dir)
    return embedding_dir / f"{stem}_pmae_face_emb.npy"


def _attach_face_identifiers(result: dict, geometry: dict) -> None:
    """Copy stable source identifiers into each face prediction."""
    faces = {int(face["face_idx"]): face for face in geometry["faces"]}
    for prediction in result.get("face_predictions", []):
        source = faces[int(prediction["face_idx"])]
        if "kernel_face_tag" in source:
            prediction["kernel_face_tag"] = source["kernel_face_tag"]
        if source.get("persistent_face_id"):
            prediction["persistent_face_id"] = source["persistent_face_id"]


def infer_geometry_data(
    geometry: dict,
    *,
    output_dir: str | Path,
    stem: str,
    checkpoint: str | Path | None = None,
    structure_checkpoint: str | Path | None = None,
    pmae_checkpoint: str | Path | None = None,
    instance_sim_gallery: str | Path | None = None,
    instance_sim_threshold: float | None = None,
    enable_instance_similarity: bool = True,
    node_threshold: float = DEFAULT_NODE_THRESHOLD,
    edge_threshold: float = DEFAULT_EDGE_THRESHOLD,
    min_instance_faces: int = DEFAULT_MIN_INSTANCE_FACES,
    hole_wall_threshold: float = 0.35,
    mount_threshold: float = 0.35,
    pmae_num_group: int = 256,
    pmae_group_size: int = 32,
    cpu: bool = False,
) -> dict:
    """Run the production graph pipeline without STEP or PythonOCC."""
    if geometry.get("schema") != "doghouse_inference_geometry.v1":
        raise ValueError(f"unexpected geometry schema: {geometry.get('schema')}")
    faces = geometry.get("faces", [])
    face_indices = [int(face.get("face_idx", -1)) for face in faces]
    if face_indices != list(range(len(faces))):
        raise ValueError("face_idx values must be contiguous and zero-based")

    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    graph_checkpoint = resolve_checkpoint(checkpoint)
    if not graph_checkpoint.is_file():
        raise FileNotFoundError(f"graph checkpoint not found: {graph_checkpoint}")

    data = build_dataset(geometry, EMPTY_LABELS)
    npz_path = output_dir / f"{stem}_doghouse_points.npz"
    np.savez_compressed(npz_path, **data)

    if checkpoint_needs_pmae(graph_checkpoint):
        pmae_checkpoint = Path(pmae_checkpoint) if pmae_checkpoint else DEFAULT_PMAE_CKPT
        if not pmae_checkpoint.is_file():
            raise FileNotFoundError(f"PMAE checkpoint not found: {pmae_checkpoint}")
        embedding_path = _generate_pmae_embeddings(
            data,
            output_dir,
            stem,
            pmae_checkpoint,
            cpu=cpu,
            num_group=pmae_num_group,
            group_size=pmae_group_size,
        )
        embeddings = np.load(embedding_path).astype(np.float32)
        expected_faces = int(data["face_features"].shape[0])
        if embeddings.shape[0] != expected_faces:
            raise ValueError(
                f"PMAE face embedding count {embeddings.shape[0]} != {expected_faces}"
            )
        data["face_pmae"] = embeddings
        np.savez_compressed(npz_path, **data)
    else:
        embedding_path = None

    device = torch.device("cuda" if torch.cuda.is_available() and not cpu else "cpu")
    result = infer_graph_npz(
        data,
        graph_checkpoint,
        device=device,
        node_threshold=node_threshold,
        edge_threshold=edge_threshold,
        min_instance_faces=min_instance_faces,
        hole_wall_threshold=hole_wall_threshold,
        mount_threshold=mount_threshold,
    )
    result = apply_graph_postprocess(
        result,
        data,
        instance_sim_gallery=instance_sim_gallery,
        instance_sim_threshold=instance_sim_threshold,
        enable_instance_sim=enable_instance_similarity,
        instance_filter=None,
        instance_filter_threshold=None,
    )

    role_checkpoint = (
        Path(structure_checkpoint)
        if structure_checkpoint
        else PRODUCTION_STRUCTURE_CHECKPOINT
    )
    if role_checkpoint.is_file():
        role_result = infer_graph_npz(
            data,
            role_checkpoint,
            device=device,
            node_threshold=node_threshold,
            edge_threshold=edge_threshold,
            min_instance_faces=1,
            hole_wall_threshold=hole_wall_threshold,
            mount_threshold=mount_threshold,
        )
        role_by_face = {
            int(row["face_idx"]): row for row in role_result.get("face_predictions", [])
        }
        for row in result.get("face_predictions", []):
            role = role_by_face.get(int(row["face_idx"]), {})
            for key in ("mount", "mount_prob", "hole_wall", "hole_wall_prob"):
                if key in role:
                    row[key] = role[key]
            if int(row.get("doghouse", 0)) > 0:
                if int(row.get("mount", 0)) > 0:
                    row["role"] = "mount"
                elif int(row.get("hole_wall", 0)) > 0:
                    row["role"] = "hole_wall"
        result["structure_checkpoint"] = str(role_checkpoint)

    result["source_geometry_schema"] = geometry["schema"]
    result["npz"] = str(npz_path)
    result["checkpoint"] = str(graph_checkpoint)
    if embedding_path is not None:
        result["pmae_face_embeddings"] = str(embedding_path)
    _attach_face_identifiers(result, geometry)
    return result
