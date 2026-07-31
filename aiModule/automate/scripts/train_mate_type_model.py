"""Train the paper-style independent SB-GCN Mate Type classifier."""

from __future__ import annotations

import argparse
import json
import math
import random
import sys
import time
from collections import Counter
from pathlib import Path

import numpy as np
import torch
from torch.nn import functional as F


ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import MateTypeModel, MateTypeModelConfig, make_mate_type_dataloader


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--index-dir",
        type=Path,
        default=ROOT / "dataset/training/mate_type_paper_10000",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=ROOT / "runs/paper_mate_type_10000_e50",
    )
    parser.add_argument("--epochs", type=int, default=50)
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--graph-width", type=int, default=64)
    parser.add_argument("--mcf-width", type=int, default=64)
    parser.add_argument(
        "--message-passing-steps",
        type=int,
        default=6,
        help="Number of inner Face-to-Face SB-GCN layers; the paper uses 6.",
    )
    parser.add_argument("--dropout", type=float, default=0.1)
    parser.add_argument("--learning-rate", type=float, default=1.0e-3)
    parser.add_argument("--weight-decay", type=float, default=0.0)
    parser.add_argument("--gradient-clip", type=float, default=1.0)
    parser.add_argument("--num-workers", type=int, default=0)
    parser.add_argument("--seed", type=int, default=20260730)
    parser.add_argument("--device", default="auto", choices=("auto", "cpu", "cuda"))
    parser.add_argument("--resume", type=Path)
    parser.add_argument("--max-train-batches", type=int)
    parser.add_argument("--max-val-batches", type=int)
    parser.add_argument("--detect-anomaly", action="store_true")
    return parser.parse_args()


def seed_everything(seed):
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def class_weights_from_rows(rows, num_classes):
    counts = Counter(int(row["mate_type_id"]) for row in rows)
    missing = [class_id for class_id in range(num_classes) if counts[class_id] == 0]
    if missing:
        raise ValueError(f"Training split is missing Mate Type IDs: {missing}")
    total = len(rows)
    return torch.tensor(
        [total / (num_classes * counts[class_id]) for class_id in range(num_classes)],
        dtype=torch.float32,
    )


def classification_statistics(confusion):
    support = confusion.sum(dim=1)
    predicted = confusion.sum(dim=0)
    true_positive = confusion.diag()
    recall = true_positive / support.clamp_min(1)
    precision = true_positive / predicted.clamp_min(1)
    f1 = 2 * precision * recall / (precision + recall).clamp_min(1.0e-12)
    present = support > 0
    total = confusion.sum().clamp_min(1)
    return {
        "accuracy": float(true_positive.sum() / total),
        "balanced_accuracy_supported": float(recall[present].mean()),
        "macro_f1_supported": float(f1[present].mean()),
        "macro_f1_all": float(f1.mean()),
        "precision": precision.tolist(),
        "recall": recall.tolist(),
        "f1": f1.tolist(),
        "support": support.long().tolist(),
        "confusion_matrix": confusion.long().tolist(),
    }


def run_epoch(
    model,
    loader,
    device,
    class_weights,
    optimizer=None,
    gradient_clip=1.0,
    max_batches=None,
):
    training = optimizer is not None
    model.train(training)
    num_classes = model.config.num_mate_types
    confusion = torch.zeros((num_classes, num_classes), dtype=torch.long)
    loss_sum = 0.0
    samples = 0
    top3 = 0
    context = torch.enable_grad() if training else torch.no_grad()
    with context:
        for batch_index, batch in enumerate(loader):
            if max_batches is not None and batch_index >= max_batches:
                break
            batch = batch.to(device)
            if training:
                optimizer.zero_grad(set_to_none=True)
            logits = model(batch)
            if logits.shape != (len(batch.sample_ids), num_classes):
                raise RuntimeError(
                    f"Unexpected Mate Type logits shape {tuple(logits.shape)} for "
                    f"{len(batch.sample_ids)} samples"
                )
            loss = F.cross_entropy(logits, batch.mate_types, weight=class_weights)
            if not torch.isfinite(loss):
                raise FloatingPointError(
                    "Non-finite Mate Type loss for samples: " + ", ".join(batch.sample_ids)
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
                        "Non-finite gradients in "
                        + ", ".join(invalid_gradients[:20])
                        + "; samples: "
                        + ", ".join(batch.sample_ids)
                    )
                gradient_norm = torch.nn.utils.clip_grad_norm_(
                    model.parameters(), gradient_clip, error_if_nonfinite=True
                )
                optimizer.step()
                if not torch.isfinite(gradient_norm):
                    raise FloatingPointError("Non-finite Mate Type gradient norm")

            detached_logits = logits.detach()
            predictions = detached_logits.argmax(dim=1)
            encoded = batch.mate_types.detach().cpu() * num_classes + predictions.cpu()
            confusion += torch.bincount(
                encoded, minlength=num_classes * num_classes
            ).reshape(num_classes, num_classes)
            top3_indices = detached_logits.topk(min(3, num_classes), dim=1).indices
            top3 += int((top3_indices == batch.mate_types[:, None]).any(dim=1).sum())
            batch_size = len(batch.sample_ids)
            loss_sum += float(loss.detach()) * batch_size
            samples += batch_size

    if samples == 0:
        raise RuntimeError("No Mate Type batches were processed")
    return {
        "loss": loss_sum / samples,
        "samples": samples,
        "top3": top3 / samples,
        **classification_statistics(confusion),
    }


def rng_state(loader):
    state = {
        "python": random.getstate(),
        "numpy": np.random.get_state(),
        "torch": torch.get_rng_state(),
        "loader": loader.generator.get_state() if loader.generator is not None else None,
    }
    if torch.cuda.is_available():
        state["cuda"] = torch.cuda.get_rng_state_all()
    return state


def restore_rng_state(state, loader):
    if not state:
        return
    random.setstate(state["python"])
    np.random.set_state(state["numpy"])
    torch.set_rng_state(state["torch"])
    if state.get("cuda") is not None and torch.cuda.is_available():
        torch.cuda.set_rng_state_all(state["cuda"])
    if state.get("loader") is not None and loader.generator is not None:
        loader.generator.set_state(state["loader"])


def atomic_torch_save(payload, path):
    temporary = path.with_suffix(path.suffix + ".tmp")
    torch.save(payload, temporary)
    temporary.replace(path)


def save_checkpoint(
    path,
    model,
    optimizer,
    scheduler,
    epoch,
    best_score,
    args,
    class_weights,
    mate_type_to_id,
    train_loader,
):
    atomic_torch_save(
        {
            "schema_version": 1,
            "task": "mate_type",
            "epoch": epoch,
            "best_validation_macro_f1": best_score,
            "model_config": model.config.to_dict(),
            "model_state": model.state_dict(),
            "optimizer_state": optimizer.state_dict(),
            "scheduler_state": scheduler.state_dict(),
            "class_weights": class_weights.cpu(),
            "mate_type_to_id": mate_type_to_id,
            "arguments": vars(args),
            "rng_state": rng_state(train_loader),
        },
        path,
    )


def main():
    args = parse_args()
    if args.epochs <= 0 or args.batch_size <= 0:
        raise ValueError("epochs and batch-size must be positive")
    if args.gradient_clip <= 0:
        raise ValueError("gradient-clip must be positive")
    seed_everything(args.seed)
    torch.autograd.set_detect_anomaly(args.detect_anomaly)
    device = torch.device(
        "cuda"
        if args.device == "auto" and torch.cuda.is_available()
        else "cpu"
        if args.device == "auto"
        else args.device
    )
    if device.type == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("CUDA was requested but is unavailable")

    frozen_marker = args.output_dir / "FROZEN.json"
    if frozen_marker.is_file():
        raise RuntimeError(f"Refusing to write into frozen run: {args.output_dir}")
    if not args.resume and any(
        (args.output_dir / name).exists() for name in ("best.pt", "last.pt", "metrics.jsonl")
    ):
        raise RuntimeError(
            f"Output directory already contains a training run: {args.output_dir}. "
            "Use --resume or choose a new directory."
        )
    args.output_dir.mkdir(parents=True, exist_ok=True)

    loader_kwargs = {
        "pin_memory": device.type == "cuda",
        "persistent_workers": args.num_workers > 0,
    }
    train_dataset, train_loader = make_mate_type_dataloader(
        args.index_dir,
        "train",
        args.batch_size,
        True,
        args.num_workers,
        args.seed,
        **loader_kwargs,
    )
    validation_dataset, validation_loader = make_mate_type_dataloader(
        args.index_dir,
        "validation",
        args.batch_size,
        False,
        args.num_workers,
        args.seed,
        **loader_kwargs,
    )
    mate_type_to_id = train_dataset.summary["mate_type_to_id"]
    if validation_dataset.summary["mate_type_to_id"] != mate_type_to_id:
        raise ValueError("Train and validation Mate Type mappings differ")
    num_classes = len(mate_type_to_id)
    if num_classes != 8:
        raise ValueError(f"Expected 8 Mate Types, found {num_classes}")
    class_weights = class_weights_from_rows(train_dataset.rows, num_classes).to(device)

    example = train_dataset[0]
    checkpoint = None
    if args.resume:
        checkpoint = torch.load(args.resume, map_location=device, weights_only=False)
        if checkpoint.get("task") != "mate_type":
            raise ValueError(f"Not an independent Mate Type checkpoint: {args.resume}")
        if checkpoint.get("mate_type_to_id") != mate_type_to_id:
            raise ValueError("Checkpoint and dataset Mate Type mappings differ")
        config = MateTypeModelConfig(**checkpoint["model_config"])
    else:
        config = MateTypeModelConfig(
            face_width=example.graph_a.faces.shape[1],
            loop_width=example.graph_a.loops.shape[1],
            edge_width=example.graph_a.edges.shape[1],
            vertex_width=example.graph_a.vertices.shape[1],
            graph_width=args.graph_width,
            mcf_width=args.mcf_width,
            message_passing_steps=args.message_passing_steps,
            dropout=args.dropout,
        )

    model = MateTypeModel(config).to(device)
    optimizer = torch.optim.Adam(
        model.parameters(), lr=args.learning_rate, weight_decay=args.weight_decay
    )
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(
        optimizer, T_max=args.epochs
    )
    start_epoch = 0
    best_score = -math.inf
    if checkpoint is not None:
        model.load_state_dict(checkpoint["model_state"], strict=True)
        optimizer.load_state_dict(checkpoint["optimizer_state"])
        scheduler.load_state_dict(checkpoint["scheduler_state"])
        start_epoch = int(checkpoint["epoch"]) + 1
        best_score = float(checkpoint.get("best_validation_macro_f1", -math.inf))
        restore_rng_state(checkpoint.get("rng_state"), train_loader)

    run_config = {
        "task": "mate_type",
        "device": str(device),
        "torch": torch.__version__,
        "cuda_runtime": torch.version.cuda,
        "model_config": config.to_dict(),
        "mate_type_to_id": mate_type_to_id,
        "class_weights": class_weights.cpu().tolist(),
        "train_samples": len(train_dataset),
        "validation_samples": len(validation_dataset),
        "arguments": {key: str(value) if isinstance(value, Path) else value for key, value in vars(args).items()},
    }
    (args.output_dir / "config.json").write_text(
        json.dumps(run_config, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(json.dumps(run_config, ensure_ascii=False))

    history_path = args.output_dir / "metrics.jsonl"
    for epoch in range(start_epoch, args.epochs):
        started = time.time()
        if device.type == "cuda":
            torch.cuda.reset_peak_memory_stats(device)
        train_metrics = run_epoch(
            model,
            train_loader,
            device,
            class_weights,
            optimizer=optimizer,
            gradient_clip=args.gradient_clip,
            max_batches=args.max_train_batches,
        )
        validation_metrics = run_epoch(
            model,
            validation_loader,
            device,
            class_weights,
            max_batches=args.max_val_batches,
        )
        scheduler.step()
        if device.type == "cuda":
            torch.cuda.synchronize(device)
        validation_score = validation_metrics["macro_f1_all"]
        record = {
            "epoch": epoch,
            "seconds": time.time() - started,
            "learning_rate": optimizer.param_groups[0]["lr"],
            "peak_gpu_memory_gib": (
                torch.cuda.max_memory_allocated(device) / (1024 ** 3)
                if device.type == "cuda"
                else None
            ),
            "train": train_metrics,
            "validation": validation_metrics,
            "validation_score": validation_score,
        }
        with history_path.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(record, ensure_ascii=False) + "\n")
        print(json.dumps(record, ensure_ascii=False))

        if validation_score > best_score:
            best_score = validation_score
            save_checkpoint(
                args.output_dir / "best.pt",
                model,
                optimizer,
                scheduler,
                epoch,
                best_score,
                args,
                class_weights,
                mate_type_to_id,
                train_loader,
            )
        save_checkpoint(
            args.output_dir / "last.pt",
            model,
            optimizer,
            scheduler,
            epoch,
            best_score,
            args,
            class_weights,
            mate_type_to_id,
            train_loader,
        )


if __name__ == "__main__":
    main()
