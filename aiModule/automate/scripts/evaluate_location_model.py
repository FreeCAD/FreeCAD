"""Evaluate an independent MCF Location checkpoint on one no-leak split."""

from __future__ import annotations

import argparse
import collections
import json
import sys
import time
from pathlib import Path

import torch

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import (
    LocationModel, LocationModelConfig, make_location_dataloader,
    multi_positive_location_loss,
)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", type=Path, default=ROOT / "runs/paper_location_full_e30/best.pt")
    parser.add_argument("--index-dir", type=Path, default=ROOT / "dataset/training/location_paper_full")
    parser.add_argument("--split", choices=("train", "validation", "test"), default="test")
    parser.add_argument("--max-candidate-pairs", type=int, default=24000)
    parser.add_argument("--max-samples", type=int, default=32)
    parser.add_argument("--num-workers", type=int, default=0)
    parser.add_argument("--device", choices=("auto", "cpu", "cuda"), default="auto")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--progress-every", type=int, default=500)
    return parser.parse_args()


def signature(dataset):
    return {
        "examples": int(dataset.summary["examples"]),
        "split_counts": dataset.summary["split_counts"],
        "cache_root": dataset.summary["cache_root"],
        "seed": int(dataset.summary["seed"]),
    }


def bucket_name(candidate_count):
    if candidate_count <= 100: return "00001-00100"
    if candidate_count <= 500: return "00101-00500"
    if candidate_count <= 1000: return "00501-01000"
    if candidate_count <= 5000: return "01001-05000"
    return "05001-10000"


def new_bucket():
    return collections.defaultdict(float)


def update_bucket(bucket, logits, labels, loss):
    order = torch.argsort(logits, descending=True)
    rank = int(torch.nonzero(labels[order].bool(), as_tuple=False)[0]) + 1
    bucket["samples"] += 1
    bucket["loss_sum"] += float(loss)
    bucket["top1"] += int(rank <= 1)
    bucket["top5"] += int(rank <= 5)
    bucket["reciprocal_rank"] += 1.0 / rank
    bucket["rank_sum"] += rank
    bucket["candidate_pairs"] += labels.numel()
    bucket["positive_pairs"] += int(labels.sum())


def finish(bucket):
    samples = int(bucket["samples"])
    if samples == 0: return None
    return {
        "samples": samples,
        "loss": bucket["loss_sum"] / samples,
        "top1": bucket["top1"] / samples,
        "top5": bucket["top5"] / samples,
        "mrr": bucket["reciprocal_rank"] / samples,
        "mean_rank": bucket["rank_sum"] / samples,
        "candidate_pairs": int(bucket["candidate_pairs"]),
        "positive_pairs": int(bucket["positive_pairs"]),
        "mean_candidates": bucket["candidate_pairs"] / samples,
    }


def main():
    args = parse_args()
    device = torch.device("cuda" if args.device == "auto" and torch.cuda.is_available()
                          else "cpu" if args.device == "auto" else args.device)
    if device.type == "cuda" and not torch.cuda.is_available(): raise RuntimeError("CUDA unavailable")
    checkpoint = torch.load(args.checkpoint, map_location=device, weights_only=False)
    if checkpoint.get("task") != "mcf_location": raise ValueError("Not a Location checkpoint")
    dataset, loader = make_location_dataloader(
        args.index_dir, args.split, args.max_candidate_pairs, args.max_samples,
        False, args.num_workers, graph_cache_size=32,
        pin_memory=device.type == "cuda", persistent_workers=args.num_workers > 0,
    )
    if checkpoint.get("dataset_signature") != signature(dataset):
        raise ValueError("Checkpoint and evaluation dataset signatures differ")
    model = LocationModel(LocationModelConfig(**checkpoint["model_config"]))
    model.load_state_dict(checkpoint["model_state"], strict=True)
    model.to(device).eval()
    overall = new_bucket()
    by_type = collections.defaultdict(new_bucket)
    by_candidates = collections.defaultdict(new_bucket)
    started = time.time()
    processed_batches = 0
    with torch.no_grad():
        for batch_index, batch in enumerate(loader, 1):
            batch = batch.to(device)
            logits = model(batch)
            losses = multi_positive_location_loss(
                logits, batch.pair_labels, batch.sample_pair_offsets, reduction="none"
            )
            if not bool(torch.isfinite(logits).all()) or not bool(torch.isfinite(losses).all()):
                raise FloatingPointError(f"Non-finite evaluation output at batch {batch_index}")
            for sample_index, mate_type in enumerate(batch.mate_types):
                start = int(batch.sample_pair_offsets[sample_index])
                end = int(batch.sample_pair_offsets[sample_index + 1])
                sample_logits, labels = logits[start:end], batch.pair_labels[start:end]
                update_bucket(overall, sample_logits, labels, losses[sample_index])
                update_bucket(by_type[mate_type], sample_logits, labels, losses[sample_index])
                update_bucket(by_candidates[bucket_name(end - start)], sample_logits, labels, losses[sample_index])
            processed_batches = batch_index
            if args.progress_every and batch_index % args.progress_every == 0:
                print(f"batches={batch_index} samples={int(overall['samples'])} elapsed={time.time()-started:.1f}s", flush=True)
    if device.type == "cuda": torch.cuda.synchronize(device)
    result = {
        "schema_version": 1, "task": "mcf_location_evaluation",
        "checkpoint": str(args.checkpoint.resolve()), "checkpoint_epoch": int(checkpoint["epoch"]),
        "checkpoint_best_validation_mrr": float(checkpoint["best_validation_mrr"]),
        "split": args.split, "device": str(device), "seconds": time.time() - started,
        "batches": processed_batches, "overall": finish(overall),
        "by_mate_type": {key: finish(value) for key, value in sorted(by_type.items())},
        "by_candidate_count": {key: finish(value) for key, value in sorted(by_candidates.items())},
    }
    output = args.output or args.checkpoint.parent / f"evaluation_{args.split}.json"
    if output.exists(): raise RuntimeError(f"Refusing to overwrite evaluation: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(output.suffix + ".tmp")
    temporary.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    temporary.replace(output)
    print(json.dumps(result, ensure_ascii=False, indent=2))
    print(f"evaluation={output}")


if __name__ == "__main__": main()
