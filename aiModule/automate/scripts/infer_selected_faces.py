"""Two-stage face-conditioned Location and Mate Type inference for two STEP parts."""

from __future__ import annotations

import argparse
import json
import math
import sys
import time
from pathlib import Path
from types import SimpleNamespace

import torch

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import (
    LocationModel, LocationModelConfig, MateTypeModel, MateTypeModelConfig, flatbatch,
)
from build_location_selection_examples import local_mcfs_for_face
try:
    from infer_mate import build_graph, mcf_json
except ModuleNotFoundError:
    from scripts.infer_mate import build_graph, mcf_json


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("part_a", type=Path); parser.add_argument("part_b", type=Path)
    parser.add_argument("--face-a", type=int, required=True, help="Zero-based exported face index")
    parser.add_argument("--face-b", type=int, required=True, help="Zero-based exported face index")
    parser.add_argument("--face-signature-a"); parser.add_argument("--face-signature-b")
    parser.add_argument("--location-checkpoint", type=Path, default=ROOT / "runs/paper_location_full_e30/best.pt")
    parser.add_argument("--mate-type-checkpoint", type=Path, default=ROOT / "runs/paper_mate_type_10000_e50/best.pt")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--top-k", type=int, default=5)
    parser.add_argument("--max-pairs", type=int, default=10000)
    parser.add_argument("--device", choices=("auto", "cpu", "cuda"), default="auto")
    return parser.parse_args()


def face_signature(face):
    return {
        "area_m2": float(face.surface_area),
        "center_m": [float(value) for value in face.center_of_gravity],
    }


def resolve_face(part, requested_index, encoded_signature):
    faces = part.brep.nodes.faces
    if not 0 <= requested_index < len(faces):
        raise IndexError(f"Requested face {requested_index} is outside exported part ({len(faces)} faces)")
    if not encoded_signature:
        return requested_index, None
    target = json.loads(encoded_signature)
    target_area = max(float(target["area_m2"]), 1.0e-18)
    target_center = torch.tensor(target["center_m"], dtype=torch.float64)
    length_scale = max(math.sqrt(target_area), 1.0e-9)
    scores = []
    for index, face in enumerate(faces):
        candidate_area = max(float(face.surface_area), 1.0e-18)
        candidate_center = torch.tensor(face.center_of_gravity, dtype=torch.float64)
        area_error = abs(math.log(candidate_area / target_area))
        center_error = float(torch.linalg.vector_norm(candidate_center - target_center)) / length_scale
        scores.append((area_error + center_error, area_error, center_error, index))
    score, area_error, center_error, resolved = min(scores)
    # Export normally preserves the index. Only remap when the geometric match
    # is convincing; otherwise retain the explicit FaceN selection.
    requested_score = scores[requested_index][0]
    if score < requested_score * 0.5 and area_error < 0.05 and center_error < 0.05:
        return resolved, {"score": score, "area_error": area_error, "center_error": center_error}
    return requested_index, {"score": requested_score, "retained_requested_index": True}


def location_batch(graph_a, graph_b, face_a, face_b, local_a, local_b, device):
    batch_a, batch_b = flatbatch([graph_a]).to(device), flatbatch([graph_b]).to(device)
    local_a = torch.tensor(local_a, dtype=torch.long, device=device)
    local_b = torch.tensor(local_b, dtype=torch.long, device=device)
    local_pairs = torch.stack((
        torch.arange(local_a.numel(), device=device).repeat_interleave(local_b.numel()),
        torch.arange(local_b.numel(), device=device).repeat(local_a.numel()),
    ), dim=1)
    return SimpleNamespace(
        graph_a=batch_a, graph_b=batch_b,
        selected_faces=torch.tensor([[face_a, face_b]], dtype=torch.long, device=device),
        local_mcfs_a=local_a, local_mcfs_b=local_b,
        local_mcf_to_sample_a=torch.zeros(local_a.numel(), dtype=torch.long, device=device),
        local_mcf_to_sample_b=torch.zeros(local_b.numel(), dtype=torch.long, device=device),
        candidate_local_pairs=local_pairs,
    )


def main():
    args = parse_args(); started = time.perf_counter()
    if args.top_k <= 0: raise ValueError("top-k must be positive")
    device = torch.device("cuda" if args.device == "auto" and torch.cuda.is_available()
                          else "cpu" if args.device == "auto" else args.device)
    graph_a, part_a = build_graph(args.part_a, return_part=True)
    graph_b, part_b = build_graph(args.part_b, return_part=True)
    face_a, match_a = resolve_face(part_a, args.face_a, args.face_signature_a)
    face_b, match_b = resolve_face(part_b, args.face_b, args.face_signature_b)
    local_a = local_mcfs_for_face(graph_a, face_a); local_b = local_mcfs_for_face(graph_b, face_b)
    pair_count = len(local_a) * len(local_b)
    if not local_a or not local_b: raise ValueError("A selected face has no local MCF candidates")
    if pair_count > args.max_pairs:
        raise ValueError(f"Selected-face candidate product {len(local_a)} x {len(local_b)} = {pair_count} exceeds {args.max_pairs}")

    location_checkpoint = torch.load(args.location_checkpoint, map_location=device, weights_only=False)
    if location_checkpoint.get("task") != "mcf_location": raise ValueError("Invalid Location checkpoint")
    location_model = LocationModel(LocationModelConfig(**location_checkpoint["model_config"]))
    location_model.load_state_dict(location_checkpoint["model_state"], strict=True)
    location_model.to(device).eval()
    batch = location_batch(graph_a, graph_b, face_a, face_b, local_a, local_b, device)
    with torch.no_grad():
        location_logits = location_model(batch)
        probabilities = torch.softmax(location_logits, dim=0)
        keep = min(args.top_k, pair_count)
        top_probabilities, top_indices = torch.topk(probabilities, keep)
        top_pairs_local = batch.candidate_local_pairs[top_indices]
        top_pairs = torch.stack((batch.local_mcfs_a[top_pairs_local[:, 0]],
                                 batch.local_mcfs_b[top_pairs_local[:, 1]]), dim=1)

    type_checkpoint = torch.load(args.mate_type_checkpoint, map_location=device, weights_only=False)
    if type_checkpoint.get("task") != "mate_type": raise ValueError("Invalid Mate Type checkpoint")
    type_model = MateTypeModel(MateTypeModelConfig(**type_checkpoint["model_config"]))
    type_model.load_state_dict(type_checkpoint["model_state"], strict=True)
    type_model.to(device).eval()
    type_batch = SimpleNamespace(graph_a=batch.graph_a, graph_b=batch.graph_b, mcf_pairs=top_pairs)
    with torch.no_grad(): type_probabilities = torch.softmax(type_model(type_batch), dim=1)
    mapping = type_checkpoint["mate_type_to_id"]
    type_names = [name for name, _ in sorted(mapping.items(), key=lambda item: item[1])]

    result = {
        "schema_version": 2, "workflow": "selected_faces_location_then_mate_type",
        "device": str(device), "elapsed_seconds": time.perf_counter() - started,
        "location_checkpoint": str(args.location_checkpoint.resolve()),
        "location_checkpoint_epoch": int(location_checkpoint["epoch"]),
        "mate_type_checkpoint": str(args.mate_type_checkpoint.resolve()),
        "mate_type_checkpoint_epoch": int(type_checkpoint["epoch"]),
        "parts": [
            {"path": str(args.part_a.resolve()), "face_count": int(graph_a.n_faces),
             "mcf_count": int(graph_a.mcfs.shape[0]), "requested_face": args.face_a,
             "selected_face": face_a, "face_match": match_a, "local_mcf_count": len(local_a)},
            {"path": str(args.part_b.resolve()), "face_count": int(graph_b.n_faces),
             "mcf_count": int(graph_b.mcfs.shape[0]), "requested_face": args.face_b,
             "selected_face": face_b, "face_match": match_b, "local_mcf_count": len(local_b)},
        ],
        "pair_count": pair_count, "recommendations": [],
    }
    for rank in range(keep):
        index_a, index_b = (int(value) for value in top_pairs[rank].tolist())
        typed = type_probabilities[rank].cpu(); type_id = int(typed.argmax())
        result["recommendations"].append({
            "rank": rank + 1, "score": float(location_logits[top_indices[rank]]),
            "probability": float(top_probabilities[rank]),
            "a": mcf_json(graph_a, index_a), "b": mcf_json(graph_b, index_b),
            "mate_type": type_names[type_id], "mate_type_confidence": float(typed[type_id]),
            "mate_type_probabilities": {name: float(typed[i]) for i, name in enumerate(type_names)},
        })
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps({"output": str(args.output), "face_a": face_a, "face_b": face_b,
                      "local_mcfs_a": len(local_a), "local_mcfs_b": len(local_b),
                      "pairs": pair_count, "top_k": keep, "seconds": result["elapsed_seconds"]}))


if __name__ == "__main__": main()
