"""Evaluate location ranking and mate-type classification on an index split."""

from __future__ import annotations

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path

import torch
from torch.nn import functional as F
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


ROOT = Path(__file__).resolve().parent.parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from automate import MateModelConfig, MatePairModel, make_mate_dataloader


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checkpoint", type=Path, default=ROOT / "runs/mate_pair_v1/best.pt")
    parser.add_argument("--index-dir", type=Path, default=ROOT / "dataset/training/index_v1")
    parser.add_argument("--split", default="test")
    parser.add_argument("--batch-size", type=int, default=4)
    parser.add_argument("--negative-count", type=int, default=15)
    parser.add_argument("--device", default="auto", choices=("auto", "cpu", "cuda"))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--confusion-matrix", type=Path)
    return parser.parse_args()


def metric_bucket():
    return defaultdict(float)


def update_ranking(bucket, logits, labels):
    order = torch.argsort(logits, descending=True)
    rank = int(torch.nonzero(labels[order] > 0.5, as_tuple=False)[0]) + 1
    bucket["samples"] += 1
    bucket["rank_sum"] += rank
    bucket["reciprocal_rank"] += 1.0 / rank
    bucket["top1"] += int(rank <= 1)
    bucket["top3"] += int(rank <= 3)
    bucket["top5"] += int(rank <= 5)


def finish(bucket):
    count = int(bucket["samples"])
    return {
        "samples": count,
        "top1": bucket["top1"] / count,
        "top3": bucket["top3"] / count,
        "top5": bucket["top5"] / count,
        "mrr": bucket["reciprocal_rank"] / count,
        "mean_rank": bucket["rank_sum"] / count,
    }


def classification_report(confusion, type_names):
    confusion = confusion.float()
    support = confusion.sum(dim=1)
    predicted = confusion.sum(dim=0)
    true_positive = confusion.diag()
    precision = true_positive / predicted.clamp_min(1)
    recall = true_positive / support.clamp_min(1)
    f1 = 2 * precision * recall / (precision + recall).clamp_min(1e-12)
    present = support > 0
    per_type = {}
    for index, name in enumerate(type_names):
        per_type[name] = {
            "support": int(support[index]),
            "precision": float(precision[index]),
            "recall": float(recall[index]),
            "f1": float(f1[index]),
        }
    return {
        "accuracy": float(true_positive.sum() / support.sum().clamp_min(1)),
        "balanced_accuracy_supported": float(recall[present].mean()),
        "macro_f1_supported": float(f1[present].mean()),
        "macro_f1_all": float(f1.mean()),
        "absent_types": [type_names[i] for i in range(len(type_names)) if not present[i]],
        "per_type": per_type,
        "confusion_matrix": confusion.long().tolist(),
    }


def plot_confusion_matrix(confusion, type_names, destination):
    values = confusion.numpy()
    row_totals = values.sum(axis=1, keepdims=True)
    normalized = values / row_totals.clip(min=1)
    figure, axes = plt.subplots(1, 2, figsize=(16, 7), constrained_layout=True)
    for axis, matrix, title, number_format in (
        (axes[0], values, "Mate type confusion matrix (counts)", "d"),
        (axes[1], normalized, "Mate type confusion matrix (row normalized)", ".2f"),
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
                axis.text(
                    column,
                    row,
                    format(int(value), number_format) if number_format == "d" else format(value, number_format),
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
        "cuda" if args.device == "auto" and torch.cuda.is_available() else
        "cpu" if args.device == "auto" else args.device
    )
    checkpoint = torch.load(args.checkpoint, map_location=device, weights_only=False)
    model = MatePairModel(MateModelConfig(**checkpoint["model_config"]))
    model.load_state_dict(checkpoint["model_state"])
    model.to(device).eval()
    dataset, loader = make_mate_dataloader(
        args.index_dir,
        split=args.split,
        batch_size=args.batch_size,
        shuffle=False,
        num_workers=0,
        negative_count=args.negative_count,
    )
    id_to_type = {value: key for key, value in dataset.summary["mate_type_to_id"].items()}
    type_names = [id_to_type[index] for index in range(len(id_to_type))]
    overall = metric_bucket()
    by_type = defaultdict(metric_bucket)
    location_loss_sum = 0.0
    pair_count = 0
    type_loss_sum = 0.0
    type_sample_count = 0
    type_confusion = torch.zeros((len(type_names), len(type_names)), dtype=torch.long)
    with torch.no_grad():
        for batch in loader:
            batch = batch.to(device)
            output = model.forward_multitask(batch)
            logits = output.pair_logits
            loss = F.binary_cross_entropy_with_logits(
                logits,
                batch.pair_labels,
                pos_weight=torch.as_tensor(args.negative_count, device=device),
                reduction="sum",
            )
            location_loss_sum += float(loss)
            pair_count += batch.pair_labels.numel()
            if output.type_logits is not None:
                positive_logits = output.type_logits[batch.positive_pair_indices]
                type_loss_sum += float(F.cross_entropy(
                    positive_logits, batch.mate_types, reduction="sum"
                ))
                predictions = positive_logits.argmax(dim=1)
                encoded = batch.mate_types.cpu() * len(type_names) + predictions.cpu()
                type_confusion += torch.bincount(
                    encoded, minlength=len(type_names) ** 2
                ).reshape(len(type_names), len(type_names))
                type_sample_count += batch.mate_types.numel()
            for sample_index, mate_type_id in enumerate(batch.mate_types.tolist()):
                mask = batch.pair_to_sample == sample_index
                update_ranking(overall, logits[mask], batch.pair_labels[mask])
                update_ranking(
                    by_type[id_to_type[mate_type_id]], logits[mask], batch.pair_labels[mask]
                )

    location_result = {
        "loss": location_loss_sum / pair_count,
        **finish(overall),
        "by_mate_type": {name: finish(bucket) for name, bucket in sorted(by_type.items())},
    }
    result = {
        "checkpoint": str(args.checkpoint.resolve()),
        "checkpoint_epoch": int(checkpoint["epoch"]),
        "split": args.split,
        "negative_count": args.negative_count,
        "location": location_result,
    }
    output = args.output or args.checkpoint.parent / f"evaluation_{args.split}.json"
    if (output.parent / "FROZEN.json").is_file() and output.exists():
        raise RuntimeError(
            f"Refusing to overwrite evaluation in frozen run directory: {output}. "
            "Pass --output in a new directory."
        )
    if type_sample_count:
        confusion_path = args.confusion_matrix or args.checkpoint.parent / f"confusion_matrix_{args.split}.png"
        result["mate_type"] = {
            "loss": type_loss_sum / type_sample_count,
            **classification_report(type_confusion, type_names),
            "confusion_matrix_image": str(confusion_path.resolve()),
        }
        plot_confusion_matrix(type_confusion, type_names, confusion_path)
        print(f"confusion_matrix_written={confusion_path}")
    else:
        result["mate_type"] = None
        result["mate_type_note"] = "Checkpoint has no mate-type head (v1 compatible mode)."
    output.write_text(json.dumps(result, indent=2, ensure_ascii=False), encoding="utf-8")
    print(json.dumps(result, indent=2, ensure_ascii=False))
    print(f"evaluation_written={output}")


if __name__ == "__main__":
    main()
