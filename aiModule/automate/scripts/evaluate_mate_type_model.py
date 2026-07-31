"""Evaluate an independent Mate Type checkpoint on a held-out split."""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

import matplotlib
import torch
from torch.nn import functional as F

matplotlib.use("Agg")
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import MateTypeModel, MateTypeModelConfig, make_mate_type_dataloader


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--checkpoint",
        type=Path,
        default=ROOT / "runs/paper_mate_type_10000_e50/best.pt",
    )
    parser.add_argument(
        "--index-dir",
        type=Path,
        default=ROOT / "dataset/training/mate_type_paper_10000",
    )
    parser.add_argument("--split", default="test")
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--device", default="auto", choices=("auto", "cpu", "cuda"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--confusion-matrix", type=Path)
    return parser.parse_args()


def classification_report(confusion, type_names):
    values = confusion.float()
    support = values.sum(dim=1)
    predicted = values.sum(dim=0)
    true_positive = values.diag()
    precision = true_positive / predicted.clamp_min(1)
    recall = true_positive / support.clamp_min(1)
    f1 = 2 * precision * recall / (precision + recall).clamp_min(1.0e-12)
    present = support > 0
    return {
        "accuracy": float(true_positive.sum() / support.sum().clamp_min(1)),
        "balanced_accuracy_supported": float(recall[present].mean()),
        "macro_f1_supported": float(f1[present].mean()),
        "macro_f1_all": float(f1.mean()),
        "absent_types": [type_names[i] for i in range(len(type_names)) if not present[i]],
        "per_type": {
            name: {
                "support": int(support[index]),
                "precision": float(precision[index]),
                "recall": float(recall[index]),
                "f1": float(f1[index]),
            }
            for index, name in enumerate(type_names)
        },
        "confusion_matrix": confusion.long().tolist(),
    }


def plot_confusion_matrix(confusion, type_names, destination):
    values = confusion.numpy()
    normalized = values / values.sum(axis=1, keepdims=True).clip(min=1)
    figure, axes = plt.subplots(1, 2, figsize=(16, 7), constrained_layout=True)
    for axis, matrix, title, number_format in (
        (axes[0], values, "Mate Type confusion matrix (counts)", "d"),
        (axes[1], normalized, "Mate Type confusion matrix (row normalized)", ".2f"),
    ):
        image = axis.imshow(matrix, cmap="Blues", vmin=0)
        axis.set_xticks(range(len(type_names)), type_names, rotation=45, ha="right")
        axis.set_yticks(range(len(type_names)), type_names)
        axis.set_xlabel("Predicted")
        axis.set_ylabel("Ground truth")
        axis.set_title(title)
        threshold = matrix.max() * 0.55 if matrix.size else 0
        for row in range(len(type_names)):
            for column in range(len(type_names)):
                value = matrix[row, column]
                label = format(int(value), "d") if number_format == "d" else format(value, number_format)
                axis.text(
                    column,
                    row,
                    label,
                    ha="center",
                    va="center",
                    color="white" if value > threshold else "black",
                    fontsize=8,
                )
        figure.colorbar(image, ax=axis, fraction=0.046)
    destination.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(destination, dpi=160)
    plt.close(figure)


def main():
    args = parse_args()
    device = torch.device(
        "cuda"
        if args.device == "auto" and torch.cuda.is_available()
        else "cpu"
        if args.device == "auto"
        else args.device
    )
    if device.type == "cuda" and not torch.cuda.is_available():
        raise RuntimeError("CUDA was requested but is unavailable")

    checkpoint = torch.load(args.checkpoint, map_location=device, weights_only=False)
    if checkpoint.get("task") != "mate_type":
        raise ValueError(f"Not an independent Mate Type checkpoint: {args.checkpoint}")
    model = MateTypeModel(MateTypeModelConfig(**checkpoint["model_config"]))
    model.load_state_dict(checkpoint["model_state"], strict=True)
    model.to(device).eval()

    dataset, loader = make_mate_type_dataloader(
        args.index_dir,
        split=args.split,
        batch_size=args.batch_size,
        shuffle=False,
        num_workers=0,
        pin_memory=device.type == "cuda",
    )
    mapping = dataset.summary["mate_type_to_id"]
    if checkpoint.get("mate_type_to_id") != mapping:
        raise ValueError("Checkpoint and evaluation index Mate Type mappings differ")
    type_names = [name for name, _ in sorted(mapping.items(), key=lambda item: item[1])]
    num_classes = len(type_names)
    confusion = torch.zeros((num_classes, num_classes), dtype=torch.long)
    loss_sum = 0.0
    weighted_loss_sum = 0.0
    top3 = 0
    samples = 0
    class_weights = checkpoint.get("class_weights")
    if class_weights is not None:
        class_weights = class_weights.to(device)

    started = time.perf_counter()
    with torch.no_grad():
        for batch in loader:
            batch = batch.to(device)
            logits = model(batch)
            if not torch.isfinite(logits).all():
                raise FloatingPointError("Non-finite logits during Mate Type evaluation")
            batch_size = len(batch.sample_ids)
            loss_sum += float(F.cross_entropy(logits, batch.mate_types, reduction="sum"))
            if class_weights is not None:
                per_sample = F.cross_entropy(
                    logits, batch.mate_types, weight=class_weights, reduction="none"
                )
                weighted_loss_sum += float(per_sample.sum())
            predictions = logits.argmax(dim=1)
            encoded = batch.mate_types.cpu() * num_classes + predictions.cpu()
            confusion += torch.bincount(
                encoded, minlength=num_classes * num_classes
            ).reshape(num_classes, num_classes)
            top3_indices = logits.topk(min(3, num_classes), dim=1).indices
            top3 += int((top3_indices == batch.mate_types[:, None]).any(dim=1).sum())
            samples += batch_size
    if device.type == "cuda":
        torch.cuda.synchronize(device)
    elapsed = time.perf_counter() - started
    if samples != len(dataset):
        raise RuntimeError(f"Evaluated {samples} samples, expected {len(dataset)}")

    confusion_path = args.confusion_matrix or args.checkpoint.parent / f"confusion_matrix_{args.split}.png"
    output_path = args.output or args.checkpoint.parent / f"evaluation_{args.split}.json"
    result = {
        "schema_version": 1,
        "task": "mate_type",
        "checkpoint": str(args.checkpoint.resolve()),
        "checkpoint_epoch": int(checkpoint["epoch"]),
        "split": args.split,
        "device": str(device),
        "samples": samples,
        "elapsed_seconds": elapsed,
        "loss_unweighted": loss_sum / samples,
        "loss_weighted_per_sample": (
            weighted_loss_sum / samples if class_weights is not None else None
        ),
        "top3": top3 / samples,
        **classification_report(confusion, type_names),
        "confusion_matrix_image": str(confusion_path.resolve()),
    }
    plot_confusion_matrix(confusion, type_names, confusion_path)
    output_path.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False, indent=2))
    print(f"evaluation_written={output_path}")
    print(f"confusion_matrix_written={confusion_path}")


if __name__ == "__main__":
    main()
