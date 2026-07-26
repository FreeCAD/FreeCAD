"""Rank AutoMate MCF pairs for two STEP parts and write Top-K JSON."""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import MateModelConfig, MatePairModel, Part, PartFeatures, PartOptions, flatbatch, part_to_graph


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("part_a", type=Path)
    parser.add_argument("part_b", type=Path)
    parser.add_argument(
        "--checkpoint", type=Path, default=ROOT / "runs/mate_multitask_v2/best.pt"
    )
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--top-k", type=int, default=10)
    parser.add_argument("--chunk-size", type=int, default=65536)
    parser.add_argument("--max-pairs", type=int, default=50_000_000)
    parser.add_argument("--device", default="auto", choices=("auto", "cpu", "cuda"))
    return parser.parse_args()


def build_graph(path):
    part_options = PartOptions()
    part_options.tesselate = False
    part_options.num_uv_samples = 0
    part_options.num_random_samples = 0
    part_options.num_sdf_samples = 0
    part_options.default_mcfs = True
    part_options.collect_inferences = False
    graph_options = PartFeatures()
    graph_options.mesh = False
    graph_options.mesh_to_topology = False
    graph_options.samples = False
    graph_options.face_samples = False
    graph_options.edge_samples = False
    graph_options.random_samples = False
    graph_options.uniform_samples = False
    part = Part(str(path.resolve()), part_options)
    if not part.is_valid:
        raise ValueError(f"AutoMate could not parse {path}")
    graph = part_to_graph(part, graph_options)
    valid = torch.isfinite(graph.mcfs).all(dim=1)
    graph.mcfs = graph.mcfs[valid]
    graph.mcf_refs = graph.mcf_refs[:, valid]
    if graph.mcfs.shape[0] == 0:
        raise ValueError(f"No valid MCF candidates in {path}")
    return graph


def update_topk(best_scores, best_flat_indices, scores, flat_indices, top_k):
    scores = torch.cat((best_scores, scores))
    flat_indices = torch.cat((best_flat_indices, flat_indices))
    keep = min(top_k, scores.numel())
    values, positions = torch.topk(scores, keep)
    return values, flat_indices[positions]


def mcf_json(graph, index):
    values = graph.mcfs[index].tolist()
    return {"index": index, "axis": values[:3], "origin": values[3:]}


def main():
    args = parse_args()
    started = time.perf_counter()
    device = torch.device(
        "cuda" if args.device == "auto" and torch.cuda.is_available() else
        "cpu" if args.device == "auto" else args.device
    )
    graph_a = build_graph(args.part_a)
    graph_b = build_graph(args.part_b)
    count_a, count_b = graph_a.mcfs.shape[0], graph_b.mcfs.shape[0]
    pair_count = count_a * count_b
    if pair_count > args.max_pairs:
        raise ValueError(
            f"Candidate product {count_a} x {count_b} = {pair_count:,} exceeds "
            f"--max-pairs {args.max_pairs:,}; increase it explicitly or simplify the parts"
        )

    checkpoint = torch.load(args.checkpoint, map_location=device, weights_only=False)
    model = MatePairModel(MateModelConfig(**checkpoint["model_config"]))
    model.load_state_dict(checkpoint["model_state"])
    model.to(device).eval()
    batch_a = flatbatch([graph_a]).to(device)
    batch_b = flatbatch([graph_b]).to(device)
    best_scores = torch.empty(0, device=device)
    best_indices = torch.empty(0, dtype=torch.long, device=device)
    with torch.no_grad():
        encoded_a = model.encode_graph(batch_a)
        encoded_b = model.encode_graph(batch_b)
        for start in range(0, pair_count, args.chunk_size):
            stop = min(start + args.chunk_size, pair_count)
            flat = torch.arange(start, stop, device=device)
            pairs = torch.stack((flat // count_b, flat % count_b), dim=1)
            scores = model.score_encoded_pairs(encoded_a, encoded_b, pairs)
            best_scores, best_indices = update_topk(
                best_scores, best_indices, scores, flat, args.top_k
            )

        best_pairs = torch.stack(
            (best_indices // count_b, best_indices % count_b), dim=1
        )
        best_output = model.predict_encoded_pairs(encoded_a, encoded_b, best_pairs)
        best_scores = best_output.pair_logits
        type_probabilities = (
            torch.softmax(best_output.type_logits, dim=1)
            if best_output.type_logits is not None else None
        )

    type_names = [
        "BALL", "CYLINDRICAL", "FASTENED", "PARALLEL",
        "PIN_SLOT", "PLANAR", "REVOLUTE", "SLIDER",
    ]

    result = {
        "schema_version": 1,
        "checkpoint": str(args.checkpoint.resolve()),
        "checkpoint_epoch": int(checkpoint["epoch"]),
        "device": str(device),
        "elapsed_seconds": time.perf_counter() - started,
        "parts": [
            {"path": str(args.part_a.resolve()), "mcf_count": count_a},
            {"path": str(args.part_b.resolve()), "mcf_count": count_b},
        ],
        "pair_count": pair_count,
        "recommendations": [],
    }
    for rank, (score, flat_index) in enumerate(
        zip(best_scores.cpu().tolist(), best_indices.cpu().tolist()), 1
    ):
        index_a, index_b = divmod(flat_index, count_b)
        recommendation = {
            "rank": rank,
            "score": score,
            "probability": torch.sigmoid(torch.tensor(score)).item(),
            "a": mcf_json(graph_a, index_a),
            "b": mcf_json(graph_b, index_b),
        }
        if type_probabilities is not None:
            probabilities = type_probabilities[rank - 1].cpu()
            type_index = int(probabilities.argmax())
            recommendation["mate_type"] = type_names[type_index]
            recommendation["mate_type_confidence"] = float(probabilities[type_index])
            recommendation["mate_type_probabilities"] = {
                name: float(probabilities[index])
                for index, name in enumerate(type_names)
            }
        result["recommendations"].append(recommendation)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps({
        "output": str(args.output), "mcfs_a": count_a, "mcfs_b": count_b,
        "pairs": pair_count, "top_k": len(result["recommendations"]),
        "elapsed_seconds": result["elapsed_seconds"],
    }))


if __name__ == "__main__":
    main()
