"""Train the independent paper-style face-conditioned MCF Location model."""

from __future__ import annotations

import argparse
import json
import math
import random
import sys
import time
from pathlib import Path

import numpy as np
import torch

ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import (
    LocationModel, LocationModelConfig, location_ranking_statistics,
    make_location_dataloader, multi_positive_location_loss,
)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--index-dir", type=Path, default=ROOT / "dataset/training/location_paper_full")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "runs/paper_location_full_e30")
    parser.add_argument("--epochs", type=int, default=30)
    parser.add_argument("--max-candidate-pairs", type=int, default=12000)
    parser.add_argument("--max-samples", type=int, default=32)
    parser.add_argument("--graph-width", type=int, default=64)
    parser.add_argument("--mcf-width", type=int, default=64)
    parser.add_argument("--message-passing-steps", type=int, default=6)
    parser.add_argument("--dropout", type=float, default=0.1)
    parser.add_argument("--learning-rate", type=float, default=1.0e-3)
    parser.add_argument("--weight-decay", type=float, default=0.0)
    parser.add_argument("--gradient-clip", type=float, default=1.0)
    parser.add_argument("--num-workers", type=int, default=0)
    parser.add_argument("--seed", type=int, default=20260803)
    parser.add_argument("--device", choices=("auto", "cpu", "cuda"), default="auto")
    parser.add_argument("--resume", type=Path)
    parser.add_argument("--max-train-batches", type=int)
    parser.add_argument("--max-val-batches", type=int)
    parser.add_argument("--stop-after-epoch", type=int)
    parser.add_argument("--detect-anomaly", action="store_true")
    return parser.parse_args()


def seed_everything(seed):
    random.seed(seed); np.random.seed(seed); torch.manual_seed(seed)
    if torch.cuda.is_available(): torch.cuda.manual_seed_all(seed)


def dataset_signature(dataset):
    return {
        "examples": int(dataset.summary["examples"]),
        "split_counts": dataset.summary["split_counts"],
        "cache_root": dataset.summary["cache_root"],
        "seed": int(dataset.summary["seed"]),
    }


def rng_state():
    state = {"python": random.getstate(), "numpy": np.random.get_state(),
             "torch": torch.get_rng_state()}
    if torch.cuda.is_available(): state["cuda"] = torch.cuda.get_rng_state_all()
    return state


def restore_rng_state(state):
    if not state: return
    random.setstate(state["python"]); np.random.set_state(state["numpy"])
    torch.set_rng_state(state["torch"].cpu())
    if state.get("cuda") is not None and torch.cuda.is_available():
        torch.cuda.set_rng_state_all([value.cpu() for value in state["cuda"]])


def finish_metrics(totals, loss_sum):
    samples = int(totals["samples"])
    if samples == 0: raise RuntimeError("No Location batches were processed")
    return {
        "loss": loss_sum / samples, "samples": samples,
        "candidate_pairs": int(totals["candidate_pairs"]),
        "positive_pairs": int(totals["positive_pairs"]),
        "mean_candidates": totals["candidate_pairs"] / samples,
        "top1": totals["top1"] / samples, "top5": totals["top5"] / samples,
        "mrr": totals["reciprocal_rank"] / samples,
        "mean_rank": totals["rank_sum"] / samples,
    }


def run_epoch(model, loader, device, optimizer=None, gradient_clip=1.0, max_batches=None):
    training = optimizer is not None
    model.train(training)
    totals = {"samples": 0, "top1": 0, "top5": 0, "reciprocal_rank": 0.0,
              "rank_sum": 0.0, "candidate_pairs": 0, "positive_pairs": 0}
    loss_sum = 0.0
    context = torch.enable_grad() if training else torch.no_grad()
    with context:
        for batch_index, batch in enumerate(loader):
            if max_batches is not None and batch_index >= max_batches: break
            batch = batch.to(device)
            if training: optimizer.zero_grad(set_to_none=True)
            logits = model(batch)
            if logits.shape != batch.pair_labels.shape:
                raise RuntimeError(f"Location logits shape mismatch: {tuple(logits.shape)}")
            loss = multi_positive_location_loss(logits, batch.pair_labels, batch.sample_pair_offsets)
            if not torch.isfinite(loss):
                invalid_pairs = ~torch.isfinite(logits)
                invalid_samples = torch.unique(batch.pair_to_sample[invalid_pairs]).tolist()
                affected = [batch.sample_ids[index] for index in invalid_samples]
                raise FloatingPointError(
                    f"Non-finite Location loss at batch {batch_index}; "
                    f"invalid_logits={int(invalid_pairs.sum())}; affected_samples="
                    + ", ".join(affected)
                )
            if training:
                loss.backward()
                invalid = [name for name, parameter in model.named_parameters()
                           if parameter.grad is not None and not torch.isfinite(parameter.grad).all()]
                if invalid:
                    raise FloatingPointError("Non-finite gradients in " + ", ".join(invalid[:20]))
                norm = torch.nn.utils.clip_grad_norm_(model.parameters(), gradient_clip, error_if_nonfinite=True)
                optimizer.step()
                if not torch.isfinite(norm): raise FloatingPointError("Non-finite gradient norm")
                invalid_parameters = [name for name, parameter in model.named_parameters()
                                      if not torch.isfinite(parameter).all()]
                if invalid_parameters:
                    raise FloatingPointError(
                        f"Non-finite parameters after batch {batch_index}: "
                        + ", ".join(invalid_parameters[:20])
                    )
            stats = location_ranking_statistics(
                logits.detach(), batch.pair_labels, batch.sample_pair_offsets
            )
            for key in totals: totals[key] += stats[key]
            loss_sum += float(loss.detach()) * stats["samples"]
    return finish_metrics(totals, loss_sum)


def atomic_torch_save(payload, path):
    temporary = path.with_suffix(path.suffix + ".tmp")
    torch.save(payload, temporary); temporary.replace(path)


def save_checkpoint(path, model, optimizer, scheduler, epoch, best_mrr, args, signature):
    atomic_torch_save({
        "schema_version": 1, "task": "mcf_location", "epoch": epoch,
        "best_validation_mrr": best_mrr, "model_config": model.config.to_dict(),
        "model_state": model.state_dict(), "optimizer_state": optimizer.state_dict(),
        "scheduler_state": scheduler.state_dict(), "dataset_signature": signature,
        "arguments": vars(args), "rng_state": rng_state(),
    }, path)


def main():
    args = parse_args()
    if args.epochs <= 0 or args.max_candidate_pairs <= 0 or args.max_samples <= 0:
        raise ValueError("epochs and batch budgets must be positive")
    if args.message_passing_steps != 6: raise ValueError("The paper configuration requires 6 layers")
    seed_everything(args.seed); torch.autograd.set_detect_anomaly(args.detect_anomaly)
    device = torch.device("cuda" if args.device == "auto" and torch.cuda.is_available()
                          else "cpu" if args.device == "auto" else args.device)
    if device.type == "cuda" and not torch.cuda.is_available(): raise RuntimeError("CUDA unavailable")
    if (args.output_dir / "FROZEN.json").is_file(): raise RuntimeError("Refusing to modify frozen run")
    if not args.resume and any((args.output_dir / name).exists() for name in ("best.pt", "last.pt", "metrics.jsonl")):
        raise RuntimeError("Output directory contains a run; use --resume or another directory")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    kwargs = {"pin_memory": device.type == "cuda", "persistent_workers": args.num_workers > 0}
    train_dataset, train_loader = make_location_dataloader(
        args.index_dir, "train", args.max_candidate_pairs, args.max_samples,
        True, args.num_workers, args.seed, **kwargs)
    validation_dataset, validation_loader = make_location_dataloader(
        args.index_dir, "validation", args.max_candidate_pairs, args.max_samples,
        False, args.num_workers, args.seed, **kwargs)
    signature = dataset_signature(train_dataset)
    if dataset_signature(validation_dataset) != signature: raise ValueError("Dataset summaries differ")
    example = train_dataset[0]
    checkpoint = torch.load(args.resume, map_location=device, weights_only=False) if args.resume else None
    if checkpoint:
        if checkpoint.get("task") != "mcf_location": raise ValueError("Not a Location checkpoint")
        if checkpoint.get("dataset_signature") != signature: raise ValueError("Checkpoint dataset differs")
        config = LocationModelConfig(**checkpoint["model_config"])
    else:
        config = LocationModelConfig(
            face_width=example.graph_a.faces.shape[1], loop_width=example.graph_a.loops.shape[1],
            edge_width=example.graph_a.edges.shape[1], vertex_width=example.graph_a.vertices.shape[1],
            graph_width=args.graph_width, mcf_width=args.mcf_width,
            message_passing_steps=args.message_passing_steps, dropout=args.dropout,
        )
    model = LocationModel(config).to(device)
    optimizer = torch.optim.Adam(model.parameters(), lr=args.learning_rate, weight_decay=args.weight_decay)
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=args.epochs)
    start_epoch, best_mrr = 0, -math.inf
    if checkpoint:
        model.load_state_dict(checkpoint["model_state"], strict=True)
        optimizer.load_state_dict(checkpoint["optimizer_state"])
        scheduler.load_state_dict(checkpoint["scheduler_state"])
        start_epoch = int(checkpoint["epoch"]) + 1
        best_mrr = float(checkpoint.get("best_validation_mrr", -math.inf))
        restore_rng_state(checkpoint.get("rng_state"))
    config_record = {
        "task": "mcf_location", "device": str(device), "torch": torch.__version__,
        "cuda_runtime": torch.version.cuda, "model_config": config.to_dict(),
        "dataset_signature": signature, "train_samples": len(train_dataset),
        "validation_samples": len(validation_dataset),
        "arguments": {k: str(v) if isinstance(v, Path) else v for k, v in vars(args).items()},
    }
    (args.output_dir / "config.json").write_text(json.dumps(config_record, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(config_record, ensure_ascii=False))
    history = args.output_dir / "metrics.jsonl"
    for epoch in range(start_epoch, args.epochs):
        train_loader.batch_sampler.set_epoch(epoch)
        started = time.time()
        if device.type == "cuda": torch.cuda.reset_peak_memory_stats(device)
        train_metrics = run_epoch(model, train_loader, device, optimizer, args.gradient_clip, args.max_train_batches)
        validation_metrics = run_epoch(model, validation_loader, device, max_batches=args.max_val_batches)
        scheduler.step()
        if device.type == "cuda": torch.cuda.synchronize(device)
        record = {"epoch": epoch, "seconds": time.time() - started,
                  "learning_rate": optimizer.param_groups[0]["lr"],
                  "peak_gpu_memory_gib": torch.cuda.max_memory_allocated(device) / 1024**3 if device.type == "cuda" else None,
                  "peak_gpu_memory_reserved_gib": torch.cuda.max_memory_reserved(device) / 1024**3 if device.type == "cuda" else None,
                  "train": train_metrics, "validation": validation_metrics,
                  "validation_score": validation_metrics["mrr"]}
        with history.open("a", encoding="utf-8") as stream: stream.write(json.dumps(record) + "\n")
        print(json.dumps(record), flush=True)
        if validation_metrics["mrr"] > best_mrr:
            best_mrr = validation_metrics["mrr"]
            save_checkpoint(args.output_dir / "best.pt", model, optimizer, scheduler, epoch, best_mrr, args, signature)
        save_checkpoint(args.output_dir / "last.pt", model, optimizer, scheduler, epoch, best_mrr, args, signature)
        if args.stop_after_epoch is not None and epoch >= args.stop_after_epoch:
            print(f"STOP_AFTER_EPOCH={epoch}", flush=True); break


if __name__ == "__main__": main()
