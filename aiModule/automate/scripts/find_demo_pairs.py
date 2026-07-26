"""Find test-set part pairs whose known mate ranks well in exhaustive inference."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import torch


ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import MateModelConfig, MatePairModel, flatbatch


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", type=Path, default=ROOT / "runs/mate_pair_v1/best.pt")
    parser.add_argument("--index-dir", type=Path, default=ROOT / "dataset/training/index_v1")
    parser.add_argument("--mate-type", default="FASTENED")
    parser.add_argument("--split", default="test", choices=("train", "validation", "test"))
    parser.add_argument("--inspect", type=int, default=30)
    parser.add_argument("--max-pairs", type=int, default=5_000_000)
    parser.add_argument("--chunk-size", type=int, default=65536)
    parser.add_argument("--top", type=int, default=10)
    parser.add_argument("--output", type=Path, default=ROOT / "runs/mate_pair_v1/demo_pairs.json")
    return parser.parse_args()


def load_graph(cache_root, relative_path):
    payload = torch.load(cache_root / relative_path, map_location="cpu", weights_only=False)
    return payload["graph"]


def main():
    args = parse_args()
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    summary = json.loads((args.index_dir / "summary.json").read_text(encoding="utf-8"))
    cache_root = Path(summary["cache_root"])
    rows = [
        json.loads(line)
        for line in (args.index_dir / f"{args.split}.jsonl").read_text(encoding="utf-8").splitlines()
    ]
    candidates = []
    for row in rows:
        if row["mate_type"] != args.mate_type:
            continue
        graph_a = load_graph(cache_root, row["sides"][0]["cache"])
        graph_b = load_graph(cache_root, row["sides"][1]["cache"])
        pair_count = graph_a.mcfs.shape[0] * graph_b.mcfs.shape[0]
        if pair_count <= args.max_pairs:
            candidates.append((pair_count, row, graph_a, graph_b))
    candidates.sort(key=lambda item: item[0])

    checkpoint = torch.load(args.checkpoint, map_location=device, weights_only=False)
    model = MatePairModel(MateModelConfig(**checkpoint["model_config"]))
    model.load_state_dict(checkpoint["model_state"])
    model.to(device).eval()
    type_names = [
        name for name, _ in sorted(
            summary["mate_type_to_id"].items(), key=lambda item: item[1]
        )
    ]
    results = []
    with torch.no_grad():
        for pair_count, row, graph_a, graph_b in candidates[: args.inspect]:
            count_b = graph_b.mcfs.shape[0]
            positive_a = int(row["sides"][0]["candidate_index"])
            positive_b = int(row["sides"][1]["candidate_index"])
            positive_flat = positive_a * count_b + positive_b
            encoded_a = model.encode_graph(flatbatch([graph_a]).to(device))
            encoded_b = model.encode_graph(flatbatch([graph_b]).to(device))
            positive_pair = torch.tensor([[positive_a, positive_b]], device=device)
            positive_output = model.predict_encoded_pairs(
                encoded_a, encoded_b, positive_pair
            )
            positive_score = float(positive_output.pair_logits[0])
            predicted_type = None
            type_confidence = None
            if positive_output.type_logits is not None:
                probabilities = torch.softmax(positive_output.type_logits[0], dim=0)
                predicted_type_id = int(probabilities.argmax())
                predicted_type = type_names[predicted_type_id]
                type_confidence = float(probabilities[predicted_type_id])
            greater = 0
            for start in range(0, pair_count, args.chunk_size):
                stop = min(start + args.chunk_size, pair_count)
                flat = torch.arange(start, stop, device=device)
                pairs = torch.stack((flat // count_b, flat % count_b), dim=1)
                scores = model.score_encoded_pairs(encoded_a, encoded_b, pairs)
                greater += int((scores > positive_score).sum())
            results.append({
                "sample_id": row["sample_id"],
                "mate_type": row["mate_type"],
                "rank": greater + 1,
                "positive_score": positive_score,
                "predicted_mate_type": predicted_type,
                "mate_type_confidence": type_confidence,
                "pair_count": pair_count,
                "mcf_counts": [graph_a.mcfs.shape[0], graph_b.mcfs.shape[0]],
                "part_ids": [row["sides"][0]["part_id"], row["sides"][1]["part_id"]],
                "step_paths": [
                    str((ROOT / "dataset/step" / f"{row['sides'][0]['part_id']}.step").resolve()),
                    str((ROOT / "dataset/step" / f"{row['sides'][1]['part_id']}.step").resolve()),
                ],
                "candidate_indices": [positive_a, positive_b],
            })
            print(
                f"rank={greater + 1} predicted={predicted_type} "
                f"pairs={pair_count} sample={row['sample_id']}", flush=True
            )
    results.sort(key=lambda item: (
        item["predicted_mate_type"] != args.mate_type,
        item["rank"],
        -float(item["mate_type_confidence"] or 0.0),
        item["pair_count"],
    ))
    output = {
        "checkpoint_epoch": checkpoint["epoch"],
        "split": args.split,
        "results": results[: args.top],
    }
    args.output.write_text(json.dumps(output, indent=2), encoding="utf-8")
    print(json.dumps(output, indent=2))
    print(f"demo_pairs_written={args.output}")


if __name__ == "__main__":
    main()
