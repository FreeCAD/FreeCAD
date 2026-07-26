"""Train shared SB-GCN location ranking and mate-type classification heads."""

from __future__ import annotations

import argparse
import json
import math
import random
import sys
import time
from collections import defaultdict
from pathlib import Path

import numpy as np
import torch
from torch.nn import functional as F


ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import MateModelConfig, MatePairModel, make_mate_dataloader


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--index-dir", type=Path, default=ROOT / "dataset/training/index_v1")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "runs/mate_multitask_v2")
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--negative-count", type=int, default=15)
    parser.add_argument("--graph-width", type=int, default=64)
    parser.add_argument("--mcf-width", type=int, default=64)
    parser.add_argument("--message-passing-steps", type=int, default=2)
    parser.add_argument("--dropout", type=float, default=0.1)
    parser.add_argument("--num-mate-types", type=int, default=8)
    parser.add_argument("--type-loss-weight", type=float, default=1.0)
    parser.add_argument("--learning-rate", type=float, default=3e-4)
    parser.add_argument("--weight-decay", type=float, default=1e-4)
    parser.add_argument("--num-workers", type=int, default=0)
    parser.add_argument("--seed", type=int, default=20260725)
    parser.add_argument("--device", default="auto", choices=("auto", "cpu", "cuda"))
    parser.add_argument("--resume", type=Path)
    parser.add_argument("--max-train-batches", type=int)
    parser.add_argument("--max-val-batches", type=int)
    parser.add_argument(
        "--detect-anomaly",
        action="store_true",
        help="Enable PyTorch autograd anomaly tracing for numerical debugging.",
    )
    return parser.parse_args()


def seed_everything(seed):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def ranking_statistics(logits, batch):
    top1 = 0
    reciprocal_rank = 0.0
    samples = len(batch.sample_ids)
    for sample_index in range(samples):
        mask = batch.pair_to_sample == sample_index
        sample_logits = logits[mask]
        sample_labels = batch.pair_labels[mask]
        order = torch.argsort(sample_logits, descending=True)
        ranked_labels = sample_labels[order]
        rank = int(torch.nonzero(ranked_labels > 0.5, as_tuple=False)[0]) + 1
        top1 += int(rank == 1)
        reciprocal_rank += 1.0 / rank
    return top1, reciprocal_rank, samples


def type_statistics(confusion):
    support = confusion.sum(dim=1)
    predicted = confusion.sum(dim=0)
    true_positive = confusion.diag()
    recall = true_positive / support.clamp_min(1)
    precision = true_positive / predicted.clamp_min(1)
    f1 = 2 * precision * recall / (precision + recall).clamp_min(1e-12)
    present = support > 0
    return {
        "accuracy": float(true_positive.sum() / confusion.sum().clamp_min(1)),
        "balanced_accuracy_supported": float(recall[present].mean()),
        "macro_f1_supported": float(f1[present].mean()),
        "macro_f1_all": float(f1.mean()),
        "support": support.long().tolist(),
        "confusion_matrix": confusion.long().tolist(),
    }


def run_epoch(
    model,
    loader,
    device,
    optimizer=None,
    pos_weight=1.0,
    class_weights=None,
    type_loss_weight=1.0,
    max_batches=None,
):
    training = optimizer is not None
    model.train(training)
    totals = defaultdict(float)
    type_confusion = torch.zeros(
        (model.config.num_mate_types, model.config.num_mate_types), dtype=torch.long
    )
    context = torch.enable_grad() if training else torch.no_grad()
    with context:
        for batch_index, batch in enumerate(loader):
            if max_batches is not None and batch_index >= max_batches:
                break
            batch = batch.to(device)
            if training:
                optimizer.zero_grad(set_to_none=True)
            output = model.forward_multitask(batch)
            logits = output.pair_logits
            location_loss = F.binary_cross_entropy_with_logits(
                output.pair_logits,
                batch.pair_labels,
                pos_weight=torch.as_tensor(pos_weight, device=device),
            )
            type_loss = None
            if output.type_logits is not None:
                positive_type_logits = output.type_logits[batch.positive_pair_indices]
                type_loss = F.cross_entropy(
                    positive_type_logits,
                    batch.mate_types,
                    weight=class_weights,
                )
                loss = location_loss + type_loss_weight * type_loss
            else:
                loss = location_loss
            if not torch.isfinite(loss):
                raise FloatingPointError(
                    "non-finite loss for samples: " + ", ".join(batch.sample_ids)
                )
            if training:
                loss.backward()
                invalid_gradients = [
                    name
                    for name, parameter in model.named_parameters()
                    if parameter.grad is not None
                    and not torch.isfinite(parameter.grad).all()
                ]
                if invalid_gradients:
                    raise FloatingPointError(
                        "non-finite gradients in "
                        + ", ".join(invalid_gradients[:20])
                        + "; samples: "
                        + ", ".join(batch.sample_ids)
                    )
                gradient_norm = torch.nn.utils.clip_grad_norm_(
                    model.parameters(), 5.0, error_if_nonfinite=True
                )
                optimizer.step()
                if not torch.isfinite(gradient_norm):
                    raise FloatingPointError(
                        "non-finite gradient for samples: " + ", ".join(batch.sample_ids)
                    )

            predictions = logits >= 0
            labels = batch.pair_labels > 0.5
            top1, reciprocal_rank, samples = ranking_statistics(logits.detach(), batch)
            totals["loss_sum"] += float(loss.detach()) * samples
            totals["location_loss_sum"] += float(location_loss.detach()) * labels.numel()
            totals["pairs"] += labels.numel()
            totals["correct"] += int((predictions == labels).sum())
            totals["tp"] += int((predictions & labels).sum())
            totals["fp"] += int((predictions & ~labels).sum())
            totals["fn"] += int((~predictions & labels).sum())
            totals["top1"] += top1
            totals["rr"] += reciprocal_rank
            totals["samples"] += samples
            if type_loss is not None:
                totals["type_loss_sum"] += float(type_loss.detach()) * samples
                type_predictions = positive_type_logits.argmax(dim=1)
                encoded = (
                    batch.mate_types.detach().cpu() * model.config.num_mate_types
                    + type_predictions.detach().cpu()
                )
                type_confusion += torch.bincount(
                    encoded, minlength=model.config.num_mate_types ** 2
                ).reshape(model.config.num_mate_types, model.config.num_mate_types)

    if totals["pairs"] == 0:
        raise RuntimeError("No batches were processed")
    precision = totals["tp"] / max(totals["tp"] + totals["fp"], 1)
    recall = totals["tp"] / max(totals["tp"] + totals["fn"], 1)
    result = {
        "loss": totals["loss_sum"] / totals["samples"],
        "location": {
            "loss": totals["location_loss_sum"] / totals["pairs"],
            "accuracy": totals["correct"] / totals["pairs"],
            "precision": precision,
            "recall": recall,
            "top1": totals["top1"] / totals["samples"],
            "mrr": totals["rr"] / totals["samples"],
            "samples": int(totals["samples"]),
            "pairs": int(totals["pairs"]),
        },
    }
    if model.config.num_mate_types > 0:
        result["type"] = {
            "loss": totals["type_loss_sum"] / totals["samples"],
            **type_statistics(type_confusion),
        }
    return result


def save_checkpoint(path, model, optimizer, scheduler, epoch, best_score, args):
    torch.save(
        {
            "epoch": epoch,
            "best_validation_score": best_score,
            "model_config": model.config.to_dict(),
            "model_state": model.state_dict(),
            "optimizer_state": optimizer.state_dict(),
            "scheduler_state": scheduler.state_dict(),
            "arguments": vars(args),
        },
        path,
    )


def main():
    args = parse_args()
    seed_everything(args.seed)
    torch.autograd.set_detect_anomaly(args.detect_anomaly)
    device = torch.device(
        "cuda" if args.device == "auto" and torch.cuda.is_available() else
        "cpu" if args.device == "auto" else args.device
    )
    if device.type == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("CUDA was requested but is unavailable")
    frozen_marker = args.output_dir / "FROZEN.json"
    if frozen_marker.is_file():
        raise RuntimeError(
            f"Refusing to write into frozen run directory: {args.output_dir}. "
            "Choose a new --output-dir."
        )
    args.output_dir.mkdir(parents=True, exist_ok=True)

    train_dataset, train_loader = make_mate_dataloader(
        args.index_dir, "train", args.batch_size, True, args.num_workers,
        args.negative_count, args.seed, pin_memory=device.type == "cuda",
    )
    validation_dataset, validation_loader = make_mate_dataloader(
        args.index_dir, "validation", args.batch_size, False, args.num_workers,
        args.negative_count, args.seed, pin_memory=device.type == "cuda",
    )
    example = train_dataset[0]
    checkpoint = None
    if args.resume:
        checkpoint = torch.load(args.resume, map_location=device, weights_only=False)
        config = MateModelConfig(**checkpoint["model_config"])
    else:
        config = MateModelConfig(
            face_width=example.graph_a.faces.shape[1],
            loop_width=example.graph_a.loops.shape[1],
            edge_width=example.graph_a.edges.shape[1],
            vertex_width=example.graph_a.vertices.shape[1],
            graph_width=args.graph_width,
            mcf_width=args.mcf_width,
            message_passing_steps=args.message_passing_steps,
            dropout=args.dropout,
            num_mate_types=args.num_mate_types,
            normalize_graph_inputs=args.num_mate_types > 0,
        )
    model = MatePairModel(config).to(device)
    optimizer = torch.optim.AdamW(
        model.parameters(), lr=args.learning_rate, weight_decay=args.weight_decay
    )
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=args.epochs)
    start_epoch = 0
    best_score = -math.inf
    if checkpoint is not None:
        model.load_state_dict(checkpoint["model_state"])
        optimizer.load_state_dict(checkpoint["optimizer_state"])
        scheduler.load_state_dict(checkpoint["scheduler_state"])
        start_epoch = int(checkpoint["epoch"]) + 1
        best_score = float(
            checkpoint.get(
                "best_validation_score", checkpoint.get("best_validation_mrr", -math.inf)
            )
        )

    print(json.dumps({"device": str(device), "model_config": config.to_dict()}, ensure_ascii=False))
    history_path = args.output_dir / "metrics.jsonl"
    for epoch in range(start_epoch, args.epochs):
        started = time.time()
        train_dataset.set_epoch(epoch)
        train_metrics = run_epoch(
            model,
            train_loader,
            device,
            optimizer=optimizer,
            pos_weight=args.negative_count,
            class_weights=train_dataset.class_weights.to(device),
            type_loss_weight=args.type_loss_weight,
            max_batches=args.max_train_batches,
        )
        validation_metrics = run_epoch(
            model,
            validation_loader,
            device,
            pos_weight=args.negative_count,
            class_weights=validation_dataset.class_weights.to(device),
            type_loss_weight=args.type_loss_weight,
            max_batches=args.max_val_batches,
        )
        scheduler.step()
        record = {
            "epoch": epoch,
            "seconds": time.time() - started,
            "learning_rate": optimizer.param_groups[0]["lr"],
            "train": train_metrics,
            "validation": validation_metrics,
        }
        validation_score = validation_metrics["location"]["mrr"]
        if "type" in validation_metrics:
            validation_score = 0.5 * (
                validation_score + validation_metrics["type"]["macro_f1_supported"]
            )
        record["validation_score"] = validation_score
        with history_path.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(record, ensure_ascii=False) + "\n")
        print(json.dumps(record, ensure_ascii=False))
        if validation_score > best_score:
            best_score = validation_score
            save_checkpoint(
                args.output_dir / "best.pt", model, optimizer, scheduler, epoch, best_score, args
            )
        save_checkpoint(
            args.output_dir / "last.pt", model, optimizer, scheduler, epoch, best_score, args
        )


if __name__ == "__main__":
    main()
